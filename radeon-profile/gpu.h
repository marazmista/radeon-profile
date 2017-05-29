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
        gpuTemeperatureData.current =
                gpuTemeperatureData.max =
                gpuTemeperatureData.min =
                gpuTemeperatureData.sum = 0;

        connect(&futureGpuUsage, SIGNAL(finished()),this,SLOT(handleGpuUsageResult()));
    }

    ~gpu() {
        delete driverHandler;
    }

    globalStuff::gpuClocksStruct gpuClocksData;
    globalStuff::gpuTemperatureStruct gpuTemeperatureData;
    globalStuff::gpuUsageStruct gpuUsageData;
    globalStuff::gpuConstParams gpuParams;
    globalStuff::gpuPwmStruct gpuPwmData;

    char currentGpuIndex;
    QString currentPowerProfile, currentPowerLevel;
    QList<globalStuff::gpuSysInfo> gpuList;

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
    void setPowerProfile(globalStuff::powerProfiles _newPowerProfile);
    void setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    void detectCards();
    bool initialize();
    void reconfigureDaemon();
    bool daemonConnected();
    bool overclock(int value);
    void resetOverclock();
    const globalStuff::driverFeatures& getDriverFeatures();

private slots:
    void handleGpuUsageResult() {
        gpuUsageData = futureGpuUsage.result();
        gpuUsageData.convertToString();
    }

private:
    dXorg *driverHandler;
    QFutureWatcher<globalStuff::gpuUsageStruct> futureGpuUsage;
};

#endif // GPU_H
