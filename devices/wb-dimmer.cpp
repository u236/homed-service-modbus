#include <math.h>
#include "color.h"
#include "expose.h"
#include "wb-dimmer.h"

void WirenBoard::WBMdm::init(const Device &device, const QMap <QString, QVariant> &)
{
    m_type = "wbMdm";
    m_description = "Wiren Board WB-MDM3 Mosfet Dimmer";
    m_modes = {"log", "linear", "switch"};

    for (quint8 i = 1; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));
        Expose light(new LightObject), dimmerMode(new SelectObject("dimmerMode")), dimmerFront(new SelectObject("dimmerFront"));

        light->setMultiple(true);
        light->setParent(endpoint.data());
        endpoint->exposes().append(light);

        dimmerMode->setMultiple(true);
        dimmerMode->setParent(endpoint.data());
        endpoint->exposes().append(dimmerMode);

        dimmerFront->setMultiple(true);
        dimmerFront->setParent(endpoint.data());
        endpoint->exposes().append(dimmerFront);

        m_endpoints.insert(i, endpoint);
    }

    for (quint8 i = 11; i <= 16; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));
        Expose input(new BinaryObject("input")), action(new SensorObject("action"));

        input->setMultiple(true);
        input->setParent(endpoint.data());
        endpoint->exposes().append(input);

        action->setMultiple(true);
        action->setParent(endpoint.data());
        endpoint->exposes().append(action);

        m_endpoints.insert(i, endpoint);
    }

    m_options.insert("light",        QList <QVariant> {"level"});

    m_options.insert("endpointName", QMap <QString, QVariant> {{"1", "CH1"}, {"2", "CH2"}, {"3", "O3"}, {"11", "IN1"}, {"12", "IN2"}, {"13", "IN3"}, {"14", "IN4"}, {"15", "IN5"}, {"16", "IN6"}});
    m_options.insert("dimmerMode",   QMap <QString, QVariant> {{"type", "select"}, {"enum", QVariant(m_modes)}, {"icon", "mdi:cog"}});
    m_options.insert("dimmerFront",  QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"leading", "trailing"}}, {"icon", "mdi:cog"}});
    m_options.insert("input",        QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("action",       QMap <QString, QVariant> {{"type", "sensor"}, {"enum", QList <QVariant> {"singleClick", "doubleClick"}}, {"icon", "mdi:gesture-double-tap"}});

}

void WirenBoard::WBMdm::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"status", "level", "dimmerMode", "dimmerFront"};

    if (!endpointId || endpointId > 3)
        return;

    switch (actions.indexOf(name))
    {
        case 0: // status
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

        case 1: // level
        {
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, endpointId - 1, static_cast <quint16> (round(data.toInt() / 2.55))));
            return;
        }

        case 2: // dimmerMode
        {
            int value = m_modes.indexOf(data.toString());

            if (value < 0)
                return;

            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0032 + endpointId - 1, static_cast <quint16> (value)));
            break;
        }

        case 3: // dimmerFront
        {
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x003C + endpointId - 1, data.toString() == "trailing" ? 0x0001 : 0x0000));
            break;
        }

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBMdm::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;
}

QByteArray WirenBoard::WBMdm::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0032, 3);

        case 1:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x003C, 3);

        case 2:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 6);

        case 3:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, 3);

        case 4:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0000, 3);

        case 5 ... 6:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 5 ? 0x01D0 : 0x01F0, 6);
    }

    updateEndpoints();

    for (quint8 i = 0; i < 6; i++)
        m_endpoints.value(i + 11)->buffer().remove("action");

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMdm::parseReply(const QByteArray &reply)
{
    quint16 data[3];

    switch (m_sequence)
    {
        case 0:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                QString mode = m_modes.value(data[i]);

                if (mode.isEmpty())
                    continue;

                m_endpoints.value(i + 1)->buffer().insert("dimmerMode", mode);
            }

            break;
        }

        case 1:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("dimmerFront", data[i] ? "trailing" : "leading");

            break;
        }

        case 2:
        {
            quint16 data[6];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 6; i++)
                m_endpoints.value(i + 11)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 3:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 4:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("level", round(data[i] * 2.55));

            break;
        }

        case 5 ... 6:
        {
            quint16 data[6], *counter = m_sequence == 5 ? m_singleClick : m_doubleClick;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 6; i++)
            {
                if (counter[i] == data[i])
                    continue;

                if (!m_fullPoll)
                    m_endpoints.value(i + 11)->buffer().insert("action", m_sequence == 5 ? "singleClick" : "doubleClick");

                counter[i] = data[i];
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBLed::init(const Device &device, const QMap <QString, QVariant> &)
{
    switch (m_model)
    {
        case Model::wbLed0:
            m_type = "wbLed0";
            m_description = "Wiren Board WB-LED Dimmer (W1, W2, W3, W4)";
            m_mode = 0x0000;
            m_list = {1, 2, 3, 4};
            break;

        case Model::wbLed1:
            m_type = "wbLed1";
            m_description = "Wiren Board WB-LED Dimmer (W1+W2, W3, W4)";
            m_mode = 0x0001;
            m_list = {3, 4, 5};
            break;

        case Model::wbLed2:
            m_type = "wbLed2";
            m_description = "Wiren Board WB-LED Dimmer (CCT1, W3, W4)";
            m_mode = 0x0002;
            m_list = {3, 4, 8};
            break;

        case Model::wbLed16:
            m_type = "wbLed16";
            m_description = "Wiren Board WB-LED Dimmer (W1, W2, W3+W4)";
            m_mode = 0x0010;
            m_list = {1, 2, 6};
            break;

        case Model::wbLed17:
            m_type = "wbLed17";
            m_description = "Wiren Board WB-LED Dimmer (W1+W2, W3+W4)";
            m_mode = 0x0011;
            m_list = {5, 6};
            break;

        case Model::wbLed18:
            m_type = "wbLed18";
            m_description = "Wiren Board WB-LED Dimmer (CCT1, W3+W4)";
            m_mode = 0x0012;
            m_list = {6, 8};
            break;

        case Model::wbLed32:
            m_type = "wbLed32";
            m_description = "Wiren Board WB-LED Dimmer (W1, W2, CCT2)";
            m_mode = 0x0020;
            m_list = {1, 2, 9};
            break;

        case Model::wbLed33:
            m_type = "wbLed33";
            m_description = "Wiren Board WB-LED Dimmer (W1+W2, CCT2)";
            m_mode = 0x0021;
            m_list = {5, 9};
            break;

        case Model::wbLed34:
            m_type = "wbLed34";
            m_description = "Wiren Board WB-LED Dimmer (CCT1, CCT2)";
            m_mode = 0x0022;
            m_list = {8, 9};
            break;

        case Model::wbLed256:
            m_type = "wbLed256";
            m_description = "Wiren Board WB-LED Dimmer (RGB, W4)";
            m_mode = 0x0100;
            m_list = {4, 10};
            break;

        case Model::wbLed512:
            m_type = "wbLed512";
            m_description = "Wiren Board WB-LED Dimmer (W1+W2+W3+W4)";
            m_mode = 0x0200;
            m_list = {7};
            break;
    }

    for (quint8 i = 0; i <= 14; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            if (i <= 10)
            {
                Expose light(new LightObject);

                if (!m_list.contains(i))
                    continue;

                light->setMultiple(true);
                light->setParent(endpoint.data());
                endpoint->exposes().append(light);

                m_options.insert(QString("light_%1").arg(i), i < 8 ? QList <QVariant> {"level"} : i < 10 ? QList <QVariant> {"level", "colorTemperature"} : QList <QVariant> {"level", "color"});
            }
            else
            {
                Expose input(new BinaryObject("input")), action(new SensorObject("action"));

                input->setMultiple(true);
                input->setParent(endpoint.data());
                endpoint->exposes().append(input);

                action->setMultiple(true);
                action->setParent(endpoint.data());
                endpoint->exposes().append(action);
            }
        }
        else
        {
            Expose frequencyDivider(new NumberObject("frequencyDivider"));
            frequencyDivider->setParent(endpoint.data());
            endpoint->exposes().append(frequencyDivider);
        }

        m_endpoints.insert(i, endpoint);
    }

    m_options.insert("endpointName",     QMap <QString, QVariant> {{"1", "W1"}, {"2", "W2"}, {"3", "W3"}, {"4", "W4"}, {"5", "W1+W2"}, {"6", "W3+W4"}, {"7", "W1+W2+W3+W4"}, {"8", "CCT1"}, {"9", "CCT2"}, {"10", "RGB"}, {"11", "IN1"}, {"12", "IN2"}, {"13", "IN3"}, {"14", "IN4"}});
    m_options.insert("colorTemperature", QMap <QString, QVariant> {{"min", 150}, {"max", 450}, {"step", 3}});
    m_options.insert("input",            QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("action",           QMap <QString, QVariant> {{"type", "sensor"}, {"enum", QList <QVariant> {"singleClick", "doubleClick"}}, {"icon", "mdi:gesture-double-tap"}});
    m_options.insert("frequencyDivider", QMap <QString, QVariant> {{"type", "number"}, {"min", 1}, {"max", 240}, {"icon", "mdi:square-wave"}});
}

void WirenBoard::WBLed::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"status", "level", "colorTemperature", "color"};

    if (!endpointId && name == "frequencyDivider")
    {
        m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0009, static_cast <quint16> (data.toInt())));
        m_fullPoll = true;
        return;
    }

    if (!m_list.contains(endpointId))
        return;

    switch (actions.indexOf(name))
    {
        case 0: // status
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
            break;
        }

        case 1: // level
        {
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x07D0 + (endpointId <= 9 ? endpointId <= 7 ? endpointId - 1 : (endpointId - 8) * 2 + 8 : 16), static_cast <quint16> (round(data.toInt() / 2.55))));
            break;
        }

        case 2: // colorTemperature
        {
            if (endpointId != 8 && endpointId != 9)
                break;

            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x07D7 + (endpointId - 8) * 2, static_cast <quint16> ((450 - data.toInt()) / 3)));
            break;
        }

        case 3: // color
        {
            QList <QVariant> list = data.toList();
            Color color(list.value(0).toDouble() / 0xFF, list.value(1).toDouble() / 0xFF, list.value(2).toDouble() / 0xFF);
            double h, s;
            quint16 value[2];

            color.toHS(&h, &s);
            value[0] = round(h * 360);
            value[1] = round(s * 100);

            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteMultipleRegisters, 0x07DE, 2, value));
            break;
        }
    }
}

void WirenBoard::WBLed::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;
}

QByteArray WirenBoard::WBLed::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0FA0, m_mode);

        case 1:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0009, 1);

        case 2:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 4);

        case 3:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, 10);

        case 4:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x07D0, 17);

        case 5 ... 6:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 5 ? 0x01D0 : 0x01F0, 4);
    }

    updateEndpoints();

    for (quint8 i = 0; i < 4; i++)
        m_endpoints.value(i + 11)->buffer().remove("action");

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBLed::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::WriteSingleRegister, reply) != Modbus::ReplyStatus::Ok)
                break;

            break;
        }

        case 1:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("frequencyDivider", value);
            break;
        }

        case 2:
        {
            quint16 data[4];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.value(i + 11)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 3:
        {
            quint16 data[16];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 10; i++)
            {
                if (!m_list.contains(i + 1))
                    continue;

                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");
            }

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 4:
        {
            quint16 data[17];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 10; i++)
            {
                auto it = m_endpoints.find(i + 1);

                if (!m_list.contains(i + 1))
                    continue;

                switch (i)
                {
                    case 0 ... 6:
                    {
                        it.value()->buffer().insert("level", round(data[i] * 2.55));
                        break;
                    }

                    case 7 ... 8:
                    {
                        it.value()->buffer().insert("level", round(data[(i - 7) * 2 + 8] * 2.55));
                        it.value()->buffer().insert("colorTemperature", 450 - data[7 + (i - 7) * 2] * 3);
                        break;
                    }

                    case 9:
                    {
                        Color color(Color::fromHS(data[14] / 360.0, data[15] / 100.0));
                        it.value()->buffer().insert("level", data[16] * 2.55);
                        it.value()->buffer().insert("color", QList <QVariant> {static_cast <quint8> (color.r() * 255), static_cast <quint8> (color.g() * 255), static_cast <quint8> (color.b() * 255)});
                        break;
                    }
                }
            }

            break;
        }

        case 5 ... 6:
        {
            quint16 data[4], *counter = m_sequence == 5 ? m_singleClick : m_doubleClick;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                if (counter[i] == data[i])
                    continue;

                if (!m_fullPoll)
                    m_endpoints.value(i + 11)->buffer().insert("action", m_sequence == 5 ? "singleClick" : "doubleClick");

                counter[i] = data[i];
            }

            break;
        }
    }

    m_sequence++;
}
