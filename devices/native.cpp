#include "native.h"
#include "expose.h"
#include "modbus.h"

void Native::RelayController::init(const Device &device)
{
    m_type = "homedRelayController";
    m_description = "HOMEd Relay Controller";

    m_options.insert("invert", QJsonObject {{"type", "toggle"}, {"icon", "mdi:swap-horizontal-bold"}});

    for (quint8 i = 0; i < 17; i++)
    {
        Expose expose = i ? Expose(new SwitchObject) : Expose(new ToggleObject("invert"));
        Endpoint endpoint(new EndpointObject(i, device));

        expose->setMultiple(i ? true : false);
        expose->setParent(endpoint.data());

        endpoint->exposes().append(expose);
        m_endpoints.insert(i, endpoint);
    }
}

void Native::RelayController::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (name == "invert")
    {
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0030, data.toBool() ? 1 : 0));
        m_fullPoll = true;
    }
    else if (name == "status" && endpointId && endpointId <= 16)
    {
        QList <QString> list = {"on", "off", "toggle"};
        quint16 mask = 1 << (endpointId - 1);

        if (!m_update)
        {
            m_pending = m_status;
            m_update = true;
        }

        switch (list.indexOf(data.toString()))
        {
            case 0: m_pending |=  mask; break;
            case 1: m_pending &= ~mask; break;
            case 2: m_pending ^=  mask; break;
        }

        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0001, m_pending));
    }
}

void Native::RelayController::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray Native::RelayController::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0030, 1);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0001, 1);

        default:
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void Native::RelayController::parseReply(const QByteArray &reply)
{
    quint16 value;
    bool check = false;

    switch (m_sequence)
    {
        case 0:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("invert", value ? true : false);
            m_fullPoll = false;
            break;

        case 1:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 16; i++)
            {
                auto it = m_endpoints.find(i + 1);
                quint16 status = value & 1 << i;

                if (it.value()->buffer().contains("status") && (m_status & 1 << i) == status)
                    continue;

                it.value()->buffer().insert("status", status ? "on" : "off");
            }

            m_status = value;

            if (m_pending == m_status)
                m_update = false;

            check = true;
            break;
    }

    if (check)
        updateEndpoints();

    m_sequence++;
}

void Native::SwitchController::init(const Device &device)
{
    m_type = "homedSwitchController";
    m_description = "HOMEd Switch Controller";

    m_options.insert("invert", QJsonObject {{"type", "toggle"}, {"icon", "mdi:swap-horizontal-bold"}});
    m_options.insert("action", QJsonObject {{"type", "sensor"}, {"icon", "mdi:gesture-double-tap"}, {"trigger", QJsonArray {"press", "release", "hold"}}});

    for (quint8 i = 0; i < 17; i++)
    {
        Expose expose = i ? Expose(new SensorObject("action")) : Expose(new ToggleObject("invert"));
        Endpoint endpoint(new EndpointObject(i, device));

        expose->setMultiple(i ? true : false);
        expose->setParent(endpoint.data());

        endpoint->exposes().append(expose);
        m_endpoints.insert(i, endpoint);
    }

    memset(m_time, 0, sizeof(m_time));
    connect(m_timer, &QTimer::timeout, this, &SwitchController::update);
    m_timer->start(1);
}

void Native::SwitchController::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "invert")
    {
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0030, data.toBool() ? 1 : 0));
        m_fullPoll = true;
    }
}

void Native::SwitchController::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray Native::SwitchController::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0030, 1);

        case 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0001, 1);

        default:
            m_pollTime = QDateTime::currentMSecsSinceEpoch();
            m_polling = false;
            return QByteArray();
    }
}

void Native::SwitchController::parseReply(const QByteArray &reply)
{
    quint16 value;
    bool check = false;

    switch (m_sequence)
    {
        case 0:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("invert", value ? true : false);
            m_fullPoll = false;
            m_firstPoll = true;
            break;

        case 1:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            if (m_firstPoll)
            {
                m_firstPoll = false;
                m_status = value;
                break;
            }

            for (quint8 i = 0; i < 16; i++)
            {
                auto it = m_endpoints.find(i + 1);
                quint16 status = value & 1 << i;

                if ((m_status & 1 << i) == status)
                {
                    it.value()->buffer().clear();
                    continue;
                }

                m_time[i] = status ? QDateTime::currentMSecsSinceEpoch() : 0;
                it.value()->buffer().insert("action", status ? "press" : "release");
            }

            m_status = value;
            check = true;
            break;
    }

    if (check)
        updateEndpoints();

    m_sequence++;
}

void Native::SwitchController::update(void)
{
    for (quint8 i = 0; i < 16; i++)
    {
        auto it = m_endpoints.find(i + 1);

        if (!m_time[i] || m_time[i] + 1000 > QDateTime::currentMSecsSinceEpoch())
            continue;

        m_time[i] = 0;
        it.value()->buffer().insert("action", "hold");
        updateEndpoints();
    }
}
