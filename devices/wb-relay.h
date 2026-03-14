#ifndef DEVICES_WB_RELAY_H
#define DEVICES_WB_RELAY_H

#include "device.h"

namespace WirenBoard
{
    class WBMr : public DeviceObject
    {

    public:

        enum class Model {wbMrwm2, wbMrm2, wbMr3, wbMr6, wbMr6p};

        WBMr(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        quint8 m_channels, m_inputs;
        quint16 m_output[6], m_singleClick[8], m_doubleClick[8];

    };

    class WBMrwm2 : public WBMr
    {

    public:

        WBMrwm2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMr(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMrwm2) {}

    };

    class WBMrm2 : public WBMr
    {

    public:

        WBMrm2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMr(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMrm2) {}

    };

    class WBMr3 : public WBMr
    {

    public:

        WBMr3(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMr(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMr3) {}

    };

    class WBMr6 : public WBMr
    {

    public:

        WBMr6(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMr(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMr6) {}

    };

    class WBMr6p : public WBMr
    {

    public:

        WBMr6p(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMr(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMr6p) {}

    };
}

#endif
