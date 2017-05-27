// copyright marazmista @ 29.03.2014

// in here, we have class which represent graphic adapter in program //
// It chooses driver and communicate with proper class for selected driver //
// and retuns result to gui //

#ifndef GPU_H
#define GPU_H

#include "globalStuff.h"
#include "dxorg.h"

class gpu
{
public:
//    enum driverType {
//        XORG, FGLRX, DRIVER_UNKNOWN
//    };

    explicit gpu() {
        currentGpuIndex = 0;
        gpuTemeperatureData.current =
                gpuTemeperatureData.max =
                gpuTemeperatureData.min =
                gpuTemeperatureData.sum = 0;
    }

    ~gpu() {
        delete driverHandler;
    }

    globalStuff::gpuClocksStruct gpuClocksData;
    globalStuff::gpuTemperatureStruct gpuTemeperatureData;
    globalStuff::gpuUsageStruct gpuUsageData;
    globalStuff::gpuConstParams gpuParams;
    globalStuff::gpuPwmStruct gpuPwmData;

    globalStuff::driverFeatures features;

    QStringList gpuList;
    char currentGpuIndex;
    QString currentPowerProfile, currentPowerLevel;

    QList<QTreeWidgetItem *> getCardConnectors() const;
    QStringList getGLXInfo(QString gpuName) const;
    QList<QTreeWidgetItem *> getModuleInfo() const;
    QString getCurrentPowerLevel();
    QString getCurrentPowerProfile();
    void refreshPowerLevel();

    void getClocks();
    void getTemperature();
    void getPwmSpeed();
    void getGpuUsage();
    void getConstParams();

    void changeGpu(char index);
    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile);
    void setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    QStringList detectCards();
    QStringList initialize();
    void reconfigureDaemon();
    bool daemonConnected();

    bool overclock(int value);
    void resetOverclock();
private:
    dXorg *driverHandler;

};

#endif // GPU_H
