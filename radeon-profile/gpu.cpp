// copyright marazmista @ 29.03.2014

#include "gpu.h"
#include <QFile>
#include <QDir>

gpu::driver gpu::detectDriver() {
    if (QDir("/sys/class/drm/").exists())
        return XORG;

    if (QDir("/dev/ati").exists())
        return FGLRX;

    return DRIVER_UNKNOWN;
}

QStringList gpu::initialize() {
    currentDriver = detectDriver();

    switch (currentDriver) {
    case XORG: {
        gpuList = dXorg::detectCards();
        dXorg::configure(gpuList[currentGpuIndex]);
        return gpuList;
        break;
    }
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default: break;
    }
}

void gpu::changeGpu(char index) {
    currentGpuIndex = index;

    switch (currentDriver) {
    case XORG:
        dXorg::configure(gpuList[currentGpuIndex]);
        break;
    default:
        break;
    }
}

void gpu::getClocks() {
    switch (currentDriver) {
    case XORG:
        gpuData = dXorg::getClocks();
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default: break;
    }
}

void gpu::getTemperature() {
    switch (currentDriver) {
    case XORG:
        gpuTemeperatureData.current = dXorg::getTemperature();
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        return;
    default: return;
    }

    // update rest of structure with temperature data //
    gpuTemeperatureData.sum += gpuTemeperatureData.current;
    if (gpuTemeperatureData.min == 0)
        gpuTemeperatureData.min  = gpuTemeperatureData.current;

    gpuTemeperatureData.max = (gpuTemeperatureData.max < gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.max;
    gpuTemeperatureData.min = (gpuTemeperatureData.min > gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.min;
}

QList<QTreeWidgetItem *> gpu::getCardConnectors() {
    switch (currentDriver) {
    case XORG:
        return dXorg::getCardConnectors();
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default:
        break;
    }
}

QList<QTreeWidgetItem *> gpu::getModuleInfo() {
    switch (currentDriver) {
    case XORG:
        return dXorg::getModuleInfo();
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default:
        break;
    }
}

QStringList gpu::getGLXInfo(QString gpuName) {
    switch (currentDriver) {
    case XORG:
        return dXorg::getGLXInfo(gpuName);
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default:
        break;
    }
}

QString gpu::getCurrentPowerProfile() {
    switch (currentDriver) {
    case XORG:
        return dXorg::getCurrentPowerProfile();
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default: break;
    }
}

void gpu::setPowerProfile(globalStuff::powerProfiles _newPowerProfile) {
    switch (currentDriver) {
    case XORG:
        dXorg::setPowerProfile(_newPowerProfile);
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default:
        break;
    }
}

void gpu::setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) {
    switch (currentDriver) {
    case XORG:
        dXorg::setForcePowerLevel(_newForcePowerLevel);
        break;
    case FGLRX:
        break;
    case DRIVER_UNKNOWN:
        break;
    default:
        break;
    }
}
