
// copyright marazmista @ 12.05.2014

// class for communication with daemon

#ifndef DAEMONCOMM_H
#define DAEMONCOMM_H

#include <QLocalSocket>


class daemonComm : public QObject
{
    Q_OBJECT
public:
    daemonComm();
    ~daemonComm();
    void setSignalParams(const QChar &_req, const QChar &_cardIndex, const QString &_profile = "");
    void connectToDaemon();

    QLocalSocket *signalSender;

    QChar cardIndex, req;
    QString profile;

public slots:
    void sendRequest();

};

#endif // DAEMONCOMM_H
