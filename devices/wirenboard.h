#ifndef DEVICES_WIRENBOARD_H
#define DEVICES_WIRENBOARD_H

#define WBMAP_COIL_REGISTER_ADDRESS         0x1460
#define WBMAP_COIL_REGISTER_COUNT           6

#define WBMAP_FREQUENCY_REGISTER_ADDRESS    0x10F8
#define WBMAP_FREQUENCY_MULTIPLIER          10.0

#define WBMAP_VOLTAGE_REGISTER_ADDRESS      0x1410
#define WBMAP_VOLTAGE_REGISTER_COUNT        6
#define WBMAP_VOLTAGE_MULTIPLIER            1.52588e-4

#define WBMAP6S_VOLTAGE_REGISTER_ADDRESS    0x10D9
#define WBMAP6S_VOLTAGE_MULTIPLIER          10.0

#define WBMAP_CURRENT_REGISTER_ADDRESS      0x1416
#define WBMAP_CURRENT_REGISTER_COUNT        6
#define WBMAP_CURRENT_MULTIPLIER            2.44141e-4

#define WBMAP_POWER_REGISTER_ADDRESS        0x1300
#define WBMAP_POWER_REGISTER_COUNT          8
#define WBMAP_POWER_MULTIPLIER              5.12

#define WBMAP6S_POWER_REGISTER_ADDRESS      0x1302
#define WBMAP6S_POWER_REGISTER_COUNT        6
#define WBMAP6S_POWER_MULTIPLIER            0.244141

#define WBMAP12H_CHANNEL_POWER_MULTIPLIER   0.244141
#define WBMAP12H_TOTAL_POWER_MULTIPLIER     0.976562

#define WBMAP_ENERGY_REGISTER_ADDRESS       0x1200
#define WBMAP_ENERGY_REGISTER_COUNT         16
#define WBMAP_ENERGY_MULTIPLIER             0.01

#define WBMAP6S_ENERGY_REGISTER_ADDRESS     0x1204
#define WBMAP6S_ENERGY_REGISTER_COUNT       12

#define WBMAP_ANGLE_REGISTER_ADDRESS        0x10FD
#define WBMAP_ANGLE_REGISTER_COUNT          3
#define WBMAP_ANGLE_MULTIPLIER              100.0

#include "device.h"

namespace WirenBoard
{
    class WBMs : public DeviceObject
    {

    public:

        WBMs(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class WBMsw : public DeviceObject
    {

    public:

        WBMsw(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        bool m_status[3];

    };

    class WBMap3ev : public DeviceObject
    {

    public:

        WBMap3ev(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class WBMap3e : public DeviceObject
    {

    public:

        WBMap3e(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class WBMap6s : public DeviceObject
    {

    public:

        WBMap6s(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        double m_totalPower, m_totalEnergy;

    };

    class WBMap12 : public DeviceObject
    {

    public:

        enum class Model {wbMap12e, wbMap12h};

        WBMap12(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;

    };

    class WBMap12e : public WBMap12
    {

    public:

        WBMap12e(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMap12(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMap12e) {}

    };

    class WBMap12h : public WBMap12
    {

    public:

        WBMap12h(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            WBMap12(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name, Model::wbMap12h) {}

    };

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
        quint16 m_output[6];

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
        quint16 m_mode, m_output[10];
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

        QList <QString> m_modes;
        quint16 m_output[3];

    };

    class WBUps : public DeviceObject
    {

    public:

        WBUps(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };
}

#endif
