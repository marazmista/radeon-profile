// copyright marazmista @ 29.03.2014

#include "gpu.h"

gpu::driver gpu::detectDriver() {
    QStringList out = globalStuff::grabSystemInfo("lsmod");

    if (!out.filter("radeon").isEmpty())
        return XORG;
    if (!out.filter("fglrx").isEmpty())
        return FGLRX;

    return DRIVER_UNKNOWN;
}

void gpu::reconfigureDaemon() {
    dXorg::reconfigureDaemon();
}

bool gpu::daemonConnected() {
    return dXorg::daemonConnected();
}

// method for resolve which driver gpu instance will use
// and call some things that need to be done before read data
QStringList gpu::initialize(bool skipDetectDriver) {
    if (!skipDetectDriver)
        currentDriver = detectDriver();

    QStringList gpuList;

    switch (currentDriver) {
    case XORG: {
        gpuList = dXorg::detectCards();
        dXorg::configure(gpuList[currentGpuIndex]);
        features = dXorg::figureOutDriverFeatures();
        break;
    }
    case FGLRX: {
        gpuList = dFglrx::detectCards();
        dFglrx::configure(currentGpuIndex);
        features = dFglrx::figureOutDriverFeatures();
        break;
    }
    case DRIVER_UNKNOWN: {
        globalStuff::driverFeatures f;
        f.pm = globalStuff::PM_UNKNOWN;
        f.canChangeProfile = f.temperatureAvailable = f.coreVoltAvailable = f.coreClockAvailable = false;
        features = f;
        gpuList << "unknown";
    }
    }
    return gpuList;
}

globalStuff::gpuClocksStructString gpu::convertClocks(const globalStuff::gpuClocksStruct &data) {
    globalStuff::gpuClocksStructString tmp;

    tmp.coreClk = QString().setNum(data.coreClk);
    tmp.coreVolt = QString().setNum(data.coreVolt);
    tmp.memClk = QString().setNum(data.memClk);
    tmp.memVolt = QString().setNum(data.memVolt);
    tmp.powerLevel = QString().setNum(data.powerLevel);
    tmp.uvdCClk = QString().setNum(data.uvdCClk);
    tmp.uvdDClk = QString().setNum(data.uvdDClk);

    return tmp;
}

globalStuff::gpuTemperatureStructString gpu::convertTemperature(const globalStuff::gpuTemperatureStruct &data) {
    globalStuff::gpuTemperatureStructString tmp;

    tmp.current = QString().setNum(data.current) + QString::fromUtf8("\u00B0C");
    tmp.max = QString().setNum(data.max) + QString::fromUtf8("\u00B0C");
    tmp.min = QString().setNum(data.min) + QString::fromUtf8("\u00B0C");
    tmp.pwmSpeed = QString().setNum(data.pwmSpeed);

    return tmp;
}

void gpu::driverByParam(gpu::driver paramDriver) {
    this->currentDriver = paramDriver;
}

void gpu::changeGpu(char index) {
    currentGpuIndex = index;

    switch (currentDriver) {
    case XORG: {
        dXorg::configure(gpuList[currentGpuIndex]);
        features = dXorg::figureOutDriverFeatures();
        break;
    }
    case FGLRX: {
        dFglrx::configure(currentGpuIndex);
        features = dFglrx::figureOutDriverFeatures();
        break;
    }
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::getClocks() {
    switch (currentDriver) {
    case XORG: {
        gpuClocksData = dXorg::getClocks(dXorg::getClocksRawData());
        break;
    }
    case FGLRX:
        gpuClocksData = dFglrx::getClocks();
        break;
    case DRIVER_UNKNOWN: {
        globalStuff::gpuClocksStruct clk(-1);
        gpuClocksData = clk;
        break;
    }
    }
    gpuClocksDataString = convertClocks(gpuClocksData);
}

void gpu::getTemperature() {

    gpuTemeperatureData.currentBefore = gpuTemeperatureData.current;

    switch (currentDriver) {
    case XORG:
        gpuTemeperatureData.current = dXorg::getTemperature();
        break;
    case FGLRX:
        gpuTemeperatureData.current = dFglrx::getTemperature();
        break;
    case DRIVER_UNKNOWN:
        return;
    }

    // update rest of structure with temperature data //
    gpuTemeperatureData.sum += gpuTemeperatureData.current;
    if (gpuTemeperatureData.min == 0)
        gpuTemeperatureData.min  = gpuTemeperatureData.current;

    gpuTemeperatureData.max = (gpuTemeperatureData.max < gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.max;
    gpuTemeperatureData.min = (gpuTemeperatureData.min > gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.min;

    gpuTemeperatureDataString = convertTemperature(gpuTemeperatureData);
}

QList<QTreeWidgetItem *> gpu::getCardConnectors() const {

    // looks like good implementation for everything, cause of questioning xrandr
    // don't get wrong with dXorg class, just enjoy card connectors list
    return dXorg::getCardConnectors();

//    switch (currentDriver) {
//    case XORG:
//        return dXorg::getCardConnectors();
//        break;
//    case FGLRX:
//        break;
//    case DRIVER_UNKNOWN:
//        QList<QTreeWidgetItem *> list;
//        list.append(new QTreeWidgetItem(QStringList() <<"err"));
//        return list;
//    }
}

QList<QTreeWidgetItem *> gpu::getModuleInfo() const {
    QList<QTreeWidgetItem *> list;

    switch (currentDriver) {
    case XORG:
        list = dXorg::getModuleInfo();
        break;
    case FGLRX:
    case DRIVER_UNKNOWN: {
        list.append(new QTreeWidgetItem(QStringList() <<"err"));
    }
    }

    return list;
}

QStringList gpu::getGLXInfo(QString gpuName) const {
    QStringList data, gpus = globalStuff::grabSystemInfo("lspci").filter(QRegExp(".+VGA.+|.+3D.+"));
    gpus.removeAt(gpus.indexOf(QRegExp(".+Audio.+"))); //remove radeon audio device

    // loop for multi gpu
    for (int i = 0; i < gpus.count(); i++)
        data << "VGA:"+gpus[i].split(":",QString::SkipEmptyParts)[2];

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!gpuName.isEmpty())
        env.insert("DRI_PRIME",gpuName.at(gpuName.length()-1));
    QStringList driver = globalStuff::grabSystemInfo("xdriinfo",env).filter("Screen 0:",Qt::CaseInsensitive);
    if (!driver.isEmpty())  // because of segfault when no xdriinfo
        data << "Driver:"+ driver.filter("Screen 0:",Qt::CaseInsensitive)[0].split(":",QString::SkipEmptyParts)[1];

    switch (currentDriver) {
    case XORG:
        data << dXorg::getGLXInfo(gpuName, env);
        break;
    case FGLRX:
        data << dFglrx::getGLXInfo();
        break;
    case DRIVER_UNKNOWN:
        break;
    }
    return data;
}

QString gpu::getCurrentPowerLevel() const {
    switch (currentDriver) {
    case XORG:
        return dXorg::getCurrentPowerLevel().trimmed();
        break;
    default:
        return "";
        break;
    }
}

QString gpu::getCurrentPowerProfile() const {
    switch (currentDriver) {
    case XORG:
        return dXorg::getCurrentPowerProfile().trimmed();
        break;
    default:
        return "";
        break;
    }
}

void gpu::refreshPowerLevel() {
    currentPowerLevel = getCurrentPowerLevel();
    currentPowerProfile = getCurrentPowerProfile();
}

void gpu::setPowerProfile(globalStuff::powerProfiles _newPowerProfile) const {
    switch (currentDriver) {
    case XORG:
        dXorg::setPowerProfile(_newPowerProfile);
        break;
    case FGLRX:
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) const {
    switch (currentDriver) {
    case XORG:
        dXorg::setForcePowerLevel(_newForcePowerLevel);
        break;
    case FGLRX:
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::setPwmValue(int value) const {
    switch (currentDriver) {
    case XORG:
        dXorg::setPwmValue(value);
        break;
    case FGLRX:
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::setPwmManualControl(bool manual) const {
    switch (currentDriver) {
    case XORG:
        dXorg::setPwmManuaControl(manual);
        break;
    case FGLRX:
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::getPwmSpeed() {
    switch (currentDriver) {
    case XORG:
            gpuTemeperatureData.pwmSpeed = ((float)dXorg::getPwmSpeed() / features.pwmMaxSpeed ) * 100;
        break;
    case FGLRX:
    case DRIVER_UNKNOWN:
        break;
    }
}
