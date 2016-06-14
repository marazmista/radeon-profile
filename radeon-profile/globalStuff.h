// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <map>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QDebug>

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
#define file_GPUoverclockLevel "pp_sclk_od"
#define file_memoryOverclockLevel "pp_mclk_od"

#define logDateFormat "yyyy-MM-dd_hh-mm-ss"

#define appVersion 20160527

#define logEvent qDebug() << "EVENT" <<

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
        ushort interval; // seconds
        bool daemonAutoRefresh, rootMode;
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
    short coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk, powerLevel;
    bool coreClkOk, memClkOk, coreVoltOk, memVoltOk, uvdCClkOk, uvdDClkOk, powerLevelOk;

    gpuClocksStruct() {
        coreClk = memClk = coreVolt = memVolt = uvdCClk = uvdDClk = powerLevel = -1;
        coreClkOk = memClkOk = coreVoltOk = memVoltOk = uvdCClkOk = uvdDClkOk = powerLevelOk = false;
    }

    gpuClocksStruct(short _coreClk, short _memClk, short _coreVolt, short _memVolt, short _uvdCClk, short _uvdDclk, short _pwrLevel ) {
        coreClk = _coreClk;
        memClk = _memClk;
        coreVolt = _coreVolt;
        memVolt = _memVolt;
        uvdCClk = _uvdCClk;
        uvdDClk = _uvdDclk;
        powerLevel = _pwrLevel;
        coreClkOk = memClkOk = coreVoltOk = memVoltOk = uvdCClkOk = uvdDClkOk = powerLevelOk = true;
    }

} gpuClocksStruct;

typedef struct gpuClocksStructString {
    QString powerLevel, coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
} gpuClocksStructString;


// structure which holds what can be display on ui and on its base
// we enable ui elements
typedef struct driverFeatures {
    bool canChangeProfile,
        coreClockAvailable,
        coreMaxClkAvailable,
        coreMinClkAvailable,
        memClockAvailable,
        coreVoltAvailable,
        memVoltAvailable,
        temperatureAvailable,
        pwmAvailable,
        GPUoverClockAvailable,
        memoryOverclockAvailable;
    powerMethod pm;
    ushort pwmMaxSpeed;

    driverFeatures() {
        canChangeProfile = coreClockAvailable = coreMaxClkAvailable = coreMinClkAvailable =
                memClockAvailable = coreVoltAvailable = memVoltAvailable = temperatureAvailable =
                pwmAvailable = GPUoverClockAvailable = memoryOverclockAvailable = false;
        pm = PM_UNKNOWN;
        pwmMaxSpeed = 0;
    }

} driverFeatures;

typedef struct gpuTemperatureStruct {
    float current, currentBefore, max, min, sum;
    ushort pwmSpeed; // Pwm speed percentage on max PWM speed

    gpuTemperatureStruct(){
        current = currentBefore = max = min = sum = pwmSpeed = 0;
    }
} gpuTemperatureStruct;

typedef struct gpuTemperatureStructString {
    QString current, max, min, pwmSpeed;
} gpuTemperatureStructString;


#endif // PUBLICSTUFF_H
