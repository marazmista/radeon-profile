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
    struct InitializationConfig {
        bool daemonAutoRefresh, daemonData, rootMode;

        InitializationConfig() :
            daemonAutoRefresh(true),
            daemonData(false),
            rootMode(false) { }

        InitializationConfig(bool isRoot, bool data, bool autoRefresh) {
            rootMode = isRoot;
            daemonData = autoRefresh;
            daemonData = data;
        }
    };

    dXorg() : ioctlHnd(nullptr) { }
    dXorg(const GPUSysInfo &si, const InitializationConfig &config);

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
    QString getCurrentPowerProfileMode();

    int getPowerCapSelected() const;
    int getPowerCapAverage() const;

    void setPowerProfile(const QString &newPowerProfile);
    void setForcePowerLevel(const QString &newForcePowerLevel);

    void figureOutGpuDataFilePaths(const QString &gpuName);
    void configure();
    void reconfigureDaemon();
    GPUClocks getFeaturesFallback();
    void setupRegex(const QString &data);
    int getCurrentPowerPlayTableId(const QString &file);
    void setNewValue(const QString &filePath, const QString &newValue);
    void setNewValue(const QString &filePath, const QStringList &newValues);
    void readOcTableAndRanges();
    void setOcTable(const QString &tableType, const FVTable &table);
    InitializationConfig getInitConfig();
    void refreshPowerPlayTables();
    PowerProfiles getPowerProfiles(const PowerMethod powerMethod);
    
private:
    QChar gpuSysIndex;
    QSharedMemory sharedMem;
    int sensorsGPUtempIndex;
    short rxMatchIndex, clocksValueDivider;
    RxPatterns rxPatterns;
    InitializationConfig initConfig;

    ioctlHandler *ioctlHnd;

    QString getClocksRawData();
    QString findSysfsHwmonForGPU();
    PowerMethod getPowerMethodFallback();
    TemperatureSensor getTemperatureSensor();
    QString findSysFsHwmonForGpu();
    bool getIoctlAvailability();
    void figureOutDriverFeatures();
    void figureOutConstParams();
    void setupIoctl();
    void setupSharedMem();
    void sendSharedMemInfoToDaemon();
    QStringList loadPowerPlayTable(const QString &file);
    QString createDaemonSetCmd(const QString &file, const QString &tableIndex);
    const std::tuple<QMap<QString, FVTable>, QMap<QString, OCRange>> parseOcTable();
};


#endif // DXORG_H
