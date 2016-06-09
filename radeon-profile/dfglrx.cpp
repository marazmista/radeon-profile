// copyright marazmista @ 29.03.2014

#include "dfglrx.h"


dFglrx::dFglrx(){
    driverModule = "fglrx";

    qDebug() << "Using fglrx driver";
}

void dFglrx::changeGPU(ushort _gpuIndex) {
    if(_gpuIndex >= gpuList.size())
        qWarning() << "Requested unexisting card " << _gpuIndex;
    else
        currentGpuIndex = _gpuIndex;
}

gpuClocksStruct dFglrx::getClocks(bool forFeatures){
    UNUSED(forFeatures);
    const QStringList out = globalStuff::grabSystemInfo("aticonfig --odgc --adapter=" + QString::number(currentGpuIndex)).filter("Clocks");
    //     QFile f("/home/mm/odgc");
    //     f.open(QIODevice::ReadOnly);
    //     QStringList out = QString(f.readAll()).split('\n');
    //     f.close();
    gpuClocksStruct tData;
    if (!out.empty()) {
        QRegExp rx;
        rx.setPattern("\\s+\\d+\\s+\\d+");
        rx.indexIn(out[0]);
        QStringList gData = rx.cap(0).trimmed().split("           ");

        tData.coreClk = gData[0].toShort();
        tData.coreClkOk = tData.coreClk > 0;
        tData.memClk = gData[1].toShort();
        tData.memClkOk = tData.memClk > 0;
    }
    return tData;
}

float dFglrx::getTemperature() const {
    QStringList out = globalStuff::grabSystemInfo("aticonfig --odgt --adapter=" + QString::number(currentGpuIndex));
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

QStringList dFglrx::detectCards() const {
//     QFile f("/home/mm/lsa");
//    f.open(QIODevice::ReadOnly);
//    QStringList out = QString(f.readAll()).split('\n');
//    f.close();
    return globalStuff::grabSystemInfo("aticonfig --list-adapters").filter("Radeon");;
}

QStringList dFglrx::getGLXInfo(const QString & gpuName) const {
    QStringList list = gpu::getGLXInfo(gpuName);
    list.append(globalStuff::grabSystemInfo("fglrxinfo").filter(QRegExp("\\.+")));
    return list;
}

driverFeatures dFglrx::figureOutDriverFeatures() {
    driverFeatures features;
    features.coreClockAvailable = true;
    features.memClockAvailable = true;
    features.temperatureAvailable = true;
    // Other features are automatically initialized as false
    return features;
}

