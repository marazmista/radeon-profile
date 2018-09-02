// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QFile>
#include <QMap>

#define dpm_battery "battery"
#define dpm_performance "performance"
#define dpm_balanced "balanced"
#define dpm_high "high"
#define dpm_auto "auto"
#define dpm_low "low"
#define dpm_manual "manual"
#define dpm_profile_standard "profile_standard"
#define dpm_profile_min_sclk "profile_min_sclk"
#define dpm_profile_min_mclk "profile_min_mclk"
#define dpm_profile_peak "profile_peak"

#define profile_auto "auto"
#define profile_default "default"
#define profile_high "high"
#define profile_mid "mid"
#define profile_low "low"

#define pwm_disabled '0'
#define pwm_manual '1'
#define pwm_auto '2'

#define logDateFormat "yyyy-MM-dd_hh-mm-ss"


enum ValueID {
    CLK_CORE,
    CLK_MEM,
    VOLT_CORE,
    VOLT_MEM,
    CLK_UVD,
    DCLK_UVD,
    TEMPERATURE_CURRENT,
    TEMPERATURE_BEFORE_CURRENT,
    TEMPERATURE_MIN,
    TEMPERATURE_MAX,
    GPU_USAGE_PERCENT,
    GPU_VRAM_USAGE_PERCENT,
    GPU_VRAM_USAGE_MB,
    FAN_SPEED_PERCENT,
    FAN_SPEED_RPM,
    POWER_LEVEL
};

enum ValueUnit {
    MEGAHERTZ,
    PERCENT,
    CELSIUS,
    MEGABYTE,
    MILIVOLT,
    RPM,
    NONE
};

Q_DECLARE_METATYPE(ValueID)
Q_DECLARE_METATYPE(ValueUnit)

enum PowerProfiles {
    BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
};

enum ForcePowerLevels {
    F_AUTO, F_LOW, F_HIGH, F_MANUAL, F_PROFILE_STANDARD, F_PROFILE_MIN_SCLK, F_PROFILE_MIN_MCLK, F_PROFILE_PEAK
};

enum class ClocksDataSource {
    IOCTL, PM_FILE, SOURCE_UNKNOWN
};

enum class DriverModule {
    RADEON, AMDGPU, MODULE_UNKNOWN
};

enum PowerMethod {
    DPM = 0,  // kernel >= 3.11
    PROFILE = 1,  // kernel <3.11 or dpm disabled
    PM_UNKNOWN = 2
};

enum class TemperatureSensor {
    SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
    CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
    PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
    MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
    TS_UNKNOWN
};

struct GPUSysInfo {
    QString sysName, driverModuleString;
    DriverModule module;
};

struct RPValue {
    ValueUnit unit;
    float value;
    QString strValue;

    RPValue() {
        value = -1;
    }

    RPValue(ValueUnit u, float v = -1) {
        unit = u;
        value = v;
        strValue = toString();
    }

    void setValue(float v) {
        value = v;
        strValue = toString();
    }

    QString toString() {
        if (value == -1)
            return "";

        switch (unit) {
            case ValueUnit::MEGAHERTZ:
                return QString::number(value) + "MHz";
            case ValueUnit::MEGABYTE:
                return QString::number(value) + "MB";
            case ValueUnit::PERCENT:
                return QString::number(value, 'f', 1) + "%";
            case ValueUnit::MILIVOLT:
                return QString::number(value) + "mV";
            case ValueUnit::CELSIUS:
                return QString::number(value) + QString::fromUtf8("\u00B0C");
            case ValueUnit::RPM:
                return QString::number(value) + "RPM";
            default:
                return QString::number(value);
        }

        return "";
    }
};

typedef QMap<ValueID, RPValue> GPUDataContainer;
typedef QMap<int, QString> PowerPlayTable;
typedef QMap<int, unsigned int> FanProfileSteps;

// structure which holds what can be display on ui and on its base
// we enable ui elements
struct DriverFeatures {
    bool
    canChangeProfile = false,
    ocCoreAvailable = false,
    ocMemAvailable = false,
    freqCoreAvailable = false,
    freqMemAvailable = false;

    PowerMethod currentPowerMethod;
    ClocksDataSource clocksDataSource = ClocksDataSource::SOURCE_UNKNOWN;
    TemperatureSensor currentTemperatureSensor;
    GPUSysInfo sysInfo;
    PowerPlayTable sclkTable, mclkTable;

    DriverFeatures() {
        currentPowerMethod = PowerMethod::PM_UNKNOWN;
        currentTemperatureSensor = TemperatureSensor::TS_UNKNOWN;
    }
};

struct DeviceSysFs {
    QString
    power_method,
    power_profile,
    power_dpm_state,
    power_dpm_force_performance_level,
    pp_sclk_od,
    pp_mclk_od,
    pp_dpm_sclk,
    pp_dpm_mclk,
    gpu_busy_percent;

    DeviceSysFs() { }


    DeviceSysFs(const QString &devicePath) {
        power_method = devicePath + "power_method";
        power_profile = devicePath + "power_profile";
        power_dpm_state = devicePath + "power_dpm_state";
        power_dpm_force_performance_level = devicePath + "power_dpm_force_performance_level";
        pp_sclk_od = devicePath + "pp_sclk_od";
        pp_mclk_od = devicePath + "pp_mclk_od";
        pp_dpm_sclk = devicePath + "pp_dpm_sclk";
        pp_dpm_mclk = devicePath + "pp_dpm_mclk";
        gpu_busy_percent = devicePath + "gpu_busy_percent";

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

        if (!QFile::exists(pp_dpm_sclk))
            pp_dpm_sclk = "";

        if (!QFile::exists(pp_dpm_mclk))
            pp_dpm_mclk = "";

        if (!QFile::exists(gpu_busy_percent))
            gpu_busy_percent = "";
    }
};

struct DeviceFilePaths {
    QString debugfs_pm_info, moduleParams;
    DeviceSysFs sysFs;
};

struct HwmonAttributes {
    QString temp1, temp1_crit, pwm1, pwm1_enable, pwm1_max, fan1_input;

    HwmonAttributes() { }

    HwmonAttributes(const QString &hwmonPath) {
        temp1 = hwmonPath + "/temp1_input";
        temp1_crit = hwmonPath + "/temp1_crit";
        pwm1 = hwmonPath + "/pwm1";
        pwm1_enable = hwmonPath + "/pwm1_enable";
        pwm1_max = hwmonPath + "/pwm1_max";
        fan1_input = hwmonPath + "/fan1_input";

        if (!QFile::exists(temp1))
            temp1 = "";

        if (!QFile::exists(temp1_crit))
            temp1_crit = "";

        if (!QFile::exists(pwm1_enable))
            pwm1 = pwm1_enable = pwm1_max = "";

        if (!QFile::exists(fan1_input))
            fan1_input = "";
    }
};

struct GPUClocks {
    int coreClk = -1, memClk = -1, coreVolt = -1, memVolt = -1, uvdCClk = -1, uvdDClk = -1, powerLevel = -1;
};

struct GPUFanSpeed {
    float fanSpeedPercent = -1;
    int fanSpeedRpm = -1;
};

struct GPUUsage {
    float gpuUsage = -1, gpuVramUsagePercent = -1;
    long gpuVramUsage = -1;
};

struct GPUConstParams {
     int pwmMaxSpeed = -1, maxCoreClock = -1, maxMemClock = -1, temp1_crit = -1;
     float VRAMSize = -1;
};

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

    static ValueUnit getUnitFomValueId(ValueID id) {
        switch (id) {
            case ValueID::CLK_CORE:
            case ValueID::CLK_MEM:
            case ValueID::CLK_UVD:
            case ValueID::DCLK_UVD:
                return ValueUnit::MEGAHERTZ;

            case ValueID::VOLT_CORE:
            case ValueID::VOLT_MEM:
                return ValueUnit::MILIVOLT;

            case ValueID::FAN_SPEED_PERCENT:
            case ValueID::GPU_USAGE_PERCENT:
            case ValueID::GPU_VRAM_USAGE_PERCENT:
                return ValueUnit::PERCENT;

            case ValueID::FAN_SPEED_RPM:
                return ValueUnit::RPM;

            case ValueID::TEMPERATURE_BEFORE_CURRENT:
            case ValueID::TEMPERATURE_CURRENT:
            case ValueID::TEMPERATURE_MAX:
            case ValueID::TEMPERATURE_MIN:
                return ValueUnit::CELSIUS;

            case ValueID::GPU_VRAM_USAGE_MB:
                return ValueUnit::MEGABYTE;

            default:
                return ValueUnit::NONE;
        }
        return ValueUnit::NONE;
    }

    static bool isValueIdPlottable(ValueID id) {
        switch (id) {
            case ValueID::CLK_CORE:
            case ValueID::CLK_MEM:
            case ValueID::VOLT_CORE:
            case ValueID::VOLT_MEM:
            case ValueID::FAN_SPEED_PERCENT:
            case ValueID::GPU_USAGE_PERCENT:
            case ValueID::GPU_VRAM_USAGE_PERCENT:
            case ValueID::FAN_SPEED_RPM:
            case ValueID::TEMPERATURE_CURRENT:
            case ValueID::TEMPERATURE_MAX:
            case ValueID::TEMPERATURE_MIN:
            case ValueID::GPU_VRAM_USAGE_MB:
                return true;

            default:
               break;
        }
         return false;
    }

    static QString getNameOfValueID(ValueID id) {
        switch (id) {
            case ValueID::CLK_CORE: return QObject::tr("Core clock");
            case ValueID::CLK_MEM:  return QObject::tr("Memory clock");
            case ValueID::VOLT_CORE:  return QObject::tr("Core volt");
            case ValueID::VOLT_MEM:  return QObject::tr("Memory volt");
            case ValueID::FAN_SPEED_PERCENT:  return QObject::tr("Fan speed");
            case ValueID::GPU_USAGE_PERCENT:  return QObject::tr("GPU usage");
            case ValueID::GPU_VRAM_USAGE_PERCENT:  return QObject::tr("GPU Vram usage");
            case ValueID::FAN_SPEED_RPM:  return QObject::tr("Fan speed RPM");
            case ValueID::TEMPERATURE_CURRENT:  return QObject::tr("Temperature");
            case ValueID::TEMPERATURE_MAX: return QObject::tr("Temperature (max)");
            case ValueID::TEMPERATURE_MIN: return QObject::tr("Temperature (min)");
            case ValueID::GPU_VRAM_USAGE_MB:  return QObject::tr("GPU Vram megabyte usage");
            case ValueID::POWER_LEVEL: return QObject::tr("Power level");
            default:
                break;
        }

        return "";
    }

    static QString getNameOfValueIDWithUnit(ValueID id) {
        switch (id) {
            case ValueID::CLK_CORE: return QObject::tr("Core clock [MHz]");
            case ValueID::CLK_MEM:  return QObject::tr("Memory clock [MHz]");
            case ValueID::VOLT_CORE:  return QObject::tr("Core volt [mV]");
            case ValueID::VOLT_MEM:  return QObject::tr("Memory volt [mV]");
            case ValueID::FAN_SPEED_PERCENT:  return QObject::tr("Fan speed [%]");
            case ValueID::GPU_USAGE_PERCENT:  return QObject::tr("GPU usage [%]");
            case ValueID::GPU_VRAM_USAGE_PERCENT:  return QObject::tr("GPU Vram usage [%]");
            case ValueID::FAN_SPEED_RPM:  return QObject::tr("Fan speed [rpm]");
            case ValueID::TEMPERATURE_CURRENT:  return QObject::tr("Temperature [")+QString::fromUtf8("\u00B0C]");
            case ValueID::TEMPERATURE_MAX: return QObject::tr("Temperature (max) [")+QString::fromUtf8("\u00B0C]");
            case ValueID::TEMPERATURE_MIN: return QObject::tr("Temperature (min) [")+QString::fromUtf8("\u00B0C]");
            case ValueID::GPU_VRAM_USAGE_MB:  return QObject::tr("GPU Vram usage [MB]");
            case ValueID::POWER_LEVEL: return QObject::tr("Power level ");
            default:
                break;
        }

        return "";
    }

    static QString getNameOfUnit(ValueUnit u) {
        switch (u) {
            case ValueUnit::MEGAHERTZ: return QObject::tr("Megahertz [MHz]");
            case ValueUnit::MEGABYTE: return QObject::tr("Megabyte [MB]");
            case ValueUnit::PERCENT: return QObject::tr("Percent [%]");
            case ValueUnit::MILIVOLT: return QObject::tr("Milivolt [mV]");
            case ValueUnit::CELSIUS: return QObject::tr("Temperature [")+QString::fromUtf8("\u00B0C]");
            case ValueUnit::RPM: return QObject::tr("Speed [RPM]");
            default:
                break;
        }

        return "";
    }

    static QStringList createDPMCombo() {
        return QStringList() << dpm_battery << dpm_balanced << dpm_performance;
    }

    static QStringList createPowerLevelCombo(const DriverModule dm) {
        switch (dm) {
            case DriverModule::RADEON:
                return QStringList() << dpm_auto << dpm_low << dpm_high;
            case DriverModule::AMDGPU:
                return QStringList() << dpm_auto << dpm_low << dpm_high << dpm_manual <<
                                        dpm_profile_standard << dpm_profile_min_sclk << dpm_profile_min_mclk << dpm_profile_peak;
            default:
                break;
        }

        return QStringList();
    }

    static QStringList createProfileCombo() {
        return QStringList() << profile_auto << profile_default << profile_high << profile_mid << profile_low;
    }

    // settings from config used across the source
    static struct globalCfgStruct {
        float interval;
        bool daemonAutoRefresh, rootMode, daemonData = false;
    } globalConfig;
};


#endif // PUBLICSTUFF_H
