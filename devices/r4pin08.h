#ifndef DEVICES_R4PIN08_H
#define DEVICES_R4PIN08_H

#include "device.h"

namespace R4PIN08
{
    class Controller : public DeviceObject
    {

    public:

        enum class Model
        {
            r4pin08m0,
            r4pin08m1,
            r4pin08m2,
            r4pin08m3,
            r4pin08m4
        };

        Controller(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        quint8 m_inputs, m_outputs;
        quint16 m_input[8], m_output[8];

    };

    class M0 : public Controller
    {

    public:

        M0(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m0) {}

    };

    class M1 : public Controller
    {

    public:

        M1(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m1) {}

    };

    class M2 : public Controller
    {

    public:

        M2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m2) {}

    };

    class M3 : public Controller
    {

    public:

        M3(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m3) {}

    };

    class M4 : public Controller
    {

    public:

        M4(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::r4pin08m4) {}

    };
}

#endif
