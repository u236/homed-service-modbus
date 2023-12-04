#include <math.h>
#include "expose.h"
#include "modbus.h"
#include "wirenboard.h"

void WirenBoard::WBMap3e::init(const Device &device)
{
    m_type = "wbMap3e";
    m_description = "Wiren Board WB-MAP3E energy meter";

    m_options.insert("ratio", QJsonObject {{"min", 0}, {"max", 65535}, {"icon", "mdi:cog"}});
    m_options.insert("delay", QJsonObject {{"min", -32768}, {"max", 32767}, {"icon", "mdi:cog"}});

    for (quint8 i = 0; i < 4; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose ratio = Expose(new NumberObject("ratio")), delay = Expose(new NumberObject("delay")), voltage = Expose(new SensorObject("voltage")), current = Expose(new SensorObject("current")), power = Expose(new SensorObject("power")), energy = Expose(new SensorObject("energy"));

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delay->setMultiple(true);
            delay->setParent(endpoint.data());
            endpoint->exposes().append(delay);

            voltage->setMultiple(true);
            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            current->setMultiple(true);
            current->setParent(endpoint.data());
            endpoint->exposes().append(current);

            power->setMultiple(true);
            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setMultiple(true);
            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);
        }
        else
        {
            Expose frequency = Expose(new SensorObject("frequency")), power = Expose(new SensorObject("power")), energy = Expose(new SensorObject("energy"));

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);
        }

        m_endpoints.insert(i, endpoint);
    }
}

void WirenBoard::WBMap3e::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 registerAddress;

    if (!endpointId || endpointId > 3)
        return;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + endpointId - 1;
    else if (name == "delay")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + endpointId + 2;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap3e::startPoll(void)
{
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_sequence = m_fullPoll ? 0 : 1;
}

QByteArray WirenBoard::WBMap3e::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:  return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS, WBMAP_COIL_REGISTER_COUNT);
        case 1:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);
        case 2:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP3_VOLTAGE_REGISTER_ADDRESS, WBMAP3_VOLTAGE_REGISTER_COUNT);
        case 3:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS, WBMAP_CURRENT_REGISTER_COUNT);
        case 4:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS, WBMAP_POWER_REGISTER_COUNT);
        case 5:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS, WBMAP_ENERGY_REGISTER_COUNT);
        default: return QByteArray();
    }
}

void WirenBoard::WBMap3e::parseReply(const QByteArray &reply)
{
    bool check = false;

    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delay", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", round(static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPILER) / 1000.0);
            break;
        }

        case 2:
        {
            quint16 data[WBMAP3_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("voltage", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP3_VOLTAGE_MULTIPILER) / 1000.0);

            break;
        }

        case 3:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("current", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPILER) / 1000.0);

            break;
        }

        case 4:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.find(i).value()->buffer().insert("power", round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP3_POWER_MULTIPILER) / 1000.0);

            break;
        }

        case 5:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
               m_endpoints.find(i).value()->buffer().insert("energy", round(static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPILER) / 1000.0);

            check = true;
            break;
        }
    }

    if (check)
        updateEndpoints();

    m_sequence++;
}
