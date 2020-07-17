// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "gpu.h"
#include "radeon_profile.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QStringList>

dXorg::dXorg(const GPUSysInfo &si, const InitializationConfig &config) : ioctlHnd(nullptr) {
    features.sysInfo = si;
    initConfig = config;
    configure();
    figureOutConstParams();
}

void dXorg::configure() {
    figureOutGpuDataFilePaths(features.sysInfo.sysName);
    setupIoctl();

    if (initConfig.rootMode) {
        figureOutDriverFeatures();
        return;
    }

    if (radeon_profile::dcomm.isConnected() && initConfig.daemonData) {
        qDebug() << "Confguring shared memory for daemon";
        setupSharedMem();
        sendSharedMemInfoToDaemon();

        // fist call after setup to read pm_file, so notihing is in sharedmem and we need to wait for data
        // because we need correctly figure out what is available
        // see: https://stackoverflow.com/a/11487434/2347196
        QTime delayTime = QTime::currentTime().addMSecs(1200);
        qDebug() << "Waiting for first daemon data read...";
        while (QTime::currentTime() < delayTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    figureOutDriverFeatures();
}

dXorg::InitializationConfig dXorg::getInitConfig() {
    return initConfig;
}

void dXorg::setupIoctl() {
    if (features.sysInfo.module == DriverModule::RADEON)
        ioctlHnd = new radeonIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
    else if (features.sysInfo.module == DriverModule::AMDGPU)
        ioctlHnd = new amdgpuIoctlHandler(features.sysInfo.sysName[4].toLatin1() - '0');
}

QString getValueFromSysFsFile(QString fileName) {
    QFile f(fileName);
    QString value("-1");

    if (f.open(QIODevice::ReadOnly))
        value = QString(f.readAll()).trimmed();

    f.close();
    return value;
}


//https://stackoverflow.com/a/18866593
QString getRandomString() {
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

   QString randomString;
   qsrand(QTime::currentTime().msecsSinceStartOfDay());

   for (int i = 0; i < 10; ++i)
       randomString.append(possibleCharacters.at(qrand() % possibleCharacters.length()));

   return randomString;
}

void dXorg::setupSharedMem() {
    qDebug() << "Configure shared mem";

    // create the shared mem block. The if comes from that configure method
    // is called on every change gpu, so later, shared mem already exists
    if (sharedMem.isAttached())
        return;

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

void dXorg::sendSharedMemInfoToDaemon() {
    QString command;
    command.append(DAEMON_SIGNAL_CONFIG).append(SEPARATOR);
    command.append("pm_info").append(SEPARATOR).append(driverFiles.debugfs_pm_info).append(SEPARATOR);
    command.append(DAEMON_SHAREDMEM_KEY).append(SEPARATOR).append(sharedMem.key()).append(SEPARATOR);

    if (!initConfig.daemonAutoRefresh)
        command.append(DAEMON_SIGNAL_READ_CLOCKS).append(SEPARATOR);

    qDebug() << "Sending daemon shared mem info: " << command;
    radeon_profile::dcomm.sendCommand(command);
}

void dXorg::figureOutGpuDataFilePaths(const QString &gpuName) {
    QString devicePath = "/sys/class/drm/" + gpuName + "/device/";
    driverFiles.moduleParams = devicePath + "driver/module/parameters/";
    driverFiles.debugfs_pm_info = "/sys/kernel/debug/dri/" + gpuName.right(1) + "/"+features.sysInfo.driverModuleString + "_pm_info"; // this path contains only index
    driverFiles.sysFs = DeviceSysFs(devicePath);

    // look for hwmon devices in card dir
    QString hwmonDevicePath = globalStuff::grabSystemInfo("ls "+ devicePath + "hwmon/")[0];

    hwmonDevicePath =  devicePath + "hwmon/" + ((hwmonDevicePath.isEmpty() ? "hwmon0/" : hwmonDevicePath + "/"));

    driverFiles.hwmonAttributes = HwmonAttributes(hwmonDevicePath);

    qDebug() << "hwmon path: " << hwmonDevicePath;
}

// method for gather info about clocks from deamon or from debugfs if root
QString dXorg::getClocksRawData() {
    QString data;

    data = getValueFromSysFsFile(driverFiles.debugfs_pm_info);
    if (data != "-1")
        return data;

    if (radeon_profile::dcomm.isConnected()) {
        if (!initConfig.daemonAutoRefresh){
            qDebug() << "Asking the daemon to read clocks";
            radeon_profile::dcomm.sendCommand(QString(DAEMON_SIGNAL_READ_CLOCKS).append(SEPARATOR)); // SIGNAL_READ_CLOCKS + SEPARATOR
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
                            clocksData.coreClk = QString::number(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                        break;

                    case 3:
                        if (dataStr[i].contains("current memory clock"))
                            clocksData.memClk = QString::number(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toFloat();
                        break;

                    case 4:
                        if (dataStr[i].contains("voltage"))
                            clocksData.coreVolt = QString::number(dataStr[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toFloat();
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
        case TemperatureSensor::CARD_HWMON:
            return getValueFromSysFsFile(driverFiles.hwmonAttributes.temp1).toFloat() / 1000;
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
        case TemperatureSensor::TS_UNKNOWN:
            temp = "-1";
            break;
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

PowerMethod dXorg::getPowerMethodFallback() {
    QString s = getValueFromSysFsFile(driverFiles.sysFs.power_method);

    if (s.contains("dpm",Qt::CaseInsensitive))
        return PowerMethod::DPM;
    else if (s.contains("profile",Qt::CaseInsensitive))
        return PowerMethod::PROFILE;
    else
        return PowerMethod::PM_UNKNOWN;
}

TemperatureSensor dXorg::getTemperatureSensor() {

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    QString tmpValue = getValueFromSysFsFile(driverFiles.hwmonAttributes.temp1);
    if (!tmpValue.isEmpty() || tmpValue != "-1")
        return TemperatureSensor::CARD_HWMON;
    else {
        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        driverFiles.hwmonAttributes.temp1 = findSysfsHwmonForGPU();
        if (!driverFiles.hwmonAttributes.temp1.isEmpty())
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
            QString file("/sys/class/hwmon/"+hwmonDev[i]+"/device/"+temp[o]);
            QString s = getValueFromSysFsFile(file);
            if (!s.isEmpty() && s.contains("VGA_TEMP"))
                return file.replace("label", "input");
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
    switch (features.currentPowerMethod) {
        case PowerMethod::DPM:
            return getValueFromSysFsFile(driverFiles.sysFs.power_dpm_state);

        case PowerMethod::PROFILE:
            return getValueFromSysFsFile(driverFiles.sysFs.power_profile);

        case PowerMethod::PP_MODE:
            return getCurrentPowerProfileMode();

        default:
            return "err";
    }
}

QString dXorg::getCurrentPowerLevel() {
    return getValueFromSysFsFile(driverFiles.sysFs.power_dpm_force_performance_level);
}

void dXorg::setNewValue(const QString &filePath, const QString &newValue) {
    if (radeon_profile::dcomm.isConnected())
        radeon_profile::dcomm.sendCommand(createDaemonSetCmd(filePath, newValue));
    else {
        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << newValue + "\n";

            if (!file.flush())
                qWarning() << "Failed to flush " << filePath;

            file.close();

        } else
            qWarning() << "Unable to open " << filePath << " to write " << newValue;
    }
}

void dXorg::setNewValue(const QString &filePath, const QStringList &newValues) {
    if (radeon_profile::dcomm.isConnected()) {
        QString cmd;

        for (auto &value : newValues)
            cmd.append(createDaemonSetCmd(filePath, value));

        radeon_profile::dcomm.sendCommand(cmd);
    } else {
        QFile file(filePath);
        QTextStream stream(&file);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            for (auto &value : newValues) {
                stream << value + "\n";
                if (!file.flush())
                    qWarning() << "Failed to flush " << filePath;
            }
            file.close();
        } else
            qWarning() << "Unable to open " << filePath;
    }
}

void dXorg::setPowerProfile(const QString &newPowerProfile) {
    switch (features.currentPowerMethod) {
        case PowerMethod::DPM:
            setNewValue(driverFiles.sysFs.power_dpm_state, features.powerProfiles.at(newPowerProfile.toInt()).name);
            break;
        case PowerMethod::PP_MODE:
            setNewValue(driverFiles.sysFs.pp_power_profile_mode, newPowerProfile);
            break;
        case PowerMethod::PROFILE:
            setNewValue(driverFiles.sysFs.power_profile, features.powerProfiles.at(newPowerProfile.toInt()).name);
            break;
        default:
            break;
    }
}

void dXorg::setForcePowerLevel(const QString &newForcePowerLevel) {
    setNewValue(driverFiles.sysFs.power_dpm_force_performance_level, newForcePowerLevel);
}

GPUFanSpeed dXorg::getFanSpeed() {
    GPUFanSpeed tmp;

    if (driverFiles.hwmonAttributes.pwm1.isEmpty())
        return tmp;

    tmp.fanSpeedPercent = (getValueFromSysFsFile(driverFiles.hwmonAttributes.pwm1).toFloat() / params.pwmMaxSpeed) * 100;

    if (!driverFiles.hwmonAttributes.fan1_input.isEmpty())
        tmp.fanSpeedRpm = getValueFromSysFsFile(driverFiles.hwmonAttributes.fan1_input).toInt();

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

        case DriverModule::MODULE_UNKNOWN:
            return;
    }
}

void dXorg::figureOutDriverFeatures() {
    features.isDpmStateAvailable = !driverFiles.sysFs.power_dpm_state.isEmpty();
    features.isPowerProfileModesAvailable = !driverFiles.sysFs.pp_power_profile_mode.isEmpty();

    features.currentPowerMethod = getPowerMethodFallback();
    qDebug() << "Detected power method based on sysfs power_method file:" << features.currentPowerMethod;

    if (features.isDpmStateAvailable)
        features.currentPowerMethod = PowerMethod::DPM;

    if (features.isPowerProfileModesAvailable)
        features.currentPowerMethod = PowerMethod::PP_MODE;

    features.powerProfiles = getPowerProfiles(features.currentPowerMethod);

    if (getIoctlAvailability() && !initConfig.daemonData)
        features.clocksDataSource = ClocksDataSource::IOCTL;
    else {
        features.clocksDataSource = ClocksDataSource::PM_FILE;
        QString data = getClocksRawData();
        setupRegex(data);
        GPUClocks test = getClocks();

        // still, sometimes there is miscomunication between daemon,
        // but vales are there, so look again in the file which daemon has
        // copied to /tmp/
        if (test.coreClk == -1)
            test = getFeaturesFallback();
    }

    auto checkWritePermission = [](QString fileName) {
        QFile f(fileName);
        bool isWritePossible = false;

        if (f.open(QIODevice::WriteOnly)) {
            isWritePossible = true;
            f.close();
        }

        return isWritePossible;
    };

    if (initConfig.rootMode || radeon_profile::dcomm.isConnected())
        features.isChangeProfileAvailable = true;
    else {
        switch (features.currentPowerMethod) {
            case PowerMethod::DPM:
                qDebug() << "Power method: DPM";
                features.isChangeProfileAvailable = checkWritePermission(driverFiles.sysFs.power_dpm_state);
                break;
           case PowerMethod::PROFILE:
                qDebug() << "Power method: Profile";
                features.isChangeProfileAvailable = checkWritePermission(driverFiles.sysFs.power_profile);;
                break;
            case PowerMethod::PP_MODE:
                qDebug() << "Power method: Power Profile Modes";
                features.isChangeProfileAvailable = checkWritePermission(driverFiles.sysFs.pp_power_profile_mode);;
                break;
            default:
                qDebug() << "Power method unknown";
                break;
        }
    }

    features.currentTemperatureSensor = getTemperatureSensor();

    features.isFanControlAvailable = !driverFiles.hwmonAttributes.pwm1.isEmpty();

    features.isPercentCoreOcAvailable = !driverFiles.sysFs.pp_sclk_od.isEmpty();
    features.isPercentMemOcAvailable = !driverFiles.sysFs.pp_mclk_od.isEmpty();

    refreshPowerPlayTables();
    features.isDpmCoreFreqTableAvailable = features.sclkTable.count() > 0;
    features.isDpmMemFreqTableAvailable = features.mclkTable.count() > 0;

    features.isPowerCapAvailable = !driverFiles.hwmonAttributes.power1_cap.isEmpty();

    if (!driverFiles.sysFs.pp_od_clk_voltage.isEmpty()) {
        features.isOcTableAvailable = true;

        readOcTableAndRanges();

        features.isVDDCCurveAvailable = features.currentStatesTables.contains(OD_VDDC_CURVE);
    }
}

void dXorg::refreshPowerPlayTables()
{
    if (!driverFiles.sysFs.pp_dpm_sclk.isEmpty())
        features.sclkTable = loadPowerPlayTable(driverFiles.sysFs.pp_dpm_sclk);

    if (!driverFiles.sysFs.pp_dpm_mclk.isEmpty())
        features.mclkTable = loadPowerPlayTable(driverFiles.sysFs.pp_dpm_mclk);
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

    if (!driverFiles.hwmonAttributes.temp1_crit.isEmpty())
        params.temp1_crit = getValueFromSysFsFile(driverFiles.hwmonAttributes.temp1_crit).toInt() / 1000;


    if (!driverFiles.hwmonAttributes.pwm1_max.isEmpty())
        params.pwmMaxSpeed = getValueFromSysFsFile(driverFiles.hwmonAttributes.pwm1_max).toInt();

    if (!driverFiles.hwmonAttributes.power1_cap_min.isEmpty())
        params.power1_cap_min = getValueFromSysFsFile(driverFiles.hwmonAttributes.power1_cap_min).toInt() / MICROWATT_DIVIDER;

    if (!driverFiles.hwmonAttributes.power1_cap_max.isEmpty())
        params.power1_cap_max = getValueFromSysFsFile(driverFiles.hwmonAttributes.power1_cap_max).toInt() / MICROWATT_DIVIDER;

}

GPUClocks dXorg::getFeaturesFallback() {
    GPUClocks fallbackfeatures;
    QString s = getValueFromSysFsFile("/tmp/"+features.sysInfo.driverModuleString+"_pm_info");

    // just look for it, if it is, the value is not important at this point
    if (s.contains("sclk"))
        fallbackfeatures.coreClk = 0;
    if (s.contains("mclk"))
        fallbackfeatures.memClk = 0;
    if (s.contains("vddc"))
        fallbackfeatures.coreVolt = 0;
    if (s.contains("vddci"))
        fallbackfeatures.memVolt = 0;

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

QStringList dXorg::loadPowerPlayTable(const QString &file) {
    QStringList ppt;
    QStringList sl = getValueFromSysFsFile(file).split('\n', QString::SkipEmptyParts);

    if (sl.isEmpty() || sl.at(0) == "-1")
        return ppt;

    for (QString &s : sl)
        ppt.append(s.remove("*").trimmed());

    return ppt;
}

int dXorg::getCurrentPowerPlayTableId(const QString &file) {

    QStringList sl = getValueFromSysFsFile(file).split(QString::SkipEmptyParts);

    if (sl.isEmpty() || sl.at(0) == "-1")
        return 0;

    for (int i = 0; i < sl.count(); ++i) {
        if (sl[i].endsWith("*"))
            return i;
    }

    return 0;
}

int dXorg::getPowerCapSelected() const {
    return getValueFromSysFsFile(driverFiles.hwmonAttributes.power1_cap).toInt() / MICROWATT_DIVIDER;
}

int dXorg::getPowerCapAverage() const {
    return getValueFromSysFsFile(driverFiles.hwmonAttributes.power1_average).toInt() / MICROWATT_DIVIDER;
}

const std::tuple<MapFVTables, MapOCRanges> dXorg::parseOcTable() {
    MapFVTables tables;
    MapOCRanges ocRanges;

    QStringList sl = getValueFromSysFsFile(driverFiles.sysFs.pp_od_clk_voltage).remove(' ')
            .replace(QRegularExpression(":|MHz|mV", QRegularExpression::CaseInsensitiveOption), "|").split('\n');

    // OD_VDDC_CURVE only in Vega20+
    bool vega20Mode = sl.contains(QString(OD_VDDC_CURVE).append('|'));

    for (auto i = 0; i < sl.length(); ++i) {

        if (vega20Mode) {
            if (sl.at(i).contains(OD_SCLK)) {
                QStringList stateMin = sl[++i].split("|", QString::SkipEmptyParts);
                QStringList stateMax = sl[++i].split("|", QString::SkipEmptyParts);
                ocRanges.insert(OD_SCLK, OCRange(stateMin[1].toUInt(), stateMax[1].toUInt()));
                continue;
            }

            if (sl.at(i).contains(OD_MCLK)) {
                QStringList stateMax = sl[++i].split("|", QString::SkipEmptyParts);
                ocRanges.insert(OD_MCLK, OCRange(0, stateMax[1].toUInt()));
                continue;
            }
        }

        QString tableKey;
        for (; i < sl.length(); ++i) {
            auto tableItems = sl[i].split("|", QString::SkipEmptyParts);

            if (tableItems.length() == 1) {

                if (tableItems[0] == OD_RANGE) {
                    for (++i; i < sl.length(); ++i) {

                        auto state = sl[i].split("|", QString::SkipEmptyParts);
                        if (state.length() == 1)
                            break;

                        ocRanges.insert(state[0], OCRange(state[1].toUInt(), state[2].toUInt()));
                    }
                } else
                    tableKey = tableItems[0]; // next table key

            } else {

                FVTable fvt;
                for (; i < sl.length(); ++i) {
                    auto state = sl[i].split("|", QString::SkipEmptyParts);

                    if (state.length() == 1) {
                        --i; // go back to next table start
                        break;
                    }

                    fvt.insert(state[0].toUInt(), FreqVoltPair(state[1].toUInt(), state[2].toUInt()));
                }

                tables.insert(tableKey, fvt);
            }
        }
    }

    return std::make_tuple(tables, ocRanges);
}

void dXorg::readOcTableAndRanges() {
    auto ocTable = parseOcTable();

    features.currentStatesTables = std::get<0>(ocTable);
    features.ocRages = std::get<1>(ocTable);
}

void dXorg::setOcTable(const QString &tableType, const FVTable &table) {
    QString ocTableValues;

    for (const auto &s : table.keys()) {
        const FreqVoltPair &fvt = table.value(s);
        ocTableValues.append(tableType + " "+ QString::number(s) + " " + QString::number(fvt.frequency) + " " + QString::number(fvt.voltage));
    }

    setNewValue(driverFiles.sysFs.pp_od_clk_voltage, ocTableValues);
}

PowerProfiles dXorg::getPowerProfiles(const PowerMethod powerMethod) {
    PowerProfiles ppModes;

    switch (powerMethod) {
        case PowerMethod::DPM: {
            qDebug() << "Power method: DPM. Creating profiles list";

            auto currentProfile = getCurrentPowerProfile();

            ppModes.append(PowerProfileDefinition(0, currentProfile == dpm_battery, dpm_battery));
            ppModes.append(PowerProfileDefinition(1, currentProfile == dpm_balanced, dpm_balanced));
            ppModes.append(PowerProfileDefinition(2, currentProfile == dpm_performance, dpm_performance));
        }
            break;

        case PowerMethod::PROFILE: {
            qDebug() << "Power method: Profile. Creating profiles list";

            auto currentProfile = getCurrentPowerProfile();

            ppModes.append(PowerProfileDefinition(0, currentProfile == profile_auto, profile_auto));
            ppModes.append(PowerProfileDefinition(1, currentProfile == profile_default, profile_default));
            ppModes.append(PowerProfileDefinition(3, currentProfile == profile_high, profile_high));
            ppModes.append(PowerProfileDefinition(4, currentProfile == profile_mid, profile_mid));
            ppModes.append(PowerProfileDefinition(5, currentProfile == profile_low, profile_low));
        }
            break;

        case PowerMethod::PP_MODE: {
            qDebug() << "Power method: Power Profile Modes. Creating profiles list";

            QStringList sl = getValueFromSysFsFile(driverFiles.sysFs.pp_power_profile_mode).split('\n');

            for (int i = 1; i < sl.count(); ++i) {
                if (!sl[i].contains(":"))
                    continue;

                QStringList profileLine = sl[i].split(" " , QString::SkipEmptyParts);

                ppModes.append(PowerProfileDefinition(profileLine[0].toUInt(), sl[i].contains("*"), profileLine[1].remove("*").remove(':')));
            }
        }
            break;
        default:
            qDebug() << "Unable to create power profiles list";
            break;
    }

    return ppModes;
}

QString dXorg::getCurrentPowerProfileMode() {
    QStringList sl = getValueFromSysFsFile(driverFiles.sysFs.pp_power_profile_mode).split("\n");

    for (auto &s : sl) {
        if (s.contains('*'))
            return QString(s[0]);
     }

    return "-1";
}
