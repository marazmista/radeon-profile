#ifndef DINTEL_H
#define DINTEL_H

#include "gpu.h"

class dIntel : public gpu
{
public:
    dIntel();

    void changeGPU(ushort gpuIndex);
    driverFeatures figureOutDriverFeatures();

    gpuClocksStruct getClocks(bool forFeatures = false);
    float getTemperature() const;

private:
    QString actFreq, curFreq, minFreq, maxFreq;
};

#endif // DINTEL_H
