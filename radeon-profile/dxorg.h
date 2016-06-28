// copyright marazmista @ 29.03.2014

// class for support open source driver for Radeon cards //

#ifndef DXORG_H
#define DXORG_H

#include "globalStuff.h"
#include "daemonComm.h"
#include "gpu.h"

#include <QString>

typedef enum tempSensor {
    SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
    CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
    PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
    MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
    TS_UNKNOWN
} tempSensor;

class dXorg : public gpu
{
    Q_OBJECT
public:
    dXorg(QString module = QString());

    gpuClocksStruct getClocks(bool forFeatures = false);
    temp getTemperature() const;
    QString getCurrentPowerLevel() const;
    QString getCurrentPowerProfile() const;
    ushort getPwmSpeed() const;

    void setPowerProfile(powerProfiles newPowerProfile);
    void setForcePowerLevel(forcePowerLevels newForcePowerLevel);
    void setPwmManualControl(bool manual);
    void setPwmValue(ushort value);

    void changeGPU(ushort gpuIndex);
    driverFeatures figureOutDriverFeatures();
    void reconfigureDaemon();
    bool daemonConnected() const;

    bool overclockGPU(int value);

    bool overclockMemory(int value);


private:


    struct driverFilePaths {
        QString powerMethodFilePath,
            profilePath,
            dpmStateFilePath,
            clocksPath,
            forcePowerLevelFilePath,
            sysfsHwmonTempPath,
            pwmEnablePath,
            pwmSpeedPath,
            pwmMaxSpeedPath,
            GPUoverDrivePath,
            memoryOverDrivePath;
    } filePaths;

    daemonComm dcomm;
    ushort sensorsGPUtempIndex;
    tempSensor currentTempSensor;

    void figureOutGpuDataFilePaths(QString gpuName);
    gpuClocksStruct getFeaturesFallback();
    QString findSysfsHwmonForGPU() const;
    powerMethod getPowerMethod() const;
    tempSensor testSensor();
    QString findSysFsHwmonForGpu();

    /**
     * @brief sendValue Writes a string to a file, either thorugh the daemon or directly
     * @param filePath File to write
     * @param value String to write
     * @return success or failure
     */
    bool sendValue(const QString & filePath, const QString & value);
};

#endif // DXORG_H
