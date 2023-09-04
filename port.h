#ifndef PORT_H
#define PORT_H

#include <QtSerialPort>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include "device.h"

class PortThread;
typedef QSharedPointer <PortThread> Port;

class PortThread : public QThread
{
    Q_OBJECT

public:

    PortThread(quint8 portId, const QString &portName, DeviceList *devices);
    ~PortThread(void);

private:

    QSerialPort *m_serial;
    QTimer *m_receiveTimer, *m_pollTimer;

    quint8 m_portId;
    QString m_portName;

    QByteArray m_replyData;
    DeviceList *m_devices;

    void sendRequest(const Device &device, const QByteArray &request);

private slots:

    void threadStarted(void);
    void threadFinished(void);

    void startTimer(void);
    void readyRead(void);
    void poll(void);

signals:

    void replyReceived(void);
    void updateAvailability(DeviceObject *device);

};

#endif
