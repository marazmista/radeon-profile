
// copyright marazmista @ 12.05.2014

#include "daemonComm.h"
#include <QDebug>

daemonComm::daemonComm() {
    signalSender = new QLocalSocket(this);
    feedback.setDevice(signalSender);
    connect(signalSender,SIGNAL(readyRead()), this, SLOT(receiveFeedback()));
}

daemonComm::~daemonComm() {
    delete signalSender;
}

void daemonComm::connectToDaemon() {
    qDebug() << "Connecting to daemon";
    signalSender->abort();
    signalSender->connectToServer("/tmp/radeon-profile-daemon-server");
}

void daemonComm::sendCommand(const QString command) {
    if(signalSender->write(command.toLatin1(),command.length()) == -1) // If sending signal fails
        qWarning() << "Failed sending signal: " << command;
}

void daemonComm::receiveFeedback() {
    feedback.startTransaction();

    QString confirmMsg ;
    feedback >> confirmMsg;

    if (!feedback.commitTransaction())
        return;

    qDebug() << "ekstra " << confirmMsg.toLatin1();
}
