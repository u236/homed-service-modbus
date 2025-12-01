#include "expose.h"
#include "kincony.h"

void Kincony::KC868::init(const Device &device, const QMap <QString, QVariant> &)
{
    switch (m_model)
    {
        case Model::kc868a4:
            m_type = "kc868a4";
            m_description = "Kincony KC868-A4 Controller";
            m_channels = 4;
            m_adc = true;
            m_dac = true;
            break;

        case Model::kc868a6:
            m_type = "kc868a6";
            m_description = "Kincony KC868-A6 Controller";
            m_channels = 6;
            m_adc = true;
            m_dac = true;
            break;

        case Model::kc868a8:
            m_type = "kc868a8";
            m_description = "Kincony KC868-A8 Controller";
            m_channels = 8;
            m_adc = true;
            m_dac = false;
            break;

        case Model::kc868a16:
            m_type = "kc868a16";
            m_description = "Kincony KC868-A16 Controller";
            m_channels = 16;
            m_adc = true;
            m_dac = false;
            break;

        case Model::kc868a32:
            m_type = "kc868a32";
            m_description = "Kincony KC868-A32 Controller";
            m_channels = 32;
            m_adc = false;
            m_dac = false;
            break;

        case Model::kc868a64:
            m_type = "kc868a64";
            m_description = "Kincony KC868-A64 Controller";
            m_channels = 64;
            m_adc = true;
            m_dac = false;
            break;

        case Model::kc868a128:
            m_type = "kc868a128";
            m_description = "Kincony KC868-A128 Controller";
            m_channels = 128;
            m_adc = true;
            m_dac = false;
            break;
    }

    for (quint8 i = 1; i <= m_channels; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));
        Expose output(new SwitchObject), input(new BinaryObject("input"));

        output->setMultiple(true);
        output->setParent(endpoint.data());
        endpoint->exposes().append(output);

        input->setMultiple(true);
        input->setParent(endpoint.data());
        endpoint->exposes().append(input);

        if (m_adc && i <= 4)
        {
            Expose analogInput(new SensorObject("analogInput"));
            analogInput->setMultiple(true);
            analogInput->setParent(endpoint.data());
            endpoint->exposes().append(analogInput);
        }

        if (m_dac && i <= 2)
        {
            Expose analogOutput(new SensorObject("analogOutput"));
            analogOutput->setMultiple(true);
            analogOutput->setParent(endpoint.data());
            endpoint->exposes().append(analogOutput);
        }

        m_endpoints.insert(i, endpoint);
    }

    m_options.insert("input", QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});

    if (m_adc)
        m_options.insert("analogInput",  QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});

    if (m_dac)
        m_options.insert("analogOutput", QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 255}, {"control", true}});
}

void Kincony::KC868::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (name == "status" && endpointId && endpointId <= m_channels)
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
    else if (name == "analogOutput" && endpointId && endpointId <= m_channels)
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, endpointId - 1, static_cast <quint16> (data.toInt() & 0xFF)));
}

void Kincony::KC868::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Kincony::KC868::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, m_channels);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, m_channels);

        case 2:

            if (!m_adc)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0000, 4);

        case 3:

            if (!m_dac)
            {
                m_sequence++;
                return pollRequest();
            }

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Kincony::KC868::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[128];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
            {
                quint8 index = m_channels % 8 ? i : m_channels - 8 * (i / 8 + 1) + i % 8;
                m_endpoints.value(index + 1)->buffer().insert("status", data[i] ? "on" : "off");
                m_output[index] = data[i];
            }

            break;
        }

        case 1:
        {
            quint16 data[128];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
            {
                quint8 index = m_channels % 8 ? i : m_channels - 8 * (i / 8 + 1) + i % 8;
                m_endpoints.value(index + 1)->buffer().insert("input", data[i] ? false : true);
            }

            break;
        }

        case 2:
        {
            quint16 data[4];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.value(i + 1)->buffer().insert("analogInput", data[i]);

            break;
        }

        case 3:
        {
            quint16 data;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(1)->buffer().insert("analogOutput", data >> 8);
            m_endpoints.value(2)->buffer().insert("analogOutput", data & 0x00FF);
            break;
        }
    }

    m_sequence++;
}
