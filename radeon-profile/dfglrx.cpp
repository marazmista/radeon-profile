// copyright marazmista @ 29.03.2014

#include "dfglrx.h"

// define static members //
char dFglrx::gpuIndex = 0;
// end //

void dFglrx::configure(char _gpuIndex) {
    gpuIndex = _gpuIndex;
}

globalStuff::gpuClocksStruct dFglrx::getClocks() {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --odgc --adapter=" + gpuIndex);
    // QFile f("/mnt/stuff/catalyst/odgc");

    // f.open(QIODevice::ReadOnly);
    // QStringList out = QString(f.readAll()).split('\n');
    // f.close();

    out = out.filter("Clocks");

    QRegExp rx;
    rx.setPattern("\\s+\\d+\\s+\\d+");
    rx.indexIn(out[0]);

    QStringList gData = rx.cap(0).trimmed().split(" ");
    globalStuff::gpuClocksStruct tData;
    tData.coreClk = gData[0].toInt();
    tData.memClk = gData[1].toInt();
    tData.coreVolt = -1;
    tData.memVolt = -1;
    tData.powerLevel = -1;
    tData.uvdCClk = -1;
    tData.uvdDClk = -1;
    return tData;
}

float dFglrx::getTemperature() {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --odgt --adapter=" + gpuIndex);
// QFile f("/mnt/stuff/catalyst/odgt");

// f.open(QIODevice::ReadOnly);
// QStringList out = QString(f.readAll()).split('\n');
// f.close();

   out = out.filter("Sensor");

   QRegExp rx;
   rx.setPattern("\\d+\\.\\d+");
   rx.indexIn(out[0]);
   return rx.cap(0).toFloat();
}

QStringList dFglrx::detectCards() {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --list-adapters");

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


}

