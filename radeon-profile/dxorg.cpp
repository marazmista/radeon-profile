// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "gpu.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QCoreApplication>
#include <QDebug>

dXorg::dXorg(const globalStuff::gpuSysInfo &si) {
    features.sysInfo = si;
    configure();
}

void dXorg::configure() {
    if (features.sysInfo.module == globalStuff::RADEON)
        ioctlHnd = new radeonIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
    else if (features.sysInfo.module == globalStuff::AMDGPU)
        ioctlHnd = new amdgpuIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
    else
        ioctlHnd = nullptr;

    if (!globalStuff::globalConfig.rootMode)
        dcomm.connectToDaemon();

    figureOutGpuDataFilePaths(features.sysInfo.sysName);
    figureOutDriverFeatures();

    if (!features.clocksSource == globalStuff::IOCTL) {
        qDebug() << "Running in non-root mode, connecting and configuring the daemon";
        // create the shared mem block. The if comes from that configure method
        // is called on every change gpu, so later, shared mem already exists
        if (!sharedMem.isAttached()) {
            qDebug() << "Shared memory is not attached, creating it";
            sharedMem.setKey("radeon-profile");

            if (!sharedMem.create(SHARED_MEM_SIZE)) {
                if (sharedMem.error() == QSharedMemory::AlreadyExists){
                    qDebug() << "Shared memory already exists, attaching to it";

                    if (!sharedMem.attach())
                        qCritical() << "Unable to attach to the shared memory: " << sharedMem.errorString();

                } else
                    qCritical() << "Unable to create the shared memory: " << sharedMem.errorString();
            }
            // If QSharedMemory::create() returns true, it has already automatically attached
        }
    }

    if (daemonConnected()) {
        qDebug() << "Daemon is connected, configuring it";
        //  Configure the daemon to read the data
        QString command; // SIGNAL_CONFIG + SEPARATOR + CLOCKS_PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_CONFIG).append(SEPARATOR); // Configuration flag
        command.append(filePaths.clocksPath).append(SEPARATOR); // Path where the daemon will read clocks

        if (features.clocksSource == globalStuff::IOCTL)
            command.append(DAEMON_DISABLE_SHAREDMEM).append(SEPARATOR);

        qDebug() << "Sending daemon config command: " << command;
        dcomm.sendCommand(command);

        reconfigureDaemon(); // Set up timer
    } else
        qCritical() << "Daemon is not connected, therefore it can't be configured";
}

void dXorg::reconfigureDaemon() { // Set up the timer
    if (daemonConnected()) {
        qDebug() << "Configuring daemon timer";
        QString command;

        if (globalStuff::globalConfig.daemonAutoRefresh){ // SIGNAL_TIMER_ON + SEPARATOR + INTERVAL + SEPARATOR
            command.append(DAEMON_SIGNAL_TIMER_ON).append(SEPARATOR); // Enable timer
            command.append(QString::number(globalStuff::globalConfig.interval)).append(SEPARATOR); // Set timer interval
        }
        else // SIGNAL_TIMER_OFF + SEPARATOR
            command.append(DAEMON_SIGNAL_TIMER_OFF).append(SEPARATOR); // Disable timer

        qDebug() << "Sending daemon reconfig signal: " << command;
        dcomm.sendCommand(command);
    } else
        qWarning() << "Daemon is not connected, therefore it's timer can't be reconfigured";
}

bool dXorg::daemonConnected() {
    return dcomm.connected();
}

void dXorg::figureOutGpuDataFilePaths(const QString &gpuName) {
    QString devicePath = "/sys/class/drm/" + gpuName + "/device/";

    filePaths.powerMethodFilePath = devicePath + file_PowerMethod;
    filePaths.profilePath = devicePath + file_powerProfile;
    filePaths.dpmStateFilePath = devicePath + file_powerDpmState;
    filePaths.forcePowerLevelFilePath = devicePath + file_powerDpmForcePerformanceLevel;
    filePaths.moduleParamsPath = devicePath + "driver/module/holders/" + features.sysInfo.driverModuleString + "/parameters/";
    filePaths.clocksPath = "/sys/kernel/debug/dri/" + gpuName.right(1) + "/"+features.sysInfo.driverModuleString + "_pm_info"; // this path contains only index
    filePaths.overDrivePath = devicePath + file_overclockLevel;


    QString hwmonDevicePath = globalStuff::grabSystemInfo("ls "+ devicePath+ "hwmon/")[0]; // look for hwmon devices in card dir
    hwmonDevicePath =  devicePath + "hwmon/" + ((hwmonDevicePath.isEmpty() ? "hwmon0" : hwmonDevicePath));

    filePaths.sysfsHwmonTempPath = hwmonDevicePath  + "/temp1_input";

    if (QFile::exists(hwmonDevicePath + "/pwm1_enable")) {
        filePaths.pwmEnablePath = hwmonDevicePath + "/pwm1_enable";

        if (QFile::exists(hwmonDevicePath + "/pwm1"))
            filePaths.pwmSpeedPath = hwmonDevicePath + "/pwm1";

        if (QFile::exists(hwmonDevicePath + "/pwm1_max"))
            filePaths.pwmMaxSpeedPath = hwmonDevicePath + "/pwm1_max";
    } else {
        filePaths.pwmEnablePath = "";
        filePaths.pwmSpeedPath = "";
        filePaths.pwmMaxSpeedPath = "";
    }
}

// method for gather info about clocks from deamon or from debugfs if root
QString dXorg::getClocksRawData(bool resolvingGpuFeatures) {
    QFile clocksFile(filePaths.clocksPath);
    QString data;

    if (clocksFile.open(QIODevice::ReadOnly)) {
        data = QString(clocksFile.readAll()).trimmed();
        clocksFile.close();
        return data;
    }

    if (daemonConnected()) {
        if (!globalStuff::globalConfig.daemonAutoRefresh){
            qDebug() << "Asking the daemon to read clocks";
            dcomm.sendCommand(QString(DAEMON_SIGNAL_READ_CLOCKS).append(SEPARATOR)); // SIGNAL_READ_CLOCKS + SEPARATOR
        }

        // fist call, so notihing is in sharedmem and we need to wait for data
        // because we need correctly figure out what is available
        // see: https://stackoverflow.com/a/11487434/2347196
        if (Q_UNLIKELY(resolvingGpuFeatures)) {
            QTime delayTime = QTime::currentTime().addMSecs(1200);
            qDebug() << "Waiting for first daemon data read...";
            while (QTime::currentTime() < delayTime)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

       if (sharedMem.lock()) {
            const char *to = (const char*)sharedMem.constData();
            if (to != NULL) {
                qDebug() << "Reading data from shared memory";
                data = QString(QByteArray::fromRawData(to, SHARED_MEM_SIZE)).trimmed();
            } else
                qWarning() << "Shared memory data pointer is invalid: " << sharedMem.errorString();
            sharedMem.unlock();
        } else
            qWarning() << "Unable to lock the shared memory: " << sharedMem.errorString();
    }

    #ifdef QT_DEBUG
         if (resolvingGpuFeatures)
             qDebug() << data;
    #endif

    return data;
}

globalStuff::gpuClocksStruct dXorg::getClocks() {
    switch (features.clocksSource) {
        case globalStuff::IOCTL:
            return getClocksFromIoctl();
        case globalStuff::PM_FILE:
            return getClocksFromPmFile();
        case globalStuff::SOURCE_UNKNOWN:
            break;
    }

    return globalStuff::gpuClocksStruct();
}

globalStuff::gpuClocksStruct dXorg::getClocksFromIoctl() {
    globalStuff::gpuClocksStruct clocksData;

    ioctlHnd->getCoreClock(&clocksData.coreClk);
    ioctlHnd->getMemoryClock(&clocksData.memClk);

    return clocksData;
}


globalStuff::gpuClocksStruct dXorg::getClocksFromPmFile() {
    globalStuff::gpuClocksStruct clocksData;
    QString data = dXorg::getClocksRawData();

    // if nothing is there returns empty (-1) struct
    if (data.isEmpty()){
        qDebug() << "Can't get clocks, no data available";
        return clocksData;
    }

    switch (features.currentPowerMethod) {
    case globalStuff::DPM: {
        QRegExp rx;

        rx.setPattern(rxPatterns.powerLevel);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            clocksData.powerLevel = rx.cap(0).split(' ')[2].toShort();

        rx.setPattern(rxPatterns.sclk);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            clocksData.coreClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat() / dXorg::clocksValueDivider;

        rx.setPattern(rxPatterns.mclk);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            clocksData.memClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat() / dXorg::clocksValueDivider;

        rx.setPattern(rxPatterns.vclk);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            clocksData.uvdCClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat() / dXorg::clocksValueDivider;
            clocksData.uvdCClk  = (clocksData.uvdCClk  == 0) ? -1 :  clocksData.uvdCClk;
        }

        rx.setPattern(rxPatterns.dclk);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            clocksData.uvdDClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat() / dXorg::clocksValueDivider;
            clocksData.uvdDClk = (clocksData.uvdDClk == 0) ? -1 : clocksData.uvdDClk;
        }

        rx.setPattern(rxPatterns.vddc);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            clocksData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat();

        rx.setPattern(rxPatterns.vddci);
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            clocksData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toFloat();

        return clocksData;
        break;
    }
    case globalStuff::PROFILE: {
        QStringList dataStr = data.split("\n");
        for (int i=0; i< dataStr.count(); i++) {
            switch (i) {
            case 1: {
                if (dataStr[i].contains("current engine clock")) {
                    clocksData.coreClk = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                    break;
                }
            };
            case 3: {
                if (dataStr[i].contains("current memory clock")) {
                    clocksData.memClk = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                    break;
                }
            }
            case 4: {
                if (dataStr[i].contains("voltage")) {
                    clocksData.coreVolt = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toFloat();
                    break;
                }
            }
            }
        }
        return clocksData;
        break;
    }
    case globalStuff::PM_UNKNOWN: {
        qWarning() << "Unknown power method, can't get clocks";
        return clocksData;
        break;
    }
    }
    return clocksData;
}

float dXorg::getTemperature() {
    QString temp;

    switch (features.currentTemperatureSensor) {
    case globalStuff::SYSFS_HWMON:
    case globalStuff::CARD_HWMON: {
        QFile hwmon(filePaths.sysfsHwmonTempPath);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        return temp.toFloat() / 1000;
        break;
    }
    case globalStuff::PCI_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case globalStuff::MB_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case globalStuff::TS_UNKNOWN: {
        temp = "-1";
        break;
    }
    }
    return temp.toFloat();
}

globalStuff::gpuUsageStruct dXorg::getGpuUsage() {
    globalStuff::gpuUsageStruct data;

    ioctlHnd->getGpuUsage(&data.gpuLoad, 500000, 150);
    ioctlHnd->getVramUsagePercentage(&data.gpuVramLoadPercent);
    ioctlHnd->getVramUsage(&data.gpuVramLoad);

    return data;
}

globalStuff::powerMethod dXorg::getPowerMethod() {
    if (QFile::exists(filePaths.dpmStateFilePath))
        return globalStuff::DPM;

    QFile powerMethodFile(filePaths.powerMethodFilePath);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            return globalStuff::DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            return globalStuff::PROFILE;
        else
            return globalStuff::PM_UNKNOWN;
    }

    return globalStuff::PM_UNKNOWN;
}

globalStuff::tempSensor dXorg::getTemperatureSensor() {
    QFile hwmon(filePaths.sysfsHwmonTempPath);

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty())
            return globalStuff::CARD_HWMON;
    } else {
        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        filePaths.sysfsHwmonTempPath = findSysfsHwmonForGPU();
        if (!filePaths.sysfsHwmonTempPath.isEmpty())
            return globalStuff::SYSFS_HWMON;

        // if above fails, use lm_sensors
        QStringList out = globalStuff::grabSystemInfo("sensors");
        if (out.indexOf(QRegExp(features.sysInfo.driverModuleString+"-pci.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp(features.sysInfo.driverModuleString+"-pci.+"));  // in order to not search for it again in timer loop
            return globalStuff::PCI_SENSOR;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("VGA_TEMP.+"));
            return globalStuff::MB_SENSOR;
        }
    }
    return globalStuff::TS_UNKNOWN;
}

QString dXorg::findSysfsHwmonForGPU() {
    QStringList hwmonDev = globalStuff::grabSystemInfo("ls /sys/class/hwmon/");
    for (int i = 0; i < hwmonDev.count(); i++) {
        QStringList temp = globalStuff::grabSystemInfo("ls /sys/class/hwmon/"+hwmonDev[i]+"/device/").filter("label");

        for (int o = 0; o < temp.count(); o++) {
            QFile f("/sys/class/hwmon/"+hwmonDev[i]+"/device/"+temp[o]);
            if (f.open(QIODevice::ReadOnly))
                if (f.readLine(20).contains("VGA_TEMP")) {
                    f.close();
                    return f.fileName().replace("label", "input");
                }
        }
    }
    return "";
}

QStringList dXorg::getGLXInfo(QProcessEnvironment env) {
    return globalStuff::grabSystemInfo("glxinfo -B",env).filter(QRegExp(".+"));
}

QList<QTreeWidgetItem *> dXorg::getModuleInfo() {
    QList<QTreeWidgetItem *> data;
    QStringList modInfo = globalStuff::grabSystemInfo("modinfo -p "+features.sysInfo.driverModuleString);
    modInfo.sort();

    for (int i =0; i < modInfo.count(); i++) {
        if (modInfo[i].contains(":")) {
            // show nothing in case of an error
            if (modInfo[i].startsWith("modinfo: ERROR: ")) {
                continue;
            }
            // read module param name and description from modinfo command
            QString modName = modInfo[i].split(":",QString::SkipEmptyParts)[0],
                    modDesc = modInfo[i].split(":",QString::SkipEmptyParts)[1],
                    modValue;

            // read current param values
            QFile mp(filePaths.moduleParamsPath+modName);
            modValue = (mp.open(QIODevice::ReadOnly)) ?  modValue = mp.readLine(20) : "unknown";

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.left(modName.indexOf('\n')) << modValue.left(modValue.indexOf('\n')) << modDesc.left(modDesc.indexOf('\n')));
            data.append(item);
            mp.close();
        }
    }
    return data;
}

QString dXorg::getCurrentPowerProfile() {
    switch (features.currentPowerMethod) {
    case globalStuff::DPM: {
        QFile dpmProfile(filePaths.dpmStateFilePath);
        if (dpmProfile.open(QIODevice::ReadOnly))
           return QString(dpmProfile.readLine(13));
        else
            return "err";
        break;
    }
    case globalStuff::PROFILE: {
        QFile profile(filePaths.profilePath);
        if (profile.open(QIODevice::ReadOnly))
            return QString(profile.readLine(13));
        break;
    }
    case globalStuff::PM_UNKNOWN:
        break;
    }

    return "err";
}

QString dXorg::getCurrentPowerLevel() {
    QFile forceProfile(filePaths.forcePowerLevelFilePath);
    if (forceProfile.open(QIODevice::ReadOnly))
        return QString(forceProfile.readLine(13));

    return "err";
}

void dXorg::setNewValue(const QString &filePath, const QString &newValue) {
    QFile file(filePath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream stream(&file);
        stream << newValue + "\n";
        if( ! file.flush() )
            qWarning() << "Failed to flush " << filePath;
        file.close();
    }else
        qWarning() << "Unable to open " << filePath << " to write " << newValue;
}

void dXorg::setPowerProfile(globalStuff::powerProfiles _newPowerProfile) {
    QString newValue;
    switch (_newPowerProfile) {
    case globalStuff::BATTERY:
        newValue = dpm_battery;
        break;
    case globalStuff::BALANCED:
        newValue = dpm_balanced;
        break;
    case globalStuff::PERFORMANCE:
        newValue = dpm_performance;
        break;
    case globalStuff::AUTO:
        newValue = profile_auto;
        break;
    case globalStuff::DEFAULT:
        newValue = profile_default;
        break;
    case globalStuff::HIGH:
        newValue = profile_high;
        break;
    case globalStuff::MID:
        newValue = profile_mid;
        break;
    case globalStuff::LOW:
        newValue = profile_low;
        break;
    default: break;
    }

    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(newValue).append(SEPARATOR); // Power profile to be set
        command.append(filePaths.dpmStateFilePath).append(SEPARATOR); // The path where the power profile should be written in

        qDebug() << "Sending daemon power profile signal: " << command;
        dcomm.sendCommand(command);
    } else {
        // enum is int, so first three values are dpm, rest are profile
        if (_newPowerProfile <= globalStuff::PERFORMANCE)
            setNewValue(filePaths.dpmStateFilePath,newValue);
        else
            setNewValue(filePaths.profilePath,newValue);
    }
}

void dXorg::setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) {
    QString newValue;
    switch (_newForcePowerLevel) {
    case globalStuff::F_AUTO:
        newValue = dpm_auto;
        break;
    case globalStuff::F_HIGH:
        newValue = dpm_high;
        break;
    case globalStuff::F_LOW:
        newValue = dpm_low;
    default:
        break;
    }

    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(newValue).append(SEPARATOR); // Power profile to be forcibly set
        command.append(filePaths.forcePowerLevelFilePath).append(SEPARATOR); // The path where the power profile should be written in

        qDebug() << "Sending daemon forced power profile signal: " << command;
        dcomm.sendCommand(command);
    } else
        setNewValue(filePaths.forcePowerLevelFilePath, newValue);
}

void dXorg::setPwmValue(unsigned int value) {
    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(QString::number(value)).append(SEPARATOR); // PWM value to be set
        command.append(filePaths.pwmSpeedPath).append(SEPARATOR); // The path where the PWM value should be written in

        qDebug() << "Sending daemon fan pwm speed signal: " << command;
        dcomm.sendCommand(command);
    } else
        setNewValue(filePaths.pwmSpeedPath,QString().setNum(value));
}

void dXorg::setPwmManualControl(bool manual) {
    char mode = manual ? pwm_manual : pwm_auto;

    if (daemonConnected()) {
        //  Tell the daemon to set the pwm mode into the right file
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(mode).append(SEPARATOR); // The PWM mode to set
        command.append(filePaths.pwmEnablePath).append(SEPARATOR); // The path where the PWM mode should be written in

        qDebug() << "Sending daemon fan pwm enable signal: " << command;
        dcomm.sendCommand(command);
    } else  //  No daemon available
        setNewValue(filePaths.pwmEnablePath, QString(mode));
}

int dXorg::getPwmSpeed() {
    QFile f(filePaths.pwmSpeedPath);

    int val = 0;
    if (f.open(QIODevice::ReadOnly)) {
       val = QString(f.readLine(4)).toInt();
       f.close();
    }

    return val;
}

void dXorg::setupRegex(const QString &data) {
    QRegExp rx;

    switch (features.sysInfo.module) {
        case globalStuff::RADEON:
            rx.setPattern("sclk:\\s\\d+");
            rx.indexIn(data);
            if (!rx.cap(0).isEmpty()) {
                dXorg::rxPatterns.powerLevel = "power\\slevel\\s\\d",
                        dXorg::rxPatterns.sclk = "sclk:\\s\\d+",
                        dXorg::rxPatterns.mclk = "mclk:\\s\\d+",
                        dXorg::rxPatterns.vclk = "vclk:\\s\\d+",
                        dXorg::rxPatterns.dclk = "dclk:\\s\\d+",
                        dXorg::rxPatterns.vddc = "vddc:\\s\\d+",
                        dXorg::rxPatterns.vddci = "vddci:\\s\\d+";

                rxMatchIndex = 1;
                clocksValueDivider = 100;

                return;
            }

        case globalStuff::AMDGPU:
            rx.setPattern("\\[\\s+sclk\\s+\\]:\\s\\d+");
            rx.indexIn(data);
            if (!rx.cap(0).isEmpty()) {
                dXorg::rxPatterns.sclk = "\\[\\s+sclk\\s+\\]:\\s\\d+",
                        dXorg::rxPatterns.mclk = "\\[\\s+mclk\\s+\\]:\\s\\d+";

                rxMatchIndex = 3;
                clocksValueDivider = 1;
                return;
            }


            rx.setPattern("\\d+\\s\\w+\\s\\(SCLK\\)");
            rx.indexIn(data);
            if (!rx.cap(0).isEmpty()) {
                dXorg::rxPatterns.sclk = "\\d+\\s\\w+\\s\\(SCLK\\)",
                        dXorg::rxPatterns.mclk = "\\d+\\s\\w+\\s\\(MCLK\\)";

                rxMatchIndex = 0;
                clocksValueDivider = 1;
                return;
            }
        case globalStuff::MODULE_UNKNOWN:
            return;
    }
}

void dXorg::figureOutDriverFeatures() {
    if (getIoctlAvailability()) {
        features.clocksSource = globalStuff::IOCTL;
        features.coreClockAvailable = true;
        features.memClockAvailable = true;
    } else {
        features.clocksSource = globalStuff::PM_FILE;
        QString data = getClocksRawData(true);
        setupRegex(data);
        globalStuff::gpuClocksStruct test = getClocks();

        // still, sometimes there is miscomunication between daemon,
        // but vales are there, so look again in the file which daemon has
        // copied to /tmp/
        if (test.coreClk == -1)
            test = getFeaturesFallback();

        features.coreClockAvailable = !(test.coreClk == -1);
        features.memClockAvailable = !(test.memClk == -1);
        features.coreVoltAvailable = !(test.coreVolt == -1);
        features.memVoltAvailable = !(test.memVolt == -1);
    }

    features.currentTemperatureSensor = getTemperatureSensor();
    features.currentPowerMethod = getPowerMethod();

    features.temperatureAvailable =  (features.currentTemperatureSensor == globalStuff::TS_UNKNOWN) ? false : true;

    switch (features.currentPowerMethod) {
    case globalStuff::DPM: {
        if (globalStuff::globalConfig.rootMode || daemonConnected())
            features.canChangeProfile = true;
        else {
            QFile f(filePaths.dpmStateFilePath);
            if (f.open(QIODevice::WriteOnly)){
                features.canChangeProfile = true;
                f.close();
            }
        }
        break;
    }
    case globalStuff::PROFILE: {
        QFile f(filePaths.profilePath);
        if (f.open(QIODevice::WriteOnly)) {
            features.canChangeProfile = true;
            f.close();
        }
    }
    case globalStuff::PM_UNKNOWN:
        break;
    }

    if (!filePaths.pwmEnablePath.isEmpty()) {
        QFile f(filePaths.pwmEnablePath);
        f.open(QIODevice::ReadOnly);

        if (QString(f.readLine(2))[0] != pwm_disabled) {
            features.pwmAvailable = true;


        }
        f.close();
    }

    features.overclockAvailable = QFile::exists(filePaths.overDrivePath);
}

bool dXorg::getIoctlAvailability() {
    if (ioctlHnd == nullptr || !ioctlHnd->isValid())
        return false;

    int c, m;
    if (!ioctlHnd->getClocks(&c, &m))
        return false;

    return true;
}

globalStuff::gpuConstParams dXorg::getGpuConstParams() {
    globalStuff::gpuConstParams params;

    QFile fPwmMax(filePaths.pwmMaxSpeedPath);
    if (fPwmMax.open(QIODevice::ReadOnly)) {
        params.pwmMaxSpeed = fPwmMax.readLine(4).toInt();
        fPwmMax.close();
    }

    if (ioctlHnd != nullptr && ioctlHnd->isValid()) {
        ioctlHnd->getMaxCoreClock(&params.maxCoreClock);
        ioctlHnd->getMaxMemoryClock(&params.maxMemClock);
    }

    return params;
}

globalStuff::gpuClocksStruct dXorg::getFeaturesFallback() {
    globalStuff::gpuClocksStruct fallbackfeatures;
    QFile f("/tmp/"+features.sysInfo.driverModuleString+"_pm_info");
    if (f.open(QIODevice::ReadOnly)) {
        QString s = QString(f.readAll());

        // just look for it, if it is, the value is not important at this point
        if (s.contains("sclk"))
            fallbackfeatures.coreClk = 0;
        if (s.contains("mclk"))
            fallbackfeatures.memClk = 0;
        if (s.contains("vddc"))
            fallbackfeatures.coreVolt = 0;
        if (s.contains("vddci"))
            fallbackfeatures.memVolt = 0;

        f.close();
    }

    return fallbackfeatures;
}

bool dXorg::overclock(const int percentage) {
    if ((percentage > 20) || (percentage < 0))
        qWarning() << "Error overclocking: invalid percentage passed: " << percentage;
    else if (daemonConnected()) { // Signal the daemon to set the overclock value
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(QString::number(percentage)).append(SEPARATOR); // The overclock level
        command.append(filePaths.overDrivePath).append(SEPARATOR); // The path where the overclock level should be written in

        qDebug() << "Sending overclock signal: " << command;
        dcomm.sendCommand(command);
        return true;
    } else if(globalStuff::globalConfig.rootMode){ // Root mode, set it directly
        setNewValue(filePaths.overDrivePath, QString::number(percentage));

        return true;
    } else // Overclock requires root access to sysfs
        qWarning() << "Error overclocking: daemon is not connected and no root access is available";

    return false;
}

void dXorg::resetOverclock() {
    overclock(0);
}
