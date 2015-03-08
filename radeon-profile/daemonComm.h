
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
    void connectToDaemon();
    void sendCommand(const QString command);

    inline bool connected() {
        if (signalSender->state() == QLocalSocket::ConnectedState)
            return true;
        else return false;
    }

    QLocalSocket *signalSender;

    struct {
            QString config,read_clocks,setValue,timer_on,timer_off;
        } daemonSignal;

public slots:
    void onConnect();

};

#endif // DAEMONCOMM_H
