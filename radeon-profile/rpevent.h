#ifndef EVENT_H
#define EVENT_H

#include "globalStuff.h"

enum RPEventType {
    TEMPERATURE, BINARY
};

struct CheckInfoStruct {
    unsigned short checkTemperature;
};

class RPEvent {
public:

    RPEvent() { }

    bool enabled;
    QString name, activationBinary, fanProfileNameChange, powerProfileChange, powerLevelChange;
    unsigned short fixedFanSpeedChange, activationTemperature, fanComboIndex;
    RPEventType type;

    bool isActivationConditonFulfilled(const CheckInfoStruct &check) {
        switch (type) {
            case RPEventType::TEMPERATURE:
                return activationTemperature < check.checkTemperature;
            case RPEventType::BINARY:
                return !globalStuff::grabSystemInfo("pidof \""+activationBinary+"\"")[0].isEmpty();
        }

        return false;
    }
};

#endif // EVENT_H
