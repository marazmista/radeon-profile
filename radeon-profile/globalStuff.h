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

    struct gpuClocksStruct{
        int coreClk, memClk, coreVolt, memVolt, uvdCClk, uvdDClk;
        char powerLevel;
    };

    struct driverFeatures {
        bool canChangeProfile, clocksAvailable, voltAvailable, temperatureaAvailable;
    };

    struct gpuTemperatureStruct{
        float current, max, min, sum;
    };

    enum powerProfiles {
        BATTERY, BALANCED, PERFORMANCE, AUTO, DEFAULT, LOW, MID, HIGH
    };

    enum forcePowerLevels {
        F_AUTO, F_LOW, F_HIGH
    };
};
#endif // PUBLICSTUFF_H
