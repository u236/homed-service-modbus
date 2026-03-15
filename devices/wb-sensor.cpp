#include <math.h>
#include "expose.h"
#include "wb-sensor.h"

void WirenBoard::WBM1w2::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbM1w2";
    m_description = "Wiren Board WB-M1W2 Temperature Sensor";

    for (quint8 i = 1; i <= 2; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose temperature(new SensorObject("temperature")), input(new BinaryObject("input")), action(new SensorObject("action")), operationMode(new SelectObject("operationMode"));

            temperature->setMultiple(true);
            temperature->setParent(endpoint.data());
            endpoint->exposes().append(temperature);

            input->setMultiple(true);
            input->setParent(endpoint.data());
            endpoint->exposes().append(input);

            action->setMultiple(true);
            action->setParent(endpoint.data());
            endpoint->exposes().append(action);

            operationMode->setMultiple(true);
            operationMode->setParent(endpoint.data());
            endpoint->exposes().append(operationMode);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("input",         QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("action",        QMap <QString, QVariant> {{"type", "sensor"}, {"enum", QList <QVariant> {"singleClick", "doubleClick"}}, {"icon", "mdi:gesture-double-tap"}});
    m_options.insert("operationMode", QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"temperature", "input"}}, {"icon", "mdi:cog"}});
}

void WirenBoard::WBM1w2::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (!endpointId || endpointId > 2 || name != "operationMode")
        return;

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0113 + endpointId - 1, data.toString() == "input" ? 0x0001 : 0x0000));
    m_fullPoll = true;
}

void WirenBoard::WBM1w2::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBM1w2::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0113, 2);

        case 1:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, 2);

        case 2:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0007, 2);

        case 3 ... 4:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 3 ? 0x01D0 : 0x01F0, 2);
    }

    updateEndpoints();

    for (quint8 i = 0; i < 2; i++)
        m_endpoints.value(i + 1)->buffer().remove("action");

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBM1w2::parseReply(const QByteArray &reply)
{
    quint16 data[2];

    switch (m_sequence)
    {
        case 0:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("operationMode", data[i] ? "input" : "temperature");

            break;
        }

        case 1:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 2:
        {
            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                double temperature = NAN;

                if (data[i] != 0x7FFF)
                    temperature = static_cast <qint16> (data[i]) / 16.0;

                m_endpoints.value(i + 1)->buffer().insert("temperature", temperature);
            }

            break;
        }

        case 3 ... 4:
        {
            quint16 *counter = m_sequence == 3 ? m_singleClick : m_doubleClick;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                if (counter[i] == data[i])
                    continue;

                if (!m_fullPoll)
                    m_endpoints.value(i + 1)->buffer().insert("action", m_sequence == 3 ? "singleClick" : "doubleClick");

                counter[i] = data[i];
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMs::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMs";
    m_description = "Wiren Board WB-MS Modbus Sensor";

    for (quint8 i = 0; i <= 2; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose temperature(new SensorObject("temperature")), input(new BinaryObject("input")), operationMode(new SelectObject("operationMode"));

            temperature->setMultiple(true);
            temperature->setParent(endpoint.data());
            endpoint->exposes().append(temperature);

            input->setMultiple(true);
            input->setParent(endpoint.data());
            endpoint->exposes().append(input);

            operationMode->setMultiple(true);
            operationMode->setParent(endpoint.data());
            endpoint->exposes().append(operationMode);
        }
        else
        {
            Expose temperature(new SensorObject("temperature")), humidity(new SensorObject("humidity")), illuminance(new SensorObject("illuminance")), voc(new SensorObject("voc"));

            temperature->setParent(endpoint.data());
            endpoint->exposes().append(temperature);

            humidity->setParent(endpoint.data());
            endpoint->exposes().append(humidity);

            illuminance->setParent(endpoint.data());
            endpoint->exposes().append(illuminance);

            voc->setParent(endpoint.data());
            endpoint->exposes().append(voc);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("endpointName",  QMap <QString, QVariant> {{"1", "W1"}, {"2", "W2"}});
    m_options.insert("input",         QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("operationMode", QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"temperature", "input"}}, {"icon", "mdi:cog"}});
}

void WirenBoard::WBMs::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (!endpointId || endpointId > 2 || name != "operationMode")
        return;

    m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0113 + endpointId - 1, data.toString() == "input" ? 0x0001 : 0x0000));
    m_fullPoll = true;
}

void WirenBoard::WBMs::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBMs::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0113, 2);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputStatus,      0x0000, 2);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0002, 1);
        case 3: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0004, 4);
        case 4: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x000B, 1);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMs::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("operationMode", data[i] ? "input" : "temperature");

            break;
        }

        case 1:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 2:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("illuminance", value);
            break;
        }

        case 3:
        {
            quint16 data[4];
            double temperature = NAN, humidity = NAN, temperatureW1 = NAN, temperatureW2 = NAN;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            if (data[0] != 0x7FFF)
                temperature = static_cast <qint16> (data[0]) / 100.0;

            if (data[1] != 0xFFFF)
                humidity = data[1] / 100.0;

            if (data[2] != 0x7FFF)
                temperatureW1 = static_cast <qint16> (data[2]) / 16.0;

            if (data[3] != 0x7FFF)
                temperatureW2 = static_cast <qint16> (data[3]) / 16.0;

            m_endpoints.value(0)->buffer().insert("temperature", temperature);
            m_endpoints.value(0)->buffer().insert("humidity", humidity);
            m_endpoints.value(1)->buffer().insert("temperature", temperatureW1);
            m_endpoints.value(2)->buffer().insert("temperature", temperatureW2);

            break;
        }

        case 4:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            if (value != 0xFFFF)
                m_endpoints.value(0)->buffer().insert("voc", value);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMsw::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMsw";
    m_description = "Wiren Board WB-MSW Wall-mounted Sensor";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose output(new SwitchObject);
            output->setMultiple(true);
            output->setParent(endpoint.data());
            endpoint->exposes().append(output);
        }
        else
        {
            Expose blinkInterval(new NumberObject("blinkInterval")), blinkDuration(new NumberObject("blinkDuration")), occupancy(new SensorObject("occupancy")), temperature(new SensorObject("temperature")), humidity(new SensorObject("humidity")), illuminance(new SensorObject("illuminance")), noise(new SensorObject("noise")), co2(new SensorObject("co2")), co2AutoCalibration(new ToggleObject("co2AutoCalibration")), voc(new SensorObject("voc"));

            blinkInterval->setParent(endpoint.data());
            endpoint->exposes().append(blinkInterval);

            blinkDuration->setParent(endpoint.data());
            endpoint->exposes().append(blinkDuration);

            occupancy->setParent(endpoint.data());
            endpoint->exposes().append(occupancy);

            temperature->setParent(endpoint.data());
            endpoint->exposes().append(temperature);

            humidity->setParent(endpoint.data());
            endpoint->exposes().append(humidity);

            illuminance->setParent(endpoint.data());
            endpoint->exposes().append(illuminance);

            noise->setParent(endpoint.data());
            endpoint->exposes().append(noise);

            co2->setParent(endpoint.data());
            endpoint->exposes().append(co2);

            co2AutoCalibration->setParent(endpoint.data());
            endpoint->exposes().append(co2AutoCalibration);

            voc->setParent(endpoint.data());
            endpoint->exposes().append(voc);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("endpointName",  QMap <QString, QVariant> {{"1", "Buzzer"}, {"2", "Red"}, {"3", "Green"}});
    m_options.insert("blinkInterval", QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 10}, {"unit", "sec"}, {"icon", "mdi:led-on"}});
    m_options.insert("blinkDuration", QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 50}, {"unit", "ms"}, {"icon", "mdi:led-on"}});
    m_options.insert("noise",         QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "dB"}, {"icon", "mdi:ear-hearing"}});

    m_endpoints.value(0)->buffer().insert("occupancy", false);

    connect(m_timer, &QTimer::timeout, this, &WBMsw::update);
    m_timer->start(1000);
}

void WirenBoard::WBMsw::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"status", "blinkInterval", "blinkDuration", "co2AutoCalibration"};
    int index = actions.indexOf(name);

    switch (index)
    {
        case 0: // status
        {
            QList <QString> list = {"on", "off", "toggle"};
            bool value;

            if (!endpointId || endpointId > 3)
                return;

            switch (list.indexOf(data.toString()))
            {
                case 0:  value = true; break;
                case 1:  value = false; break;
                case 2:  value = m_output[endpointId - 1] ? false : true; break;
                default: return;
            }

            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId == 1 ? 0x0000 : 0x000A + endpointId - 2, value ? 0xFF00 : 0x0000));
            return;
        }

        case 1: // blinkInterval
        case 2: // blinkDuration
        {
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, index == 1 ? 0x0061 : 0x0062, static_cast <quint16> (data.toInt())));
            break;
        }

        case 3: // co2AutoCalibration
        {
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x005F, data.toBool() ? 0x0001 : 0x0000));
            break;
        }

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBMsw::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;
}

QByteArray WirenBoard::WBMsw::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x005F, 1);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0061, 2);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x0000, 1);
        case 3: return m_modbus->makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x000A, 2);
        case 4: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0003, 3);
        case 5: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0008, 4);
        case 6: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x011B, 1);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMsw::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("co2AutoCalibration", value ? true : false);
            break;
        }

        case 1:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("blinkInterval", data[0]);
            m_endpoints.value(0)->buffer().insert("blinkDuration", data[1]);
            break;
        }

        case 2:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(1)->buffer().insert("status", value ? "on" : "off");
            m_output[0] = value ? true : false;
            break;
        }

        case 3:
        {
            quint16 data[3];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                m_endpoints.value(i + 2)->buffer().insert("status", data[i] ? "on" : "off");
                m_output[i + 1] = data[i] ? true : false;
            }

            break;
        }

        case 4:
        {
            quint16 data[3];
            double noise = NAN, temperature = NAN, humidity = NAN;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            if (data[0] != 0xFFFF)
                noise = data[0] / 100.0;

            if (data[1] != 0x7FFF)
                temperature = static_cast <qint16> (data[1]) / 100.0;

            if (data[2] != 0xFFFF)
                humidity = data[2] / 100.0;

            m_endpoints.value(0)->buffer().insert("noise", noise);
            m_endpoints.value(0)->buffer().insert("temperature", temperature);
            m_endpoints.value(0)->buffer().insert("humidity", humidity);
            break;
        }

        case 5:
        {
            quint16 data[4];
            double co2 = NAN, illuminance = NAN, voc = NAN;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            if (data[0] != 0xFFFF)
                co2 = data[0];

            if (data[1] != 0xFFFF || data[2] != 0xFFFF)
                illuminance = Modbus::toUInt32BE(data + 1) / 100.0;

            if (data[3] != 0xFFFF)
                voc = data[3];

            m_endpoints.value(0)->buffer().insert("co2", co2);
            m_endpoints.value(0)->buffer().insert("illuminance", illuminance);
            m_endpoints.value(0)->buffer().insert("voc", voc);
            break;
        }

        case 6:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok || value < WBMSW_OCCUPANCY_THRESHOLD)
                break;

            m_endpoints.value(0)->buffer().insert("occupancy", true);
            m_time = QDateTime::currentSecsSinceEpoch();
            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMsw::update(void)
{
    if (!m_time || m_time + WBMSW_OCCUPANCY_TIMEOUT > QDateTime::currentSecsSinceEpoch())
        return;

    m_endpoints.value(0)->buffer().insert("occupancy", false);
    updateEndpoints();
}

void WirenBoard::WBMai6::init(const Device &device, const QMap <QString, QVariant> &)
{
    QList <QVariant> pSensors, nSensors;

    m_type = "wbMai6";
    m_description = "Wiren Board WB-MAI6 Analog Input Controller";

    m_types =
    {
        "Disabled",
        "0-30 V Unipolar",
        "-2 to 2 V Differential",
        "2-wire Resistance",
        "3-Wire Resistance",
        "0 tp 20 mA Current",
        "Type K Thermocouple",
        "NTC 10K Thermistor",
        "2-wire RTD Pt 50",
        "2-wire RTD Pt 100",
        "2-wire RTD Pt 500",
        "2-wire RTD Pt 1000",
        "2-wire RTD 50P",
        "2-wire RTD 100P",
        "2-wire RTD 500P",
        "2-wire RTD 1000P",
        "2-wire RTD 50M",
        "2-wire RTD 100M",
        "2-wire RTD 500M",
        "2-wire RTD 1000M",
        "2-wire RTD Ni 100",
        "2-wire RTD Ni 500",
        "2-wire RTD Ni 1000",
        "3-wire RTD Pt 50",
        "3-wire RTD Pt 100",
        "3-wire RTD Pt 500",
        "3-wire RTD Pt 1000",
        "3-wire RTD 50P",
        "3-wire RTD 100P",
        "3-wire RTD 500P",
        "3-wire RTD 1000P",
        "3-wire RTD 50M",
        "3-wire RTD 100M",
        "3-wire RTD 500M",
        "3-wire RTD 1000M",
        "3-wire RTD Ni 100",
        "3-wire RTD Ni 500",
        "3-wire RTD Ni 1000",
        "0 to 5 mA Sensor",
        "0 to 20 mA Sensor",
        "4 to 20 mA Sensor",
        "0 to 1 V Sensor",
        "0 to 10 V Sensor",
        "-50 to 50 mV Sensor",
        "Dry Contact Sensor",
        "Hall Effect Sensor"
    };

    m_settings =
    {
        {0x0000, 0,    0.0, true},  // Disabled
        {0x0001, 1e-6, 0.0, true},  // 0-30 V Unipolar
        {0x0101, 1e-6, 0.0, false}, // -2 to 2 V Differential
        {0x0002, 1e-5, 0.0, true},  // -wire Resistance
        {0x0102, 1e-5, 0.0, false}, // 3-Wire Resistance
        {0x0003, 1e-9, 0.0, true},  // 0 tp 20 mA Current
        {0x1000, 1e-5, 0.1, false}, // Type K Thermocouple
        {0x1701, 1e-5, 0.1, true},  // NTC 10K Thermistor
        {0x1100, 1e-5, 0.1, true},  // 2-wire RTD Pt 50
        {0x1101, 1e-5, 0.1, true},  // 2-wire RTD Pt 100
        {0x1102, 1e-5, 0.1, true},  // 2-wire RTD Pt 500
        {0x1103, 1e-5, 0.1, true},  // 2-wire RTD Pt 1000
        {0x1110, 1e-5, 0.1, true},  // 2-wire RTD 50P
        {0x1111, 1e-5, 0.1, true},  // 2-wire RTD 100P
        {0x1112, 1e-5, 0.1, true},  // 2-wire RTD 500P
        {0x1113, 1e-5, 0.1, true},  // 2-wire RTD 1000P
        {0x1120, 1e-5, 0.1, true},  // 2-wire RTD 50M
        {0x1121, 1e-5, 0.1, true},  // 2-wire RTD 100M
        {0x1122, 1e-5, 0.1, true},  // 2-wire RTD 500M
        {0x1123, 1e-5, 0.1, true},  // 2-wire RTD 1000M
        {0x1130, 1e-5, 0.1, true},  // 2-wire RTD Ni 100
        {0x1131, 1e-5, 0.1, true},  // 2-wire RTD Ni 500
        {0x1132, 1e-5, 0.1, true},  // 2-wire RTD Ni 1000
        {0x1200, 1e-5, 0.1, false}, // 3-wire RTD Pt 50
        {0x1201, 1e-5, 0.1, false}, // 3-wire RTD Pt 100
        {0x1202, 1e-5, 0.1, false}, // 3-wire RTD Pt 500
        {0x1203, 1e-5, 0.1, false}, // 3-wire RTD Pt 1000
        {0x1210, 1e-5, 0.1, false}, // 3-wire RTD 50P
        {0x1211, 1e-5, 0.1, false}, // 3-wire RTD 100P
        {0x1212, 1e-5, 0.1, false}, // 3-wire RTD 500P
        {0x1213, 1e-5, 0.1, false}, // 3-wire RTD 1000P
        {0x1220, 1e-5, 0.1, false}, // 3-wire RTD 50M
        {0x1221, 1e-5, 0.1, false}, // 3-wire RTD 100M
        {0x1222, 1e-5, 0.1, false}, // 3-wire RTD 500M
        {0x1223, 1e-5, 0.1, false}, // 3-wire RTD 1000M
        {0x1230, 1e-5, 0.1, false}, // 3-wire RTD Ni 100
        {0x1231, 1e-5, 0.1, false}, // 3-wire RTD Ni 500
        {0x1232, 1e-5, 0.1, false}, // 3-wire RTD Ni 1000
        {0x1300, 1e-9, 1.0, true},  // 0 to 5 mA Sensor
        {0x1301, 1e-9, 1.0, true},  // 0 to 20 mA Sensor
        {0x1302, 1e-9, 1.0, true},  // 4 to 20 mA Sensor
        {0x1400, 1e-6, 1.0, true},  // 0 to 1 V Sensor
        {0x1401, 1e-6, 1.0, true},  // 0 to 10 V Sensor
        {0x1500, 1e-6, 1.0, false}, // -50 to 50 mV Sensor
        {0x1600, 1e-5, 1.0, true},  // Dry Contact Sensor
        {0x1800, 1e-6, 1.0, true}   // Hall Effect Sensor
    };

    for (int i = 0; i < m_types.count(); i++)
    {
        pSensors.append(m_types.at(i));

        if (!m_settings.at(i).nChannel)
            continue;

        nSensors.append(m_types.at(i));
    }

    for (quint8 i = 1; i <= 6; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose pSensorType(new SelectObject("pSensorType")), pValueMin(new NumberObject("pValueMin")), pValueMax(new NumberObject("pValueMax")), pInput(new SensorObject("pInput")), pValue(new SensorObject("pValue")), nSensorType(new SelectObject("nSensorType")), nValueMin(new NumberObject("nValueMin")), nValueMax(new NumberObject("nValueMax")), nInput(new SensorObject("nInput")), nValue(new SensorObject("nValue"));

            pSensorType->setMultiple(true);
            pSensorType->setParent(endpoint.data());
            endpoint->exposes().append(pSensorType);

            pValueMin->setMultiple(true);
            pValueMin->setParent(endpoint.data());
            endpoint->exposes().append(pValueMin);

            pValueMax->setMultiple(true);
            pValueMax->setParent(endpoint.data());
            endpoint->exposes().append(pValueMax);

            pInput->setMultiple(true);
            pInput->setParent(endpoint.data());
            endpoint->exposes().append(pInput);

            pValue->setMultiple(true);
            pValue->setParent(endpoint.data());
            endpoint->exposes().append(pValue);

            nSensorType->setMultiple(true);
            nSensorType->setParent(endpoint.data());
            endpoint->exposes().append(nSensorType);

            nValueMin->setMultiple(true);
            nValueMin->setParent(endpoint.data());
            endpoint->exposes().append(nValueMin);

            nValueMax->setMultiple(true);
            nValueMax->setParent(endpoint.data());
            endpoint->exposes().append(nValueMax);

            nInput->setMultiple(true);
            nInput->setParent(endpoint.data());
            endpoint->exposes().append(nInput);

            nValue->setMultiple(true);
            nValue->setParent(endpoint.data());
            endpoint->exposes().append(nValue);
        }

        m_endpoints.insert(i, endpoint);
    }

    m_options.insert("pSensorType", QMap <QString, QVariant> {{"type", "select"}, {"enum", pSensors}, {"icon", "mdi:cog"}});
    m_options.insert("pValueMin",   QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:cog"}});
    m_options.insert("pValueMax",   QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:cog"}});
    m_options.insert("pInput",      QMap <QString, QVariant> {{"type", "sensor"}, {"round", 3}, {"icon", "mdi:import"}});
    m_options.insert("pValue",      QMap <QString, QVariant> {{"type", "sensor"}, {"round", 3}, {"icon", "mdi:import"}});

    m_options.insert("nSensorType", QMap <QString, QVariant> {{"type", "select"}, {"enum", nSensors}, {"icon", "mdi:cog"}});
    m_options.insert("nValueMin",   QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:cog"}});
    m_options.insert("nValueMax",   QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:cog"}});
    m_options.insert("nInput",      QMap <QString, QVariant> {{"type", "sensor"}, {"round", 3}, {"icon", "mdi:import"}});
    m_options.insert("nValue",      QMap <QString, QVariant> {{"type", "sensor"}, {"round", 3}, {"icon", "mdi:import"}});
}

void WirenBoard::WBMai6::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> list = {"pSensorType", "nSensorType", "pValueMin", "nValueMin", "pValueMax", "nValueMax"};
    int index = list.indexOf(name);

    if (!endpointId || endpointId > 6)
        return;

    switch (index)
    {
        case 0: // pSensorType
        case 1: // nSensorType

            for (int i = 0; i < m_types.count(); i++)
            {
                if (m_types.at(i) != data.toString() || (index && !m_settings.at(i).nChannel))
                    continue;

                m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, (index ? 0x0401 : 0x0400) + endpointId * 0x1000, m_settings.at(i).type));
                break;
            }

            break;

        case 2: // pValueMin
        case 3: // nValueMin
        case 4: // pValueMax
        case 5: // nValueMax
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister,  0x0408 + endpointId * 0x1000 + index - 2, static_cast <quint16> (data.toInt())));
            break;

        default:
            break;
    }

    m_fullPoll = true;
}

void WirenBoard::WBMai6::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 12;
    m_polling = true;
}

QByteArray WirenBoard::WBMai6::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 5:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0400 + (m_sequence + 1) * 0x1000, 2);

        case 6 ... 11:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0408 + (m_sequence - 5) * 0x1000, 4);

        case 12 ... 17:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0500 + (m_sequence - 11) * 0x1000, 6);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBMai6::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 5:
        {
            quint16 data[2];
            auto it = m_endpoints.find(m_sequence + 1);

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < m_settings.count(); i++)
            {
                if (m_settings.at(i).type == data[0])
                {
                    it.value()->buffer().insert("pSensorType", m_types.at(i));
                    m_pChannel[m_sequence] = m_settings.at(i);
                }

                if (m_settings.at(i).type == data[1])
                {
                    it.value()->buffer().insert("nSensorType", m_types.at(i));
                    m_nChannel[m_sequence] = m_settings.at(i);
                }
            }

            break;
        }

        case 6 ... 11:
        {
            quint16 data[4];
            auto it = m_endpoints.find(m_sequence - 5);

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            it.value()->buffer().insert("pValueMin", data[0]);
            it.value()->buffer().insert("nValueMin", data[1]);
            it.value()->buffer().insert("pValueMax", data[2]);
            it.value()->buffer().insert("nValueMax", data[3]);
            break;
        }

        case 12 ... 17:
        {
            quint16 data[6];
            double pInput = NAN, nInput = NAN, pValue = NAN, nValue = NAN;
            auto it = m_endpoints.find(m_sequence - 11);

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            if (data[0] != 0x7FFF && data[1] != 0xFFFF)
                pInput = static_cast <qint32> (static_cast <qint32> (data[0]) << 16 | static_cast <qint32> (data[1])) * m_pChannel[m_sequence - 12].inputMultiplier;

            if (data[2] != 0x7FFF && data[3] != 0xFFFF)
                nInput = static_cast <qint32> (static_cast <qint32> (data[2]) << 16 | static_cast <qint32> (data[3])) * m_nChannel[m_sequence - 12].inputMultiplier;

            if (data[4] != 0x7FFF)
            {
                double multiplier = m_pChannel[m_sequence - 12].valueMultiplier;
                pValue = multiplier != 0.0 ? static_cast <qint16> (data[4]) * multiplier : pInput;
            }

            if (data[5] != 0x7FFF)
            {
                double multiplier = m_nChannel[m_sequence - 12].valueMultiplier;
                nValue = multiplier != 0.0 ? static_cast <qint16> (data[5]) * multiplier : nInput;
            }

            it.value()->buffer().insert("pInput", pInput);
            it.value()->buffer().insert("nInput", nInput);
            it.value()->buffer().insert("pValue", pValue);
            it.value()->buffer().insert("nValue", nValue);
            break;
        }
    }

    m_sequence++;
}
