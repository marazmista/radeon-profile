// copyright marazmista @ 29.03.2014

// in here, we have class which represent graphic adapter in program //
// It chooses driver and communicate with proper class for selected driver //
// and retuns result to gui //

#ifndef GPU_H
#define GPU_H

#include "globalStuff.h"
#include "dxorg.h"
#include <QtConcurrent/QtConcurrent>

class gpu : public QObject
{

    Q_OBJECT
public:
    explicit gpu(QObject *parent = 0 ) : QObject(parent) {
        currentGpuIndex = 0;
        connect(&futureGpuUsage, SIGNAL(finished()),this,SLOT(handleGpuUsageResult()));
    }

    ~gpu() {
        delete driverHandler;
    }

    // main map that has all info available by ValueID
    QMap<ValueID, RPValue> gpuData;
    QList<GpuSysInfo> gpuList;
    GpuConstParams gpuParams;

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

    void changeGpu(int index);
    void setPowerProfile(PowerProfiles _newPowerProfile);
    void setForcePowerLevel(ForcePowerLevels _newForcePowerLevel);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    void detectCards();
    bool initialize();
    void reconfigureDaemon();
    bool daemonConnected();
    bool overclock(int value);
    void resetOverclock();
    const DriverFeatures& getDriverFeatures();

private slots:
    void handleGpuUsageResult() {
         GpuUsageStruct tmp = futureGpuUsage.result();

         if (tmp.gpuLoad != -1)
             gpuData.insert(ValueID::GPU_LOAD_PERCENT, RPValue(ValueUnit::PERCENT, tmp.gpuLoad));

        if (tmp.gpuVramLoad != -1)
            gpuData.insert(ValueID::GPU_VRAM_USAGE_MB, RPValue(ValueUnit::MEGABYTE, tmp.gpuVramLoad));

        if (tmp.gpuVramLoadPercent != -1) {
            gpuData.insert(ValueID::GPU_VRAM_USAGE_PERCENT, RPValue(ValueUnit::PERCENT, tmp.gpuVramLoadPercent));
        }
    }

private:
    dXorg *driverHandler;
    QFutureWatcher<GpuUsageStruct> futureGpuUsage;
};

#endif // GPU_H
