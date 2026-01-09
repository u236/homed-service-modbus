#include <QtEndian>
#include "expose.h"
#include "other.h"

void Other::JTH2d1::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose temperature(new SensorObject("temperature")), humidity(new SensorObject("humidity"));

    m_type = "jth2d1";
    m_description = "JTH-2D1 Temperature and Humidity Sensor";

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    humidity->setParent(endpoint.data());
    endpoint->exposes().append(humidity);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);
}

void Other::JTH2d1::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Other::JTH2d1::pollRequest(void)
{
    if (!m_sequence)
        return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0300, 2);

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Other::JTH2d1::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("temperature", static_cast <qint16> (data[0]) / 10.0);
            m_endpoints.value(0)->buffer().insert("humidity", data[1] / 10.0);
            break;
        }
    }

    m_sequence++;
}

void Other::T13::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose operationMode(new SelectObject("operationMode")), frequency(new NumberObject("frequency")), voltage(new SensorObject("voltage")), current(new SensorObject("current")), temperature(new SensorObject("temperature"));

    m_type = "t13";
    m_description = "T13-750W-12-H Frequency Converter";
    m_modes = {"forward", "stop", "reverse"};

    operationMode->setParent(endpoint.data());
    endpoint->exposes().append(operationMode);

    frequency->setParent(endpoint.data());
    endpoint->exposes().append(frequency);

    voltage->setParent(endpoint.data());
    endpoint->exposes().append(voltage);

    current->setParent(endpoint.data());
    endpoint->exposes().append(current);

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);

    m_options.insert("operationMode", QMap <QString, QVariant> {{"type", "select"}, {"enum", QVariant(m_modes)}, {"control", true}, {"icon", "mdi:sync"}});
    m_options.insert("frequency",     QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 50}, {"step", 0.1}, {"unit", "Hz"}, {"control", true}, {"icon", "mdi:sine-wave"}});
}

void Other::T13::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "operationMode")
    {
        switch (m_modes.indexOf(data.toString()))
        {
            case 0: m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0001)); break;
            case 1: m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0002)); break;
            case 2: m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0005)); break;
        }
    }
    else if (name == "frequency")
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0002, static_cast <quint16> (data.toDouble() * 10)));
}

void Other::T13::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Other::T13::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0002, 2);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0008, 3);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Other::T13::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequency", data[0] / 10.0);

            switch (data[1])
            {
                case 1: m_endpoints.value(0)->buffer().insert("operationMode", "forward"); break;
                case 2: m_endpoints.value(0)->buffer().insert("operationMode", "stop"); break;
                case 5: m_endpoints.value(0)->buffer().insert("operationMode", "reverse"); break;
            }

            break;
        }

        case 1:
        {
            quint16 data[3];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("voltage",     data[0] / 10.0);
            m_endpoints.value(0)->buffer().insert("current",     data[1] / 10.0);
            m_endpoints.value(0)->buffer().insert("temperature", data[2]);
            break;
        }
    }

    m_sequence++;
}

void Other::M0701s::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose status(new SwitchObject), frequency(new NumberObject("frequency")), voltage(new SensorObject("voltage")), current(new SensorObject("current")), temperature(new SensorObject("temperature"));

    m_type = "m0701s";
    m_description = "VFC-M0701S Frequency Converter";

    status->setParent(endpoint.data());
    endpoint->exposes().append(status);

    frequency->setParent(endpoint.data());
    endpoint->exposes().append(frequency);

    voltage->setParent(endpoint.data());
    endpoint->exposes().append(voltage);

    current->setParent(endpoint.data());
    endpoint->exposes().append(current);

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);

    m_options.insert("frequency", QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 400}, {"unit", "Hz"}, {"control", true}, {"icon", "mdi:sine-wave"}});
}

void Other::M0701s::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "status")
    {
        QList <QString> list = {"on", "off", "toggle"};
        bool value;

        switch (list.indexOf(data.toString()))
        {
            case 0:  value = true; break;
            case 1:  value = false; break;
            case 2:  value = m_status ? false : true; break;
            default: return;
        }

        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x9CA7, value ? 0x0001 : 0x0000));
    }
    else if (name == "frequency")
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x9CA6, static_cast <quint16> (data.toDouble() * 100)));
}

void Other::M0701s::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Other::M0701s::pollRequest(void)
{
    if (!m_sequence)
        return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x9CF4, 6);

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Other::M0701s::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[6];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_status = data[0] != 1;

            m_endpoints.value(0)->buffer().insert("status",      m_status ? "on" : "off");
            m_endpoints.value(0)->buffer().insert("frequency",   data[1] / 100.0);
            m_endpoints.value(0)->buffer().insert("current",     data[3] / 10.0);
            m_endpoints.value(0)->buffer().insert("voltage",     data[4] / 10.0);
            m_endpoints.value(0)->buffer().insert("temperature", data[5]);
            break;
        }
    }

    m_sequence++;
}

void Other::PZEM6l24::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "pzem6l24";
    m_description = "Peacefair PZEM-6L24 Energy Meter";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), frequency(new SensorObject("frequency"));

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

            frequency->setMultiple(true);
            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);
        }
        else
        {
            Expose totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

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
}

void Other::PZEM6l24::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Other::PZEM6l24::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 12);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x000E, 6);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0028, 6);
        case 3: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0020, 2);
        case 4: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x003A, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Other::PZEM6l24::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[12];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(i + 1);
                it.value()->buffer().insert("voltage",   qbswap(data[i]) / 10.0);
                it.value()->buffer().insert("current",   qbswap(data[i + 3]) / 100.0);
                it.value()->buffer().insert("frequency", qbswap(data[i + 6]) / 100.0);
                it.value()->buffer().insert("angle",     static_cast <qint16> (qbswap(data[i + 9])) / 100.0);
            }

            break;
        }

        case 1:
        case 2:
        {
            quint16 data[6];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert(m_sequence == 1 ? "power" : "energy", static_cast <double> (static_cast <quint32> (qbswap(data[i * 2])) | static_cast <quint32> (qbswap(data[i * 2 + 1])) << 16) / 10);

            break;
        }

        case 3:
        case 4:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert(m_sequence == 3 ? "totalPower" : "totalEnergy", static_cast <double> (static_cast <quint32> (qbswap(data[0])) | static_cast <quint32> (qbswap(data[1])) << 16) / 10);
            break;
        }
    }

    m_sequence++;
}
