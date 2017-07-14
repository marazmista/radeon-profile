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
    QString name, activationBinary, fanProfileNameChange;
    PowerProfiles dpmProfileChange;
    ForcePowerLevels powerLevelChange;
    unsigned short fixedFanSpeedChange, activationTemperature, fanComboIndex;
    RPEventType type;

    bool isActivationConditonFulfilled(const CheckInfoStruct &check) {
        switch (type) {
            case RPEventType::TEMPERATURE:
                return activationTemperature < check.checkTemperature;
                break;
            case RPEventType::BINARY:
                return !globalStuff::grabSystemInfo("pidof \""+activationBinary+"\"")[0].isEmpty();
                break;
        }

        return false;
    }

    template <typename T>
    T getEnumFromCombo(const unsigned int comboIndex) {
        return static_cast<T>(comboIndex - 1);
    }
};

#endif // EVENT_H
