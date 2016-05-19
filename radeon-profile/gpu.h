// copyright marazmista @ 29.03.2014

// in here, we have class which represent graphic adapter in program //
// It chooses driver and communicate with proper class for selected driver //
// and retuns result to gui //

#ifndef GPU_H
#define GPU_H

#include "globalStuff.h"
#include "dfglrx.h"
#include "dxorg.h"

typedef enum driver {
    XORG, FGLRX, DRIVER_UNKNOWN
} driver;

class gpu
{
public:

    explicit gpu() {
        currentGpuIndex = 0;
        gpuTemeperatureData.current =
                gpuTemeperatureData.max =
                gpuTemeperatureData.min =
                gpuTemeperatureData.sum = 0;
    }

    gpuClocksStruct gpuClocksData;
    gpuClocksStructString gpuClocksDataString;
    gpuTemperatureStruct gpuTemeperatureData;
    gpuTemperatureStructString gpuTemeperatureDataString;
    QStringList gpuList;
    char currentGpuIndex;
    driverFeatures features;
    QString currentPowerProfile, currentPowerLevel;

    QList<QTreeWidgetItem *> getCardConnectors() const;
    QStringList getGLXInfo(QString gpuName) const;
    QList<QTreeWidgetItem *> getModuleInfo() const;
    QString getCurrentPowerLevel() const;
    QString getCurrentPowerProfile() const;
    void refreshPowerLevel();
    gpuClocksStructString convertClocks(const gpuClocksStruct &data);
    gpuTemperatureStructString convertTemperature(const gpuTemperatureStruct &data);

    void getClocks();
    void getTemperature();
    void getPwmSpeed();

    void changeGpu(char index);
    void driverByParam(driver);
    void setPowerProfile(powerProfiles newPowerProfile) const;
    void setForcePowerLevel(forcePowerLevels newForcePowerLevel) const;
    void setPwmManualControl(bool manual) const;
    void setPwmValue(int value) const;

    QStringList initialize(bool skipDetectDriver = false);
    void reconfigureDaemon();
    bool daemonConnected();

    bool overclock(int value);

private:
    driver currentDriver;
    driver detectDriver();

};

#endif // GPU_H
