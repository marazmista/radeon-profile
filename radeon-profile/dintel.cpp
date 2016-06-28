#include "dintel.h"

#define file_actFreq "gt_act_freq_mhz"
#define file_curFreq "gt_cur_freq_mhz"
// Difference between atc_freq and cur_freq: https://lists.freedesktop.org/archives/intel-gfx/2015-January/059063.html
#define file_minFreq "gt_min_freq_mhz"
#define file_maxFreq "gt_max_freq_mhz"



dIntel::dIntel() : gpu()
{
    driverModule = "i915";

    gpuList = detectCards();

    qDebug() << "Using intel i915 driver";
}

void dIntel::changeGPU(ushort gpuIndex){
    if(gpuIndex >= gpuList.size()){
        qWarning() << "Requested unexisting card " << gpuIndex;
        return;
    }

    currentGpuIndex = gpuIndex;
    const QString gpuName = gpuList[gpuIndex], device = "/sys/class/drm/" + gpuName + '/';
    qDebug() << "dIntel: selecting gpu " << gpuName;

    actFreq = device + file_actFreq;
    curFreq = device + file_curFreq;
    minFreq = device + file_minFreq;
    maxFreq = device + file_maxFreq;


    features = figureOutDriverFeatures();
}

driverFeatures dIntel::figureOutDriverFeatures(){
    driverFeatures features;

    gpuClocksData = getClocks(true);
    features.coreClockAvailable = gpuClocksData.coreClkOk;

    features.temperatureAvailable = getTemperature() > 0;

    return features;
}

gpuClocksStruct dIntel::getClocks(bool forFeatures) {
    if(forFeatures){
        const QString max = readFile(maxFreq), min = readFile(minFreq);

        maxCoreFreq = max.toUShort(&maxCoreFreqAvailable);
        maxCoreFreqAvailable &= maxCoreFreq > 0;
        if(maxCoreFreqAvailable)
            maxCoreFreqString = max + " MHz";

        minCoreFreq = min.toUShort(&minCoreFreqAvailable);
        minCoreFreqAvailable &= minCoreFreq > 0;
        if(minCoreFreqAvailable)
            minCoreFreqString = min + " MHz";
    }

    gpuClocksStruct data;
    data.coreClk = readFile(actFreq).toUShort();
    data.coreClkOk = data.coreClk > 0;
    return data;
}

temp dIntel::getTemperature() const {
    // Integrated graphics temperature == CPU temperature
    const QStringList sensors = globalStuff::grabSystemInfo("sensors").filter("Core ");

    // Average temperature
    temp sum = 0;
    ushort count = 0;

    for(const QString & line : sensors){
        const float temp = line.split(" ",QString::SkipEmptyParts)[2].remove("+").remove("C").remove("°").toFloat();
        if(temp > 0){
            sum += temp;
            count++;
        }
    }

    if(count == 0)
        return 0;
    else
        return sum / count;
}
