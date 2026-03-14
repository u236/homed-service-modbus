#include "expose.h"
#include "wb-relay.h"

void WirenBoard::WBMr::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    switch (m_model)
    {
        case Model::wbMrwm2:
            m_type = "wbMrwm2";
            m_description = "Wiren Board WB-MRWM2 Relay Controller";
            m_channels = 2;
            m_inputs = 2;
            break;

        case Model::wbMrm2:
            m_type = "wbMrm2";
            m_description = "Wiren Board WB-MRM2-mini Relay Controller";
            m_channels = 2;
            m_inputs = 2;
            break;

        case Model::wbMr3:
            m_type = "wbMr3";
            m_description = "Wiren Board WB-MR3LV/MRWL3 Relay Controller";
            m_channels = 3;
            m_inputs = 8;
            break;

        case Model::wbMr6:
            m_type = "wbMr6";
            m_description = "Wiren Board WB-MR6C/MR6-LV Relay Controller";
            m_channels = 6;
            m_inputs = 8;
            break;

        case Model::wbMr6p:
            m_type = "wbMr6p";
            m_description = "Wiren Board WB-MR6CU/MRPS6 Relay Controller";
            m_channels = 6;
            m_inputs = 0;
            break;
    }

    for (quint8 i = m_inputs == 8 ? 0 : 1; i <= m_channels; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose output(new SwitchObject);

            output->setMultiple(true);
            output->setParent(endpoint.data());
            endpoint->exposes().append(output);

            if (m_inputs)
            {
                Expose input(new BinaryObject("input")), action(new SensorObject("action"));

                input->setMultiple(true);
                input->setParent(endpoint.data());
                endpoint->exposes().append(input);

                action->setMultiple(true);
                action->setParent(endpoint.data());
                endpoint->exposes().append(action);
            }

            if (m_model == Model::wbMrwm2)
            {
                Expose voltage(new SensorObject("voltage")), power(new SensorObject("power")), energy(new SensorObject("energy")), alarm(new BinaryObject("alarm")), voltageProtection(new ToggleObject("voltageProtection")), voltageLow(new NumberObject("voltageLow")), voltageHigh(new NumberObject("voltageHigh"));

                voltage->setMultiple(true);
                voltage->setParent(endpoint.data());
                endpoint->exposes().append(voltage);

                power->setMultiple(true);
                power->setParent(endpoint.data());
                endpoint->exposes().append(power);

                energy->setMultiple(true);
                energy->setParent(endpoint.data());
                endpoint->exposes().append(energy);

                alarm->setMultiple(true);
                alarm->setParent(endpoint.data());
                endpoint->exposes().append(alarm);

                voltageProtection->setMultiple(true);
                voltageProtection->setParent(endpoint.data());
                endpoint->exposes().append(voltageProtection);

                voltageLow->setMultiple(true);
                voltageLow->setParent(endpoint.data());
                endpoint->exposes().append(voltageLow);

                voltageHigh->setMultiple(true);
                voltageHigh->setParent(endpoint.data());
                endpoint->exposes().append(voltageHigh);
            }
        }
        else
        {
            Expose input(new BinaryObject("input_0")), action(new SensorObject("action_0"));

            input->setParent(endpoint.data());
            endpoint->exposes().append(input);

            action->setParent(endpoint.data());
            endpoint->exposes().append(action);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("input",             QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("action",            QMap <QString, QVariant> {{"type", "sensor"}, {"enum", QList <QVariant> {"singleClick", "doubleClick"}}, {"icon", "mdi:gesture-double-tap"}});
    m_options.insert("voltageProtection", QMap <QString, QVariant> {{"type", "toggle"}, {"icon", "mdi:sine-wave"}});
    m_options.insert("voltageLow",        QMap <QString, QVariant> {{"type", "number"}, {"min", 120}, {"max", 220}, {"unit", "V"}, {"icon", "mdi:sine-wave"}});
    m_options.insert("voltageHigh",       QMap <QString, QVariant> {{"type", "number"}, {"min", 230}, {"max", 277}, {"unit", "V"}, {"icon", "mdi:sine-wave"}});
}

void WirenBoard::WBMr::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"status", "voltageProtection", "voltageLow", "voltageHigh"};
    int index = actions.indexOf(name);

    if (!endpointId || endpointId > m_channels)
        return;

    if (!index)
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
        return;
    }

    if (m_model != Model::wbMrwm2)
        return;

    switch (index)
    {
        case 1: // voltageProtection
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x06A0 + endpointId - 1, data.toBool() ? 0x0001 : 0x0000));
            break;

        case 2: // voltageLow
        case 3: // voltageHigh
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, (index == 2 ? 0x06A8 : 0x06B0) + endpointId - 1, static_cast <quint16> (data.toInt() * 100)));
            break;

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBMr::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll && m_model == Model::wbMrwm2 ? 0 : 3;
    m_polling = true;
}

QByteArray WirenBoard::WBMr::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 2:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x06A0 + m_sequence * 8, 2);

        case 3:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, m_channels);

        case 4:

            if (!m_inputs)
                break;

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, m_inputs);

        case 5 ... 6:

            if (!m_inputs)
                break;

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 5 ? 0x01D0 : 0x01F0, m_inputs);

        case 7:

            if (m_model != Model::wbMrwm2)
                break;

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0050, 2);

        case 8 ... 9:

            if (m_model != Model::wbMrwm2)
                break;

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 8 ? 0x0038 : 0x0040, 2);

        case 10:

            if (m_model != Model::wbMrwm2)
                break;

            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0048, 4);
    }

    updateEndpoints();

    for (quint8 i = m_inputs == 8 ? 0 : 1; i <= m_channels; i++)
        m_endpoints.value(i)->buffer().remove(i ? "action" : "action_0");

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMr::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 2:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                if (!m_sequence)
                    m_endpoints.value(i + 1)->buffer().insert("voltageProtection", data[i] ? true : false);
                else
                    m_endpoints.value(i + 1)->buffer().insert(m_sequence == 1 ? "voltageLow" : "voltageHigh", data[i] / 100.0);
            }

            break;
        }

        case 3:
        {
            quint16 data[6];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 4:
        {
            quint16 data[8];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            if (m_inputs == 8)
                m_endpoints.value(0)->buffer().insert("input_0", data[7] ? true : false);

            break;
        }

        case 5 ... 6:
        {
            quint16 data[8], *counter = m_sequence == 5 ? m_singleClick : m_doubleClick;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
            {
                if (counter[i] == data[i])
                    continue;

                if (!m_fullPoll)
                    m_endpoints.value(i + 1)->buffer().insert("action", m_sequence == 5 ? "singleClick" : "doubleClick");

                counter[i] = data[i];
            }

            if (m_inputs == 8 && counter[7] != data[7])
            {
                if (!m_fullPoll)
                    m_endpoints.value(0)->buffer().insert("action_0", m_sequence == 5 ? "singleClick" : "doubleClick");

                counter[7] = data[7];
            }

            break;
        }

        case 7:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("alarm", data[i] ? false : true);

            break;
        }

        case 8 ... 9:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert(m_sequence == 8 ? "voltage" : "power", data[i] / (m_sequence == 8 ? 100.0 : 10.0));

            break;
        }

        case 10:
        {
            quint16 data[4];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("energy", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) / 1000);

            break;
        }
    }

    m_sequence++;
}
