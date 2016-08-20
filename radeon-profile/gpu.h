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

    explicit gpu() {
        currentGpuIndex = 0;
        gpuTemeperatureData.current =
                gpuTemeperatureData.max =
                gpuTemeperatureData.min =
                gpuTemeperatureData.sum = 0;
    }

    globalStuff::gpuClocksStruct gpuClocksData;
    globalStuff::gpuClocksStructString gpuClocksDataString;
    globalStuff::gpuTemperatureStruct gpuTemeperatureData;
    globalStuff::gpuTemperatureStructString gpuTemeperatureDataString;
    QStringList gpuList;
    char currentGpuIndex;
    globalStuff::driverFeatures features;
    QString currentPowerProfile, currentPowerLevel;
    static QString driverModuleName;

    QList<QTreeWidgetItem *> getCardConnectors() const;
    QStringList getGLXInfo(QString gpuName) const;
    QList<QTreeWidgetItem *> getModuleInfo() const;
    QString getCurrentPowerLevel() const;
    QString getCurrentPowerProfile() const;
    void refreshPowerLevel();
    globalStuff::gpuClocksStructString convertClocks(const globalStuff::gpuClocksStruct &data);
    globalStuff::gpuTemperatureStructString convertTemperature(const globalStuff::gpuTemperatureStruct &data);

    void getClocks();
    void getTemperature();
    void getPwmSpeed();

    void changeGpu(char index);
    void driverByParam(gpu::driver);
    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile) const;
    void setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) const;
    void setPwmManualControl(bool manual) const;
    void setPwmValue(int value) const;

    QStringList initialize(bool skipDetectDriver = false);
    void reconfigureDaemon();
    bool daemonConnected();

    bool overclock(int value);
    void resetOverclock();

private:
    driver currentDriver;
    driver detectDriver();

};

#endif // GPU_H
