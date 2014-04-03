// copyright marazmista @ 29.03.2014

// class for stuff that is used in other classes around source code //

#ifndef PUBLICSTUFF_H
#define PUBLICSTUFF_H

#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>

class globalStuff {
public:
    static QStringList grabSystemInfo(const QString cmd) {
        QProcess *p = new QProcess();
        p->setProcessChannelMode(QProcess::MergedChannels);

        p->start(cmd,QIODevice::ReadOnly);
        p->waitForFinished();

        return QString(p->readAllStandardOutput()).split('\n');
    }

    static QStringList grabSystemInfo(const QString cmd, const QProcessEnvironment env) {
        QProcess *p = new QProcess();
        p->setProcessChannelMode(QProcess::MergedChannels);
        p->setProcessEnvironment(env);

        p->start(cmd,QIODevice::ReadOnly);
        p->waitForFinished();

        return QString(p->readAllStandardOutput()).split('\n');
    }

    enum powerProfiles {
        BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
    };

    enum forcePowerLevels {
        F_AUTO, F_LOW, F_HIGH
    };

    enum powerMethod {
        DPM = 0,  // kernel >= 3.11
        PROFILE = 1,  // kernel <3.11 or dpm disabled
        PM_UNKNOWN = 2
    };

    struct gpuClocksStruct{
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

    struct driverFeatures {
        bool canChangeProfile, clocksAvailable, voltAvailable, temperatureAvailable;
        globalStuff::powerMethod pm;
    };

    struct gpuTemperatureStruct{
        float current, max, min, sum;
    };

};
#endif // PUBLICSTUFF_H
