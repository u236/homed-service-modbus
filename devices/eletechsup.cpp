#include <math.h>
#include "eletechsup.h"
#include "expose.h"

void Eletechsup::N4Dsa02::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "n4dsa02";
    m_description = "Eletechsup N4DSA02 Temperature Sensor";

    for (quint8 i = 1; i <= 2; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));
        Expose temperature(new SensorObject("temperature")), temperatureOffset(new NumberObject("temperatureOffset"));

        temperature->setMultiple(true);
        temperature->setParent(endpoint.data());
        endpoint->exposes().append(temperature);

        temperatureOffset->setMultiple(true);
        temperatureOffset->setParent(endpoint.data());
        endpoint->exposes().append(temperatureOffset);

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);
    m_options.insert("temperatureOffset", QMap <QString, QVariant> {{"type", "number"}, {"min", -5}, {"max", 5}, {"step", 0.1}, {"unit", "°C"}, {"icon", "mdi:thermometer"}});
}

void Eletechsup::N4Dsa02::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (!endpointId || endpointId > 2 || name != "temperatureOffset")
        return;

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0004 + endpointId - 1, static_cast <quint16> (static_cast <qint16> (data.toDouble() * 10))));
    m_fullPoll = true;
}

void Eletechsup::N4Dsa02::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray Eletechsup::N4Dsa02::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0004, 2);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0000, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Eletechsup::N4Dsa02::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("temperatureOffset", static_cast <qint16> (data[i]) / 10.0);

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                double temperature = NAN;

                if (data[i] != 0x8000)
                    temperature = static_cast <qint16> (data[i]) / 10.0;

                m_endpoints.value(i + 1)->buffer().insert("temperature", temperature);
            }

            break;
        }
    }

    m_sequence++;
}


void Eletechsup::R4Pin08::init(const Device &device, const QMap <QString, QVariant> &)
{
    switch (m_model)
    {
        case Model::r4pin08m0:
            m_type = "r4pin08m0";
            m_description = "Eletechsup R4PIN08-8DI Controller";
            m_inputs = 8;
            m_outputs = 0;
            break;

        case Model::r4pin08m1:
            m_type = "r4pin08m1";
            m_description = "Eletechsup R4PIN08-8DO Controller";
            m_inputs = 0;
            m_outputs = 8;
            break;

        case Model::r4pin08m2:
            m_type = "r4pin08m2";
            m_description = "Eletechsup R4PIN08-4DI-4DO Controller";
            m_inputs = 4;
            m_outputs = 4;
            break;

        case Model::r4pin08m3:
            m_type = "r4pin08m3";
            m_description = "Eletechsup R4PIN08-2DI-6DO Controller";
            m_inputs = 2;
            m_outputs = 6;
            break;

        case Model::r4pin08m4:
            m_type = "r4pin08m4";
            m_description = "Eletechsup R4PIN08-6DI-2DO Controller";
            m_inputs = 6;
            m_outputs = 2;
            break;
    }

    for (quint8 i = 0; i <= 8; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose expose = i < m_inputs + 1 ? Expose(new BinaryObject("input")) : Expose(new SwitchObject);
            expose->setMultiple(true);
            expose->setParent(endpoint.data());
            endpoint->exposes().append(expose);
        }
        else
        {
            if (m_inputs)
            {
                Expose expose(new ToggleObject("inputMode"));
                expose->setParent(endpoint.data());
                endpoint->exposes().append(expose);
            }

            if (m_outputs)
            {
                Expose expose(new ToggleObject("outputMode"));
                expose->setParent(endpoint.data());
                endpoint->exposes().append(expose);
            }
        }

        m_endpoints.insert(i, endpoint);
    }

    if (m_inputs)
    {
        m_options.insert("inputMode",  QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"low", "high"}}, {"icon", "mdi:swap-horizontal-bold"}});
        m_options.insert("input",      QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    }

    if (m_outputs)
        m_options.insert("outputMode", QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"low", "high"}}, {"icon", "mdi:swap-horizontal-bold"}});

    memset(m_input, 0xFF, sizeof(m_input));
    memset(m_output, 0xFF, sizeof(m_output));
}

void Eletechsup::R4Pin08::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (m_inputs && name == "inputMode")
    {
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x00F5, data.toString() == "high" ? 0x0001 : 0x0000));
        m_fullPoll = true;
    }
    else if (m_outputs && name == "outputMode")
    {
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x00F6, data.toString() == "high" ? 0x0001 : 0x0000));
        m_fullPoll = true;
    }
    else if (m_outputs && name == "status" && endpointId > m_inputs && endpointId <= m_inputs + m_outputs)
    {
        QList <QString> list = {"on", "off", "toggle"};
        bool value;

        switch (list.indexOf(data.toString()))
        {
            case 0:  value = true; break;
            case 1:  value = false; break;
            case 2:  value = m_output[endpointId - 1] ? false : true; break;
            default: return;
        }

        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId - 1, value ? 0xFF00 : 0x0000));
    }
}

void Eletechsup::R4Pin08::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;
}

QByteArray Eletechsup::R4Pin08::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:

            if (!m_inputs)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x00F5, 1);

        case 1:

            if (!m_outputs)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x00F6, 1);

        case 2:

            if (!m_inputs)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 8);

        case 3:

            if (!m_outputs)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, 8);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Eletechsup::R4Pin08::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 1:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert(m_sequence ? "outputMode" : "inputMode", value ? "high" : "low");
            m_fullPoll = false;
            break;
        }

        case 2:
        {
            quint16 data[8];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok || !memcmp(m_input, data, sizeof(m_input)))
                break;

            for (quint8 i = 0; i < m_inputs; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            memcpy(m_input, data, sizeof(m_input));
            break;
        }

        case 3:
        {
            quint16 data[8];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok || !memcmp(m_output, data, sizeof(m_output)))
                break;

            for (quint8 i = m_inputs; i < m_inputs + m_outputs; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }
    }

    m_sequence++;
}
