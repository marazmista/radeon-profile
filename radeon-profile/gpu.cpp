// copyright marazmista @ 29.03.2014

#include "gpu.h"
#include <QFile>

gpu::driver gpu::detectDriver() {
    QStringList out = globalStuff::grabSystemInfo("lsmod");

    if (!out.filter("radeon").isEmpty())
        return XORG;
    if (!out.filter("fglrx").isEmpty())
        return FGLRX;

    return DRIVER_UNKNOWN;
}

QStringList gpu::initialize(bool skipDetectDriver) {
    if (!skipDetectDriver)
        currentDriver = detectDriver();

    switch (currentDriver) {
    case XORG: {
        gpuList = dXorg::detectCards();
        dXorg::configure(gpuList[currentGpuIndex]);
        gpuFeatures = dXorg::figureOutDriverFeatures();
        return gpuList;
    }
    case FGLRX: {
        gpuList = dFglrx::detectCards();
        dFglrx::configure(currentGpuIndex);
        gpuFeatures = dFglrx::figureOutDriverFeatures();
        return gpuList;
    }
        break;
    case DRIVER_UNKNOWN: {
        globalStuff::driverFeatures f;
        f.pm = globalStuff::PM_UNKNOWN;
        f.canChangeProfile = f.temperatureAvailable = f.voltAvailable = f.clocksAvailable = false;
        gpuFeatures = f;
        return QStringList() << "unknown";
    }
    }
}

void gpu::driverByParam(gpu::driver paramDriver) {
    this->currentDriver = paramDriver;
}

void gpu::changeGpu(char index) {
    currentGpuIndex = index;

    switch (currentDriver) {
    case XORG: {
        dXorg::configure(gpuList[currentGpuIndex]);
        gpuFeatures = dXorg::figureOutDriverFeatures();
        break;
    }
    case FGLRX: {
        dFglrx::configure(currentGpuIndex);
        gpuFeatures = dFglrx::figureOutDriverFeatures();
        break;
    }
    case DRIVER_UNKNOWN:
        break;
    }
}

void gpu::getClocks() {
    switch (currentDriver) {
    case XORG:
        gpuData = dXorg::getClocks();
        break;
    case FGLRX:
        gpuData = dFglrx::getClocks();
        break;
    case DRIVER_UNKNOWN: {
        globalStuff::gpuClocksStruct clk(-1);
        gpuData = clk;
        break;
    }
    }
}

void gpu::getTemperature() {
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
    switch (currentDriver) {
    case XORG:
        return dXorg::getModuleInfo();
        break;
    case FGLRX:
    case DRIVER_UNKNOWN: {
        QList<QTreeWidgetItem *> list;
        list.append(new QTreeWidgetItem(QStringList() <<"err"));
        return list;
    }
    }
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
    data.removeDuplicates();
    return data;
}

QString gpu::getCurrentPowerProfile() const {
    switch (currentDriver) {
    case XORG:
        return dXorg::getCurrentPowerProfile();
        break;
    case FGLRX:
        return "Catalyst";
        break;
    case DRIVER_UNKNOWN:
        return "unknown";
    }
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
