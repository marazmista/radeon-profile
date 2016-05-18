
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"
#include <QDebug>


void daemonComm::connectToDaemon() {
    qDebug() << "Connecting to daemon";
    abort();
    connectToServer("radeon-profile-daemon-server");
}

bool daemonComm::connected() {
    return (state() == QLocalSocket::ConnectedState);
}

void daemonComm::sendCommand(const QString command) {
    if(write(command.toLatin1(),command.length()) == -1) // If sending signal fails
        qWarning() << "Failed sending signal: " << command;
}

void daemonComm::sendConfig (const QString clocksPath){
    qDebug() << "Sending daemon CONFIG signal with clocks path " << clocksPath;
    sendCommand(QString() + DAEMON_SIGNAL_CONFIG + SEPARATOR + clocksPath + SEPARATOR);
}

void daemonComm::sendReadClocks (){
    qDebug() << "Sending daemon READ_CLOCKS signal";
    sendCommand(QString() + DAEMON_SIGNAL_READ_CLOCKS + SEPARATOR);
}

void daemonComm::sendSetValue (const QString value, const QString path){
    qDebug() << "Sending daemon SET_VALUE signal with value " << value << " for path " << path;
    sendCommand(QString() + DAEMON_SIGNAL_SET_VALUE + SEPARATOR + value + SEPARATOR + path + SEPARATOR);
}

void daemonComm::sendTimerOn (const unsigned int interval){
    qDebug() << "Sending daemon TIMER_ON signal with interval " << interval;
    sendCommand(QString() + DAEMON_SIGNAL_TIMER_ON + SEPARATOR + QString::number(interval) + SEPARATOR);
}

void daemonComm::sendTimerOff (){
    qDebug() << "Sending daemon TIMER_OFF signal";
    sendCommand(QString() + DAEMON_SIGNAL_TIMER_OFF + SEPARATOR);
}
