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

class gpu : public QObject
{
    Q_OBJECT
public:


    gpuClocksStruct gpuClocksData;
    gpuClocksStructString gpuClocksDataString;
    gpuTemperatureStruct gpuTemeperatureData;
    gpuTemperatureStructString gpuTemeperatureDataString;
    QStringList gpuList;
    int currentGpuIndex = 0;
    driverFeatures features;
    QString currentPowerProfile, currentPowerLevel;

    QList<QTreeWidgetItem *> getCardConnectors() const;
    QStringList getGLXInfo(QString gpuName) const;
    QList<QTreeWidgetItem *> getModuleInfo() const;
    QString getCurrentPowerLevel() const;
    QString getCurrentPowerProfile() const;
    void refreshPowerLevel();
    void convertClocks();
    void convertTemperature();

    void getClocks();
    void getTemperature();
    void getPwmSpeed();

    void changeGpu(char index);
    void driverByParam(driver);
    void setPowerProfile(powerProfiles newPowerProfile) const;
    void setForcePowerLevel(forcePowerLevels newForcePowerLevel) const;
    void setPwmManualControl(bool manual) const;
    void setPwmValue(int value) const;

    void initialize(bool skipDetectDriver = false);
    void reconfigureDaemon();
    bool daemonConnected();

    /**
     * @brief overclockGPU Overclock the GPU core clock by a percentage
     * @param value Percentage to overclock
     * @return True if the overclock is successful, false otherwise
     */
    bool overclockGPU(int value);

    /**
     * @brief overclockMemory Overclock the GPU memory clock by a percentage
     * @param value Percentage to overclock
     * @return True if the overclock is successful, false otherwise
     */
    bool overclockMemory(int value);

private:
    driver currentDriver;
    driver detectDriver();

};

#endif // GPU_H
