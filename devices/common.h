#ifndef DEVICES_COMMON_H
#define DEVICES_COMMON_H

#include "device.h"

namespace Devices
{
    class RelayController : public DeviceObject
    {

    public:

        RelayController(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
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

        SwitchController(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
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
