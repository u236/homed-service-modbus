#include "expose.h"
#include "wb-map.h"

void WirenBoard::WBMap3ev::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap3ev";
    m_description = "Wiren Board WB-MAP3EV Voltage Meter";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), angle(new SensorObject("angle"));

            voltage->setMultiple(true);
            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);
        }
        else
        {
            Expose frequency(new SensorObject("frequency"));
            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);
}

void WirenBoard::WBMap3ev::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray WirenBoard::WBMap3ev::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMap3ev::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 1:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("voltage", Modbus::toUInt32BE(data + i * 2) * WBMAP_VOLTAGE_MULTIPLIER / 1000);

            break;
        }

        case 2:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("angle", static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap3e::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap3e";
    m_description = "Wiren Board WB-MAP3E Energy Meter";

    for (quint8 i = 0; i <= 3; i++)
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

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
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

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
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
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS, WBMAP_COIL_REGISTER_COUNT);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);
        case 3: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS, WBMAP_CURRENT_REGISTER_COUNT);
        case 4: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS, WBMAP_POWER_REGISTER_COUNT);
        case 5: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS, WBMAP_ENERGY_REGISTER_COUNT);
        case 6: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMap3e::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            break;
        }

        case 1:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 2:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("voltage", Modbus::toUInt32BE(data + i * 2) * WBMAP_VOLTAGE_MULTIPLIER / 1000);

            break;
        }

        case 3:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("current", Modbus::toUInt32BE(data + i * 2) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 4:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.value(i)->buffer().insert(i ? "power" : "totalPower", Modbus::toUInt32BE(data + i * 2) * WBMAP_POWER_MULTIPLIER / 1000);

            break;
        }

        case 5:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.value(i)->buffer().insert(i ? "energy" : "totalEnergy", Modbus::toUInt64LE(data + i * 4) * WBMAP_ENERGY_MULTIPLIER / 1000);

            break;
        }

        case 6:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("angle", static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap6s::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap6s";
    m_description = "Wiren Board WB-MAP6S Energy Meter";

    for (quint8 i = 0; i <= 6; i++)
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

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
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

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
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
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 2:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_VOLTAGE_REGISTER_ADDRESS, 1);

        case 3:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 4 ... 5:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 4) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 6 ... 7:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_POWER_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP6S_POWER_REGISTER_COUNT);

        case 8 ... 9:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_ENERGY_REGISTER_ADDRESS + (m_sequence - 8) * 0x1000, WBMAP6S_ENERGY_REGISTER_COUNT);
    }

    m_endpoints.value(0)->buffer().insert("totalPower", m_totalPower);
    m_endpoints.value(0)->buffer().insert("totalEnergy", m_totalEnergy);

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMap6s::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 1:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + 3 - i);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            break;
        }

        case 2:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("voltage", static_cast <double> (value) * WBMAP6S_VOLTAGE_MULTIPLIER / 1000);
            break;
        }

        case 3:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 4 ... 5:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value((m_sequence - 4) * 3 + 3 - i)->buffer().insert("current", Modbus::toUInt32BE(data + i * 2) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 6 ... 7:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = Modbus::toUInt32BE(data + i * 2) * WBMAP6S_POWER_MULTIPLIER / 1000;
                m_endpoints.value((m_sequence - 6) * 3 + 3 - i)->buffer().insert("power", value);
                m_totalPower += value;
            }

            break;
        }

        case 8 ... 9:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = Modbus::toUInt64LE(data + i * 4) * WBMAP_ENERGY_MULTIPLIER / 1000;
                m_endpoints.value((m_sequence - 8) * 3 + 3 - i)->buffer().insert("energy", value);
                m_totalEnergy += value;
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap12::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
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

    for (quint8 i = 0; i <= 12; i++)
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

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
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

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
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
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 4:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 5:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);

        case 6 ... 9:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 10 ... 13:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS + (m_sequence - 10) * 0x1000, WBMAP_POWER_REGISTER_COUNT);

        case 14 ... 17:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS + (m_sequence - 14) * 0x1000, WBMAP_ENERGY_REGISTER_COUNT);

        case 18:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMap12::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 3:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            break;
        }

        case 4:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 5:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = Modbus::toUInt32BE(data + i * 2) * WBMAP_VOLTAGE_MULTIPLIER / 1000;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.value(i + j * 3 + 1)->buffer().insert("voltage", value);
            }

            break;
        }

        case 6 ... 9:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value((m_sequence - 6) * 3 + i + 1)->buffer().insert("current", Modbus::toUInt32BE(data + i * 2) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 10 ... 13:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = Modbus::toUInt32BE(data + i * 2) * (m_model == Model::wbMap12h ? i ? WBMAP12H_CHANNEL_POWER_MULTIPLIER : WBMAP12H_TOTAL_POWER_MULTIPLIER : WBMAP_POWER_MULTIPLIER) / 1000;

                if (i)
                    m_endpoints.value((m_sequence - 10) * 3 + i)->buffer().insert("power", value);
                else
                    m_endpoints.value(0)->buffer().insert(QString("totalPower_%1").arg(m_sequence - 9), value);
            }

            break;
        }

        case 14 ... 17:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = Modbus::toUInt64LE(data + i * 4) * WBMAP_ENERGY_MULTIPLIER / 1000;

                if (i)
                    m_endpoints.value((m_sequence - 14) * 3 + i)->buffer().insert("energy", value);
                else
                    m_endpoints.value(0)->buffer().insert(QString("totalEnergy_%1").arg(m_sequence - 13), value);
            }

            break;
        }

        case 18:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.value(i + j * 3 + 1)->buffer().insert("angle", value);
            }

            break;
        }
    }

    m_sequence++;
}
