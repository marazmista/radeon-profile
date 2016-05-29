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
QString dXorg::driverName;
// end //

daemonComm *dcomm = new daemonComm();

void dXorg::configure(QString gpuName) {
    qDebug() << "dXorg: selecting gpu " << gpuName;
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

        reconfigureDaemon();
    }
}

void dXorg::reconfigureDaemon() { // Set up the timer
    dcomm->connectToDaemon();
    if (daemonConnected()) {
        dcomm->sendConfig(filePaths.clocksPath); //  Configure the daemon to read the data

        if (globalStuff::globalConfig.daemonAutoRefresh)
            dcomm->sendTimerOn(globalStuff::globalConfig.interval); // Enable timer
        else
            dcomm->sendTimerOff(); // Disable timer

    } else
        qCritical() << "Daemon is not connected and can't be configured";
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
    filePaths.moduleParamsPath = devicePath + "driver/module/holders/"+driverName+"/parameters/";
    filePaths.clocksPath = "/sys/kernel/debug/dri/"+QString(gpuSysIndex)+"/"+driverName+"_pm_info"; // this path contains only index
    filePaths.GPUoverDrivePath = devicePath + file_GPUoverclockLevel;
    filePaths.memoryOverDrivePath = devicePath + file_memoryOverclockLevel;


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

    const QString out = QString(data).simplified();
    if (out.isEmpty())
        qWarning() << "No data was found";
    else
        qDebug() << out;

    return out;
}

gpuClocksStruct dXorg::getClocks(const QString &data) {
    gpuClocksStruct tData; // empty struct

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
        if (!rx.cap(0).isEmpty()){
            tData.powerLevel = rx.cap(0).split(' ')[2].toShort();
            tData.powerLevelOk = tData.powerLevel != -1;
        }

        rx.setPattern("sclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.coreClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.coreClkOk = tData.coreClk > 0;
        }

        rx.setPattern("mclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.memClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.memClkOk = tData.memClk > 0;
        }

        rx.setPattern("vclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdCClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdCClkOk = tData.uvdCClk > 0;
        }

        rx.setPattern("dclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdDClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdDClkOk = tData.uvdDClk > 0;
        }

        rx.setPattern("vddc:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();
            tData.coreVoltOk = tData.coreVolt > 0;
        }

        rx.setPattern("vddci:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();
            tData.memVoltOk = tData.memVolt > 0;
        }

        return tData;
        break;
    }
    case PROFILE: {
        const QStringList clocksData = data.split("\n");
        const int count = clocksData.count();

        if ((1 < count) && clocksData[1].contains("current engine clock")) {
            tData.coreClk = clocksData[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000;
            tData.coreClkOk = tData.coreClk > 0;
        }

        if ((3 < count) && clocksData[3].contains("current memory clock")) {
            tData.memClk = clocksData[3].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000;
            tData.memClkOk = tData.memClk > 0;
        }

        if ((4 < count) && clocksData[4].contains("voltage")) {
            tData.coreVolt = clocksData[4].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat();
            tData.coreVoltOk = tData.coreVolt > 0;
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
        if (out.indexOf(QRegExp(driverName + "-pci.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp(driverName + "-pci.+"));  // in order to not search for it again in timer loop
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
    QStringList modInfo = globalStuff::grabSystemInfo("modinfo -p " + driverName);
    modInfo.sort();

    for (const QString line : modInfo) {
        if (line.contains(":") && ! line.startsWith("modinfo: ERROR: ")) {
            // show nothing in case of an error

            // read module param name and description from modinfo command
            const QString modName = line.split(":",QString::SkipEmptyParts)[0], // Module name
                    modDesc = line.split(":",QString::SkipEmptyParts)[1]; // Module description

            // read current param values
            QFile mp(filePaths.moduleParamsPath+modName);
            const QString modValue = (mp.open(QIODevice::ReadOnly)) ?  mp.readLine(20) : tr("Unknown"); // Module value

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.simplified() << modValue.simplified() << modDesc.simplified());
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
            if (QString(f.readLine(50)).contains("DRIVER=" + driverName))
                data.append(f.fileName().split('/').at(4));
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
            return tr("No info");
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

    return tr("No info");
}

QString dXorg::getCurrentPowerLevel() {
    QFile forceProfile(filePaths.forcePowerLevelFilePath);
    if (forceProfile.open(QIODevice::ReadOnly))
        return QString(forceProfile.readLine(13));

    return tr("No info");
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
    if ( ! test.coreClkOk)
        test = getFeaturesFallback();

    features.coreClockAvailable = test.coreClkOk;
    features.memClockAvailable = test.memClkOk;
    features.coreVoltAvailable = test.coreVoltOk;
    features.memVoltAvailable = test.memVoltOk;

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
    features.GPUoverClockAvailable = QFile::exists(filePaths.GPUoverDrivePath);
    features.memoryOverclockAvailable = QFile::exists(filePaths.memoryOverDrivePath);
    // Overclock is still not tested (it will be fully available only with Linux 4.7/4.8), disable it in release mode
#endif

    return features;
}

gpuClocksStruct dXorg::getFeaturesFallback() {
    QFile f("/tmp/" + driverName + "_pm_info");
    gpuClocksStruct fallbackFeatures;

    if (f.open(QIODevice::ReadOnly)) {
        const QString s = QString(f.readAll());

        // just look for it, if it is, the value is not important at this point
        if (s.contains("sclk"))
            fallbackFeatures.coreClkOk = true;
        if (s.contains("mclk"))
            fallbackFeatures.memClkOk = true;
        if (s.contains("vddc"))
            fallbackFeatures.coreVoltOk = true;
        if (s.contains("vddci"))
            fallbackFeatures.memVoltOk = true;

        f.close();
    }

    return fallbackFeatures;
}

bool dXorg::overClockGPU(const int percentage){
    if((percentage > 20) || (percentage < 0))
        qWarning() << "Error overclocking GPU: invalid percentage passed: " << percentage;
    else if (daemonConnected()){ // Signal the daemon to set the overclock value
        dcomm->sendSetValue(QString::number(percentage), filePaths.GPUoverDrivePath);
        return true;
    } else if(globalStuff::globalConfig.rootMode){ // Root mode, set it directly
        setNewValue(filePaths.GPUoverDrivePath, QString::number(percentage));

        return true;
    } else // Overclock requires root access to sysfs
        qWarning() << "Error overclocking GPU: daemon is not connected and root access is not available";

    return false;
}

bool dXorg::overClockMemory(const int percentage){
    if((percentage > 20) || (percentage < 0))
        qWarning() << "Error overclocking memory: invalid percentage passed: " << percentage;
    else if (daemonConnected()){ // Signal the daemon to set the overclock value
        dcomm->sendSetValue(QString::number(percentage), filePaths.memoryOverDrivePath);
        return true;
    } else if(globalStuff::globalConfig.rootMode){ // Root mode, set it directly
        setNewValue(filePaths.memoryOverDrivePath, QString::number(percentage));

        return true;
    } else // Overclock requires root access to sysfs
        qWarning() << "Error overclocking memory: daemon is not connected and root access is not available";

    return false;
}
