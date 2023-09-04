#ifndef DEVICE_H
#define DEVICE_H

#include <QDateTime>
#include <QQueue>
#include <QSharedPointer>
#include <QTimer>
#include "endpoint.h"

class EndpointObject : public AbstractEndpointObject
{

public:

    EndpointObject(quint8 id, const Device &device) : AbstractEndpointObject(id, device) {}

    inline QMap <QString, QVariant> &status(void) { return m_status; }

private:

    QMap <QString, QVariant> m_status;

};

class DeviceObject : public AbstractDeviceObject
{
    Q_OBJECT

public:

    DeviceObject(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
        AbstractDeviceObject(name), m_address(QString("%1.%2").arg(portId).arg(slaveId)), m_port(portId), m_portId(slaveId), m_baudRate(baudRate), m_pollInterval(pollInterval), m_pollTime(0), m_sequence(0), m_fullPoll(true), m_firstPoll(true) {}

    virtual void init(const Device &) {}
    virtual void enqueueAction(quint8, const QString &, const QVariant &) {}

    virtual QByteArray pollRequest(void) = 0;
    virtual void parseReply(const QByteArray &) = 0;
    virtual void startPoll(void) = 0;

    inline QString address(void) { return m_address; }

    inline quint8 portId(void) { return m_port; }
    inline quint8 slaveId(void) { return m_portId; }
    inline quint32 baudRate(void) { return m_baudRate; }
    inline quint32 pollInterval(void) { return m_pollInterval; }
    inline qint64 pollTime(void) { return m_pollTime; }

    inline QQueue <QByteArray> &actionQueue(void) { return m_actionQueue; }

protected:

    QString m_address;

    quint8 m_port, m_portId;
    quint32 m_baudRate, m_pollInterval;

    qint64 m_pollTime;
    quint8 m_sequence;
    bool m_fullPoll, m_firstPoll;

    QQueue <QByteArray> m_actionQueue;

signals:

    void endpointUpdated(quint8 endpointId);

};

namespace Devices
{
    class RelayController : public DeviceObject
    {

    public:

        RelayController(quint8 portId,quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, name), m_status(0), m_pending(0), m_update(false) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_status, m_pending;
        bool m_update;

    };

    class SwitchController : public DeviceObject
    {
        Q_OBJECT

    public:

        SwitchController(quint8 portId,quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, name), m_timer(new QTimer(this)), m_status(0) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        QTimer *m_timer;
        quint16 m_status;

        qint64 m_time[16];
        bool m_hold[16];

    private slots:

        void update(void);

    };
}

#endif
