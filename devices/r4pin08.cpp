#include "r4pin08.h"
#include "expose.h"
#include "modbus.h"

void R4PIN08::DI8::init(const Device &device)
{
    m_type = "r4pin08di8";
    m_description = "Eletechsup R4PIN08-8DI Controller";

    m_options.insert("inputLevel", QJsonObject {{"type", "select"}, {"enum", QJsonArray {"low", "high"}}, {"icon", "mdi:swap-horizontal-bold"}});
    m_options.insert("input", QJsonObject {{"type", "sensor"}, {"icon", "mdi:import"}});

    for (quint8 i = 0; i < 9; i++)
    {
        Expose expose = i ? Expose(new SensorObject("input")) : Expose(new ToggleObject("inputLevel"));
        Endpoint endpoint(new EndpointObject(i, device));

        expose->setMultiple(i ? true : false);
        expose->setParent(endpoint.data());

        endpoint->exposes().append(expose);
        m_endpoints.insert(i, endpoint);
    }

    memset(m_status, 0xFF, sizeof(m_status));
}

void R4PIN08::DI8::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "inputLevel")
    {
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x00F5, data.toString() == "high" ? 1 : 0));
        m_fullPoll = true;
    }
}

void R4PIN08::DI8::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray R4PIN08::DI8::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x00F5, 1);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 8);

        default:
            updateEndpoints();
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void R4PIN08::DI8::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("inputLevel", value ? "high" : "low");
            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 status[8];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, status) != Modbus::ReplyStatus::Ok || !memcmp(m_status, status, sizeof(m_status)))
                break;

            for (quint8 i = 0; i < 8; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("input", status[i] ? true : false);

            memcpy(m_status, status, sizeof(m_status));
            break;
        }
    }

    m_sequence++;
}
