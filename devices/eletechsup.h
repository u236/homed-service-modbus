#ifndef DEVICES_ELETECHSUP_H
#define DEVICES_ELETECHSUP_H

#include "device.h"

namespace Eletechsup
{
    class N4Dsa02 : public DeviceObject
    {

    public:

        N4Dsa02(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };


    class R4Pin08 : public DeviceObject
    {

    public:

        enum class Model {r4pin08m0, r4pin08m1, r4pin08m2, r4pin08m3, r4pin08m4};

        R4Pin08(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        quint8 m_inputs, m_outputs;
        quint16 m_input[8], m_output[8];

    };

    class R4Pin08M0 : public R4Pin08
    {

    public:

        R4Pin08M0(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            R4Pin08(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m0) {}

    };

    class R4Pin08M1 : public R4Pin08
    {

    public:

        R4Pin08M1(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            R4Pin08(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m1) {}

    };

    class R4Pin08M2 : public R4Pin08
    {

    public:

        R4Pin08M2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            R4Pin08(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m2) {}

    };

    class R4Pin08M3 : public R4Pin08
    {

    public:

        R4Pin08M3(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            R4Pin08(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m3) {}

    };

    class R4Pin08M4 : public R4Pin08
    {

    public:

        R4Pin08M4(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            R4Pin08(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m4) {}

    };
}

#endif
