#include <math.h>
#include "expose.h"
#include "modbus.h"
#include "wirenboard.h"

void WirenBoard::WBMap3e::init(const Device &device)
{
    m_type = "wbMap3e";
    m_description = "Wiren Board WB-MAP3E energy meter";

    m_options.insert("delta",     QJsonObject {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",     QJsonObject {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
    m_options.insert("frequency", QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "measurement"}, {"unit", "Hz"}, {"round", 1}});
    m_options.insert("voltage",   QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("current",   QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("power",     QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("energy",    QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});

    for (quint8 i = 0; i < 4; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose ratio = Expose(new NumberObject("ratio")), delta = Expose(new NumberObject("delta")), voltage = Expose(new SensorObject("voltage")), current = Expose(new SensorObject("current")), power = Expose(new SensorObject("power")), energy = Expose(new SensorObject("energy"));

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);

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
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + endpointId + 2;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap3e::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBMap3e::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS, WBMAP_COIL_REGISTER_COUNT);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 2:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);

        case 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS, WBMAP_CURRENT_REGISTER_COUNT);

        case 4:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS, WBMAP_POWER_REGISTER_COUNT);

        case 5:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS, WBMAP_ENERGY_REGISTER_COUNT);

        default:
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
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
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
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
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("voltage", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPILER) / 1000.0);

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
                m_endpoints.find(i).value()->buffer().insert("power", round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP3E_POWER_MULTIPILER) / 1000.0);

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

void WirenBoard::WBMap12h::init(const Device &device)
{
    m_type = "wbMap12h";
    m_description = "Wiren Board WB-MAP12H energy meter";

    m_options.insert("delta",     QJsonObject {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",     QJsonObject {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
    m_options.insert("frequency", QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "measurement"}, {"unit", "Hz"}, {"round", 1}});
    m_options.insert("voltage",   QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("current",   QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("power",     QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("energy",    QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});

    for (quint8 i = 0; i < 13; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose ratio = Expose(new NumberObject("ratio")), delta = Expose(new NumberObject("delta")), voltage = Expose(new SensorObject("voltage")), current = Expose(new SensorObject("current")), power = Expose(new SensorObject("power")), energy = Expose(new SensorObject("energy"));

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);

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

void WirenBoard::WBMap12h::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 offset, registerAddress;

    if (!endpointId || endpointId > 12)
        return;

    offset = (endpointId - 1) % 3 + (endpointId - 1) / 3 * 0x1000;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset;
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset + 3;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap12h::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 4;
    m_polling = true;

    m_totalPower = 0;
    m_totalEnergy = 0;
}

QByteArray WirenBoard::WBMap12h::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 4:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 5:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);

        case 6 ... 9:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 10 ... 13:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS + (m_sequence - 10) * 0x1000, WBMAP_POWER_REGISTER_COUNT);

        case 14 ... 17:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS + (m_sequence - 14) * 0x1000, WBMAP_ENERGY_REGISTER_COUNT);

        default:
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void WirenBoard::WBMap12h::parseReply(const QByteArray &reply)
{
    bool check = false;

    switch (m_sequence)
    {
        case 0 ... 3:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 4:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", round(static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPILER) / 1000.0);
            break;
        }

        case 5:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPILER) / 1000.0;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.find(i + j * 3 + 1 ).value()->buffer().insert("voltage", value);
            }

            break;
        }

        case 6 ... 9:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find((m_sequence - 6) * 3 + i + 1).value()->buffer().insert("current", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPILER) / 1000.0);

            break;
        }

        case 10 ... 13:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                if (i)
                    m_endpoints.find((m_sequence - 10) * 3 + i).value()->buffer().insert("power", round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP12H_POWER_MULTIPILER_C) / 1000.0);
                else
                    m_totalPower += round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP12H_POWER_MULTIPILER_T) / 1000.0;
            }

            break;
        }

        case 14 ... 17:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = round(static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPILER) / 1000.0;

                if (i)
                    m_endpoints.find((m_sequence - 14) * 3 + i).value()->buffer().insert("energy", value);
                else
                    m_totalEnergy += value;
            }

            if (m_sequence == 17)
                check = true;

            break;
        }
    }

    if (check)
    {
        auto it = m_endpoints.find(0);
        it.value()->buffer().insert("power", m_totalPower);
        it.value()->buffer().insert("energy", m_totalEnergy);
        updateEndpoints();
    }

    m_sequence++;
}
