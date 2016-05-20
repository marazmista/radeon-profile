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

#define label_powerLevel QObject::tr("Power level")
#define label_currentGPUClock QObject::tr("GPU clock")
#define label_currentMemClock QObject::tr("Memory clock")
#define label_uvdVideoCoreClock QObject::tr("UVD core clock (cclk)")
#define label_uvdDecoderClock QObject::tr("UVD decoder clock (dclk)")
#define label_vddc QObject::tr("GPU voltage (vddc)")
#define label_vddci QObject::tr("I/O voltage (vddci)")
#define label_currentGPUTemperature QObject::tr("GPU temperature")
#define label_temperature QObject::tr("Temperature")

// uiElements.cpp
#define label_timeAxis QObject::tr("Time (s)")
#define label_temperatureAxis QObject::tr("Temperature (°C)")
#define label_clockAxis QObject::tr("Clock (MHz)")
#define label_voltageAxis QObject::tr("Voltage (mV)")
#define label_fanSpeed QObject::tr("Fan speed")
#define label_dpm QObject::tr("DPM")
#define label_changeProfile QObject::tr("Change standard profile")
#define label_quit QObject::tr("Quit")
#define label_refreshWhenHidden QObject::tr("Keep refreshing when hidden")
#define label_battery QObject::tr("Battery")
#define label_balanced QObject::tr("Balanced")
#define label_performance QObject::tr("Performance")
#define label_resetMinMax QObject::tr("Reset min/max temperature")
#define label_resetGraphs QObject::tr("Reset graphs vertical scale")
#define label_showLegend QObject::tr("Show legend")
#define label_graphOffset QObject::tr("Graph offset on right")
#define label_auto QObject::tr("Auto")
#define label_low QObject::tr("Low")
#define label_high QObject::tr("High")
#define label_forcePowerLevel QObject::tr("Force power level")
#define label_copyToClipboard QObject::tr("Copy to clipboard")
#define label_resetStatistics QObject::tr("Reset statistics")

// uiEvents.cpp
#define label_dataDisabled QObject::tr("GPU data is disabled")
#define label_fanControlInfo QObject::tr("Fan control information")
#define label_fanControlWarning QObject::tr("Don't overheat your card! Be careful! Don't use this if you don't know what you're doing! \n\nHovewer, looks like card won't apply too low values due its internal protection. \n\nClosing application will restore fan control to Auto. If application crashes, last fan value will remain, so you have been warned!")
#define label_fanSpeedRange QObject::tr("Speed [%] (20-100)")
#define label_selectProfile QObject::tr("Select new power profile")
#define label_profileSelection QObject::tr("Profile selection")
#define label_processStillRunning QObject::tr("Process is still running. Close tab?")
#define label_howToEdit QObject::tr("This step already exists. To edit it double click it")
#define label_cantDeleteThisItem QObject::tr("You can't delete the first and the last item")
#define label_cantEditThisItem QObject::tr("You can't edit the first and the last item")
#define label_overclockFailed QObject::tr("An error occurred, overclock failed")

// execbin.cpp
#define label_processRunning QObject::tr("Process state: running")
#define label_processNotRunning QObject::tr("Process state: not running")
#define label_saveOutputToFile QObject::tr("Save output to file")
#define label_command QObject::tr("Command")
#define label_output QObject::tr("Output")
#define label_save QObject::tr("Save")

// execTab.cpp
#define label_error QObject::tr("Error")
#define label_emptyProfileName QObject::tr("Profile name can't be empty!")
#define label_noBinarySelected QObject::tr("No binary is selected!")
#define label_binaryNotFound QObject::tr("Binary not found in /usr/bin: ")
#define label_enterValue QObject::tr("Enter value")
#define label_valueFor QObject::tr("Enter valid value for ")
#define label_question QObject::tr("Question")
#define label_askRemoveItem QObject::tr("Remove this item?")
#define label_remove QObject::tr("Remove")
#define label_selectBinary QObject::tr("Select binary")
#define label_selectLogFile QObject::tr("Select log file")
#define label_cantRunNotExists QObject::tr("Can't run something that not exists!")
#define label_run QObject::tr("Run")
#define label_askRunStart QObject::tr("Run: \"")
#define label_askRunEnd QObject::tr("\"?")

// gpu.cpp
#define label_unknown QObject::tr ("Unknown")
#define label_resolution QObject::tr("Resolution")
#define label_minimumRes QObject::tr("Minimum resolution")
#define label_maximumRes QObject::tr("Maximum resolution")
#define label_virtualSize QObject::tr("Virtual size")
#define label_outputs QObject::tr("Outputs")
#define label_active QObject::tr("Active")
#define label_verticalHz QObject::tr(" Hz vertical, ")
#define label_horizontalKHz QObject::tr(" KHz horizontal, ")
#define label_dotClock QObject::tr(" MHz dot clock")
#define label_yes QObject::tr("Yes")
#define label_no QObject::tr("No")
#define label_refreshRate QObject::tr("Refresh rate")
#define label_offset QObject::tr("Offset")
#define label_softBrightness QObject::tr("Brightness (software)")
#define label_size QObject::tr("Size")
#define label_supportedModes QObject::tr("Supported modes")
#define label_properties QObject::tr("Properties")
#define label_serialNumber QObject::tr("Serial number")
#define label_notAvailable QObject::tr("Not available")
#define label_connectedWith QObject::tr("Connected with ")
#define label_disconnected QObject::tr("Disconnected")
#define format_screenIndex QObject::tr("Virtual screen n°%n", NULL, screenIndex)
#define format_scrWidthMM QObject::tr("%n mm x ", NULL, DisplayWidthMM(display, screenIndex))
#define format_scrHeightMM QObject::tr("%n mm", NULL, DisplayHeightMM(display, screenIndex))
#define format_monWidth QObject::tr("%n mm x ", NULL, outputInfo->mm_width)
#define format_monHeight QObject::tr("%n mm ", NULL, outputInfo->mm_height)
#define format_monDiagonal QObject::tr("(%n inches)", NULL, diagonal)
#define format_outputConnected QObject::tr("%n connected, ", NULL, screenConnectedOutputs)
#define format_outputActive QObject::tr("%n active", NULL, screenActiveOutputs)
#define label_noInfo QObject::tr("No info")

// radeon_profile.cpp
#define label_backToProfiles QObject::tr("Back to profiles") // As "return to profiles"
#define label_errorReadingData QObject::tr("Can't read data")
#define label_howToReadData QObject::tr("You need debugfs mounted and either root rights or the daemon running")
#define label_noOverclock QObject::tr("Your driver or your GPU does not support overclocking")

#define logDateFormat "yyyy-MM-dd_hh-mm-ss"

#define appVersion 20160515
#define format_version QObject::tr("version %n", NULL, appVersion)


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
