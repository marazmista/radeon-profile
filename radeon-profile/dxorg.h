// copyright marazmista @ 29.03.2014

// class for support open source driver for Radeon cards //

#ifndef DXORG_H
#define DXORG_H

#include "globalStuff.h"
#include "ioctlHandler.h"

#include <QString>
#include <QList>
#include <QTreeWidgetItem>
#include <QSharedMemory>
#include <QFile>


#define SHARED_MEM_SIZE 2048

class dXorg
{
    struct RxPatterns {
        QString powerLevel, sclk, mclk, vclk, dclk, vddc, vddci;
    };

public:
    dXorg() : ioctlHnd(nullptr) { }
    dXorg(const GPUSysInfo &si);

    ~dXorg() {
        cleanup();
    }

    void cleanup() {
        delete ioctlHnd;

        if (sharedMem.isAttached()){
            // In case the closing signal interrupts a sharedMem lock+read+unlock phase, sharedmem is unlocked
            sharedMem.unlock();
            sharedMem.detach();
        }
        // Before being deleted, the class deletes the sharedMem
        sharedMem.deleteLater();
    }

    DriverFeatures features;
    GPUConstParams params;
    DeviceFilePaths driverFiles;

    GPUClocks getClocksFromPmFile();
    GPUClocks getClocksFromIoctl();
    GPUClocks getClocks();

    float getTemperature();
    GPUUsage getGPUUsage();
    GPUFanSpeed getFanSpeed();

    QList<QTreeWidgetItem *> getModuleInfo();
    QString getCurrentPowerLevel();
    QString getCurrentPowerProfile();

    int getPowerCapCurrent() const;

    void setPowerProfile(PowerProfiles newPowerProfile);
    void setForcePowerLevel(ForcePowerLevels newForcePowerLevel);

    void figureOutGpuDataFilePaths(const QString &gpuName);
    void configure();
    void reconfigureDaemon();
    GPUClocks getFeaturesFallback();
    void setupRegex(const QString &data);
    int getCurrentPowerPlayTableId(const QString &file);
    void setNewValue(const QString &filePath, const QString &newValue);
    void readOcTableAndRanges();
    void setOcTable(const QString &tableType, const FVTable &table);

private:
    QChar gpuSysIndex;
    QSharedMemory sharedMem;
    int sensorsGPUtempIndex;
    short rxMatchIndex, clocksValueDivider;
    RxPatterns rxPatterns;

    ioctlHandler *ioctlHnd;

    QString getClocksRawData(bool forFeatures = false);
    QString findSysfsHwmonForGPU();
    PowerMethod getPowerMethod();
    TemperatureSensor getTemperatureSensor();
    QString findSysFsHwmonForGpu();
    bool getIoctlAvailability();
    void figureOutDriverFeatures();
    void figureOutConstParams();
    void setupIoctl();
    void setupSharedMem();
    void setupDaemon();
    PowerPlayTable loadPowerPlayTable(const QString &file);
    QString createDaemonSetCmd(const QString &file, const QString &tableIndex);
        const std::tuple<QMap<QString, FVTable>, QMap<QString, OCRange>> parseOcTable();
};


#endif // DXORG_H
