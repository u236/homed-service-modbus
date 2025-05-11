#ifndef DEVICES_NEPTUN_H
#define DEVICES_NEPTUN_H

#include "device.h"

namespace Neptun
{
    class SmartPlus : public DeviceObject
    {

    public:

        SmartPlus(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_status(0) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_status;

    };
}

#endif

