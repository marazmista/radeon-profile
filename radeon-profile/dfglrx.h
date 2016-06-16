// copyright marazmista @ 29.03.2014

// class for support FGLRX driver for Radeon cards //

#ifndef DFGLRX_H
#define DFGLRX_H

#include "globalStuff.h"
#include "gpu.h"


class dFglrx : public gpu
{
public:
    dFglrx();

    gpuClocksStruct getClocks(bool forFeatures = false);
    float getTemperature() const;
    QStringList getGLXInfo(const QString & gpuName) const;
    QStringList detectCards() const;
    driverFeatures figureOutDriverFeatures();

};

#endif // DFGLRX_H
