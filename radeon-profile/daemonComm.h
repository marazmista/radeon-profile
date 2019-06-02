
// copyright marazmista @ 12.05.2014

// class for communication with daemon

#ifndef DAEMONCOMM_H
#define DAEMONCOMM_H

#include <QLocalSocket>
#include <QDataStream>
#include <QTimer>

#define SEPARATOR '#'
#define DAEMON_SIGNAL_CONFIG '0'
#define DAEMON_SIGNAL_READ_CLOCKS '1'
#define DAEMON_SIGNAL_SET_VALUE '2'
#define DAEMON_SIGNAL_TIMER_ON '4'
#define DAEMON_SIGNAL_TIMER_OFF '5'
#define DAEMON_SHAREDMEM_KEY '6'
#define DAEMON_ALIVE '7'

class DaemonComm : public QObject
{
    Q_OBJECT

public:

    enum ConfirmationMehtod {
        DISABLED,
        ON_REQUEST,
        PERIODICALLY
    };

    DaemonComm();
    ~DaemonComm();
    void connectToDaemon();
    void disconnectDaemon();
    void sendCommand(const QString command);
    void setConnectionConfirmationMethod(const ConfirmationMehtod method);

    inline bool isConnected() {
        return (signalSender->state() == QLocalSocket::ConnectedState);
    }

    inline const QLocalSocket* getSocketPtr() {
        return signalSender;
    }


public slots:
    void receiveFromDaemon();
    void sendConnectionConfirmation();

private:
    QDataStream feedback;
    QLocalSocket *signalSender;
    QTimer *confirmationTimer;
};

#endif // DAEMONCOMM_H
