#ifndef EVENT_H
#define EVENT_H

#include "globalStuff.h"

enum rpeventType {
    TEMPEREATURE, BIANRY
};

struct checkInfoStruct {
    unsigned short checkTemperature;
};

class RPEvent {
public:

    RPEvent() { }

    bool enabled;
    QString name, activationBinary, fanProfileNameChange;
    globalStuff::powerProfiles dpmProfileChange;
    globalStuff::forcePowerLevels powerLevelChange;
    unsigned short fixedFanSpeedChange, activationTemperature, fanComboIndex;
    rpeventType type;

    bool isActivationConditonFullfilled(const checkInfoStruct &check) {
        switch (type) {
            case rpeventType::TEMPEREATURE:
                return activationTemperature < check.checkTemperature;
                break;
            case rpeventType::BIANRY:
                return !globalStuff::grabSystemInfo("pidof \""+activationBinary+"\"")[0].isEmpty();
                break;
        }

        return false;
    }

};

//template <typename T>
//class Event : public IEvent {

//public:
//    T value;

//    Event() { }
//    Event(const T &v) {
//        value = v;
//    }

//    virtual bool isActivationConditonFullfilled(const T &checkValue) {
//       return false;

//    }

//    void setValue(const T &v) {
//        this->value = v;
//    }
//};
#endif // EVENT_H
