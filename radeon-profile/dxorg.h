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


#define SHARED_MEM_SIZE 128

class dXorg
{
    struct rxPatternsStruct {
        QString powerLevel, sclk, mclk, vclk, dclk, vddc, vddci;
    };

    struct deviceSysFsStruct {
        QString
        power_method,
        power_profile,
        power_dpm_state,
        power_dpm_force_performance_level,
        pp_sclk_od,
        pp_mclk_od;

        deviceSysFsStruct() { }

        deviceSysFsStruct(const QString &devicePath) {
            power_method = devicePath + "/power_method";
            power_profile = devicePath + "/power_profile";
            power_dpm_state = devicePath + "/power_dpm_state";
            power_dpm_force_performance_level = devicePath + "/power_dpm_force_performance_level";
            pp_sclk_od = devicePath + "/pp_sclk_od";
            pp_mclk_od = devicePath + "/pp_mclk_od";

            if (!QFile::exists(power_method))
                power_method = "";

            if (!QFile::exists(power_profile))
                power_profile = "";

            if (!QFile::exists(power_dpm_state))
                power_dpm_state = "";

            if (!QFile::exists(power_dpm_force_performance_level))
                power_dpm_force_performance_level = "";

            if (!QFile::exists(pp_sclk_od))
                pp_sclk_od = "";

            if (!QFile::exists(pp_mclk_od))
                pp_mclk_od = "";
        }
    };

    struct deviceFilePathsStruct {
        QString debugfs_pm_info, moduleParams;
        deviceSysFsStruct sysFs;
    };

    struct hwmonAttributesStruct {
        QString temp1, pwm1, pwm1_enable, pwm1_max, fan1_input;

        hwmonAttributesStruct() { }

        hwmonAttributesStruct(const QString &hwmonPath) {
            temp1 = hwmonPath + "/temp1_input";
            pwm1 = hwmonPath + "/pwm1";
            pwm1_enable = hwmonPath + "/pwm1_enable";
            pwm1_max = hwmonPath + "/pwm1_max";
            fan1_input = hwmonPath + "/fan1_input";

            if (!QFile::exists(temp1))
                temp1 = "";

            if (!QFile::exists(pwm1_enable))
                pwm1 = pwm1_enable = pwm1_max = "";

            if (!QFile::exists(fan1_input))
                fan1_input = "";
        }
    };

public:
    dXorg() { }
    dXorg(const GpuSysInfo &si);

    void cleanup() {
        delete ioctlHnd;

        if(sharedMem.isAttached()){
            // In case the closing signal interrupts a sharedMem lock+read+unlock phase, sharedmem is unlocked
            sharedMem.unlock();
            sharedMem.detach();
        }
        // Before being deleted, the class deletes the sharedMem
        sharedMem.deleteLater();
    }

    GpuClocksStruct getClocksFromPmFile();
    GpuClocksStruct getClocksFromIoctl();
    GpuClocksStruct getClocks();

    float getTemperature();
    GpuLoadStruct getGpuLoad();
    GpuPwmStruct getPwmSpeed();

    QStringList getGLXInfo(QProcessEnvironment env);
    QList<QTreeWidgetItem *> getModuleInfo();
    QString getCurrentPowerLevel();
    QString getCurrentPowerProfile();

    void setPowerProfile(PowerProfiles _newPowerProfile);
    void setForcePowerLevel(ForcePowerLevels);
    void setPwmManualControl(bool manual);
    void setPwmValue(unsigned int value);

    void figureOutGpuDataFilePaths(const QString &gpuName);
    void configure();
    GpuConstParams getGpuConstParams();
    void reconfigureDaemon();
    bool daemonConnected();
    GpuClocksStruct getFeaturesFallback();
    void setupRegex(const QString &data);
    bool overclock(int percentage);
    void resetOverclock();
    DriverFeatures features;

private:
    QChar gpuSysIndex;
    QSharedMemory sharedMem;
    int sensorsGPUtempIndex;
    short rxMatchIndex, clocksValueDivider;
    deviceFilePathsStruct deviceFiles;
    rxPatternsStruct rxPatterns;
    hwmonAttributesStruct hwmonAttributes;

    daemonComm dcomm;
    ioctlHandler *ioctlHnd;


    QString getClocksRawData(bool forFeatures = false);
    QString findSysfsHwmonForGPU();
    PowerMethod getPowerMethod();
    TemperatureSensor getTemperatureSensor();
    void setNewValue(const QString &filePath, const QString &newValue);
    QString findSysFsHwmonForGpu();
    bool getIoctlAvailability();
    void figureOutDriverFeatures();
};

#endif // DXORG_H
