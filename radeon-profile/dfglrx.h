// copyright marazmista @ 29.03.2014

// class for support FGLRX driver for Radeon cards //

#ifndef DFGLRX_H
#define DFGLRX_H

#include "globalStuff.h"

#include <QTreeWidgetItem>

class dFglrx
{
public:

    static gpuClocksStruct getClocks();
    static float getTemperature();
    static QList<QTreeWidgetItem *> getCardConnectors();
    static QStringList getGLXInfo();
    static QStringList detectCards();
    static void configure(char _gpuIndex);
    static driverFeatures figureOutDriverFeatures();
private:
   static char gpuIndex;

};

#endif // DFGLRX_H
