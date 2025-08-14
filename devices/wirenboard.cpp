#include <math.h>
#include "expose.h"
#include "modbus.h"
#include "wirenboard.h"

void WirenBoard::WBMap3e::init(const Device &device)
{
    m_type = "wbMap3e";
    m_description = "Wiren Board WB-MAP3E Energy Meter";

    m_options.insert("frequency",   QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "measurement"}, {"unit", "Hz"}, {"round", 1}});
    m_options.insert("voltage",     QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("current",     QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("power",       QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("totalPower",  QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("energy",      QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("totalEnergy", QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("angle",       QJsonObject {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QJsonObject {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QJsonObject {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});

    for (quint8 i = 0; i < 4; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

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

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose frequency(new SensorObject("frequency")), totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

            totalPower->setParent(endpoint.data());
            endpoint->exposes().append(totalPower);

            totalEnergy->setParent(endpoint.data());
            endpoint->exposes().append(totalEnergy);
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

        case 6:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void WirenBoard::WBMap3e::parseReply(const QByteArray &reply)
{
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

            m_endpoints.find(0).value()->buffer().insert("frequency", round(static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER) / 1000.0);
            break;
        }

        case 2:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("voltage", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPLIER) / 1000.0);

            break;
        }

        case 3:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("current", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER) / 1000.0);

            break;
        }

        case 4:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.find(i).value()->buffer().insert(i ? "power" : "totalPower", round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP_POWER_MULTIPLIER) / 1000.0);

            break;
        }

        case 5:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
               m_endpoints.find(i).value()->buffer().insert(i ? "energy" : "totalEnergy", round(static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER) / 1000.0);

            break;
        }

        case 6:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("angle", round(static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER) / 1000.0);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap6s::init(const Device &device)
{
    m_type = "wbMap6s";
    m_description = "Wiren Board WB-MAP6S Energy Meter";

    m_options.insert("frequency",   QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "measurement"}, {"unit", "Hz"}, {"round", 1}});
    m_options.insert("voltage",     QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("current",     QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("power",       QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("totalPower",  QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("energy",      QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("totalEnergy", QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("delta",       QJsonObject {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QJsonObject {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});

    for (quint8 i = 0; i < 7; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

            current->setMultiple(true);
            current->setParent(endpoint.data());
            endpoint->exposes().append(current);

            power->setMultiple(true);
            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setMultiple(true);
            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose voltage(new SensorObject("voltage")), frequency(new SensorObject("frequency")), totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

            totalPower->setParent(endpoint.data());
            endpoint->exposes().append(totalPower);

            totalEnergy->setParent(endpoint.data());
            endpoint->exposes().append(totalEnergy);
        }

        m_endpoints.insert(i, endpoint);
    }
}

void WirenBoard::WBMap6s::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 offset, registerAddress;

    if (!endpointId || endpointId > 6)
        return;

    offset = (6 - endpointId) % 3 + (endpointId - 1) / 3 * 0x1000;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset;
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset + 3;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap6s::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;

    m_totalPower = 0;
    m_totalEnergy = 0;
}

QByteArray WirenBoard::WBMap6s::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 2:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_VOLTAGE_REGISTER_ADDRESS, 1);

        case 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 4 ... 5:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 4) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 6 ... 7:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_POWER_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP6S_POWER_REGISTER_COUNT);

        case 8 ... 9:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_ENERGY_REGISTER_ADDRESS + (m_sequence - 8) * 0x1000, WBMAP6S_ENERGY_REGISTER_COUNT);

        default:
        {
            auto it = m_endpoints.find(0);

            it.value()->buffer().insert("totalPower", m_totalPower);
            it.value()->buffer().insert("totalEnergy", m_totalEnergy);

            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
        }
    }
}


void WirenBoard::WBMap6s::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 1:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + 3 - i);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 2:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("voltage", round(static_cast <double> (value) * WBMAP6S_VOLTAGE_MULTIPLIER) / 1000.0);
            break;
        }

        case 3:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", round(static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER) / 1000.0);
            break;
        }

        case 4 ... 5:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find((m_sequence - 4) * 3 + 3 - i).value()->buffer().insert("current", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER) / 1000.0);

            break;
        }

        case 6 ... 7:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * WBMAP6S_POWER_MULTIPLIER) / 1000.0;
                m_endpoints.find((m_sequence - 6) * 3 + 3 - i).value()->buffer().insert("power", value);
                m_totalPower += value;
            }

            break;
        }

        case 8 ... 9:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = round(static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER) / 1000.0;
                m_endpoints.find((m_sequence - 8) * 3 + 3 - i).value()->buffer().insert("energy", value);
                m_totalEnergy += value;
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap12::init(const Device &device)
{
    switch (m_model)
    {
        case Model::wbMap12e:
            m_type = "wbMap12e";
            m_description = "Wiren Board WB-MAP12H Energy Meter";
            break;

        case Model::wbMap12h:
            m_type = "wbMap12h";
            m_description = "Wiren Board WB-MAP12H Energy Meter";
            break;
    }

    m_options.insert("frequency",   QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "measurement"}, {"unit", "Hz"}, {"round", 1}});
    m_options.insert("voltage",     QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("current",     QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("power",       QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("totalPower",  QJsonObject {{"type", "sensor"}, {"class", "power"}, {"state", "measurement"}, {"unit", "W"}, {"round", 2}});
    m_options.insert("energy",      QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("totalEnergy", QJsonObject {{"type", "sensor"}, {"class", "energy"}, {"state", "total_increasing"}, {"unit", "kWh"}, {"round", 2}});
    m_options.insert("angle",       QJsonObject {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QJsonObject {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QJsonObject {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});

    for (quint8 i = 0; i < 13; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

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

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose frequency(new SensorObject("frequency"));

            for (quint8 j = 1; j <= 4; j++)
            {
                Expose totalPower(new SensorObject(QString("totalPower_%1").arg(j))), totalEnergy(new SensorObject(QString("totalEnergy_%1").arg(j)));

                totalPower->setParent(endpoint.data());
                endpoint->exposes().append(totalPower);

                totalEnergy->setParent(endpoint.data());
                endpoint->exposes().append(totalEnergy);
            }

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

        }

        m_endpoints.insert(i, endpoint);
    }
}

void WirenBoard::WBMap12::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
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

void WirenBoard::WBMap12::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 4;
    m_polling = true;
}

QByteArray WirenBoard::WBMap12::pollRequest(void)
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

        case 18:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);

        default:
        {
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
        }
    }
}

void WirenBoard::WBMap12::parseReply(const QByteArray &reply)
{
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

            m_endpoints.find(0).value()->buffer().insert("frequency", round(static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER) / 1000.0);
            break;
        }

        case 5:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPLIER) / 1000.0;

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
                m_endpoints.find((m_sequence - 6) * 3 + i + 1).value()->buffer().insert("current", round(static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER) / 1000.0);

            break;
        }

        case 10 ... 13:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = round(static_cast <double> (static_cast <qint32> (data[i * 2]) << 16 | static_cast <qint32> (data[i * 2 + 1])) * (m_model == Model::wbMap12h ? i ? WBMAP12H_CHANNEL_POWER_MULTIPLIER : WBMAP12H_TOTAL_POWER_MULTIPLIER : WBMAP_POWER_MULTIPLIER)) / 1000.0;

                if (i)
                    m_endpoints.find((m_sequence - 10) * 3 + i).value()->buffer().insert("power", value);
                else
                    m_endpoints.find(0).value()->buffer().insert(QString("totalPower_%1").arg(m_sequence - 9), value);
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
                double value = round(static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER) / 1000.0;

                if (i)
                    m_endpoints.find((m_sequence - 14) * 3 + i).value()->buffer().insert("energy", value);
                else
                    m_endpoints.find(0).value()->buffer().insert(QString("totalEnergy_%1").arg(m_sequence - 13), value);
            }

            break;
        }

        case 18:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = round(static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER) / 1000.0;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.find(i + j * 3 + 1 ).value()->buffer().insert("angle", value);
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMr6::init(const Device &device)
{
    m_type = "wbMr6";
    m_description = "Wiren Board WB-MR6 Relay Controller";
    m_options.insert("input", QJsonObject {{"type", "sensor"}, {"icon", "mdi:import"}});

    for (quint8 i = 0; i < 7; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose output(new SwitchObject), input(new BinaryObject("input"));

            output->setMultiple(true);
            output->setParent(endpoint.data());
            endpoint->exposes().append(output);

            input->setMultiple(true);
            input->setParent(endpoint.data());
            endpoint->exposes().append(input);
        }
        else
        {
            Expose input(new BinaryObject("input_0"));
            input->setParent(endpoint.data());
            endpoint->exposes().append(input);
        }

        m_endpoints.insert(i, endpoint);
    }
}

void WirenBoard::WBMr6::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (name == "status" && endpointId && endpointId <= 6)
    {
        QList <QString> list = {"on", "off", "toggle"};
        quint16 value;

        switch (list.indexOf(data.toString()))
        {
            case 0:  value = 1; break;
            case 1:  value = 0; break;
            case 2:  value = m_output[endpointId - 1] ? 0 : 1; break;
            default: return;
        }

        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId - 1, value ? 0xFF00 : 0x0000));
    }
}

void WirenBoard::WBMr6::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray WirenBoard::WBMr6::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, 6);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 8);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void WirenBoard::WBMr6::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[6];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 6; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 1:
        {
            quint16 data[8];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 6; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            m_endpoints.value(0)->buffer().insert("input_0", data[7] ? true : false);
            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBUps::init(const Device &device)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose battety(new SensorObject("battery")), batteryStatus(new SensorObject("batteryStatus")), temperature(new SensorObject("temperature")), temperatureStatus(new SensorObject("temperatureStatus")), inputVoltage(new SensorObject("inputVoltage")), outputVoltage(new SensorObject("outputVoltage")), batteryVoltage(new SensorObject("batteryVoltage")), batteryCurrent(new SensorObject("batteryCurrent")), chargeCurrent(new SensorObject("chargeCurrent")), dischargeCurrent(new SensorObject("dischargeCurrent")), operationMode(new SelectObject("operationMode")), outputVoltageLimit(new NumberObject("outputVoltageLimit")), chargeCurrentLimit(new NumberObject("chargeCurrentLimit"));

    m_type = "wbUps";
    m_description = "Wiren Board UPS v3 Backup Power Supply";

    m_options.insert("battery",            QJsonObject {{"type", "sensor"}, {"class", "battery"}, {"unit", "%"}, {"round", 1}});
    m_options.insert("batteryStatus",      QJsonObject {{"type", "sensor"}, {"icon", "mdi:battery-charging"}});
    m_options.insert("temperature",        QJsonObject {{"type", "sensor"}, {"class", "temperature"}, {"state", "measurement"}, {"unit", "°C"}, {"round", 1}});
    m_options.insert("temperatureStatus",  QJsonObject {{"type", "sensor"}, {"icon", "mdi:thermometer"}});
    m_options.insert("inputVoltage",       QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("outputVoltage",      QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("batteryVoltage",     QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}, {"round", 1}});
    m_options.insert("batteryCurrent",     QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("chargeCurrent",      QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("dischargeCurrent",   QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}, {"round", 3}});
    m_options.insert("operationMode",      QJsonObject {{"type", "select"}, {"enum", QJsonArray {"auto", "manual"}}, {"icon", "mdi:cog"}});
    m_options.insert("outputVoltageLimit", QJsonObject {{"type", "number"}, {"min", 9}, {"max", 26.5}, {"step", 0.1}, {"unit", "V"}, {"icon", "mdi:sine-wave"}});
    m_options.insert("chargeCurrentLimit", QJsonObject {{"type", "number"}, {"min", 0.3}, {"max", 2}, {"step", 0.1}, {"unit", "A"}, {"icon", "mdi:current-ac"}});

    battety->setParent(endpoint.data());
    endpoint->exposes().append(battety);

    batteryStatus->setParent(endpoint.data());
    endpoint->exposes().append(batteryStatus);

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    temperatureStatus->setParent(endpoint.data());
    endpoint->exposes().append(temperatureStatus);

    inputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(inputVoltage);

    outputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltage);

    batteryVoltage->setParent(endpoint.data());
    endpoint->exposes().append(batteryVoltage);

    batteryCurrent->setParent(endpoint.data());
    endpoint->exposes().append(batteryCurrent);

    chargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrent);

    dischargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(dischargeCurrent);

    operationMode->setParent(endpoint.data());
    endpoint->exposes().append(operationMode);

    outputVoltageLimit->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltageLimit);

    chargeCurrentLimit->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrentLimit);

    m_endpoints.insert(0, endpoint);
}

void WirenBoard::WBUps::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    QList <QString> list = {"operationMode", "outputVoltageLimit", "chargeCurrentLimit"};
    int index = list.indexOf(name);

    switch (index)
    {
        case 0: // operationMode
        {
            quint16 value = data.toString() == "manual" ? 1 : 0;
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0010, value));
            break;
        }

        case 1: // outputVoltageLimit
        case 2: // chargeCurrentLimit
        {
            quint16 value = static_cast <quint16> (data.toDouble() * 1000);
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, index == 1 ? 0x0011 : 0x0012, value));
            break;
        }

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBUps::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBUps::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0010, 3);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 10);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void WirenBoard::WBUps::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[3];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("operationMode",         data[0] ? "manual" : "auto");
            m_endpoints.value(0)->buffer().insert("outputVoltageLimit", data[1] / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrentLimit", data[2] / 1000.0);

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[10];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            switch (data[0])
            {
                case 0: m_endpoints.value(0)->buffer().insert("batteryStatus", "charge"); break;
                case 1: m_endpoints.value(0)->buffer().insert("batteryStatus", "charged"); break;
                case 2: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharge"); break;
                case 3: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharged"); break;
                case 4: m_endpoints.value(0)->buffer().insert("batteryStatus", "alarm"); break;
            }

            switch (data[1])
            {
                case 0: m_endpoints.value(0)->buffer().insert("temperatureStatus", "normal"); break;
                case 1: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overcool"); break;
                case 2: m_endpoints.value(0)->buffer().insert("temperatureStatus", "cool"); break;
                case 3: m_endpoints.value(0)->buffer().insert("temperatureStatus", "heat"); break;
                case 4: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overheat"); break;
            }

            m_endpoints.value(0)->buffer().insert("inputVoltage",     data[2] / 1000.0);
            m_endpoints.value(0)->buffer().insert("outputVoltage",    data[3] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryVoltage",   data[4] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryCurrent",   static_cast <qint16> (data[5]) / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrent",    data[6] / 1000.0);
            m_endpoints.value(0)->buffer().insert("dischargeCurrent", data[7] / 1000.0);
            m_endpoints.value(0)->buffer().insert("battery",          data[8] / 100.0);
            m_endpoints.value(0)->buffer().insert("temperature",      data[9] / 100.0);
            break;
        }
    }

    m_sequence++;
}
