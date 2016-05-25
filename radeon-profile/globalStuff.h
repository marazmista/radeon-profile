// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <map>
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


#define appVersion 20160515


class globalStuff {
public:
    static QStringList grabSystemInfo(const QString cmd, const QProcessEnvironment env = QProcessEnvironment()) {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.setProcessEnvironment(env);

        p.start(cmd,QIODevice::ReadOnly);
        p.waitForFinished();

        return QString(p.readAllStandardOutput()).split('\n');
    }

    // settings from config used across the source
    static struct globalCfgStruct{
        float interval;
        bool daemonAutoRefresh, rootMode;
        int graphOffset;
    } globalConfig;
};

typedef enum powerProfiles {
    BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
} powerProfiles;

const std::map<powerProfiles, QString> profileToString =
    {{BATTERY, dpm_battery},
     {BALANCED, dpm_balanced},
     {PERFORMANCE, dpm_performance},
     {AUTO, dpm_auto},
     {DEFAULT, profile_default},
     {LOW, profile_low},
     {MID, profile_mid},
     {HIGH, profile_high}};

typedef enum forcePowerLevels {
    F_AUTO, F_LOW, F_HIGH
} forcePowerLevels;

const std::map<forcePowerLevels, QString> powerLevelToString =
    {{F_AUTO, profile_auto},
     {F_LOW, profile_low},
     {F_HIGH, profile_high}};

typedef enum powerMethod {
    DPM = 0,  // kernel >= 3.11
    PROFILE = 1,  // kernel <3.11 or dpm disabled
    PM_UNKNOWN = 2
} powerMethod;

typedef struct gpuClocksStruct {
    int coreClk = -1,
        memClk = -1,
        coreVolt = -1,
        memVolt = -1,
        uvdCClk = -1,
        uvdDClk = -1;
    char powerLevel = -1;

    bool coreClkOk = false,
        memClkOk = false,
        coreVoltOk = false,
        memVoltOk = false,
        uvdCClkOk = false,
        uvdDClkOk = false,
        powerLevelOk = false;

    gpuClocksStruct() { }

    gpuClocksStruct(int _coreClk, int _memClk, int _coreVolt, int _memVolt, int _uvdCClk, int _uvdDclk, char _pwrLevel ) {
        coreClk = _coreClk;
        memClk = _memClk;
        coreVolt = _coreVolt;
        memVolt = _memVolt;
        uvdCClk = _uvdCClk;
        uvdDClk = _uvdDclk;
        powerLevel = _pwrLevel;
        coreClkOk = memClkOk = coreVoltOk = memVoltOk = uvdCClkOk = uvdDClkOk = powerLevelOk = true;
    }

    void invalidate(){
        coreClkOk = memClkOk = coreVoltOk = memVoltOk = uvdCClkOk = uvdDClkOk = powerLevelOk = false;
    }
} gpuClocksStruct;

typedef struct gpuClocksStructString {
    QString powerLevel, coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
} gpuClocksStructString;


// structure which holds what can be display on ui and on its base
// we enable ui elements
typedef struct driverFeatures {
    bool canChangeProfile = false,
        coreClockAvailable = false,
        memClockAvailable = false,
        coreVoltAvailable = false,
        memVoltAvailable = false,
        temperatureAvailable = false,
        pwmAvailable = false,
        overClockAvailable = false;
    powerMethod pm = PM_UNKNOWN;
    int pwmMaxSpeed = 0;

    driverFeatures() { }

} driverFeatures;

typedef struct gpuTemperatureStruct {
    float current = 0,
        currentBefore = 0,
        max = 0,
        min = 0,
        sum = 0;
    int pwmSpeed = 0;
} gpuTemperatureStruct;

typedef struct gpuTemperatureStructString {
    QString current, max, min, pwmSpeed;
} gpuTemperatureStructString;


#endif // PUBLICSTUFF_H
