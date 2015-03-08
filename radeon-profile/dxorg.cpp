// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "globalStuff.h"

#include <QFile>
#include <QTextStream>

// define static members //
dXorg::tempSensor dXorg::currentTempSensor = dXorg::TS_UNKNOWN;
globalStuff::powerMethod dXorg::currentPowerMethod;
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
        // create the shared mem block. The if comes from that configure method
        // is called on every change gpu, so later, shared mem already exists
        if (!sharedMem.isAttached()) {
            sharedMem.setKey("radeon-profile");
           if (!sharedMem.create(128))
               qDebug() << sharedMem.errorString();
           if (!sharedMem.attach())
               qDebug() << sharedMem.errorString();
        }

        dcomm->connectToDaemon();
        if (daemonConnected()) {
            // sending card index and timer interval if selected
            dcomm->sendCommand(dcomm->daemonSignal.config + gpuSysIndex + ((globalStuff::globalConfig.daemonAutoRefresh) ? dcomm->daemonSignal.timer_on + QString().setNum(globalStuff::globalConfig.interval) : ""));
        }
    }
}

void dXorg::reconfigureDaemon() {
    if (daemonConnected()) {
        if (globalStuff::globalConfig.daemonAutoRefresh)
            dcomm->sendCommand(dcomm->daemonSignal.timer_on + QString().setNum(globalStuff::globalConfig.interval));
        else
            dcomm->sendCommand(dcomm->daemonSignal.timer_off);
    }
}

bool dXorg::daemonConnected() {
    return dcomm->connected();
}

void dXorg::figureOutGpuDataFilePaths(QString gpuName) {
    gpuSysIndex = gpuName.at(gpuName.length()-1);
    filePaths.powerMethodFilePath = "/sys/class/drm/"+gpuName+"/device/power_method",
            filePaths.profilePath = "/sys/class/drm/"+gpuName+"/device/power_profile",
            filePaths.dpmStateFilePath = "/sys/class/drm/"+gpuName+"/device/power_dpm_state",
            filePaths.clocksPath = "/sys/kernel/debug/dri/"+QString(gpuSysIndex)+"/radeon_pm_info", // this path contains only index
            filePaths.forcePowerLevelFilePath = "/sys/class/drm/"+gpuName+"/device/power_dpm_force_performance_level",
            filePaths.moduleParamsPath = "/sys/class/drm/"+gpuName+"/device/driver/module/holders/radeon/parameters/";

    QString hwmonDevice = globalStuff::grabSystemInfo("ls /sys/class/drm/"+gpuName+"/device/hwmon/")[0]; // look for hwmon devices in card dir
    filePaths.sysfsHwmonPath = "/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/temp1_input";

    if (QFile::exists("/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/pwm1_enable"))
        filePaths.pwmEnablePath = "/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/pwm1_enable";
    else
        filePaths.pwmEnablePath = "";

    if (QFile::exists("/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/pwm1"))
        filePaths.pwmSpeedPath = "/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/pwm1";
}

// method for gather info about clocks from deamon or from debugfs if root
QString dXorg::getClocksRawData() {
    QFile clocksFile(filePaths.clocksPath);
    QString data;

    if (clocksFile.open(QIODevice::ReadOnly)) // check for debugfs access
            data = QString(clocksFile.readAll());
    else if (daemonConnected()) {
        if (!globalStuff::globalConfig.daemonAutoRefresh)
            dcomm->sendCommand(dcomm->daemonSignal.read_clocks);

       if (sharedMem.lock()) {
//     if (sharedMem.error() == QSharedMemory::NoError) {
            char *to = (char*)sharedMem.constData();
            char a[128] = {0};
            strncpy(a,to,sizeof(a));
            sharedMem.unlock();
            data  = QString::fromAscii(a).trimmed();
        } else
            qDebug() << sharedMem.errorString();
    }

    if (data == "")
        data = "null\n";

    return data;
}

globalStuff::gpuClocksStruct dXorg::getClocks() {
    globalStuff::gpuClocksStruct tData(-1); // empty struct

    QStringList out = getClocksRawData().split('\n');

    // if nothing is there returns empty (-1) struct
    if (out[0] == "null")
        return tData;

    switch (currentPowerMethod) {
    case globalStuff::DPM: {
        QRegExp rx;

        rx.setPattern("power\\slevel\\s\\d");
        rx.indexIn(out[1]);
        if (!rx.cap(0).isEmpty())
            tData.powerLevel = rx.cap(0).split(' ')[2].toShort();

        rx.setPattern("sclk:\\s\\d+");
        rx.indexIn(out[1]);
        if (!rx.cap(0).isEmpty())
            tData.coreClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("mclk:\\s\\d+");
        rx.indexIn(out[1]);
        if (!rx.cap(0).isEmpty())
            tData.memClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("vclk:\\s\\d+");
        rx.indexIn(out[0]);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdCClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdCClk  = (tData.uvdCClk  == 0) ? -1 :  tData.uvdCClk;
        }

        rx.setPattern("dclk:\\s\\d+");
        rx.indexIn(out[0]);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdDClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdDClk = (tData.uvdDClk == 0) ? -1 : tData.uvdDClk;
        }

        rx.setPattern("vddc:\\s\\d+");
        rx.indexIn(out[1]);
        if (!rx.cap(0).isEmpty())
            tData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        rx.setPattern("vddci:\\s\\d+");
        rx.indexIn(out[1]);
        if (!rx.cap(0).isEmpty())
            tData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        return tData;
        break;
    }
    case globalStuff::PROFILE: {
        for (int i=0; i< out.count(); i++) {
            switch (i) {
            case 1: {
                if (out[i].contains("current engine clock")) {
                    tData.coreClk = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            };
            case 3: {
                if (out[i].contains("current memory clock")) {
                    tData.memClk = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            }
            case 4: {
                if (out[i].contains("voltage")) {
                    tData.coreVolt = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toDouble();
                    break;
                }
            }
            }
        }
        return tData;
        break;
    }
    case globalStuff::PM_UNKNOWN: {
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
        QFile hwmon(filePaths.sysfsHwmonPath);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        return temp.toDouble() / 1000;
        break;
    }
    case PCI_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        return temp.toDouble();
        break;
    }
    case MB_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        return temp.toDouble();
        break;
    }
    case TS_UNKNOWN: {
        return -1;
        break;
    }
    }
}

QList<QTreeWidgetItem *> dXorg::getCardConnectors() {
    QList<QTreeWidgetItem *> cardConnectorsList;
    QStringList out = globalStuff::grabSystemInfo("xrandr -q --verbose"), screens;
    screens = out.filter(QRegExp("Screen\\s\\d"));
    for (int i = 0; i < screens.count(); i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << screens[i].split(':')[0] << screens[i].split(",")[1].remove(" current "));
        cardConnectorsList.append(item);
    }
    cardConnectorsList.append(new QTreeWidgetItem(QStringList() << "------"));

    for(int i = 0; i < out.size(); i++) {
        if(!out[i].startsWith("\t") && out[i].contains("connected")) {
            QString connector = out[i].split(' ')[0],
                    status = out[i].split(' ')[1],
                    res = out[i].split(' ')[2].split('+')[0];

            if(status == "connected") {
                QString monitor, edid = monitor = "";

                // find EDID
                for (int i = out.indexOf(QRegExp(".+EDID.+"))+1; i < out.count(); i++)
                    if (out[i].startsWith(("\t\t")))
                        edid += out[i].remove("\t\t");
                    else
                        break;

                // Parse EDID
                // See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
                if(edid.size() >= 256) {
                    QStringList hex;
                    bool found = false, ok = true;
                    int i2 = 108;

                    for(int i3 = 0; i3 < 4; i3++) {
                        if(edid.mid(i2, 2).toInt(&ok, 16) == 0 && ok &&
                                edid.mid(i2 + 2, 2).toInt(&ok, 16) == 0) {
                            // Other Monitor Descriptor found
                            if(ok && edid.mid(i2 + 6, 2).toInt(&ok, 16) == 0xFC && ok) {
                                // Monitor name found
                                for(int i4 = i2 + 10; i4 < i2 + 34; i4 += 2)
                                    hex << edid.mid(i4, 2);
                                found = true;
                                break;
                            }
                        }
                        if(!ok)
                            break;
                        i2 += 36;
                    }
                    if(ok && found) {
                        // Hex -> String
                        for(i2 = 0; i2 < hex.size(); i2++) {
                            monitor += QString(hex[i2].toInt(&ok, 16));
                            if(!ok)
                                break;
                        }

                        if(ok)
                            monitor = " (" + monitor.left(monitor.indexOf('\n')) + ")";
                    }
                }
                status += monitor + " @ " + QString((res.contains('x')) ? res : "unknown");
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << connector << status);
            cardConnectorsList.append(item);
        }
    }
    return cardConnectorsList;
}


globalStuff::powerMethod dXorg::getPowerMethod() {
    QFile powerMethodFile(filePaths.powerMethodFilePath);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            return globalStuff::DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            return globalStuff::PROFILE;
        else
            return globalStuff::PM_UNKNOWN;
    } else
        return globalStuff::PM_UNKNOWN;
}

dXorg::tempSensor dXorg::testSensor() {
    QFile hwmon(filePaths.sysfsHwmonPath);

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty())
            return CARD_HWMON;
    } else {

        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        filePaths.sysfsHwmonPath = findSysfsHwmonForGPU();
        if (!filePaths.sysfsHwmonPath.isEmpty())
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
        } else
            return TS_UNKNOWN;
    }
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

QStringList dXorg::getGLXInfo(QString gpuName, QProcessEnvironment env) {
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
    QFile forceProfile(filePaths.forcePowerLevelFilePath);
    QString pp, err = "err";

    switch (currentPowerMethod) {
    case globalStuff::DPM: {
        QFile profile(filePaths.dpmStateFilePath);
        if (profile.open(QIODevice::ReadOnly)) {
            pp = profile.readLine(13);
            if (forceProfile.open(QIODevice::ReadOnly))
                pp += " | " + forceProfile.readLine(5);
        } else
            pp = err;
        break;
    }
    case globalStuff::PROFILE: {
        QFile profile(filePaths.profilePath);
        if (profile.open(QIODevice::ReadOnly))
            pp = profile.readLine(13);
        break;
    }
    case globalStuff::PM_UNKNOWN: {
        pp = err;
        break;
    }
    }
    return pp.remove('\n');
}

void dXorg::setNewValue(const QString &filePath, const QString &newValue) {
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&file);
    stream << newValue + "\n";
    file.flush();
    file.close();
}

void dXorg::setPowerProfile(globalStuff::powerProfiles _newPowerProfile) {
    QString newValue;
    switch (_newPowerProfile) {
    case globalStuff::BATTERY:
        newValue = "battery";
        break;
    case globalStuff::BALANCED:
        newValue = "balanced";
        break;
    case globalStuff::PERFORMANCE:
        newValue = "performance";
        break;
    case globalStuff::AUTO:
        newValue = "auto";
        break;
    case globalStuff::DEFAULT:
        newValue = "default";
        break;
    case globalStuff::HIGH:
        newValue = "high";
        break;
    case globalStuff::MID:
        newValue = "mid";
        break;
    case globalStuff::LOW:
        newValue = "low";
        break;
    default: break;
    }

    if (daemonConnected())
        dcomm->sendCommand(dcomm->daemonSignal.set_profile + newValue);
    else {
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
        newValue = "auto";
        break;
    case globalStuff::F_HIGH:
        newValue = "high";
        break;
    case globalStuff::F_LOW:
        newValue = "low";
    default:
        break;
    }

    if (daemonConnected())
        dcomm->sendCommand(dcomm->daemonSignal.force_pl + newValue);
    else
        setNewValue(filePaths.forcePowerLevelFilePath, newValue);
}

void dXorg::setPwmValue(int value) {
//    if (daemonConnected())  {
//        ;//TODO
//    } else {
        setNewValue(filePaths.pwmSpeedPath,QString().setNum(value));
  //  }
}

void dXorg::setPwmManuaControl(bool manual) {
    if (manual)
       setNewValue(filePaths.pwmEnablePath,"1");
    else
        setNewValue(filePaths.pwmEnablePath,"2");
}

int dXorg::getPwmSpeed() {
    QFile f(filePaths.pwmSpeedPath);

    if (f.open(QIODevice::ReadOnly))
        return QString(f.readLine(4)).toInt();
}

globalStuff::driverFeatures dXorg::figureOutDriverFeatures() {
    globalStuff::driverFeatures features;

    features.temperatureAvailable =  (currentTempSensor == dXorg::TS_UNKNOWN) ? false : true;

    globalStuff::gpuClocksStruct test = dXorg::getClocks();
    features.clocksAvailable = (test.coreClk == -1) ? false : true;
    features.voltAvailable = (test.coreVolt == -1) ? false : true;

    features.pm = currentPowerMethod;
    features.canChangeProfile = false;
    switch (currentPowerMethod) {
    case globalStuff::DPM: {
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

    features.pwmAvailable = false;
    if (filePaths.pwmEnablePath != "") {
        QFile f(filePaths.pwmEnablePath);
        f.open(QIODevice::ReadOnly);
        if (QString(f.readLine(1)) != "0")
            features.pwmAvailable = true;
    }
    return features;
}
