#ifndef DEVICES_WB_DIMMER_H
#define DEVICES_WB_DIMMER_H

#include "device.h"

namespace WirenBoard
{
    class WBMdm : public DeviceObject
    {

    public:

        WBMdm(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_output[3], m_singleClick[6], m_doubleClick[6];
        QList <QString> m_modes;

    };

    class WBLed : public DeviceObject
    {

    public:

        enum class Model {wbLed0, wbLed1, wbLed2, wbLed16, wbLed17, wbLed18, wbLed32, wbLed33, wbLed34, wbLed256, wbLed512};

        WBLed(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        quint16 m_mode, m_output[10], m_singleClick[4], m_doubleClick[4];
        QList <quint8> m_list;

    };

    class WBLed0 : public WBLed
    {

    public:

        WBLed0(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed0) {}

    };

    class WBLed1 : public WBLed
    {

    public:

        WBLed1(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed1) {}

    };

    class WBLed2 : public WBLed
    {

    public:

        WBLed2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed2) {}

    };

    class WBLed16 : public WBLed
    {

    public:

        WBLed16(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed16) {}

    };

    class WBLed17 : public WBLed
    {

    public:

        WBLed17(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed17) {}

    };

    class WBLed18 : public WBLed
    {

    public:

        WBLed18(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed18) {}

    };

    class WBLed32 : public WBLed
    {

    public:

        WBLed32(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed32) {}

    };

    class WBLed33 : public WBLed
    {

    public:

        WBLed33(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed33) {}

    };

    class WBLed34 : public WBLed
    {

    public:

        WBLed34(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed34) {}

    };

    class WBLed256 : public WBLed
    {

    public:

        WBLed256(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed256) {}

    };

    class WBLed512 : public WBLed
    {

    public:

        WBLed512(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBLed(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbLed512) {}

    };
}

#endif
