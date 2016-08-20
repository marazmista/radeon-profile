// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>

#define dpm_battery "battery"
#define dpm_performance "performance"
#define dpm_balanced "balanced"
#define dpm_high "high"
#define dpm_auto "auto"
#define dpm_low "low"

#define profile_auto "auto"
#define profile_default "default"
#define profile_high "high"
#define profile_mid "mid"
#define profile_low "low"

#define pwm_disabled '0'
#define pwm_manual '1'
#define pwm_auto '2'

#define file_PowerMethod "power_method"
#define file_powerProfile "power_profile"
#define file_powerDpmState "power_dpm_state"
#define file_powerDpmForcePerformanceLevel "power_dpm_force_performance_level"
#define file_overclockLevel "pp_sclk_od"

#define logDateFormat "yyyy-MM-dd_hh-mm-ss"



class globalStuff {
public:
    static QStringList grabSystemInfo(const QString cmd) {
        QProcess *p = new QProcess();
        p->setProcessChannelMode(QProcess::MergedChannels);

        p->start(cmd,QIODevice::ReadOnly);
        p->waitForFinished();

        QString a = p->readAllStandardOutput();
        delete p;
        return a.split('\n');
    }

    static QStringList grabSystemInfo(const QString cmd, const QProcessEnvironment env) {
        QProcess *p = new QProcess();
        p->setProcessChannelMode(QProcess::MergedChannels);
        p->setProcessEnvironment(env);

        p->start(cmd,QIODevice::ReadOnly);
        p->waitForFinished();

        QString a = p->readAllStandardOutput();
        delete p;
        return a.split('\n');
    }

    enum powerProfiles {
        BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
    };

    enum forcePowerLevels {
        F_AUTO, F_LOW, F_HIGH
    };

//    enum pwmControl {
//        PWM_DISABLED, PWM_MANUAL, PWM_AUTO
//    };

    enum powerMethod {
        DPM = 0,  // kernel >= 3.11
        PROFILE = 1,  // kernel <3.11 or dpm disabled
        PM_UNKNOWN = 2
    };

    enum drverModule {
        RADEON, AMDGPU
    };

    struct gpuClocksStruct {
        int coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
        char powerLevel;

        gpuClocksStruct() { }

        gpuClocksStruct(int _coreClk, int _memClk, int _coreVolt, int _memVolt, int _uvdCClk, int _uvdDclk, char _pwrLevel ) {
            coreClk = _coreClk;
            memClk = _memClk;
            coreVolt = _coreVolt;
            memVolt = _memVolt;
            uvdCClk = _uvdCClk;
            uvdDClk = _uvdDclk;
            powerLevel = _pwrLevel;
        }

        // initialize empty struct, so when we pass -1, only values that != -1 will show
        gpuClocksStruct(int allValues) {
            coreClk = memClk =  coreVolt =   memVolt = uvdCClk =  uvdDClk = powerLevel = allValues;
        }
    };

    struct gpuClocksStructString {
      QString powerLevel, coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
    };


    // structure which holds what can be display on ui and on its base
    // we enable ui elements
    struct driverFeatures {
        bool canChangeProfile,
            coreClockAvailable,
            memClockAvailable,
            coreVoltAvailable,
            memVoltAvailable,
            temperatureAvailable,
            pwmAvailable,
            overClockAvailable;
        globalStuff::powerMethod pm;
        int pwmMaxSpeed;

        driverFeatures() {
            canChangeProfile =
                    coreClockAvailable =
                    memClockAvailable =
                    coreVoltAvailable =
                    memVoltAvailable =
                    temperatureAvailable =
                    pwmAvailable =
                    overClockAvailable = false;
            pm = PM_UNKNOWN;
            pwmMaxSpeed = 0;
        }

    };

    struct gpuTemperatureStruct{
        float current, currentBefore, max, min, sum;
        int pwmSpeed;
    };

    struct gpuTemperatureStructString {
        QString current, max, min, pwmSpeed;
    };

    // settings from config used across the source
    static struct globalCfgStruct{
        float interval;
        bool daemonAutoRefresh, rootMode;
        int graphOffset;
    } globalConfig;
};


#endif // PUBLICSTUFF_H
