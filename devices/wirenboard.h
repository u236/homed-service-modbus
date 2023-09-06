#ifndef DEVICES_WIRENBOARD_H
#define DEVICES_WIRENBOARD_H

#define WBMAP_FREQUENCY_REGISTER_ADDRESS    0x10F8
#define WBMAP_FREQUENCY_MULTIPILER          10.0

#define WBMAP3_VOLTAGE_REGISTER_ADDRESS     0x1410
#define WBMAP3_VOLTAGE_REGISTER_COUNT       6
#define WBMAP3_VOLTAGE_MULTIPILER           1.52588e-4

#define WBMAP6_VOLTAGE_REGISTER_ADDRESS     0x10D9
#define WBMAP6_VOLTAGE_MULTIPILER           10.0

#define WBMAP_CURRENT_REGISTER_ADDRESS      0x1416
#define WBMAP_CURRENT_REGISTER_COUNT        6
#define WBMAP_CURRENT_MULTIPILER            2.44141e-4


#define WBMAP_POWER_REGISTER_ADDRESS        0x1300
#define WBMAP_POWER_REGISTER_COUNT          8
#define WBMAP3_POWER_MULTIPILER             5.12
#define WBMAP6_POWER_MULTIPILER             0.244141

#define WBMAP_ENERGY_REGISTER_ADDRESS       0x1200
#define WBMAP_ENERGY_REGISTER_COUNT         16
#define WBMAP_ENERGY_MULTIPILER             0.01

#define WBMAP_COIL_REGISTER_ADDRESS         0x1460
#define WBMAP_COIL_REGISTER_COUNT           6

#include "device.h"

namespace WirenBoard
{
    class WBMap3e : public DeviceObject
    {

    public:

        WBMap3e(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, name) {}

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };
}

#endif
