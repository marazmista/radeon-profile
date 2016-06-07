
// copyright marazmista @ 12.05.2014

// class for communication with daemon

#ifndef DAEMONCOMM_H
#define DAEMONCOMM_H

#include <QLocalSocket>
#include <QSharedMemory>
#include <QString>

#define SEPARATOR '#'
#define DAEMON_SIGNAL_CONFIG '0'
#define DAEMON_SIGNAL_READ_CLOCKS '1'
#define DAEMON_SIGNAL_SET_VALUE '2'
#define DAEMON_SIGNAL_TIMER_ON '4'
#define DAEMON_SIGNAL_TIMER_OFF '5'

#define SHARED_MEM_SIZE 128 // When changin this, consider changing SHARED_MEM_SIZE in radeon-profile-daemon/rpdthread.h

/**
 * @brief The daemonComm class manages the communication with the daemon                <----------------------------------
 *                                                                                     /                                   \
 *                                                        /--> sendTimerOn() --> [wait interval] --> readMemory() --> [use data]
 * connectToDaemon() --> attachToMemory() --> sendConfig()
 *                                                        \--> sendTimerOff() --> sendReadClocks() --> readMemory() --> [use data]
 *                                                                                     \                                   /
 *                                                                                      <----------------------------------
 */
class daemonComm : private QLocalSocket
{
    Q_OBJECT
public:
    QSharedMemory sharedMem;

    // Communication with the daemon
    /**
     * @brief connectToDaemon sets up the connection with the daemon.
     * If a connection is already active, it is aborted and re-created.
     */

    void connectToDaemon();
    /**
     * @brief sendCommand
     * @param command the command to send
     * Use the functions below if possible
     * @see sendConfig()
     * @see sendReadClocks()
     * @see sendSetValue()
     * @see sendTimerOn()
     * @see sendTimerOff()
     */
    void sendCommand(const QString & command);

    /**
     * @brief connected
     * @return if the connection is active
     */
    bool connected() const;

    /**
     * @brief sendConfig Configures the daemon
     * @param clocksPath Path to read when requested
     */
    void sendConfig (const QString & clocksPath); // SIGNAL_CONFIG + SEPARATOR + CLOCKS_PATH + SEPARATOR

    /**
     * @brief sendReadClocks Ask the daemon to read clocks
     */

    void sendReadClocks (); // SIGNAL_READ_CLOCKS + SEPARATOR
    /**
     * @brief sendSetValue Write the value into the file indicated by the path
     * @param value
     * @param path
     */

    void sendSetValue (const QString & value, const QString & path); // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
    /**
     * @brief sendTimerOn Setup the daemon to automatically read the data
     * @param interval Seconds between a read and another
     */

    void sendTimerOn (const ushort seconds); // SIGNAL_TIMER_ON + SEPARATOR + INTERVAL + SEPARATOR
    /**
     * @brief sendTimerOff Disable the timer
     */
    void sendTimerOff (); // SIGNAL_TIMER_OFF + SEPARATOR

    // Shared Memory managment
    /**
     * @brief attachToMemory Connects to the shared memory
     * @return Success
     */
    bool attachToMemory();

    /**
     * @brief readMemory
     * @return A copy of the content of the shared memory
     */
    QByteArray readMemory();
};

#endif // DAEMONCOMM_H
