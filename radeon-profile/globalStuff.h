// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <cstdint>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QHash>
#include <QVector>

#define dpm_battery "battery"
#define dpm_performance "performance"
#define dpm_balanced "balanced"

#define level_high "high"
#define level_auto "auto"
#define level_low "low"
#define level_manual "manual"
#define level_profile_standard "profile_standard"
#define level_profile_min_sclk "profile_min_sclk"
#define level_profile_min_mclk "profile_min_mclk"
#define level_profile_peak "profile_peak"

#define profile_auto "auto"
#define profile_default "default"
#define profile_high "high"
#define profile_mid "mid"
#define profile_low "low"

#define pwm_disabled '0'
#define pwm_manual '1'
#define pwm_auto '2'

#define OD_RANGE "OD_RANGE"
#define OD_MCLK "OD_MCLK"
#define OD_SCLK "OD_SCLK"
#define SCLK "SCLK"
#define MCLK "MCLK"
#define VDDC "VDDC"
#define OD_VDDC_CURVE "OD_VDDC_CURVE"

#define logDateFormat "yyyy-MM-dd_hh-mm-ss"

#define MICROWATT_DIVIDER 1000000

static QString MOCK_PATH("/home/marazmista/aasd/testfies/");



/** Value keys for gpu::gpuData
 *
 * Each value stored, updated and provided by the gpu class can be referred to
 * by a unique ValueID.
 *
 * The key is split into two components:
 * type: the type of value provided by the gpu driver as defined in Type
 * instance: If hardware provides multiple values of the same type, they are
 *           differenciated by the instance component of the ValueID.
 *           For example, a GPU with multiple temp sensors will have mulitple
 *           instances of the Type::TEPERATURE_* values for each sensor.
 */
struct ValueID {

    enum Type : std::uint16_t {
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
        POWER_LEVEL,
        POWER_CAP_SELECTED,
        POWER_CAP_AVERAGE
    };

    // instances are not defined in an enum, as we may have instances
    // of unknown type that need their own unique id.
    // Known instances are defined as constexpr constants instead.
    // Unknown instance types start at INSTANCE_UNKNOWN, i.e. have the most
    // significant bit set.
    typedef std::uint16_t Instance;

    static constexpr Instance T_EDGE     = 0;
    static constexpr Instance T_JUNCTION = 1;
    static constexpr Instance T_MEM      = 2;

    static constexpr Instance INSTANCE_UNKNOWN = 1 << 15;

    // 64-bit key is split into three sections:
    // type: the type of data as defined in TYPE above
    // subtype:
    Type type;
    Instance instance = 0;

    ValueID() = default;

    ValueID(const ValueID&) = default;

    ValueID(Type v_type, Instance v_instance = 0)
        : type(v_type), instance(v_instance)
    {
    }

    operator Type() const
    {
        return type;
    }

    // returns a unique representation of a key as a 32-bit integer.
    // this key is used to uniquely refer to a valueID in config files.
    // This key is different from orderKey() to preserve backwards compatibilty
    // with previous config files: a ValueID of instance=0 maps to
    // the same int value as a type only id did.
    std::uint32_t key() const
    {
        return type + (std::uint32_t(instance) << 16);
    }

    // compare by combined key (i.e. all values)
    bool operator==(const ValueID& rhs) const
    {
        return key() == rhs.key();
    }

    // compare to Type only
    bool operator==(const ValueID::Type& rhs) const
    {
        return type == rhs;
    }

    // for ordering via comparison operator, flip the two components
    // in the key. This keeps values of the same type adjecent.
    std::uint32_t orderKey() const
    {
        return instance + (std::uint32_t(type) << 16);
    }

    // comparison operators: ordering in containers, to be usable in containers
    bool operator<(const ValueID& rhs) const
    {
        return orderKey() < rhs.orderKey();
    }

    // get ValueID of the same instance as a different type
    ValueID asType(const Type& as_type) const
    {
        return ValueID(as_type, instance);
    }


    // create ValueID from 32-bit key
    static ValueID fromKey(std::uint32_t key)
    {
       return ValueID(Type(key & 0xFFFF), key >> 16);
    }
};

enum ValueUnit {
    MEGAHERTZ,
    PERCENT,
    CELSIUS,
    MEGABYTE,
    MILIVOLT,
    RPM,
    WATT,
    NONE
};

Q_DECLARE_METATYPE(ValueID)
Q_DECLARE_METATYPE(ValueID::Type)
Q_DECLARE_METATYPE(ValueUnit)

enum class ClocksDataSource {
    IOCTL, PM_FILE, SOURCE_UNKNOWN
};

enum class DriverModule {
    RADEON, AMDGPU, MODULE_UNKNOWN
};

enum PowerMethod {
    DPM = 0,  // kernel >= 3.11
    PROFILE = 1,  // kernel <3.11 or dpm disabled
    PP_MODE = 2,
    PM_UNKNOWN = 3
};

enum class TemperatureSensor {
    SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
    CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
    PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
    MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
    TS_UNKNOWN
};

struct GPUSysInfo {
    QString sysName, driverModuleString, name;
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
            case ValueUnit::WATT:
                return QString::number(value) + "W";
            default:
                return QString::number(value);
        }
    }
};

struct FreqVoltPair {
    unsigned frequency, voltage;

    FreqVoltPair() { }

    FreqVoltPair(unsigned freq, unsigned volt) {
        frequency = freq;
        voltage = volt;
    }
};

struct OCRange {
    unsigned min, max;

    OCRange() { }

    OCRange(unsigned _min, unsigned _max) {
        min = _min;
        max = _max;
    }
};

struct PowerProfileDefinition {
    QString name;
    unsigned id;
    bool isActive;

    PowerProfileDefinition() { }

    PowerProfileDefinition(unsigned _id, bool _isActive, QString _name) {
        id = _id;
        name = _name;
        isActive = _isActive;
    }
};

typedef QMap<ValueID, RPValue> GPUDataContainer;
typedef QMap<unsigned, FreqVoltPair> FVTable;
typedef QMap<QString, FVTable> MapFVTables;
typedef QMap<QString, OCRange> MapOCRanges;
typedef QMap<int, unsigned int> FanProfileSteps;
typedef QList<PowerProfileDefinition> PowerProfiles;

struct OCProfile {
    unsigned powerCap;
    MapFVTables tables;
};

struct FanProfile {
    FanProfileSteps steps;
    int hysteresis = 0;
    ValueID::Instance sensor = ValueID::T_EDGE;

    FanProfile() = default;

    FanProfile(const FanProfileSteps& fp_steps,
               int fp_hysteresis = 0,
               ValueID::Instance fp_sensor = ValueID::T_EDGE)
        : steps(fp_steps),
          hysteresis(fp_hysteresis),
          sensor(fp_sensor)
    {
    }
};

// structure which holds what can be display on ui and on its base
// we enable ui elements
struct DriverFeatures {
    bool
    isChangeProfileAvailable = false,
    isFanControlAvailable = false,
    isPercentCoreOcAvailable = false,
    isPercentMemOcAvailable = false,
    isDpmCoreFreqTableAvailable = false,
    isDpmMemFreqTableAvailable = false,
    isPowerCapAvailable = false,
    isOcTableAvailable = false,
    isVDDCCurveAvailable = false,
    isDpmStateAvailable = false,
    isPowerProfileModesAvailable = false;

    PowerMethod currentPowerMethod = PowerMethod::PM_UNKNOWN;
    ClocksDataSource clocksDataSource = ClocksDataSource::SOURCE_UNKNOWN;
    TemperatureSensor currentTemperatureSensor = TemperatureSensor::TS_UNKNOWN;
    GPUSysInfo sysInfo;

    // base on files  pp_dpm_sclk and  pp_dpm_mclk
    QStringList sclkTable, mclkTable;

    // base on file pp_od_clk_voltage
    MapFVTables currentStatesTables;
    MapOCRanges ocRages;

    // based on pp_power_profile_mode or power_dpm_state or power_profile
    PowerProfiles powerProfiles;

    // list of temperature sensor. Maps hwmon sensor id to ValueID::instance
    // and provides a list of available instances
    QVector<ValueID::Instance> tempSensors;

    DriverFeatures() { }
};

static bool checkFileCorrectness(const QString &fileName, const bool isZeroValidValue = false) {
    if (!QFile::exists(fileName))
        return false;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QString value = QString(f.readAll().trimmed());
    f.close();

    if (value.isEmpty())
        return false;

    if (isZeroValidValue == false && value == "0")
        return false;

    return true;
}

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
    gpu_busy_percent,
    pp_od_clk_voltage,
    pp_power_profile_mode;

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
        pp_od_clk_voltage = devicePath + "pp_od_clk_voltage";
        pp_power_profile_mode = devicePath + "pp_power_profile_mode";

        if (!checkFileCorrectness(power_method))
            power_method = "";

        if (!checkFileCorrectness(power_profile))
            power_profile = "";

        if (!checkFileCorrectness(power_dpm_state))
            power_dpm_state = "";

        if (!checkFileCorrectness(power_dpm_force_performance_level))
            power_dpm_force_performance_level = "";

        if (!checkFileCorrectness(pp_sclk_od, true))
            pp_sclk_od = "";

        if (!checkFileCorrectness(pp_mclk_od, true))
            pp_mclk_od = "";

        if (!checkFileCorrectness(pp_dpm_sclk))
            pp_dpm_sclk = "";

        if (!checkFileCorrectness(pp_dpm_mclk))
            pp_dpm_mclk = "";

        if (!checkFileCorrectness(gpu_busy_percent, true))
            gpu_busy_percent = "";

        if (!checkFileCorrectness(pp_od_clk_voltage))
            pp_od_clk_voltage = "";

        if (!checkFileCorrectness(pp_power_profile_mode))
            pp_power_profile_mode = "";
    }
};


struct HwmonTempSensor {
    QString input;
    QString crit;
    QString emergency;
    QString label;

    HwmonTempSensor() = default;

    HwmonTempSensor(const HwmonTempSensor&) = default;

    HwmonTempSensor(HwmonTempSensor&&) = default;

    HwmonTempSensor(const QString& input_path,
                    const QString& crit_path,
                    const QString& emergency_path,
                    const QString& label_path)
    {
        // create empty object if input is invalid. operator bool returns false
        if (checkFileCorrectness(input_path))
            input = input_path;
        else
            return;
        // leave label, emerg and crit empty, if they don't exist or are empty
        if (checkFileCorrectness(crit_path))
            crit = crit_path;
        if (checkFileCorrectness(emergency_path))
            emergency = emergency_path;
        if (checkFileCorrectness(label_path))
            label = label_path;
    }


    HwmonTempSensor(const QString& hwmonPath, int index)
        : HwmonTempSensor(buildPath(hwmonPath, index, "input"),
                          buildPath(hwmonPath, index, "crit"),
                          buildPath(hwmonPath, index, "emergency"),
                          buildPath(hwmonPath, index, "label"))
    {
    }


    operator bool () const
    {
        return !input.isEmpty();
    }


    static QString buildPath(const QString& hwmonPath,
                             int index,
                             const char* suffix = "input")
    {
        return hwmonPath + "temp" + QString::number(index) + "_" + suffix;
    }


    static bool exists(const QString& hwmonPath, int index)
    {
        return checkFileCorrectness(buildPath(hwmonPath, index, "input"));
    }
};

struct HwmonAttributes {
    QString
    pwm1,
    pwm1_enable,
    pwm1_max,
    fan1_input,
    power1_cap_max,
    power1_cap_min,
    power1_cap,
    power1_average;

    QVector<HwmonTempSensor> temp;

    HwmonAttributes() { }

    HwmonAttributes(const QString &hwmonPath) {
        for (int i = 1; HwmonTempSensor::exists(hwmonPath, i); i++) {
            temp.push_back(HwmonTempSensor(hwmonPath, i));
        }

        pwm1 = hwmonPath + "pwm1";
        pwm1_enable = hwmonPath + "pwm1_enable";
        pwm1_max = hwmonPath + "pwm1_max";
        fan1_input = hwmonPath + "fan1_input";
        power1_cap_max = hwmonPath + "power1_cap_max";
        power1_cap_min = hwmonPath + "power1_cap_min";
        power1_cap = hwmonPath + "power1_cap";
        power1_average = hwmonPath + "power1_average";

        if (!checkFileCorrectness(pwm1_enable, true))
            pwm1 = pwm1_enable = pwm1_max = "";

        if (!checkFileCorrectness(fan1_input, true))
            fan1_input = "";

        if (!checkFileCorrectness(power1_cap))
            power1_cap = power1_cap_min = power1_cap_max = power1_average = "";
    }
};

struct DeviceFilePaths {
    QString debugfs_pm_info, moduleParams;
    DeviceSysFs sysFs;
    HwmonAttributes hwmonAttributes;
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
     int pwmMaxSpeed = -1, maxCoreClock = -1, maxMemClock = -1, power1_cap_max = -1, power1_cap_min = -1;
     float VRAMSize = -1;
     QHash<ValueID::Instance, int> temp_crit;
     QHash<ValueID::Instance, int> temp_emergency;
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

            case ValueID::POWER_CAP_SELECTED:
            case ValueID::POWER_CAP_AVERAGE:
                return ValueUnit::WATT;

            default:
                return ValueUnit::NONE;
        }
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
            case ValueID::POWER_CAP_SELECTED:
            case ValueID::POWER_CAP_AVERAGE:
                return true;

            default:
               return false;
        }
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
            case ValueID::TEMPERATURE_CURRENT: return getNameOfTempSensor(id);
            case ValueID::TEMPERATURE_MAX: return getNameOfTempSensor(id) + " " + QObject::tr("(max)");
            case ValueID::TEMPERATURE_MIN: return getNameOfTempSensor(id) + " " + QObject::tr("(min)");
            case ValueID::GPU_VRAM_USAGE_MB:  return QObject::tr("GPU Vram megabyte usage");
            case ValueID::POWER_LEVEL: return QObject::tr("Power level");
            case ValueID::POWER_CAP_SELECTED: return QObject::tr("Power cap selected");
            case ValueID::POWER_CAP_AVERAGE: return QObject::tr("Power cap average");

            default:
                 return "";
        }
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
            case ValueID::GPU_VRAM_USAGE_MB:  return QObject::tr("GPU Vram usage [MB]");
            case ValueID::POWER_LEVEL: return QObject::tr("Power level ");
            case ValueID::POWER_CAP_SELECTED: return  QObject::tr("Power cap selected [W]");
            case ValueID::POWER_CAP_AVERAGE: return  QObject::tr("Power cap average [W]");

            case ValueID::TEMPERATURE_CURRENT:
            case ValueID::TEMPERATURE_MAX:
            case ValueID::TEMPERATURE_MIN:
                return getNameOfValueID(id) + " " + QObject::tr("[\u00B0C]");

            default:
                return "";
        }
    }

    static QString getNameOfUnit(ValueUnit u) {
        switch (u) {
            case ValueUnit::MEGAHERTZ: return QObject::tr("Megahertz [MHz]");
            case ValueUnit::MEGABYTE: return QObject::tr("Megabyte [MB]");
            case ValueUnit::PERCENT: return QObject::tr("Percent [%]");
            case ValueUnit::MILIVOLT: return QObject::tr("Milivolt [mV]");
            case ValueUnit::CELSIUS: return QObject::tr("Temperature [")+QString::fromUtf8("\u00B0C]");
            case ValueUnit::RPM: return QObject::tr("Speed [RPM]");
            case ValueUnit::WATT: return QObject::tr("Watt [W]");

            default:
                return "";
        }
    }

    static QStringList createPowerLevelCombo(const DriverModule dm) {
        switch (dm) {
            case DriverModule::RADEON:
                return QStringList() << level_auto << level_low << level_high;
            case DriverModule::AMDGPU:
                return QStringList() << level_auto << level_low << level_high << level_manual <<
                                        level_profile_standard << level_profile_min_sclk << level_profile_min_mclk << level_profile_peak;
            default:
                break;
        }

        return QStringList();
    }

    /* stores labels of names from ValueID types of an unknown instance.
     * This will only be used if instances of ValueID types are supported
     * by the hardware that could not be recognized.
     * For unknown temperature sensors, the temp*_label will be stored here
     * to be used by getNameOfValueID() to give a meaningful name of the
     * sensor to the user.
     */
    static QHash<ValueID, QString> customInstanceLabels;

    static ValueID::Instance setCustomInstanceLabel(ValueID::Type type,
                                                    const QString& label)
    {
        ValueID::Instance instance = ValueID::INSTANCE_UNKNOWN
                                   + customInstanceLabels.size();
        customInstanceLabels.insert(ValueID(type, instance), label);
        return instance;
    }


private:

    static QString getNameOfTempSensor(ValueID id)
    {
        switch (id.instance) {

            case ValueID::T_EDGE: return QObject::tr("Edge Temperature");
            case ValueID::T_JUNCTION: return QObject::tr("Junction Temperature");
            case ValueID::T_MEM: return QObject::tr("Memory Temperature");

            default:
                const ValueID custom_key = id.asType(ValueID::TEMPERATURE_CURRENT);
                if (customInstanceLabels.contains(custom_key)) {
                    return customInstanceLabels[custom_key]
                         + " " + QObject::tr("Temperature");
                } else {
                    return QObject::tr("Unknown Temperature Sensor");
                }
        }
    }
};


#endif // PUBLICSTUFF_H
