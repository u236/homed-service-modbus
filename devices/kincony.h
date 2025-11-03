#ifndef DEVICES_KINCONY_H
#define DEVICES_KINCONY_H

#include "device.h"

namespace Kincony
{
    class KC868 : public DeviceObject
    {

    public:

        enum class Model {kc868a4, kc868a6, kc868a8, kc868a16, kc868a32, kc868a64, kc868a128};

        KC868(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        quint8 m_channels;
        quint16 m_output[128];
        bool m_adc, m_dac;

    };

    class KC868A4 : public KC868
    {

    public:

        KC868A4(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a4) {}

    };

    class KC868A6 : public KC868
    {

    public:

        KC868A6(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a6) {}

    };

    class KC868A8 : public KC868
    {

    public:

        KC868A8(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a8) {}

    };

    class KC868A16 : public KC868
    {

    public:

        KC868A16(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a16) {}

    };

    class KC868A32 : public KC868
    {

    public:

        KC868A32(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a32) {}

    };

    class KC868A64 : public KC868
    {

    public:

        KC868A64(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a64) {}

    };

    class KC868A128 : public KC868
    {

    public:

        KC868A128(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            KC868(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::kc868a128) {}

    };
}

#endif
