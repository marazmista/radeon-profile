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
//        delete dcomm;
        delete ioctlHnd;

        if(sharedMem.isAttached()){
            // In case the closing signal interrupts a sharedMem lock+read+unlock phase, sharedmem is unlocked
            sharedMem.unlock();
            sharedMem.detach();
        }
        // Before being deleted, the class deletes the sharedMem
        sharedMem.deleteLater();
    }

    globalStuff::gpuClocksStruct getClocksFromPmFile();
    globalStuff::gpuClocksStruct getClocksFromIoctl();
    globalStuff::gpuClocksStruct getClocks();

    float getTemperature();
    globalStuff::gpuUsageStruct getGpuUsage();
    int getPwmSpeed();

    QStringList getGLXInfo(QProcessEnvironment env);
    QList<QTreeWidgetItem *> getModuleInfo();
    QString getCurrentPowerLevel();
    QString getCurrentPowerProfile();

    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile);
    void setForcePowerLevel(globalStuff::forcePowerLevels);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    void figureOutGpuDataFilePaths(const QString &gpuName);
    void configure(const QString &gpuName);
    globalStuff::gpuConstParams getGpuConstParams();
    void reconfigureDaemon();
    bool daemonConnected();
    globalStuff::gpuClocksStruct getFeaturesFallback();
    void setupDriverModule(const QString &gpuName);
    void setupRegex(const QString &data);
    bool overclock(int percentage);
    void resetOverclock();
    globalStuff::driverFeatures features;

private:
    QChar gpuSysIndex;
    QSharedMemory sharedMem;
    int sensorsGPUtempIndex;
    short rxMatchIndex, clocksValueDivider;

    daemonComm dcomm;
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

    QString getClocksRawData(bool forFeatures = false);
    QString findSysfsHwmonForGPU();
    globalStuff::powerMethod getPowerMethod();
    globalStuff::tempSensor getTemperatureSensor();
    void setNewValue(const QString &filePath, const QString &newValue);
    QString findSysFsHwmonForGpu();
    bool getIoctlAvailability();
    void figureOutDriverFeatures();
};

#endif // DXORG_H
