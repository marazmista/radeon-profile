
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"

daemonComm::daemonComm() {
    signalSender = new QLocalSocket();
    connect(signalSender,SIGNAL(connected()),this,SLOT(onConnect()));

    // init of signals
    daemonComm::daemonSignal.config = '0';
    daemonComm::daemonSignal.read_clocks = '1';
    daemonComm::daemonSignal.setValue = '2';
    daemonComm::daemonSignal.timer_on = '4';
    daemonComm::daemonSignal.timer_off = '5';
}

daemonComm::~daemonComm() {
    delete signalSender;
}

void daemonComm::connectToDaemon() {
    signalSender->abort();
    signalSender->connectToServer("radeon-profile-daemon-server");
}

void daemonComm::sendCommand(const QString command) {
    signalSender->write(command.toAscii(),command.length());
}

void daemonComm::onConnect() {

}
