// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "gpu.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QStringList>

dXorg::dXorg(const GPUSysInfo &si) {
    features.sysInfo = si;
    configure();
    figureOutConstParams();
}

void dXorg::configure() {
    figureOutGpuDataFilePaths(features.sysInfo.sysName);
    setupIoctl();

    if (globalStuff::globalConfig.rootMode) {
        figureOutDriverFeatures();
        return;
    }

    if (globalStuff::globalConfig.daemonData)
        setupSharedMem();

    dcomm.connectToDaemon();

    if (daemonConnected())
        setupDaemon();
    else
        qCritical() << "Daemon is not connected, therefore it can't be configured";

    figureOutDriverFeatures();
}

void dXorg::setupIoctl() {
    if (features.sysInfo.module == DriverModule::RADEON)
        ioctlHnd = new radeonIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
    else if (features.sysInfo.module == DriverModule::AMDGPU)
        ioctlHnd = new amdgpuIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
}

QString getValueFromSysFsFile(QString fileName) {
    QFile f(fileName);
    QString value;
    if (f.open(QIODevice::ReadOnly))
        value = QString(f.readAll()).trimmed();

    f.close();
    return (value.isEmpty()) ? "-1" : value;
}


//https://stackoverflow.com/a/18866593
QString getRandomString() {
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

   QString randomString;
   for (int i = 0; i < 10; ++i)
       randomString.append(possibleCharacters.at(qrand() % possibleCharacters.length()));

   return randomString;
}

void dXorg::setupSharedMem() {
    qDebug() << "Configure shared mem";

    // create the shared mem block. The if comes from that configure method
    // is called on every change gpu, so later, shared mem already exists
    if (!sharedMem.isAttached()) {
        qDebug() << "Shared memory is not attached, creating it";
        sharedMem.setKey(getRandomString());

        if (!sharedMem.create(SHARED_MEM_SIZE)) {
            if (sharedMem.error() == QSharedMemory::AlreadyExists) {
                qDebug() << "Shared memory already exists, attaching to it";

                if (!sharedMem.attach())
                    qCritical() << "Unable to attach to the shared memory: " << sharedMem.errorString();

            } else
                qCritical() << "Unable to create the shared memory: " << sharedMem.errorString();
        }
    }
}

void dXorg::setupDaemon() {
    qDebug() << "Daemon connected";
    QString command;

    if (!globalStuff::globalConfig.daemonData) {
        command.append(DAEMON_SHAREDMEM_KEY).append(SEPARATOR).append('_').append(SEPARATOR);
        qDebug() << "Sending daemon config command: " << command;
        dcomm.sendCommand(command);
        return;
    }

    command.append(DAEMON_SIGNAL_CONFIG).append(SEPARATOR);
    command.append(driverFiles.debugfs_pm_info).append(SEPARATOR);
    command.append(DAEMON_SHAREDMEM_KEY).append(SEPARATOR).append(sharedMem.key()).append(SEPARATOR);

    if (globalStuff::globalConfig.daemonAutoRefresh) {
        command.append(DAEMON_SIGNAL_TIMER_ON).append(SEPARATOR);
        command.append(QString::number(globalStuff::globalConfig.interval)).append(SEPARATOR);
    }
    else
        command.append(DAEMON_SIGNAL_TIMER_OFF).append(SEPARATOR);

    qDebug() << "Sending daemon config command: " << command;
    dcomm.sendCommand(command);
}

bool dXorg::daemonConnected() {
    return dcomm.connected();
}

void dXorg::figureOutGpuDataFilePaths(const QString &gpuName) {
    QString devicePath = "/sys/class/drm/" + gpuName + "/device/";
    driverFiles.moduleParams = devicePath + "driver/module/holders/" + features.sysInfo.driverModuleString + "/parameters/";
    driverFiles.debugfs_pm_info = "/sys/kernel/debug/dri/" + gpuName.right(1) + "/"+features.sysInfo.driverModuleString + "_pm_info"; // this path contains only index
    driverFiles.sysFs = DeviceSysFs(devicePath);

    // look for hwmon devices in card dir
    QString hwmonDevicePath = globalStuff::grabSystemInfo("ls "+ devicePath+ "hwmon/")[0];

    hwmonDevicePath =  devicePath + "hwmon/" + ((hwmonDevicePath.isEmpty() ? "hwmon0" : hwmonDevicePath));

    hwmonAttributes = HwmonAttributes(hwmonDevicePath);

    qDebug() << "hwmon path: " << hwmonDevicePath;
}

// method for gather info about clocks from deamon or from debugfs if root
QString dXorg::getClocksRawData(bool resolvingGpuFeatures) {
    QFile clocksFile(driverFiles.debugfs_pm_info);
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

GPUClocks dXorg::getClocks() {
    switch (features.clocksDataSource) {
        case ClocksDataSource::IOCTL:
            return getClocksFromIoctl();
        case ClocksDataSource::PM_FILE:
            return getClocksFromPmFile();
        case ClocksDataSource::SOURCE_UNKNOWN:
            break;
    }

    return GPUClocks();
}

GPUClocks dXorg::getClocksFromIoctl() {
    GPUClocks clocksData;

    ioctlHnd->getCoreClock(&clocksData.coreClk);
    ioctlHnd->getMemoryClock(&clocksData.memClk);

    return clocksData;
}


GPUClocks dXorg::getClocksFromPmFile() {
    GPUClocks clocksData;
    QString data = dXorg::getClocksRawData();

    // if nothing is there returns empty (-1) struct
    if (data.isEmpty()) {
        qDebug() << "Can't get clocks, no data available";
        return clocksData;
    }

    switch (features.currentPowerMethod) {
        case PowerMethod::DPM: {
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
                clocksData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toInt();

            rx.setPattern(rxPatterns.vddci);
            rx.indexIn(data);
            if (!rx.cap(0).isEmpty())
                clocksData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[rxMatchIndex].toInt();

            return clocksData;
        }

        case PowerMethod::PROFILE: {
            QStringList dataStr = data.split("\n");
            for (int i = 0; i < dataStr.count(); ++i) {
                switch (i) {
                    case 1:
                        if (dataStr[i].contains("current engine clock"))
                            clocksData.coreClk = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                        break;

                    case 3:
                        if (dataStr[i].contains("current memory clock"))
                            clocksData.memClk = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                        break;

                    case 4:
                        if (dataStr[i].contains("voltage"))
                            clocksData.coreVolt = QString().setNum(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toFloat();
                        break;
                }
            }
            return clocksData;
        }

        case PowerMethod::PM_UNKNOWN:
            qWarning() << "Unknown power method, can't get clocks";
            return clocksData;
    }

    return clocksData;
}

float dXorg::getTemperature() {
    QString temp;

    switch (features.currentTemperatureSensor) {
    case TemperatureSensor::SYSFS_HWMON:
    case TemperatureSensor::CARD_HWMON: {
        QFile hwmon(hwmonAttributes.temp1);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        return temp.toFloat() / 1000;
    }
    case TemperatureSensor::PCI_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case TemperatureSensor::MB_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case TemperatureSensor::TS_UNKNOWN: {
        temp = "-1";
        break;
    }
    }
    return temp.toFloat();
}

GPUUsage dXorg::getGPUUsage() {
    GPUUsage data;

    ioctlHnd->getGpuUsage(&data.gpuUsage);

    if (data.gpuUsage == -1 && !driverFiles.sysFs.gpu_busy_percent.isEmpty())
        data.gpuUsage = getValueFromSysFsFile(driverFiles.sysFs.gpu_busy_percent).toFloat();

    ioctlHnd->getVramUsage(&data.gpuVramUsage);

    data.gpuVramUsage /= 1048576; // 1024 * 1024
    data.gpuVramUsagePercent = (100 * data.gpuVramUsage) / params.VRAMSize;

    return data;
}

PowerMethod dXorg::getPowerMethod() {
    if (QFile::exists(driverFiles.sysFs.power_dpm_state))
        return PowerMethod::DPM;

    QFile powerMethodFile(driverFiles.sysFs.power_method);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            return PowerMethod::DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            return PowerMethod::PROFILE;
        else
            return PowerMethod::PM_UNKNOWN;
    }

    return PowerMethod::PM_UNKNOWN;
}

TemperatureSensor dXorg::getTemperatureSensor() {
    QFile hwmon(hwmonAttributes.temp1);

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty())
            return TemperatureSensor::CARD_HWMON;
    } else {
        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        hwmonAttributes.temp1 = findSysfsHwmonForGPU();
        if (!hwmonAttributes.temp1.isEmpty())
            return TemperatureSensor::SYSFS_HWMON;

        // if above fails, use lm_sensors
        QStringList out = globalStuff::grabSystemInfo("sensors");
        if (out.indexOf(QRegExp(features.sysInfo.driverModuleString+"-pci.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp(features.sysInfo.driverModuleString+"-pci.+"));  // in order to not search for it again in timer loop
            return TemperatureSensor::PCI_SENSOR;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("VGA_TEMP.+"));
            return TemperatureSensor::MB_SENSOR;
        }
    }
    return TemperatureSensor::TS_UNKNOWN;
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
            QFile mp(driverFiles.moduleParams+modName);
            modValue = (mp.open(QIODevice::ReadOnly)) ?  modValue = mp.readLine(20) : "unknown";

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.left(modName.indexOf('\n')) << modValue.left(modValue.indexOf('\n')) << modDesc.left(modDesc.indexOf('\n')));
            data.append(item);
            mp.close();
        }
    }
    return data;
}

QString dXorg::getCurrentPowerProfile() {
    QFile f;

    switch (features.currentPowerMethod) {
        case PowerMethod::DPM:
            f.setFileName(driverFiles.sysFs.power_dpm_state);
            break;

        case PowerMethod::PROFILE:
            f.setFileName(driverFiles.sysFs.power_profile);
            break;

        case PowerMethod::PM_UNKNOWN:
            return "err";;
    }

    if (f.open(QIODevice::ReadOnly))
        return QString(f.readLine(13));
    else
        return "err";

}

QString dXorg::getCurrentPowerLevel() {
    QFile forceProfile(driverFiles.sysFs.power_dpm_force_performance_level);
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

void dXorg::setPowerProfile(PowerProfiles newPowerProfile) {
    QString newValue;
    switch (newPowerProfile) {
    case PowerProfiles::BATTERY:
        newValue = dpm_battery;
        break;
    case PowerProfiles::BALANCED:
        newValue = dpm_balanced;
        break;
    case PowerProfiles::PERFORMANCE:
        newValue = dpm_performance;
        break;
    case PowerProfiles::AUTO:
        newValue = profile_auto;
        break;
    case PowerProfiles::DEFAULT:
        newValue = profile_default;
        break;
    case PowerProfiles::HIGH:
        newValue = profile_high;
        break;
    case PowerProfiles::MID:
        newValue = profile_mid;
        break;
    case PowerProfiles::LOW:
        newValue = profile_low;
        break;
    }

    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(driverFiles.sysFs.power_dpm_state, newValue));
    else {
        // enum is int, so first three values are dpm, rest are profile
        if (newPowerProfile <= PowerProfiles::PERFORMANCE)
            setNewValue(driverFiles.sysFs.power_dpm_state, newValue);
        else
            setNewValue(driverFiles.sysFs.power_profile, newValue);
    }
}

void dXorg::setForcePowerLevel(ForcePowerLevels newForcePowerLevel) {
    QString newValue;
    switch (newForcePowerLevel) {
        case ForcePowerLevels::F_AUTO:
            newValue = dpm_auto;
            break;
        case ForcePowerLevels::F_HIGH:
            newValue = dpm_high;
            break;
        case ForcePowerLevels::F_LOW:
            newValue = dpm_low;
            break;
        case ForcePowerLevels::F_MANUAL:
            newValue = dpm_manual;
            break;
        case  ForcePowerLevels::F_PROFILE_STANDARD:
            newValue = dpm_profile_standard;
            break;
        case  ForcePowerLevels::F_PROFILE_MIN_SCLK:
            newValue = dpm_profile_min_sclk;
            break;
        case ForcePowerLevels::F_PROFILE_MIN_MCLK:
            newValue = dpm_profile_min_mclk;
            break;
        case ForcePowerLevels::F_PROFILE_PEAK:
            newValue = dpm_profile_peak;
            break;
    }

    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(driverFiles.sysFs.power_dpm_force_performance_level, newValue));
    else
        setNewValue(driverFiles.sysFs.power_dpm_force_performance_level, newValue);
}

void dXorg::setPwmValue(unsigned int value) {
    value = params.pwmMaxSpeed * value / 100;

    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(hwmonAttributes.pwm1, QString::number(value)));
    else
        setNewValue(hwmonAttributes.pwm1,QString().setNum(value));
}

void dXorg::setPwmManualControl(bool manual) {
    QString mode = QString(manual ? pwm_manual : pwm_auto);

    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(hwmonAttributes.pwm1_enable, mode));
    else
        setNewValue(hwmonAttributes.pwm1_enable, mode);
}

GPUFanSpeed dXorg::getFanSpeed() {
    GPUFanSpeed tmp;

    if (hwmonAttributes.pwm1.isEmpty())
        return tmp;

    QFile f(hwmonAttributes.pwm1);

    if (f.open(QIODevice::ReadOnly)) {
       tmp.fanSpeedPercent = (QString(f.readLine(4)).toFloat() / params.pwmMaxSpeed) * 100;
       f.close();
    }

    if (!hwmonAttributes.fan1_input.isEmpty()) {
        f.setFileName(hwmonAttributes.fan1_input);
        if (f.open(QIODevice::ReadOnly)) {
            tmp.fanSpeedRpm = QString(f.readLine(6)).toInt();
            f.close();
        }
    }

    return tmp;
}

void dXorg::setupRegex(const QString &data) {
    QRegExp rx;

    switch (features.sysInfo.module) {
        case DriverModule::RADEON:
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


            }
            return;

        case DriverModule::AMDGPU:
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

        case DriverModule::MODULE_UNKNOWN:
            return;
    }
}

void dXorg::figureOutDriverFeatures() {
    features.currentPowerMethod = getPowerMethod();

    if (getIoctlAvailability() && !globalStuff::globalConfig.daemonData)
        features.clocksDataSource = ClocksDataSource::IOCTL;
    else {
        features.clocksDataSource = ClocksDataSource::PM_FILE;
        QString data = getClocksRawData(true);
        setupRegex(data);
        GPUClocks test = getClocks();

        // still, sometimes there is miscomunication between daemon,
        // but vales are there, so look again in the file which daemon has
        // copied to /tmp/
        if (test.coreClk == -1)
            test = getFeaturesFallback();
    }

    features.currentTemperatureSensor = getTemperatureSensor();

    switch (features.currentPowerMethod) {
        case PowerMethod::DPM: {
            qDebug() << "Power method: DPM";

            if (globalStuff::globalConfig.rootMode || daemonConnected())
                features.canChangeProfile = true;
            else {
                QFile f(driverFiles.sysFs.power_dpm_state);
                if (f.open(QIODevice::WriteOnly)) {
                    features.canChangeProfile = true;
                    f.close();
                }
            }
            break;
        }
        case PowerMethod::PROFILE: {
            qDebug() << "Power method: Profile";

            QFile f(driverFiles.sysFs.power_profile);
            if (f.open(QIODevice::WriteOnly)) {
                features.canChangeProfile = true;
                f.close();
            }
        }
        case PowerMethod::PM_UNKNOWN:
            qDebug() << "Power method unknown";
            break;
    }

    features.ocCoreAvailable = !driverFiles.sysFs.pp_sclk_od.isEmpty();
    features.ocMemAvailable = !driverFiles.sysFs.pp_mclk_od.isEmpty();

    if (!driverFiles.sysFs.pp_dpm_sclk.isEmpty()) {
        features.sclkTable = loadPowerPlayTable(driverFiles.sysFs.pp_dpm_sclk);
        features.freqCoreAvailable = features.sclkTable.count() > 0;
    }

    if (!driverFiles.sysFs.pp_dpm_mclk.isEmpty()) {
        features.mclkTable = loadPowerPlayTable(driverFiles.sysFs.pp_dpm_mclk);
        features.freqMemAvailable = features.mclkTable.count() > 0;
    }
}

bool dXorg::getIoctlAvailability() {
    if (ioctlHnd == nullptr || !ioctlHnd->isValid()) {
        qDebug() << "IOCTL not available";
        return false;
    }

    return true;
}

void dXorg::figureOutConstParams() {
    if (ioctlHnd != nullptr && ioctlHnd->isValid()) {
        ioctlHnd->getMaxClocks(&params.maxCoreClock, &params.maxMemClock);
        ioctlHnd->getVramSize(&params.VRAMSize);

        params.maxCoreClock /= 1000;
        params.maxMemClock = (params.maxMemClock == -1) ? -1 : params.maxMemClock / 1000;
        params.VRAMSize /= 1048576;

        qDebug() << "GPU max core clk: " << params.maxCoreClock
                 << "\n max mem clk: " << params.maxMemClock
                 << "\n vram size: " << params.VRAMSize;
    }

    if (!hwmonAttributes.temp1_crit.isEmpty()) {

        QFile f(hwmonAttributes.temp1_crit);
        if (f.open(QIODevice::ReadOnly)) {
            params.temp1_crit = f.readLine(7).toInt() / 1000;
            f.close();
        }
    }


    if (!hwmonAttributes.pwm1_max.isEmpty()) {

        QFile f(hwmonAttributes.pwm1_max);
        if (f.open(QIODevice::ReadOnly)) {
            params.pwmMaxSpeed = f.readLine(4).toInt();
            f.close();
        }
    }
}

GPUClocks dXorg::getFeaturesFallback() {
    GPUClocks fallbackfeatures;
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

QString dXorg::createDaemonSetCmd(const QString &file, const QString &value)
{
    QString command;
    command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR);
    command.append(value).append(SEPARATOR);
    command.append(file).append(SEPARATOR);

    qDebug() << "Daemon command: " << command;

    return command;
}

void dXorg::setOverclockValue(const QString &file, const int percentage) {
    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(file, QString::number(percentage)));
    else
        setNewValue(file, QString::number(percentage));
}

PowerPlayTable dXorg::loadPowerPlayTable(const QString &file) {
    QFile f(file);
    PowerPlayTable ppt;

    if (!f.open(QIODevice::ReadOnly))
        return ppt;

    QStringList sl = QString(f.readAll()).split('\n', QString::SkipEmptyParts);

    for (const QString &s : sl) {
        QStringList tmp = s.split(":");
        ppt.insert(tmp[0].toInt(), tmp[1].split(" ")[1]);
    }

    return ppt;
}

void dXorg::setPowerPlayFreq(const QString &file, const int tableIndex) {
    if (daemonConnected())
        dcomm.sendCommand(createDaemonSetCmd(file, QString::number(tableIndex)));
    else
        setNewValue(file, QString::number(tableIndex));
}

int dXorg::getCurrentPowerPlayTableId(const QString &file) {
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly))
        return 0;

    QStringList sl = QString(f.readAll()).split("\n");

    for (int i = 0; i < sl.count(); ++i) {
        if (sl[i].endsWith("*"))
            return i;
    }

    return 0;
}
