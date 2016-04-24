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

#define label_currentPowerLevel QObject::tr("Power level")
#define label_currentGPUClock QObject::tr("GPU clock")
#define label_currentMemClock QObject::tr("Memory clock")
#define label_uvdVideoCoreClock QObject::tr("UVD core clock (cclk)")
#define label_uvdDecoderClock QObject::tr("UVD decoder clock (dclk)")
#define label_vddc QObject::tr("GPU voltage (vddc)")
#define label_vddci QObject::tr("I/O voltage (vddci)")
#define label_currentGPUTemperature QObject::tr("GPU temperature")
#define label_errorReadingData QObject::tr("Can't read data")
#define label_howToReadData QObject::tr("You need debugfs mounted and either root rights or the daemon running")
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
#define label_temperature QObject::tr("Temperature")
#define label_fanSpeed QObject::tr("Fan speed")
#define label_disconnected QObject::tr("Disconnected")

#define format_screenIndex QObject::tr("Virtual screen n°%n", NULL, screenIndex)
#define format_scrWidthMM QObject::tr("%n mm x ", NULL, DisplayWidthMM(display, screenIndex))
#define format_scrHeightMM QObject::tr("%n mm", NULL, DisplayHeightMM(display, screenIndex))
#define format_monWidth QObject::tr("%n mm x ", NULL, outputInfo->mm_width)
#define format_monHeight QObject::tr("%n mm ", NULL, outputInfo->mm_height)
#define format_monDiagonal QObject::tr("(%n inches)", NULL, diagonal)
#define format_currentTemp QObject::tr("%n°C", NULL, device.gpuTemeperatureData.current)
#define format_GPUClock QObject::tr("%nMHz", NULL, device.gpuClocksData.coreClk)
#define format_memoryClock QObject::tr("%nMHz", NULL, device.gpuClocksData.memClk)
#define format_UVDClock QObject::tr("%nMHz", NULL, device.gpuClocksData.uvdCClk)
#define format_UVDDecoderClock QObject::tr("%nMHz", NULL, device.gpuClocksData.uvdDClk)
#define format_GPUVoltage QObject::tr("%nmV", NULL, device.gpuClocksData.coreVolt)
#define format_memoryVoltage QObject::tr("%nmV", NULL, device.gpuClocksData.memVolt)
#define format_fanSpeed QObject::tr("%n%", NULL, device.gpuTemeperatureData.pwmSpeed)
#define format_stats_powerLevel QObject::tr("Power level %n ", NULL, device.gpuClocksData.powerLevel)
#define format_stats_GPUClock QObject::tr("(GPU@%nMHz,", NULL, device.gpuClocksData.coreClk)
#define format_stats_GPUVolt QObject::tr("%nmV)", NULL, device.gpuClocksData.coreVolt)
#define format_stats_memClock QObject::tr("(Mem@%nMHz,", NULL, device.gpuClocksData.memClk)
#define format_stats_memVolt QObject::tr("%nmV)", NULL, device.gpuClocksData.memVolt)

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

    struct gpuClocksStruct {
        int coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
        char powerLevel;

        gpuClocksStruct() { }

        gpuClocksStruct(int _coreClk, int _memClk, int _coreVolt, int _memVolt, int _uvdCClk, int _uvdDclk, char _pwrLevel ) {
            coreClk = _coreClk,
                    memClk = _memClk,
                    coreVolt = _coreVolt,
                    memVolt = _memVolt,
                    uvdCClk = _uvdCClk,
                    uvdDClk = _uvdDclk,
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
        bool canChangeProfile, coreClockAvailable, memClockAvailable, coreVoltAvailable, memVoltAvailable, temperatureAvailable, pwmAvailable;
        globalStuff::powerMethod pm;
        int pwmMaxSpeed;

        driverFeatures() {
            canChangeProfile = coreClockAvailable = memClockAvailable = coreVoltAvailable = memVoltAvailable = temperatureAvailable= pwmAvailable = false;
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
