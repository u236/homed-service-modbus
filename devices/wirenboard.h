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
    class WBMap3e : public DeviceObject
    {

    public:

        WBMap3e(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device) override;
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

        void init(const Device &device) override;
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

        enum class Model
        {
            wbMap12e,
            wbMap12h
        };

        WBMap12(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name, Model model) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_model(model) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        Model m_model;
        double m_totalPower, m_totalEnergy;

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

    class WBMr6 : public DeviceObject
    {

    public:

        WBMr6(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_output[6];

    };
}

#endif
