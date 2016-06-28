
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"
#include <QDebug>


void daemonComm::connectToDaemon() {
    qDebug() << "Connecting to daemon";
    abort();
    connectToServer("radeon-profile-daemon-server");
}

void daemonComm::sendCommand(const QString & command) {
    if(write(command.toLatin1(),command.length()) == -1) // If sending signal fails
        qWarning() << "Failed sending signal: " << command;
}

void daemonComm::sendConfig (const QString & clocksPath){
    qDebug() << "Sending daemon CONFIG signal with clocks path " << clocksPath;
    sendCommand(QString() + DAEMON_SIGNAL_CONFIG + SEPARATOR + clocksPath + SEPARATOR);
}

void daemonComm::sendReadClocks (){
    qDebug() << "Sending daemon READ_CLOCKS signal";
    sendCommand(QString() + DAEMON_SIGNAL_READ_CLOCKS + SEPARATOR);
}

void daemonComm::sendSetValue (const QString & value, const QString & path){
    qDebug() << "Sending daemon SET_VALUE signal with value " << value << " for path " << path;
    sendCommand(QString() + DAEMON_SIGNAL_SET_VALUE + SEPARATOR + value + SEPARATOR + path + SEPARATOR);
}

void daemonComm::sendTimerOn (const ushort interval){
    qDebug() << "Sending daemon TIMER_ON signal with interval " << interval;
    sendCommand(QString() + DAEMON_SIGNAL_TIMER_ON + SEPARATOR + QString::number(interval) + SEPARATOR);
}

void daemonComm::sendTimerOff (){
    qDebug() << "Sending daemon TIMER_OFF signal";
    sendCommand(QString() + DAEMON_SIGNAL_TIMER_OFF + SEPARATOR);
}

bool daemonComm::attachToMemory(){
    if (sharedMem.isAttached()){ // Already attached, skip
        qDebug() << "Shared memory is already attached";
        return true;
    }

    sharedMem.setKey("radeon-profile");
    if (sharedMem.create(SHARED_MEM_SIZE)){
        // If QSharedMemory::create() returns true, it has already automatically attached
        qDebug() << "Shared memory created and connected";
        return true;
    }

    if((sharedMem.error() == QSharedMemory::AlreadyExists) && sharedMem.attach()){
        qDebug() << "Shared memory already existed, attached to it";
        return true;
    }

    qCritical() << "Unable to attach to the shared memory: " << sharedMem.errorString();
    return false;
}

QByteArray daemonComm::readMemory() {
    if (!sharedMem.lock()){
        qWarning() << "Unable to lock the shared memory: " << sharedMem.errorString();
        return QByteArray();
    }

    const char * to = static_cast<const char*>(sharedMem.constData());
    QByteArray out;

    if (to == NULL)
        qWarning() << "Shared memory data pointer is invalid: " << sharedMem.errorString();
    else
        out = QByteArray::fromRawData(to, SHARED_MEM_SIZE).trimmed();

    if(!sharedMem.unlock())
        qWarning() << "Failed unlocking" << sharedMem.errorString();

    return out;
}
