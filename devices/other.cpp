#include "expose.h"
#include "modbus.h"
#include "other.h"

void Other::T13::init(const Device &device)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose operationMode = Expose(new SelectObject("operationMode")), frequency = Expose(new NumberObject("frequency")), voltage = Expose(new SensorObject("voltage")), current = Expose(new SensorObject("current")), temperature = Expose(new SensorObject("temperature"));

    m_type = "t13";
    m_description = "T13-750W-12-H Frequency Converter";
    m_modes = {"forward", "stop", "reverse"};

    m_options.insert("operationMode", QJsonObject {{"type", "select"}, {"enum", QJsonArray::fromStringList(m_modes)}, {"control", true}, {"icon", "mdi:sync"}});
    m_options.insert("frequency",     QJsonObject {{"type", "number"}, {"min", 0}, {"max", 50}, {"step", 0.1}, {"unit", "Hz"}, {"control", true}, {"icon", "mdi:sine-wave"}});
    m_options.insert("voltage",       QJsonObject {{"type", "sensor"}, {"class", "voltage"}, {"state", "measurement"}, {"unit", "V"}});
    m_options.insert("current",       QJsonObject {{"type", "sensor"}, {"class", "current"}, {"state", "measurement"}, {"unit", "A"}});
    m_options.insert("temperature",   QJsonObject {{"type", "sensor"}, {"class", "temperature"}, {"state", "measurement"}, {"unit", "°C"}});

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
