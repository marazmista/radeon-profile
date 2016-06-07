#include "dintel.h"

#define file_actFreq "gt_act_freq_mhz"
#define file_curFreq "gt_cur_freq_mhz"
// Difference between atc_freq and cur_freq: https://lists.freedesktop.org/archives/intel-gfx/2015-January/059063.html
#define file_minFreq "gt_min_freq_mhz"
#define file_maxFreq "gt_max_freq_mhz"



dIntel::dIntel()
{
    driverModule = "i915";

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
}

driverFeatures dIntel::figureOutDriverFeatures(){
    driverFeatures features;

    gpuClocksData = getClocks();
    features.coreClockAvailable = gpuClocksData.coreClkOk;

    return features;
}

gpuClocksStruct dIntel::getClocks(bool forFeatures) {
    gpuClocksStruct data;
    data.coreClk = readFile(actFreq).toShort();
    data.coreClkOk = data.coreClk > 0;
    return data;
}
