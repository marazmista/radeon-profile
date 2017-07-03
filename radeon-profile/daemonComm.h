
// copyright marazmista @ 12.05.2014

// class for communication with daemon

#ifndef DAEMONCOMM_H
#define DAEMONCOMM_H

#include <QLocalSocket>

#define SEPARATOR '#'
#define DAEMON_SIGNAL_CONFIG '0'
#define DAEMON_SIGNAL_READ_CLOCKS '1'
#define DAEMON_SIGNAL_SET_VALUE '2'
#define DAEMON_SIGNAL_TIMER_ON '4'
#define DAEMON_SIGNAL_TIMER_OFF '5'
#define DAEMON_SHAREDMEM_KEY '6'

class daemonComm : public QObject
{
    Q_OBJECT
public:
    daemonComm();
    ~daemonComm();
    void connectToDaemon();
    void sendCommand(const QString command);

    inline bool connected() {
        return (signalSender->state() == QLocalSocket::ConnectedState);
    }

    QLocalSocket *signalSender;


public slots:
    void onConnect();

};

#endif // DAEMONCOMM_H
