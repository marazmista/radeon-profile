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
    gpu() { currentGpuIndex = 0; }

    globalStuff::gpuClocksStruct gpuData;
    globalStuff::gpuTemperatureStruct gpuTemeperatureData;
    QStringList gpuList;
    char currentGpuIndex;

    QList<QTreeWidgetItem *> getCardConnectors();
    QStringList getGLXInfo(QString gpuName);
    QList<QTreeWidgetItem *> getModuleInfo();
    QString getCurrentPowerProfile();
    QStringList initialize();
    void changeGpu(char index);

    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile);
    void setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel);

    void getClocks();
    void getTemperature();
private:
    enum driver {
        XORG, FGLRX, DRIVER_UNKNOWN
    };

    driver currentDriver;
    driver detectDriver();

};

#endif // GPU_H
