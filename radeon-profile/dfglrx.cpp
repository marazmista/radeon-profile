// copyright marazmista @ 29.03.2014

#include "dfglrx.h"
//#include <QFile>

// define static members //
char dFglrx::gpuIndex = 0;
// end //

void dFglrx::configure(char _gpuIndex) {
    gpuIndex = _gpuIndex;
}

globalStuff::gpuClocksStruct dFglrx::getClocks(){
    QStringList out = globalStuff::grabSystemInfo("aticonfig --odgc --adapter=" + gpuIndex);
    //     QFile f("/home/mm/odgc");
    //     f.open(QIODevice::ReadOnly);
    //     QStringList out = QString(f.readAll()).split('\n');
    //     f.close();
    globalStuff::gpuClocksStruct tData(-1);
    out = out.filter("Clocks");
    if (!out.empty()) {
        QRegExp rx;
        rx.setPattern("\\s+\\d+\\s+\\d+");
        rx.indexIn(out[0]);
        QStringList gData = rx.cap(0).trimmed().split("           ");

        tData.coreClk = gData[0].toInt();
        tData.memClk = gData[1].toInt();
    }
    return tData;
}

float dFglrx::getTemperature() {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --odgt --adapter=" + gpuIndex);
    //    QFile f("/home/mm/odgt");
    //    f.open(QIODevice::ReadOnly);
    //    QStringList out = QString(f.readAll()).split('\n');
    //    f.close();
    out = out.filter("Sensor");
    if (!out.empty()) {
        QRegExp rx;
        rx.setPattern("\\d+\\.\\d+");
        rx.indexIn(out[0]);
        return rx.cap(0).toFloat();
    }
    return 0;
}

QStringList dFglrx::detectCards() {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --list-adapters");
//     QFile f("/home/mm/lsa");
//    f.open(QIODevice::ReadOnly);
//    QStringList out = QString(f.readAll()).split('\n');
//    f.close();
    out = out.filter("Radeon");
    return out;
}

QStringList dFglrx::getGLXInfo() {
    QStringList data, gpus = globalStuff::grabSystemInfo("lspci").filter("Radeon",Qt::CaseInsensitive);
    gpus.removeAt(gpus.indexOf(QRegExp(".+Audio.+"))); //remove radeon audio device

    // loop for multi gpu
    for (int i = 0; i < gpus.count(); i++)
        data << "VGA:"+gpus[i].split(":",QString::SkipEmptyParts)[2];

    QStringList out = globalStuff::grabSystemInfo("fglrxinfo");
    data << out.filter(QRegExp("\\.+"));
    return data;
}

globalStuff::driverFeatures dFglrx::figureOutDriverFeatures() {
    globalStuff::driverFeatures features;
    features.canChangeProfile = false;
    features.clocksAvailable = true;
    features.voltAvailable = false;
    features.temperatureAvailable = true;

    return features;
}

