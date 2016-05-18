
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

class daemonComm : public QLocalSocket
{
    Q_OBJECT
public:
    void connectToDaemon();
    void sendCommand(const QString command);

    bool connected();

    void sendConfig (QString clocksPath); // SIGNAL_CONFIG + SEPARATOR + CLOCKS_PATH + SEPARATOR
    void sendReadClocks (); // SIGNAL_READ_CLOCKS + SEPARATOR
    void sendSetValue (QString value, QString path); // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
    void sendTimerOn (unsigned int interval); // SIGNAL_TIMER_ON + SEPARATOR + INTERVAL + SEPARATOR
    void sendTimerOff (); // SIGNAL_TIMER_OFF + SEPARATOR
};

#endif // DAEMONCOMM_H
