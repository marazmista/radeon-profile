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
        connect(&futureGpuLoad, SIGNAL(finished()),this,SLOT(handleGpuUsageResult()));
    }

    ~gpu() {
        delete driverHandler;
    }

    // main map that has all info available by ValueID
    GPUDataContainer gpuData;
    QList<GPUSysInfo> gpuList;
    GPUConstParams gpuParams;

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
    void getGpuLoad();
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
    void finalize();

private slots:
    void handleGpuUsageResult() {
        GPULoadStruct tmp = futureGpuLoad.result();

        if (gpuData.contains(ValueID::GPU_LOAD_PERCENT))
            gpuData[ValueID::GPU_LOAD_PERCENT].setValue(tmp.gpuLoad);

        if (gpuData.contains(ValueID::GPU_VRAM_LOAD_MB))
            gpuData[ValueID::GPU_VRAM_LOAD_MB].setValue(tmp.gpuVramLoad);

        if (gpuData.contains(ValueID::GPU_VRAM_LOAD_PERCENT)) {
            gpuData[ValueID::GPU_VRAM_LOAD_PERCENT].setValue(tmp.gpuVramLoadPercent);
        }
    }

private:
    dXorg *driverHandler;
    void defineAvailableDataContainer();
    QFutureWatcher<GPULoadStruct> futureGpuLoad;

};

#endif // GPU_H
