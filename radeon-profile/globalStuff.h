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

    static QStringList createDPMCombo() {
        return QStringList() << dpm_battery << dpm_balanced << dpm_performance;
    }

    static QStringList createPowerLevelCombo() {
        return QStringList() << dpm_auto << dpm_low << dpm_high;
    }

    static QStringList createProfileCombo() {
        return QStringList() << profile_auto << profile_default << profile_high << profile_mid << profile_low;
    }

    enum powerProfiles {
        BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
    };

    enum forcePowerLevels {
        F_AUTO, F_LOW, F_HIGH
    };

    enum driverModule {
        RADEON, AMDGPU, MODULE_UNKNOWN
    };

    enum clocksDataSource {
        IOCTL, PM_FILE, SOURCE_UNKNOWN
    };

//    enum pwmControl {
//        PWM_DISABLED, PWM_MANUAL, PWM_AUTO
//    };

    enum powerMethod {
        DPM = 0,  // kernel >= 3.11
        PROFILE = 1,  // kernel <3.11 or dpm disabled
        PM_UNKNOWN = 2
    };

    enum tempSensor {
        SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
        CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
        PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
        MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
        TS_UNKNOWN
    };

    struct gpuSysInfo {
        QString sysName, driverModuleString;
        globalStuff::driverModule module;
    };

    // structure which holds what can be display on ui and on its base
    // we enable ui elements
    struct driverFeatures {
        bool
        canChangeProfile = false,

        clkCoreAvailable = false,
        clkMemAvailable = false,
        voltCoreAvailable = false,
        voltMemAvailable = false,

        temperatureAvailable = false,

        pwmAvailable = false,
        fanRpmInfoAvailable = false,

        ocCoreAvailable = false,
        ocMemAvailable = false;


        globalStuff::powerMethod currentPowerMethod;
        globalStuff::clocksDataSource clocksSource;
        globalStuff::tempSensor currentTemperatureSensor;
        globalStuff::gpuSysInfo sysInfo;

        driverFeatures() {
            currentPowerMethod = PM_UNKNOWN;
            currentTemperatureSensor = TS_UNKNOWN;
        }
    };

    struct gpuClocksStructString {
      QString powerLevel, coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
    };

    struct gpuClocksStruct {
        int coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
        char powerLevel;
        gpuClocksStructString str;

        gpuClocksStruct() {
            coreClk = memClk =  coreVolt = memVolt = uvdCClk = uvdDClk = powerLevel = -1;
        }

        void convertToString() {
            str.coreClk = (coreClk != -1) ? QString::number(coreClk)+"MHz" : "";
            str.memClk = (memClk != -1) ? QString::number(memClk)+"MHz" : "";
            str.memVolt = (memVolt != -1) ? QString::number(memVolt)+"mV" : "";
            str.coreVolt = (coreVolt != -1) ? QString::number(coreVolt)+"mV" : "";
            str.uvdCClk = (uvdCClk != -1) ? QString::number(uvdCClk)+"MHz" : "";
            str.uvdDClk =  (uvdDClk != -1) ?  QString::number(uvdDClk)+"MHz" : "";


            if (powerLevel != -1)
                str.powerLevel =  QString::number(powerLevel);
        }
    };

    struct gpuTemperatureStructString {
        QString current, max, min;
    };

    struct gpuTemperatureStruct {
        float current, currentBefore, max, min, sum;
        gpuTemperatureStructString str;

        void convertToString() {
            str.current = QString::number(current) + QString::fromUtf8("\u00B0C");
            str.max = QString::number(max) + QString::fromUtf8("\u00B0C");
            str.min = QString::number(min) + QString::fromUtf8("\u00B0C");
        }
    };

    struct gpuPwmStructString {
        QString pwmSpeed, pwmSpeedRpm;
    };

    struct gpuPwmStruct {
        int pwmSpeed = 0, pwmSpeedRpm;
        gpuPwmStructString str;

        void convertToString() {
            str.pwmSpeed = QString::number(pwmSpeed) + "%";
            str.pwmSpeedRpm = QString::number(pwmSpeedRpm) + "RPM";
        }
    };

    struct gpuUsageStructString {
        QString gpuLoad, gpuVramLoadPercent, gpuVramLoad;
    };

    struct gpuUsageStruct {
        float gpuLoad, gpuVramLoadPercent, gpuVramLoad;
        gpuUsageStructString str;

        gpuUsageStruct() {
            gpuLoad = gpuVramLoad = gpuVramLoadPercent = -1;
        }

        void convertToString() {
             str.gpuLoad = (gpuLoad != -1) ? QString::number(gpuLoad,'f',1) + "%" : "";
             str.gpuVramLoadPercent = (gpuVramLoadPercent != -1) ? QString::number(gpuVramLoadPercent) + "%" : "";
             str.gpuVramLoad = (gpuVramLoad != -1) ? QString::number(gpuVramLoad/1024/1024) + "MB" : "";
        }
    };

    struct gpuConstParams {
         int pwmMaxSpeed, maxCoreClock = -1, maxMemClock = -1, vRamSize = -1;
    };

    // settings from config used across the source
    static struct globalCfgStruct {
        float interval;
        bool daemonAutoRefresh, rootMode;
        int graphOffset;
    } globalConfig;
};


#endif // PUBLICSTUFF_H
