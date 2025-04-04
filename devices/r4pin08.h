#ifndef DEVICES_R4PIN08_H
#define DEVICES_R4PIN08_H

#include "device.h"

namespace R4PIN08
{
    class DI8 : public DeviceObject
    {

    public:

        DI8(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_status[8];

    };
}

#endif
