// copyright marazmista @ 29.03.2014

// in here, we have class which represent graphic adapter in program //
// It chooses driver and communicate with proper class for selected driver //
// and retuns result to gui //

#ifndef GPU_H
#define GPU_H

#include "globalStuff.h"
#include "dfglrx.h"
#include "dxorg.h"

class gpu
{
public:
    enum driver {
        XORG, FGLRX, DRIVER_UNKNOWN
    };

    explicit gpu() { currentGpuIndex = 0; }

    globalStuff::gpuClocksStruct gpuData;
    globalStuff::gpuTemperatureStruct gpuTemeperatureData;
    QStringList gpuList;
    char currentGpuIndex;
    globalStuff::driverFeatures gpuFeatures;

    QList<QTreeWidgetItem *> getCardConnectors() const;
    QStringList getGLXInfo(QString gpuName = "") const;
    QList<QTreeWidgetItem *> getModuleInfo() const;
    QString getCurrentPowerProfile() const;
    QStringList initialize(bool skipDetectDriver = false);
    void changeGpu(char index);
    void driverByParam(gpu::driver);
    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile) const;
    void setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) const;
    void getClocks();
    void getTemperature();

private:

    driver currentDriver;
    driver detectDriver();

};

#endif // GPU_H
