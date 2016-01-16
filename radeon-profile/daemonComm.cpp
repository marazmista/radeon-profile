
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"

daemonComm::daemonComm() {
    signalSender = new QLocalSocket();
    connect(signalSender,SIGNAL(connected()),this,SLOT(onConnect()));
}

daemonComm::~daemonComm() {
    delete signalSender;
}

void daemonComm::connectToDaemon() {
    signalSender->abort();
    signalSender->connectToServer("radeon-profile-daemon-server");
}

void daemonComm::sendCommand(const QString command) {
    signalSender->write(command.toLatin1(),command.length());
}

void daemonComm::onConnect() {

}
