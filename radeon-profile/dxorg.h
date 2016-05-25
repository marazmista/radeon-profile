// copyright marazmista @ 29.03.2014

// class for support open source driver for Radeon cards //

#ifndef DXORG_H
#define DXORG_H

#include "globalStuff.h"
#include "daemonComm.h"

#include <QString>
#include <QList>
#include <QTreeWidgetItem>
#include <QSharedMemory>
#include <QThread>

#define SHARED_MEM_SIZE 128 // When changin this, consider changing SHARED_MEM_SIZE in radeon-profile-daemon/rpdthread.h

typedef enum tempSensor {
    SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
    CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
    PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
    MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
    TS_UNKNOWN
} tempSensor;

class dXorg : public QObject
{
    Q_OBJECT
public:
    static gpuClocksStruct getClocks(const QString &data);
    static QString getClocksRawData(bool forFeatures = false);
    static float getTemperature();
    static QStringList getGLXInfo(QProcessEnvironment env);
    static QList<QTreeWidgetItem *> getModuleInfo();
    static QString getCurrentPowerLevel();
    static QString getCurrentPowerProfile();
    static int getPwmSpeed();

    static void setPowerProfile(powerProfiles newPowerProfile);
    static void setForcePowerLevel(forcePowerLevels newForcePowerLevel);
    static void setPwmManuaControl(bool manual);
    static void setPwmValue(int value);

    static QStringList detectCards();
    static void figureOutGpuDataFilePaths(QString gpuName);
    static void configure(QString gpuName);
    static driverFeatures figureOutDriverFeatures();
    static void reconfigureDaemon();
    static bool daemonConnected();
    static gpuClocksStruct getFeaturesFallback();

    /**
     * @brief overClock Overclocks the GPU
     * @param percent Overclock percentage [0-20]
     * @return Success
     */
    static bool overClock(int percentage);



private:

    static QChar gpuSysIndex;
    static QSharedMemory sharedMem;

    static struct driverFilePaths {
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
    static int sensorsGPUtempIndex;
    static tempSensor currentTempSensor;
    static powerMethod currentPowerMethod;

    static QString findSysfsHwmonForGPU();
    static powerMethod getPowerMethod();
    static tempSensor testSensor();
    static void setNewValue(const QString &filePath, const QString &newValue);
    static QString findSysFsHwmonForGpu();
};

#endif // DXORG_H
