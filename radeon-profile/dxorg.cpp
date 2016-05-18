// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "globalStuff.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QApplication>
#include <QDebug>

// define static members //
tempSensor dXorg::currentTempSensor = TS_UNKNOWN;
powerMethod dXorg::currentPowerMethod;
int dXorg::sensorsGPUtempIndex;
QChar dXorg::gpuSysIndex;
QSharedMemory dXorg::sharedMem;
dXorg::driverFilePaths dXorg::filePaths;
// end //

daemonComm *dcomm = new daemonComm();

void dXorg::configure(QString gpuName) {
    figureOutGpuDataFilePaths(gpuName);
    currentTempSensor = testSensor();
    currentPowerMethod = getPowerMethod();

    if (!globalStuff::globalConfig.rootMode) {
        qDebug() << "Running in non-root mode, connecting and configuring the daemon";
        // create the shared mem block. The if comes from that configure method
        // is called on every change gpu, so later, shared mem already exists
        if (!sharedMem.isAttached()) {
            qDebug() << "Shared memory is not attached, creating it";
            sharedMem.setKey("radeon-profile");
           if (!sharedMem.create(SHARED_MEM_SIZE)){
                if(sharedMem.error() == QSharedMemory::AlreadyExists){
                    qDebug() << "Shared memory already exists, attaching to it";
                    if (!sharedMem.attach())
                        qCritical() << "Unable to attach to the shared memory: " << sharedMem.errorString();
                } else
                    qCritical() << "Unable to create the shared memory: " << sharedMem.errorString();
           }
           // If QSharedMemory::create() returns true, it has already automatically attached
        }

        dcomm->connectToDaemon();
        if (daemonConnected()) {
            qDebug() << "Daemon is connected, configuring it";

            dcomm->sendConfig(filePaths.clocksPath); //  Configure the daemon to read the data

            reconfigureDaemon(); // Set up timer
        } else
            qCritical() << "Daemon is not connected, therefore it can't be configured";
    }
}

void dXorg::reconfigureDaemon() { // Set up the timer
    if (daemonConnected()) {
        qDebug() << "Configuring daemon timer";

        if (globalStuff::globalConfig.daemonAutoRefresh)
            dcomm->sendTimerOn(globalStuff::globalConfig.interval); // Enable timer
        else
            dcomm->sendTimerOff(); // Disable timer

    } else
        qWarning() << "Daemon is not connected, therefore it's timer can't be reconfigured";
}

bool dXorg::daemonConnected() {
    return dcomm->connected();
}

void dXorg::figureOutGpuDataFilePaths(QString gpuName) {
    gpuSysIndex = gpuName.at(gpuName.length()-1);
    QString devicePath = "/sys/class/drm/"+gpuName+"/device/";

    filePaths.powerMethodFilePath = devicePath + file_PowerMethod;
    filePaths.profilePath = devicePath + file_powerProfile;
    filePaths.dpmStateFilePath = devicePath + file_powerDpmState;
    filePaths.forcePowerLevelFilePath = devicePath + file_powerDpmForcePerformanceLevel;
    filePaths.moduleParamsPath = devicePath + "driver/module/holders/radeon/parameters/";
    filePaths.clocksPath = "/sys/kernel/debug/dri/"+QString(gpuSysIndex)+"/radeon_pm_info"; // this path contains only index
    //  filePaths.clocksPath = "/tmp/radeon_pm_info"; // testing
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
    QByteArray data;

    if (clocksFile.open(QIODevice::ReadOnly)) // check for debugfs access
            data = clocksFile.readAll();
    else if (daemonConnected()) {
        if (!globalStuff::globalConfig.daemonAutoRefresh)
            dcomm->sendReadClocks();

        // fist call, so notihing is in sharedmem and we need to wait for data
        // because we need correctly figure out what is available
        // see: https://stackoverflow.com/a/11487434/2347196
        if (resolvingGpuFeatures) {
            QTime delayTime = QTime::currentTime().addMSecs(1000);
            while (QTime::currentTime() < delayTime)
                QApplication::processEvents(QEventLoop::AllEvents, 100);
        }

       if (sharedMem.lock()) {
            const char *to = (const char*)sharedMem.constData();
            if (to != NULL) {
                qDebug() << "Reading data from shared memory";
                data = QByteArray::fromRawData(to, SHARED_MEM_SIZE);
            } else
                qWarning() << "Shared memory data pointer is invalid: " << sharedMem.errorString();
            sharedMem.unlock();
        } else
            qWarning() << "Unable to lock the shared memory: " << sharedMem.errorString();
    }

    if (data.isEmpty())
        qWarning() << "No data was found";

    return (QString)data.trimmed();
}

gpuClocksStruct dXorg::getClocks(const QString &data) {
    gpuClocksStruct tData(-1); // empty struct

    // if nothing is there returns empty (-1) struct
    if (data.isEmpty()){
        qDebug() << "Can't get clocks, no data available";
        return tData;
    }

    switch (currentPowerMethod) {
    case DPM: {
        QRegExp rx;

        rx.setPattern("power\\slevel\\s\\d");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.powerLevel = rx.cap(0).split(' ')[2].toShort();

        rx.setPattern("sclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.coreClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("mclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.memClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("vclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdCClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdCClk  = (tData.uvdCClk  == 0) ? -1 :  tData.uvdCClk;
        }

        rx.setPattern("dclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdDClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdDClk = (tData.uvdDClk == 0) ? -1 : tData.uvdDClk;
        }

        rx.setPattern("vddc:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        rx.setPattern("vddci:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        return tData;
        break;
    }
    case PROFILE: {
        QStringList clocksData = data.split("\n");
        for (int i=0; i< clocksData.count(); i++) {
            switch (i) {
            case 1: {
                if (clocksData[i].contains("current engine clock")) {
                    tData.coreClk = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            };
            case 3: {
                if (clocksData[i].contains("current memory clock")) {
                    tData.memClk = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            }
            case 4: {
                if (clocksData[i].contains("voltage")) {
                    tData.coreVolt = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toDouble();
                    break;
                }
            }
            }
        }
        return tData;
        break;
    }
    case PM_UNKNOWN: {
        qWarning() << "Unknown power method, can't get clocks";
        return tData;
        break;
    }
    }
    return tData;
}

float dXorg::getTemperature() {
    QString temp;

    switch (currentTempSensor) {
    case SYSFS_HWMON:
    case CARD_HWMON: {
        QFile hwmon(filePaths.sysfsHwmonTempPath);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        return temp.toDouble() / 1000;
        break;
    }
    case PCI_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case MB_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case TS_UNKNOWN: {
        temp = "-1";
        break;
    }
    }
    return temp.toDouble();
}

powerMethod dXorg::getPowerMethod() {
    QFile powerMethodFile(filePaths.powerMethodFilePath);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            return DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            return PROFILE;
        else
            return PM_UNKNOWN;
    } else
        return PM_UNKNOWN;
}

tempSensor dXorg::testSensor() {
    QFile hwmon(filePaths.sysfsHwmonTempPath);

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty())
            return CARD_HWMON;
    } else {
        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        filePaths.sysfsHwmonTempPath = findSysfsHwmonForGPU();
        if (!filePaths.sysfsHwmonTempPath.isEmpty())
            return SYSFS_HWMON;

        // if above fails, use lm_sensors
        QStringList out = globalStuff::grabSystemInfo("sensors");
        if (out.indexOf(QRegExp("radeon-pci.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("radeon-pci.+"));  // in order to not search for it again in timer loop
            return PCI_SENSOR;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("VGA_TEMP.+"));
            return MB_SENSOR;
        }
    }
    return TS_UNKNOWN;
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
    return globalStuff::grabSystemInfo("glxinfo",env).filter(QRegExp("direct|OpenGL.+:.+"));
}

QList<QTreeWidgetItem *> dXorg::getModuleInfo() {
    QList<QTreeWidgetItem *> data;
    QStringList modInfo = globalStuff::grabSystemInfo("modinfo -p radeon");
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

QStringList dXorg::detectCards() {
    QStringList data;
    QStringList out = globalStuff::grabSystemInfo("ls /sys/class/drm/").filter("card");
    for (char i = 0; i < out.count(); i++) {
        QFile f("/sys/class/drm/"+out[i]+"/device/uevent");
        if (f.open(QIODevice::ReadOnly)) {
            if (f.readLine(50).contains("DRIVER=radeon"))
                data.append(f.fileName().split('/')[4]);
        }
    }
    return data;
}

QString dXorg::getCurrentPowerProfile() {
    switch (currentPowerMethod) {
    case DPM: {
        QFile dpmProfile(filePaths.dpmStateFilePath);
        if (dpmProfile.open(QIODevice::ReadOnly))
           return QString(dpmProfile.readLine(13));
        else
            return "err";
        break;
    }
    case PROFILE: {
        QFile profile(filePaths.profilePath);
        if (profile.open(QIODevice::ReadOnly))
            return QString(profile.readLine(13));
        break;
    }
    case PM_UNKNOWN:
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

void dXorg::setPowerProfile(powerProfiles newPowerProfile) {
    const QString newValue = profileToString.at(newPowerProfile);

    if (daemonConnected())
        dcomm->sendSetValue(newValue, filePaths.dpmStateFilePath);
    else {
        // enum is int, so first three values are dpm, rest are profile
        if (newPowerProfile <= PERFORMANCE)
            setNewValue(filePaths.dpmStateFilePath,newValue);
        else
            setNewValue(filePaths.profilePath,newValue);
    }
}

void dXorg::setForcePowerLevel(forcePowerLevels newForcePowerLevel) {
    const QString newValue = powerLevelToString.at(newForcePowerLevel);

    if (daemonConnected())
        dcomm->sendSetValue(newValue, filePaths.forcePowerLevelFilePath);
    else
        setNewValue(filePaths.forcePowerLevelFilePath, newValue);
}

void dXorg::setPwmValue(int value) {
    const QString newValue = QString::number(value);

    if (daemonConnected())
        dcomm->sendSetValue(newValue, filePaths.pwmSpeedPath);
    else
        setNewValue(filePaths.pwmSpeedPath, newValue);
}

void dXorg::setPwmManuaControl(bool manual) {
    const QString mode = QString(manual ? pwm_manual : pwm_auto);

    if (daemonConnected()) //  Tell the daemon to set the pwm mode into the right file
        dcomm->sendSetValue(mode, filePaths.pwmEnablePath);
    else  //  No daemon available
        setNewValue(filePaths.pwmEnablePath, mode);
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

driverFeatures dXorg::figureOutDriverFeatures() {
    driverFeatures features;
    features.temperatureAvailable =  (currentTempSensor != TS_UNKNOWN);

    QString data = getClocksRawData(true);
    gpuClocksStruct test = dXorg::getClocks(data);

    // still, sometimes there is miscomunication between daemon,
    // but vales are there, so look again in the file which daemon has
    // copied to /tmp/
    if (test.coreClk == -1)
        test = getFeaturesFallback();

    features.coreClockAvailable = !(test.coreClk == -1);
    features.memClockAvailable = !(test.memClk == -1);
    features.coreVoltAvailable = !(test.coreVolt == -1);
    features.memVoltAvailable = !(test.memVolt == -1);

    features.pm = currentPowerMethod;

    switch (currentPowerMethod) {
    case DPM: {
        if (daemonConnected())
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
    case PROFILE: {
        QFile f(filePaths.profilePath);
        if (f.open(QIODevice::WriteOnly)) {
            features.canChangeProfile = true;
            f.close();
        }
    }
    case PM_UNKNOWN:
        break;
    }

    if (!filePaths.pwmEnablePath.isEmpty()) {
        QFile f(filePaths.pwmEnablePath);
        f.open(QIODevice::ReadOnly);

        if (QString(f.readLine(2))[0] != pwm_disabled) {
            features.pwmAvailable = true;

            QFile fPwmMax(filePaths.pwmMaxSpeedPath);
            if (fPwmMax.open(QIODevice::ReadOnly)) {
                features.pwmMaxSpeed = fPwmMax.readLine(4).toInt();
                fPwmMax.close();
            }
        }
        f.close();
    }

#ifdef QT_DEBUG
    features.overClockAvailable = QFile::exists(filePaths.overDrivePath);
#else // Overclock is still not tested (it will be fully available only with Linux 4.7/4.8), disable it in release mode
    features.overClockAvailable = false;
#endif

    return features;
}

gpuClocksStruct dXorg::getFeaturesFallback() {
    QFile f("/tmp/radeon_pm_info");
    if (f.open(QIODevice::ReadOnly)) {
        gpuClocksStruct fallbackFeatures;
        QString s = QString(f.readAll());

        // just look for it, if it is, the value is not important at this point
        if (s.contains("sclk"))
            fallbackFeatures.coreClk = 0;
        if (s.contains("mclk"))
            fallbackFeatures.memClk = 0;
        if (s.contains("vddc"))
            fallbackFeatures.coreClk = 0;
        if (s.contains("vddci"))
            fallbackFeatures.memClk = 0;

        f.close();
        return fallbackFeatures;
    } else
        return gpuClocksStruct(-1);
}

bool dXorg::overClock(const int percentage){
    if((percentage > 20) || (percentage < 0))
        qWarning() << "Error overclocking: invalid percentage passed: " << percentage;
    else if (daemonConnected()){ // Signal the daemon to set the overclock value
        dcomm->sendSetValue(QString::number(percentage), filePaths.overDrivePath);
        return true;
    } else if(globalStuff::globalConfig.rootMode){ // Root mode, set it directly
        setNewValue(filePaths.overDrivePath, QString::number(percentage));

        return true;
    } else // Overclock requires root access to sysfs
        qWarning() << "Error overclocking: daemon is not connected and no root access is available";

    return false;
}

void dXorg::resetOverClock(){
    overClock(0);
}
