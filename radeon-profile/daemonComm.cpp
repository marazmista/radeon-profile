
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"
#include <QDebug>

daemonComm::daemonComm() {
    signalSender = new QLocalSocket(this);
    connect(signalSender,SIGNAL(connected()),this,SLOT(onConnect()));
}

daemonComm::~daemonComm() {
    delete signalSender;
}

void daemonComm::connectToDaemon() {
    qDebug() << "Connecting to daemon";
    signalSender->abort();
    signalSender->connectToServer("radeon-profile-daemon-server");
}

void daemonComm::sendCommand(const QString command) {
    if(signalSender->write(command.toLatin1(),command.length()) == -1) // If sending signal fails
        qWarning() << "Failed sending signal: " << command;
}

void daemonComm::onConnect() {

}
