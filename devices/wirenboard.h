#ifndef DEVICES_WIRENBOARD_H
#define DEVICES_WIRENBOARD_H

#define WBMAP_COIL_REGISTER_ADDRESS         0x1460
#define WBMAP_COIL_REGISTER_COUNT           6

#define WBMAP_FREQUENCY_REGISTER_ADDRESS    0x10F8
#define WBMAP_FREQUENCY_MULTIPILER          10.0

#define WBMAP_VOLTAGE_REGISTER_ADDRESS      0x1410
#define WBMAP_VOLTAGE_REGISTER_COUNT        6
#define WBMAP_VOLTAGE_MULTIPILER            1.52588e-4

#define WBMAP_CURRENT_REGISTER_ADDRESS      0x1416
#define WBMAP_CURRENT_REGISTER_COUNT        6
#define WBMAP_CURRENT_MULTIPILER            2.44141e-4

#define WBMAP_POWER_REGISTER_ADDRESS        0x1300
#define WBMAP_POWER_REGISTER_COUNT          8
#define WBMAP3E_POWER_MULTIPILER            5.12
#define WBMAP12H_POWER_MULTIPILER_C         0.244141
#define WBMAP12H_POWER_MULTIPILER_T         0.976562

#define WBMAP_ENERGY_REGISTER_ADDRESS       0x1200
#define WBMAP_ENERGY_REGISTER_COUNT         16
#define WBMAP_ENERGY_MULTIPILER             0.01

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

    class WBMap12h : public DeviceObject
    {

    public:

        WBMap12h(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        double m_totalPower, m_totalEnergy;

    };
}

#endif
