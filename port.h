#ifndef PORT_H
#define PORT_H

#define RESET_TIMEOUT           5000
#define RFC_REQUEST_TIMEOUT     1000

#include <QHostAddress>
#include <QSerialPort>
#include <QThread>
#include "device.h"

enum class RFCMode
{
    Disabled,
    Normal,
    Simlar
};

class PortThread;
typedef QSharedPointer <PortThread> Port;

class PortThread : public QThread
{
    Q_OBJECT

public:

    PortThread(quint8 portId, const QString &portName, bool tcp, bool rfc, bool debug, DeviceList *devices);
    ~PortThread(void);

    inline quint8 portId(void) { return m_portId; }

private:

    QTimer *m_receiveTimer, *m_resetTimer, *m_pollTimer;

    QSerialPort *m_serial;
    QTcpSocket *m_socket;
    QIODevice *m_device;

    quint8 m_portId;
    QString m_portName;
    bool m_tcp, m_rfc, m_debug, m_serialError;

    QHostAddress m_adddress;
    quint16 m_port;
    bool m_connected;

    RFCMode m_rfcMode;
    qint32 m_baudRate;

    QByteArray m_replyData;
    quint32 m_replyTimeout;

    DeviceList *m_devices;

    void init(void);
    void rfcRequest(qint32 baudRate = 0);
    void sendRequest(const Device &device, const QByteArray &request);

private slots:

    void threadStarted(void);
    void threadFinished(void);

    void serialError(QSerialPort::SerialPortError error);
    void socketError(QTcpSocket::SocketError error);
    void socketConnected(void);

    void startTimer(void);
    void readyRead(void);

    void reset(void);
    void poll(void);

signals:

    void replyReceived(void);
    void updateAvailability(DeviceObject *device);

};

inline QDebug operator << (QDebug debug, PortThread *port) { return debug << "port" << port->portId(); }

#endif
