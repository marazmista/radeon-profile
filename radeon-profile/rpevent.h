#ifndef EVENT_H
#define EVENT_H

#include "globalStuff.h"
#include <QHash>

enum RPEventType {
    TEMPERATURE, BINARY
};

struct CheckInfoStruct {
    QHash<ValueID::Instance, unsigned short> checkTemperature;
};

class RPEvent {
public:

    RPEvent() { }

    bool enabled;
    QString name, activationBinary, fanProfileNameChange, powerProfileChange, powerLevelChange;
    unsigned short fixedFanSpeedChange, activationTemperature, fanComboIndex;
    ValueID::Instance sensorInstance = ValueID::T_EDGE; // instance of temp sensor
    RPEventType type;

    bool isActivationConditonFulfilled(const CheckInfoStruct &check) {
        switch (type) {
            case RPEventType::TEMPERATURE:
                return activationTemperature < check.checkTemperature[sensorInstance];
            case RPEventType::BINARY:
                return !globalStuff::grabSystemInfo("pidof \""+activationBinary+"\"")[0].isEmpty();
        }

        return false;
    }
};

#endif // EVENT_H
