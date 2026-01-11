#ifndef DEVICES_PEACEFAIR_H
#define DEVICES_PEACEFAIR_H

#include "device.h"

namespace Peacefair
{
    class PZEM0x4 : public DeviceObject
    {

    public:

        PZEM0x4(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class PZEM6l24 : public DeviceObject
    {

    public:

        PZEM6l24(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };
}

#endif

