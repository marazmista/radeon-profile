// copyright marazmista @ 29.03.2014

// class for support open source driver for Radeon cards //

#ifndef DXORG_H
#define DXORG_H

#include "globalStuff.h"
#include "daemonComm.h"
#include "ioctlHandler.h"

#include <QString>
#include <QList>
#include <QTreeWidgetItem>
#include <QSharedMemory>
#include <QThread>


#define SHARED_MEM_SIZE 128

class dXorg
{
public:
    dXorg() { }
    dXorg(const QString &gpuName);

    void cleanup() {
        delete dcomm;
        delete ioctlHnd;

        if(sharedMem.isAttached()){
            // In case the closing signal interrupts a sharedMem lock+read+unlock phase, sharedmem is unlocked
            sharedMem.unlock();
            sharedMem.detach();
        }
        // Before being deleted, the class deletes the sharedMem
        sharedMem.deleteLater();
    }

    globalStuff::gpuClocksStruct getClocks();
    QString getClocksRawData(bool forFeatures = false);
    float getTemperature();
    globalStuff::gpuUsageStruct getGpuUsage();
    QStringList getGLXInfo(QProcessEnvironment env);
    QList<QTreeWidgetItem *> getModuleInfo();
    QString getCurrentPowerLevel();
    QString getCurrentPowerProfile();
    int getPwmSpeed();

    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile);
    void setForcePowerLevel(globalStuff::forcePowerLevels);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    void figureOutGpuDataFilePaths(const QString &gpuName);
    void configure(const QString &gpuName);
    globalStuff::driverFeatures figureOutDriverFeatures();
    globalStuff::gpuConstParams getGpuConstParams();
    void reconfigureDaemon();
    bool daemonConnected();
    globalStuff::gpuClocksStruct getFeaturesFallback();
    void setupDriverModule(const QString &gpuName);
    void setupRegex(const QString &data);
    bool overclock(int percentage);
    void resetOverclock();

private:
    enum tempSensor {
        SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
        CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
        PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
        MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
        TS_UNKNOWN
    };

    QChar gpuSysIndex;
    QSharedMemory sharedMem;
    QString driverModuleName;
    daemonComm *dcomm;
    ioctlHandler *ioctlHnd;
    struct rxPatternsStruct {
        QString powerLevel, sclk, mclk, vclk, dclk, vddc, vddci;
    } rxPatterns;

    struct driverFilePaths {
        QString powerMethodFilePath,
            profilePath,
            dpmStateFilePath,
            clocksPath,
            forcePowerLevelFilePath,
            sysfsHwmonTempPath,
            moduleParamsPath,
            pwmEnablePath,
            pwmSpeedPath,
            pwmMaxSpeedPath,
            overDrivePath;
    } filePaths;

    int sensorsGPUtempIndex;
    short rxMatchIndex, clocksValueDivider;
    dXorg::tempSensor currentTempSensor;
    globalStuff::powerMethod currentPowerMethod;

    QString findSysfsHwmonForGPU();
    globalStuff::powerMethod getPowerMethod();
    tempSensor getTemperatureSensor();
    void setNewValue(const QString &filePath, const QString &newValue);
    QString findSysFsHwmonForGpu();
};

#endif // DXORG_H
