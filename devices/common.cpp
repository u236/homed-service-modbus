#include "common.h"
#include "expose.h"
#include "modbus.h"

void Common::RelayController::init(const Device &device)
{
    m_type = "homedRelayController";
    m_description = "HOMEd Relay Controller";

    for (quint8 i = 0; i <= 16; i++)
    {
        Expose expose = i ? Expose(new SwitchObject) : Expose(new BooleanObject("invert"));
        Endpoint endpoint(new EndpointObject(i, device));

        expose->setMultiple(i ? true : false);
        expose->setParent(endpoint.data());

        endpoint->exposes().append(expose);
        m_endpoints.insert(i, endpoint);
    }
}

void Common::RelayController::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
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

void Common::RelayController::startPoll(void)
{
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_sequence = m_fullPoll ? 0 : 1;
}

QByteArray Common::RelayController::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:  return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0030, 1);
        case 1:  return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0001, 1);
        default: return QByteArray();
    }
}

void Common::RelayController::parseReply(const QByteArray &reply)
{
    quint16 value;

    switch (m_sequence)
    {
        case 0:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->status().insert("invert", value ? true : false);
            emit endpointUpdated(this, 0);

            m_fullPoll = false;
            break;

        case 1:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok || (m_status == value && !m_firstPoll))
                break;

            for (quint8 i = 0; i < 16; i++)
            {
                auto it = m_endpoints.find(i + 1);
                quint16 check = value & 1 << i;

                if (!m_firstPoll && (m_status & 1 << i) == check)
                    continue;

                it.value()->status().insert("status", check ? "on" : "off");
                emit endpointUpdated(this, it.key());
            }

            m_firstPoll = false;
            m_status = value;

            if (m_pending == m_status)
                m_update = false;

            break;
    }

    m_sequence++;
}

void Common::SwitchController::init(const Device &device)
{
    m_type = "homedSwitchController";
    m_description = "HOMEd Switch Controller";
    m_options.insert("action", QVariant(QList <QString> {"press", "release", "hold"}));

    for (quint8 i = 0; i <= 16; i++)
    {
        Expose expose = i ? Expose(new Sensor::Action) : Expose(new BooleanObject("invert"));
        Endpoint endpoint(new EndpointObject(i, device));

        expose->setMultiple(i ? true : false);
        expose->setParent(endpoint.data());

        endpoint->exposes().append(expose);
        m_endpoints.insert(i, endpoint);
    }

    memset(m_time, 0, sizeof(m_time));
    memset(m_hold, 0, sizeof(m_hold));

    connect(m_timer, &QTimer::timeout, this, &SwitchController::update);
    m_timer->start(1);
}

void Common::SwitchController::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    if (name == "invert")
    {
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0030, data.toBool() ? 1 : 0));
        m_fullPoll = true;
    }
}

void Common::SwitchController::startPoll(void)
{
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_sequence = m_fullPoll ? 0 : 1;
}

QByteArray Common::SwitchController::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:  return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0030, 1);
        case 1:  return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0001, 1);
        default: return QByteArray();
    }
}

void Common::SwitchController::parseReply(const QByteArray &reply)
{
    quint16 value;

    switch (m_sequence)
    {
        case 0:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->status().insert("invert", value ? true : false);
            emit endpointUpdated(this, 0);

            m_fullPoll = false;
            break;

        case 1:

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok || (m_status == value && !m_firstPoll))
                break;

            for (quint8 i = 0; i < 16; i++)
            {
                auto it = m_endpoints.find(i + 1);
                quint16 check = value & 1 << i;

                if (!m_firstPoll && (m_status & 1 << i) == check)
                    continue;

                m_time[i] = check ? QDateTime::currentMSecsSinceEpoch() : 0;

                if (m_hold[i])
                {
                    m_hold[i] = false;
                    continue;
                }

                it.value()->status().insert("action", check ? "press" : "release");
                emit endpointUpdated(this, it.key());
            }

            m_firstPoll = false;
            m_status = value;
            break;

    }

    m_sequence++;
}

void Common::SwitchController::update(void)
{
    for (quint8 i = 0; i < 16; i++)
    {
        auto it = m_endpoints.find(i + 1);

        if (!m_time[i] || m_time[i] + 1000 > QDateTime::currentMSecsSinceEpoch())
            continue;

        m_time[i] = 0;
        m_hold[i] = true;

        it.value()->status().insert("action", "hold");
        emit endpointUpdated(this, it.key());
    }
}
