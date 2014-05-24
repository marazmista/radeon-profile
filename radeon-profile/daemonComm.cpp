
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"

daemonComm::daemonComm() {
    signalSender = new QLocalSocket();

    connect(signalSender,SIGNAL(connected()),this,SLOT(sendRequest()));
}

daemonComm::~daemonComm() {
    delete signalSender;
}

void daemonComm::connectToDaemon() {
    signalSender->abort();
    signalSender->connectToServer("radeon-profile-daemon-server");
}

void daemonComm::sendRequest() {
    QString request = QString(this->req) + QString(this->cardIndex) + this->profile;
    signalSender->write(request.toAscii(),request.length());
}

void daemonComm::setSignalParams(const QChar &_req, const QChar &_cardIndex, const QString &_profile) {
    this->req = _req;
    this->cardIndex = _cardIndex;
    this->profile = _profile;
}
