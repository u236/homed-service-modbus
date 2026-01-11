#ifndef DEVICES_OTHER_H
#define DEVICES_OTHER_H

#include "device.h"

namespace Other
{
    class JTH2d1 : public DeviceObject
    {

    public:

        JTH2d1(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class T13 : public DeviceObject
    {

    public:

        T13(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        QList <QString> m_modes;

    };

    class M0701s : public DeviceObject
    {

    public:

        M0701s(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        bool m_status;

    };
}

#endif
