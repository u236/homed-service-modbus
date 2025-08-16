#include "expose.h"
#include "modbus.h"
#include "other.h"

void Other::JTH2D1::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
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

void Other::JTH2D1::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Other::JTH2D1::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0300, 2);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void Other::JTH2D1::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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

    m_options.insert("operationMode", QJsonObject {{"type", "select"}, {"enum", QJsonArray::fromStringList(m_modes)}, {"control", true}, {"icon", "mdi:sync"}});
}

void Other::T13::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "operationMode")
    {
        switch (m_modes.indexOf(data.toString()))
        {
            case 0: m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0001)); break;
            case 1: m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0002)); break;
            case 2: m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0003, 0x0005)); break;
        }
    }
    else if (name == "frequency")
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0002, static_cast <quint16> (data.toDouble() * 10)));
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
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0002, 2);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0008, 3);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void Other::T13::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("voltage",     data[0] / 10.0);
            m_endpoints.value(0)->buffer().insert("current",     data[1] / 10.0);
            m_endpoints.value(0)->buffer().insert("temperature", data[2]);
            break;
        }
    }

    m_sequence++;
}
