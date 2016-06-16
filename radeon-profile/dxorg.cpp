// copyright marazmista @ 29.03.2014

#include "dxorg.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QApplication>
#include <QDebug>


dXorg::dXorg(QString module){
    if(module.isEmpty()){
        const QStringList lsmod = globalStuff::grabSystemInfo("lsmod");
        if(!lsmod.filter("radeon").isEmpty())
            driverModule = "radeon";
        else if (!lsmod.filter("amdgpu").isEmpty())
            driverModule = "amdgpu";
    } else
        driverModule = module;

    gpuList = detectCards();

    qDebug() << "Using Xorg " << driverModule << " driver";
}

void dXorg::changeGPU(ushort gpuIndex) {
    if(gpuIndex >= gpuList.size()){
        qWarning() << "Requested unexisting card " << gpuIndex;
        return;
    }

    currentGpuIndex = gpuIndex;
    const QString gpuName = gpuList[gpuIndex];
    qDebug() << "dXorg: selecting gpu " << gpuName;
    figureOutGpuDataFilePaths(gpuName);
    currentTempSensor = testSensor();
    currentPowerMethod = getPowerMethod();

    if (!globalStuff::globalConfig.rootMode) {
        qDebug() << "Running in non-root mode, connecting and configuring the daemon";
        dcomm.attachToMemory();


        reconfigureDaemon(); // Set up timer
    }

    features = figureOutDriverFeatures();
}

void dXorg::reconfigureDaemon() { // Set up the timer
    if(filePaths.clocksPath.isEmpty()){
        qCritical() << "Clocks path is not available, can't connect to the daemon";
        return;
    }

    dcomm.connectToDaemon();
    if (daemonConnected()) {
        dcomm.sendConfig(filePaths.clocksPath); //  Configure the daemon to read the data

        if (globalStuff::globalConfig.daemonAutoRefresh)
            dcomm.sendTimerOn(globalStuff::globalConfig.interval); // Enable timer
        else
            dcomm.sendTimerOff(); // Disable timer

    } else
        qWarning() << "Daemon is not connected, therefore it's timer can't be reconfigured";
}

bool dXorg::daemonConnected() const {
    return dcomm.connected();
}

void dXorg::figureOutGpuDataFilePaths(QString gpuName) {
    const QChar gpuSysIndex = gpuName.at(gpuName.length()-1);
    const QString devicePath = "/sys/class/drm/"+gpuName+"/device/";

    filePaths.powerMethodFilePath = devicePath + file_PowerMethod;
    filePaths.profilePath = devicePath + file_powerProfile;
    filePaths.dpmStateFilePath = devicePath + file_powerDpmState;
    filePaths.forcePowerLevelFilePath = devicePath + file_powerDpmForcePerformanceLevel;

    filePaths.clocksPath = "/sys/kernel/debug/dri/"+QString(gpuSysIndex)+"/"+driverModule+"_pm_info";

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

gpuClocksStruct dXorg::getClocks(bool resolvingGpuFeatures) {
    QFile clocksFile(filePaths.clocksPath);
    QString data;

    if (clocksFile.open(QIODevice::ReadOnly)) // check for debugfs access
        data = QString(clocksFile.readAll().trimmed());
    else if (daemonConnected()) {
        if (!globalStuff::globalConfig.daemonAutoRefresh)
            dcomm.sendReadClocks();

        // fist call, so notihing is in sharedmem and we need to wait for data
        // because we need correctly figure out what is available
        // see: https://stackoverflow.com/a/11487434/2347196
        if (resolvingGpuFeatures) {
            QTime delayTime = QTime::currentTime().addMSecs(1000);
            qDebug() << "Waiting for data...";
            while (QTime::currentTime() < delayTime)
                QApplication::processEvents(QEventLoop::AllEvents, 100);
        }

       data = QString(dcomm.readMemory());
    }

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
            tData.coreClk = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100);
            tData.coreClkOk = tData.coreClk > 0;
        }

        rx.setPattern("mclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.memClk = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100);
            tData.memClkOk = tData.memClk > 0;
        }

        rx.setPattern("vclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdCClk = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100);
            tData.uvdCClkOk = tData.uvdCClk > 0;
        }

        rx.setPattern("dclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdDClk = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100);
            tData.uvdDClkOk = tData.uvdDClk > 0;
        }

        rx.setPattern("vddc:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.coreVolt = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble());
            tData.coreVoltOk = tData.coreVolt > 0;
        }

        rx.setPattern("vddci:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()){
            tData.memVolt = static_cast<short>(rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble());
            tData.memVoltOk = tData.memVolt > 0;
        }
        break;
    }
    case PROFILE: {
        const QStringList clocksData = data.split("\n");
        const int count = clocksData.count();
qDebug() << "Reading profile clocks";
        if ((1 < count) && clocksData[1].contains("current engine clock")) {
            tData.coreClk = static_cast<short>(clocksData[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000);
            tData.coreClkOk = tData.coreClk > 0;
        }

        if ((3 < count) && clocksData[3].contains("current memory clock")) {
            tData.memClk = static_cast<short>(clocksData[3].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000);
            tData.memClkOk = tData.memClk > 0;
        }

        if ((4 < count) && clocksData[4].contains("voltage")) {
            tData.coreVolt = static_cast<short>(clocksData[4].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat());
            tData.coreVoltOk = tData.coreVolt > 0;
        }
        break;
    }
    case PM_UNKNOWN: {
        qWarning() << "Unknown power method, can't get clocks";
        break;
    }
    }
    return tData;
}

float dXorg::getTemperature() const {
    QString temp;

    switch (currentTempSensor) {
    case SYSFS_HWMON:
    case CARD_HWMON: {
        return readFile(filePaths.sysfsHwmonTempPath).toFloat() / 1000;
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
    return temp.toFloat();
}

powerMethod dXorg::getPowerMethod() const {
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
        if (out.indexOf(QRegExp(driverModule+"-pci.+")) != -1) {
            sensorsGPUtempIndex = static_cast<ushort>(out.indexOf(QRegExp(driverModule+"-pci.+")));  // in order to not search for it again in timer loop
            return PCI_SENSOR;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            sensorsGPUtempIndex = static_cast<ushort>(out.indexOf(QRegExp("VGA_TEMP.+")));
            return MB_SENSOR;
        }
    }
    return TS_UNKNOWN;
}

QString dXorg::findSysfsHwmonForGPU() const {
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

// default getGLXInfo() implementation is used

// default getModuleInfo() implementation is used

// default detectCards() implementation is used

QString dXorg::getCurrentPowerProfile() const {
    switch (currentPowerMethod) {
    case DPM: {
        QFile dpmProfile(filePaths.dpmStateFilePath);
        if (dpmProfile.open(QIODevice::ReadOnly))
           return QString(dpmProfile.readLine(13).trimmed());
        break;
    }
    case PROFILE: {
        QFile profile(filePaths.profilePath);
        if (profile.open(QIODevice::ReadOnly))
            return QString(profile.readLine(13).trimmed());
        break;
    }
    case PM_UNKNOWN:
        break;
    }

    return tr("No info");
}

QString dXorg::getCurrentPowerLevel() const {
    QFile forceProfile(filePaths.forcePowerLevelFilePath);
    if (forceProfile.open(QIODevice::ReadOnly))
        return QString(forceProfile.readLine(13).trimmed());

    return tr("No info");
}


void dXorg::setPowerProfile(powerProfiles newPowerProfile) {
    const QString newValue = profileToString.at(newPowerProfile);

    // enum is int, so first three values are dpm, rest are profile
    if (newPowerProfile <= PERFORMANCE)
        sendValue(filePaths.dpmStateFilePath,newValue);
    else
        sendValue(filePaths.profilePath,newValue);
}

void dXorg::setForcePowerLevel(forcePowerLevels newForcePowerLevel) {
    const QString newValue = powerLevelToString.at(newForcePowerLevel);

    sendValue(filePaths.forcePowerLevelFilePath, newValue);
}

void dXorg::setPwmValue(ushort value) {
    const QString newValue = QString::number(value);

    sendValue(filePaths.pwmSpeedPath, newValue);
}

void dXorg::setPwmManualControl(bool manual){
    const QString mode = QString(manual ? pwm_manual : pwm_auto);

    sendValue(filePaths.pwmEnablePath, mode);
}

ushort dXorg::getPwmSpeed() const {
    if(features.pwmMaxSpeed == 0) // Prevent division by zero
        return 100;

    QFile f(filePaths.pwmSpeedPath);

    float val = 0;
    if (f.open(QIODevice::ReadOnly)) {
       val = QString(f.readLine(4)).toFloat();
       f.close();
    }

    return static_cast<ushort>(( val / features.pwmMaxSpeed ) * 100);
}

driverFeatures dXorg::figureOutDriverFeatures() {
    driverFeatures features;
    features.temperatureAvailable =  (currentTempSensor != TS_UNKNOWN);

    gpuClocksStruct test = getClocks(true);

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
        break;
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
                features.pwmMaxSpeed = fPwmMax.readLine(4).toUShort();
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
    const QString s = readFile("/tmp/"+driverModule+"_pm_info");
    gpuClocksStruct fallbackFeatures;

    if(!s.isEmpty()){
        // just look for it, if it is, the value is not important at this point
        if (s.contains("sclk"))
            fallbackFeatures.coreClkOk = true;
        if (s.contains("mclk"))
            fallbackFeatures.memClkOk = true;
        if (s.contains("vddc"))
            fallbackFeatures.coreVoltOk = true;
        if (s.contains("vddci"))
            fallbackFeatures.memVoltOk = true;
    }

    return fallbackFeatures;
}

bool dXorg::overclockGPU(const int percentage){
    if((percentage > 20) || (percentage < 0)){
        qWarning() << "Error overclocking GPU: invalid percentage passed: " << percentage;
        return false;
    }

    return sendValue(filePaths.GPUoverDrivePath, QString::number(percentage));
}

bool dXorg::overclockMemory(const int percentage){
    if((percentage > 20) || (percentage < 0)){
        qWarning() << "Error overclocking memory: invalid percentage passed: " << percentage;
        return false;
    }

    return sendValue(filePaths.memoryOverDrivePath, QString::number(percentage));
}

bool dXorg::sendValue(const QString & filePath, const QString & value){
    if(daemonConnected()){
        dcomm.sendSetValue(value, filePath);
        return true;
    }
    else
        return setNewValue(filePath, value);
}
