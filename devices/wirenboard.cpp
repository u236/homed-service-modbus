#include <math.h>
#include "color.h"
#include "expose.h"
#include "modbus.h"
#include "wirenboard.h"

void WirenBoard::WBM1w2::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbM1w2";
    m_description = "Wiren Board WB-M1W2 Temperature Sensor";

    for (quint8 i = 1; i <= 2; i++)
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
            endpoint->exposes().append(input);;

            operationMode->setMultiple(true);
            operationMode->setParent(endpoint.data());
            endpoint->exposes().append(operationMode);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("input",         QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
    m_options.insert("operationMode", QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"temperature", "input"}}, {"icon", "mdi:cog"}});
}

void WirenBoard::WBM1w2::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    if (!endpointId || endpointId > 2 || name != "operationMode")
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0113 + endpointId - 1, data.toString() == "input" ? 0x0001 : 0x0000));
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
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0113, 2);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus,      0x0000, 2);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0007, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBM1w2::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("operationMode", data[i] ? "input" : "temperature");

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 2:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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
            endpoint->exposes().append(input);;

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

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0113 + endpointId - 1, data.toString() == "input" ? 0x0001 : 0x0000));
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
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0113, 2);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus,      0x0000, 2);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0002, 1);
        case 3: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0004, 4);
        case 4: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x000B, 1);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMs::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("operationMode", data[i] ? "input" : "temperature");

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            break;
        }

        case 2:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("illuminance", value);
            break;
        }

        case 3:
        {
            quint16 data[4];
            double temperature = NAN, humidity = NAN, temperatureW1 = NAN, temperatureW2 = NAN;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
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
            Expose status(new SwitchObject);
            status->setMultiple(true);
            status->setParent(endpoint.data());
            endpoint->exposes().append(status);
        }
        else
        {
            Expose blinkInterval(new NumberObject("blinkInterval")), blinkDuration(new NumberObject("blinkDuration")), temperature(new SensorObject("temperature")), humidity(new SensorObject("humidity")), illuminance(new SensorObject("illuminance")), noise(new SensorObject("noise")), co2(new SensorObject("co2")), co2AutoCalibration(new ToggleObject("co2AutoCalibration")), voc(new SensorObject("voc"));

            blinkInterval->setParent(endpoint.data());
            endpoint->exposes().append(blinkInterval);

            blinkDuration->setParent(endpoint.data());
            endpoint->exposes().append(blinkDuration);

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
                case 2:  value = m_status[endpointId - 1] ? false : true; break;
                default: return;
            }

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId == 1 ? 0x0000 : 0x000A + endpointId - 2, value ? 0xFF00 : 0x0000));
            return;
        }

        case 1: // blinkInterval
        case 2: // blinkDuration
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, index == 1 ? 0x0061 : 0x0062, static_cast <quint16> (data.toInt())));
            break;
        }

        case 3: // co2AutoCalibration
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x005F, data.toBool() ? 0x0001 : 0x0000));
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
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x005F, 1);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0061, 2);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x0000, 1);
        case 3: return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x000A, 2);
        case 4: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0003, 3);
        case 5: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0008, 4);
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

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("co2AutoCalibration", value ? true : false);
            break;
        }

        case 1:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("blinkInterval", data[0]);
            m_endpoints.value(0)->buffer().insert("blinkDuration", data[1]);
            break;
        }

        case 2:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(1)->buffer().insert("status", value ? "on" : "off");
            m_status[0] = value ? true : false;
            break;
        }

        case 3:
        {
            quint16 data[3];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                m_endpoints.value(i + 2)->buffer().insert("status", data[i] ? "on" : "off");
                m_status[i + 1] = data[i] ? true : false;
            }

            break;
        }

        case 4:
        {
            quint16 data[3];
            double noise = NAN, temperature = NAN, humidity = NAN;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            if (data[0] != 0xFFFF)
                co2 = data[0];

            if (data[1] != 0xFFFF && data[2] != 0xFFFF)
                illuminance = static_cast <double> (static_cast <quint32> (data[1]) << 16 | static_cast <quint32> (data[2])) / 100;

            if (data[3] != 0xFFFF)
                voc = data[3];

            m_endpoints.value(0)->buffer().insert("co2", co2);
            m_endpoints.value(0)->buffer().insert("illuminance", illuminance);
            m_endpoints.value(0)->buffer().insert("voc", voc);

            break;
        }
    }

    m_sequence++;
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

                m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, (index ? 0x0401 : 0x0400) + endpointId * 0x1000, m_settings.at(i).type));
                break;
            }

            break;

        case 2: // pValueMin
        case 3: // nValueMin
        case 4: // pValueMax
        case 5: // nValueMax
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister,  0x0408 + endpointId * 0x1000 + index - 2, static_cast <quint16> (data.toInt())));
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
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0400 + (m_sequence + 1) * 0x1000, 2);

        case 6 ... 11:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0408 + (m_sequence - 5) * 0x1000, 4);

        case 12 ... 17:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0500 + (m_sequence - 11) * 0x1000, 6);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMai6::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 5:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < m_settings.count(); i++)
            {
                if (m_settings.at(i).type == data[0])
                {
                    m_endpoints.find(m_sequence + 1).value()->buffer().insert("pSensorType", m_types.at(i));
                    m_pChannel[m_sequence] = m_settings.at(i);
                }

                if (m_settings.at(i).type == data[1])
                {
                    m_endpoints.find(m_sequence + 1).value()->buffer().insert("nSensorType", m_types.at(i));
                    m_nChannel[m_sequence] = m_settings.at(i);
                }
            }

            m_fullPoll = false;
            break;
        }

        case 6 ... 11:
        {
            quint16 data[4];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(m_sequence - 5).value()->buffer().insert("pValueMin", data[0]);
            m_endpoints.find(m_sequence - 5).value()->buffer().insert("nValueMin", data[1]);
            m_endpoints.find(m_sequence - 5).value()->buffer().insert("pValueMax", data[2]);
            m_endpoints.find(m_sequence - 5).value()->buffer().insert("nValueMax", data[3]);

            break;
        }

        case 12 ... 17:
        {
            quint16 data[6];
            double pInput = NAN, nInput = NAN, pValue = NAN, nValue = NAN;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
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

            m_endpoints.find(m_sequence - 11).value()->buffer().insert("pInput", pInput);
            m_endpoints.find(m_sequence - 11).value()->buffer().insert("nInput", nInput);
            m_endpoints.find(m_sequence - 11).value()->buffer().insert("pValue", pValue);
            m_endpoints.find(m_sequence - 11).value()->buffer().insert("nValue", nValue);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap3ev::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap3ev";
    m_description = "Wiren Board WB-MAP3EV Voltage Meter";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), angle(new SensorObject("angle"));

            voltage->setMultiple(true);
            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);
        }
        else
        {
            Expose frequency(new SensorObject("frequency"));
            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);
}

void WirenBoard::WBMap3ev::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray WirenBoard::WBMap3ev::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMap3ev::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 1:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("voltage", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPLIER / 1000);

            break;
        }

        case 2:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("angle", static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap3e::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap3e";
    m_description = "Wiren Board WB-MAP3E Energy Meter";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

            voltage->setMultiple(true);
            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            current->setMultiple(true);
            current->setParent(endpoint.data());
            endpoint->exposes().append(current);

            power->setMultiple(true);
            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setMultiple(true);
            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose frequency(new SensorObject("frequency")), totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

            totalPower->setParent(endpoint.data());
            endpoint->exposes().append(totalPower);

            totalEnergy->setParent(endpoint.data());
            endpoint->exposes().append(totalEnergy);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
}

void WirenBoard::WBMap3e::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 registerAddress;

    if (!endpointId || endpointId > 3)
        return;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + endpointId - 1;
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + endpointId + 2;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap3e::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBMap3e::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS, WBMAP_COIL_REGISTER_COUNT);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);
        case 3: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS, WBMAP_CURRENT_REGISTER_COUNT);
        case 4: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS, WBMAP_POWER_REGISTER_COUNT);
        case 5: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS, WBMAP_ENERGY_REGISTER_COUNT);
        case 6: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMap3e::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 2:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("voltage", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPLIER / 1000);

            break;
        }

        case 3:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("current", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 4:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.find(i).value()->buffer().insert(i ? "power" : "totalPower", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_POWER_MULTIPLIER / 1000);

            break;
        }

        case 5:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
                m_endpoints.find(i).value()->buffer().insert(i ? "energy" : "totalEnergy", static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER / 1000);

            break;
        }

        case 6:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find(i + 1).value()->buffer().insert("angle", static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000);

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap6s::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "wbMap6s";
    m_description = "Wiren Board WB-MAP6S Energy Meter";

    for (quint8 i = 0; i <= 6; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

            current->setMultiple(true);
            current->setParent(endpoint.data());
            endpoint->exposes().append(current);

            power->setMultiple(true);
            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setMultiple(true);
            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose voltage(new SensorObject("voltage")), frequency(new SensorObject("frequency")), totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

            totalPower->setParent(endpoint.data());
            endpoint->exposes().append(totalPower);

            totalEnergy->setParent(endpoint.data());
            endpoint->exposes().append(totalEnergy);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
}

void WirenBoard::WBMap6s::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 offset, registerAddress;

    if (!endpointId || endpointId > 6)
        return;

    offset = (6 - endpointId) % 3 + (endpointId - 1) / 3 * 0x1000;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset;
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset + 3;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap6s::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 2;
    m_polling = true;

    m_totalPower = 0;
    m_totalEnergy = 0;
}

QByteArray WirenBoard::WBMap6s::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 1:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 2:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_VOLTAGE_REGISTER_ADDRESS, 1);

        case 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 4 ... 5:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 4) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 6 ... 7:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_POWER_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP6S_POWER_REGISTER_COUNT);

        case 8 ... 9:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP6S_ENERGY_REGISTER_ADDRESS + (m_sequence - 8) * 0x1000, WBMAP6S_ENERGY_REGISTER_COUNT);
    }

    m_endpoints.find(0).value()->buffer().insert("totalPower", m_totalPower);
    m_endpoints.find(0).value()->buffer().insert("totalEnergy", m_totalEnergy);

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}


void WirenBoard::WBMap6s::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 1:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + 3 - i);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 2:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("voltage", static_cast <double> (value) * WBMAP6S_VOLTAGE_MULTIPLIER / 1000);
            break;
        }

        case 3:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 4 ... 5:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find((m_sequence - 4) * 3 + 3 - i).value()->buffer().insert("current", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 6 ... 7:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP6S_POWER_MULTIPLIER / 1000;
                m_endpoints.find((m_sequence - 6) * 3 + 3 - i).value()->buffer().insert("power", value);
                m_totalPower += value;
            }

            break;
        }

        case 8 ... 9:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER / 1000;
                m_endpoints.find((m_sequence - 8) * 3 + 3 - i).value()->buffer().insert("energy", value);
                m_totalEnergy += value;
            }

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBMap12::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    switch (m_model)
    {
        case Model::wbMap12e:
            m_type = "wbMap12e";
            m_description = "Wiren Board WB-MAP12H Energy Meter";
            break;

        case Model::wbMap12h:
            m_type = "wbMap12h";
            m_description = "Wiren Board WB-MAP12H Energy Meter";
            break;
    }

    for (quint8 i = 0; i <= 12; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), ratio(new NumberObject("ratio")), delta(new NumberObject("delta"));

            voltage->setMultiple(true);
            voltage->setParent(endpoint.data());
            endpoint->exposes().append(voltage);

            current->setMultiple(true);
            current->setParent(endpoint.data());
            endpoint->exposes().append(current);

            power->setMultiple(true);
            power->setParent(endpoint.data());
            endpoint->exposes().append(power);

            energy->setMultiple(true);
            energy->setParent(endpoint.data());
            endpoint->exposes().append(energy);

            angle->setMultiple(true);
            angle->setParent(endpoint.data());
            endpoint->exposes().append(angle);

            ratio->setMultiple(true);
            ratio->setParent(endpoint.data());
            endpoint->exposes().append(ratio);

            delta->setMultiple(true);
            delta->setParent(endpoint.data());
            endpoint->exposes().append(delta);
        }
        else
        {
            Expose frequency(new SensorObject("frequency"));

            for (quint8 j = 1; j <= 4; j++)
            {
                Expose totalPower(new SensorObject(QString("totalPower_%1").arg(j))), totalEnergy(new SensorObject(QString("totalEnergy_%1").arg(j)));

                totalPower->setParent(endpoint.data());
                endpoint->exposes().append(totalPower);

                totalEnergy->setParent(endpoint.data());
                endpoint->exposes().append(totalEnergy);
            }

            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);

        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("totalPower",  exposeOptions.value("power"));
    m_options.insert("totalEnergy", exposeOptions.value("energy"));

    m_options.insert("angle",       QMap <QString, QVariant> {{"type", "sensor"}, {"unit", "°"}, {"icon", "mdi:angle-acute"}});
    m_options.insert("delta",       QMap <QString, QVariant> {{"type", "number"}, {"min", -32768}, {"max", 32767}, {"icon", "mdi:delta"}});
    m_options.insert("ratio",       QMap <QString, QVariant> {{"type", "number"}, {"min", 0}, {"max", 65535}, {"icon", "mdi:alpha-k-box-outline"}});
}

void WirenBoard::WBMap12::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 offset, registerAddress;

    if (!endpointId || endpointId > 12)
        return;

    offset = (endpointId - 1) % 3 + (endpointId - 1) / 3 * 0x1000;

    if (name == "ratio")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset;
    else if (name == "delta")
        registerAddress = WBMAP_COIL_REGISTER_ADDRESS + offset + 3;
    else
        return;

    m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, registerAddress, static_cast <quint16> (data.toInt())));
    m_fullPoll = true;
}

void WirenBoard::WBMap12::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 4;
    m_polling = true;
}

QByteArray WirenBoard::WBMap12::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0 ... 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, WBMAP_COIL_REGISTER_ADDRESS + m_sequence * 0x1000, WBMAP_COIL_REGISTER_COUNT);

        case 4:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_FREQUENCY_REGISTER_ADDRESS, 1);

        case 5:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_VOLTAGE_REGISTER_ADDRESS, WBMAP_VOLTAGE_REGISTER_COUNT);

        case 6 ... 9:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_CURRENT_REGISTER_ADDRESS + (m_sequence - 6) * 0x1000, WBMAP_CURRENT_REGISTER_COUNT);

        case 10 ... 13:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_POWER_REGISTER_ADDRESS + (m_sequence - 10) * 0x1000, WBMAP_POWER_REGISTER_COUNT);

        case 14 ... 17:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ENERGY_REGISTER_ADDRESS + (m_sequence - 14) * 0x1000, WBMAP_ENERGY_REGISTER_COUNT);

        case 18:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   WBMAP_ANGLE_REGISTER_ADDRESS, WBMAP_ANGLE_REGISTER_COUNT);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMap12::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 3:
        {
            quint16 data[WBMAP_COIL_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(m_sequence * 3 + i + 1);
                it.value()->buffer().insert("ratio", data[i]);
                it.value()->buffer().insert("delta", static_cast <qint16> (data[i + 3]));
            }

            m_fullPoll = false;
            break;
        }

        case 4:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.find(0).value()->buffer().insert("frequency", static_cast <double> (value) * WBMAP_FREQUENCY_MULTIPLIER / 1000);
            break;
        }

        case 5:
        {
            quint16 data[WBMAP_VOLTAGE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_VOLTAGE_MULTIPLIER / 1000;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.find(i + j * 3 + 1 ).value()->buffer().insert("voltage", value);
            }

            break;
        }

        case 6 ... 9:
        {
            quint16 data[WBMAP_CURRENT_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.find((m_sequence - 6) * 3 + i + 1).value()->buffer().insert("current", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * WBMAP_CURRENT_MULTIPLIER / 1000);

            break;
        }

        case 10 ... 13:
        {
            quint16 data[WBMAP_POWER_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) * (m_model == Model::wbMap12h ? i ? WBMAP12H_CHANNEL_POWER_MULTIPLIER : WBMAP12H_TOTAL_POWER_MULTIPLIER : WBMAP_POWER_MULTIPLIER) / 1000;

                if (i)
                    m_endpoints.find((m_sequence - 10) * 3 + i).value()->buffer().insert("power", value);
                else
                    m_endpoints.find(0).value()->buffer().insert(QString("totalPower_%1").arg(m_sequence - 9), value);
            }

            break;
        }

        case 14 ... 17:
        {
            quint16 data[WBMAP_ENERGY_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 4; i++)
            {
                double value = static_cast <double> (static_cast <quint64> (data[i * 4 + 3]) << 48 | static_cast <quint64> (data[i * 4 + 2]) << 32 | static_cast <quint64> (data[i * 4 + 1]) << 16 | static_cast <quint64> (data[i * 4])) * WBMAP_ENERGY_MULTIPLIER / 1000;

                if (i)
                    m_endpoints.find((m_sequence - 14) * 3 + i).value()->buffer().insert("energy", value);
                else
                    m_endpoints.find(0).value()->buffer().insert(QString("totalEnergy_%1").arg(m_sequence - 13), value);
            }

            break;
        }

        case 18:
        {
            quint16 data[WBMAP_ANGLE_REGISTER_COUNT];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                double value = static_cast <double> (static_cast <qint16> (data[i])) * WBMAP_ANGLE_MULTIPLIER / 1000;

                for (quint8 j = 0; j < 4; j++)
                    m_endpoints.find(i + j * 3 + 1 ).value()->buffer().insert("angle", value);
            }

            break;
        }
    }

    m_sequence++;
}

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
                Expose input(new BinaryObject("input"));
                input->setMultiple(true);
                input->setParent(endpoint.data());
                endpoint->exposes().append(input);
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
            Expose input(new BinaryObject("input_0"));
            input->setParent(endpoint.data());
            endpoint->exposes().append(input);
        }

        m_endpoints.insert(i, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("input",             QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:import"}});
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

        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId - 1, value ? 0xFF00 : 0x0000));
        return;
    }

    if (m_model != Model::wbMrwm2)
        return;

    switch (index)
    {
        case 1: // voltageProtection
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x06A0 + endpointId - 1, data.toBool() ? 0x0001 : 0x0000));
            break;

        case 2: // voltageLow
        case 3: // voltageHigh
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, (index == 2 ? 0x06A8 : 0x06B0) + endpointId - 1, static_cast <quint16> (data.toInt() * 100)));
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
            return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x06A0 + m_sequence * 8, 2);

        case 3:
            return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus, 0x0000, m_channels);

        case 4:

            if (!m_inputs)
                break;

            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0000, m_inputs);

        case 5:

            if (m_model != Model::wbMrwm2)
                break;

            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus, 0x0050, 2);

        case 6 ... 7:

            if (m_model != Model::wbMrwm2)
                break;

            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 6 ? 0x0038 : 0x0040, 2);

        case 8:

            if (m_model != Model::wbMrwm2)
                break;

            return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0048, 4);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMr::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0 ... 2:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
            {
                if (!m_sequence)
                    m_endpoints.value(i + 1)->buffer().insert("voltageProtection", data[i] ? true : false);
                else
                    m_endpoints.value(i + 1)->buffer().insert(m_sequence == 1 ? "voltageLow" : "voltageHigh", data[i] / 100.0);
            }

            m_fullPoll = false;
            break;
        }

        case 3:
        {
            quint16 data[6];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 4:
        {
            quint16 data[8];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < m_channels; i++)
                m_endpoints.value(i + 1)->buffer().insert("input", data[i] ? true : false);

            if (m_inputs == 8)
                m_endpoints.value(0)->buffer().insert("input_0", data[7] ? true : false);

            break;
        }

        case 5:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("alarm", data[i] ? false : true);

            break;
        }

        case 6 ... 7:
        {
            quint16 data[2];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert(m_sequence == 6 ? "voltage" : "power", data[i] / (m_sequence == 6 ? 100.0 : 10.0));

            break;
        }

        case 8:
        {
            quint16 data[4];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 2; i++)
                m_endpoints.value(i + 1)->buffer().insert("energy", static_cast <double> (static_cast <quint32> (data[i * 2]) << 16 | static_cast <quint32> (data[i * 2 + 1])) / 1000);

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

    for (quint8 i = 1; i <= 10; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));
        Expose light(new LightObject);

        if (!m_list.contains(i))
            continue;

        light->setMultiple(true);
        light->setParent(endpoint.data());
        endpoint->exposes().append(light);

        m_options.insert(QString("light_%1").arg(i), i < 8 ? QList <QVariant> {"level"} : i < 10 ? QList <QVariant> {"level", "colorTemperature"} : QList <QVariant> {"level", "color"});
        m_endpoints.insert(i, endpoint);
    }

    m_options.insert("endpointName", QMap <QString, QVariant> {{"1", "W1"}, {"2", "W2"}, {"3", "W3"}, {"4", "W4"}, {"5", "W1+W2"}, {"6", "W3+W4"}, {"7", "W1+W2+W3+W4"}, {"8", "CCT1"}, {"9", "CCT2"}, {"10", "RGB"}});
    m_options.insert("colorTemperature", QMap <QString, QVariant> {{"min", 150}, {"max", 450}, {"step", 3}});
}

void WirenBoard::WBLed::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"status", "level", "colorTemperature", "color"};

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

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId - 1, value ? 0xFF00 : 0x0000));
            break;
        }

        case 1: // level
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x07D0 + (endpointId <= 9 ? endpointId <= 7 ? endpointId - 1 : (endpointId - 8) * 2 + 8 : 16), static_cast <quint16> (round(data.toInt() / 2.55))));
            break;
        }

        case 2: // colorTemperature
        {
            if (endpointId != 8 && endpointId != 9)
                break;

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x07D7 + (endpointId - 8) * 2, static_cast <quint16> ((450 - data.toInt()) / 3)));
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

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteMultipleRegisters, 0x07DE, 2, value));
            break;
        }
    }
}

void WirenBoard::WBLed::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBLed::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister,  0x0FA0, m_mode);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x0000, 10);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x07D0, 17);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBLed::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            if (Modbus::parseReply(m_slaveId, Modbus::WriteSingleRegister, reply) != Modbus::ReplyStatus::Ok)
                break;

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[16];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
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

        case 2:
        {
            quint16 data[17];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 10; i++)
            {
                if (!m_list.contains(i + 1))
                    continue;

                switch (i)
                {
                    case 0 ... 6:
                    {
                        m_endpoints.value(i + 1)->buffer().insert("level", round(data[i] * 2.55));
                        break;
                    }

                    case 7 ... 8:
                    {
                        m_endpoints.value(i + 1)->buffer().insert("level", round(data[(i - 7) * 2 + 8] * 2.55));
                        m_endpoints.value(i + 1)->buffer().insert("colorTemperature", 450 - data[7 + (i - 7) * 2] * 3);
                        break;
                    }

                    case 9:
                    {
                        Color color(Color::fromHS(data[14] / 360.0, data[15] / 100.0));
                        m_endpoints.value(i + 1)->buffer().insert("level", data[16] * 2.55);
                        m_endpoints.value(i + 1)->buffer().insert("color", QList <QVariant> {static_cast <quint8> (color.r() * 255), static_cast <quint8> (color.g() * 255), static_cast <quint8> (color.b() * 255)});
                        break;
                    }
                }
            }

            break;
        }
    }

    m_sequence++;
}

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

    m_options.insert("light",       QList <QVariant> {"level"});
    m_options.insert("dimmerMode",  QMap <QString, QVariant> {{"type", "select"}, {"enum", QVariant(m_modes)}, {"icon", "mdi:cog"}});
    m_options.insert("dimmerFront", QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"leading", "trailing"}}, {"icon", "mdi:cog"}});
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

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, endpointId - 1, value ? 0xFF00 : 0x0000));
            return;
        }

        case 1: // level
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, endpointId - 1, static_cast <quint16> (round(data.toInt() / 2.55))));
            return;
        }

        case 2: // dimmerMode
        {
            int value = m_modes.indexOf(data.toString());

            if (value < 0)
                return;

            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0032 + endpointId - 1, static_cast <quint16> (value)));
            break;
        }

        case 3: // dimmerFront
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x003C + endpointId - 1, data.toString() == "trailing" ? 0x0001 : 0x0000));
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
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0032, 3);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x003C, 3);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus,       0x0000, 3);
        case 3: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0000, 3);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBMdm::parseReply(const QByteArray &reply)
{
    quint16 data[3];

    switch (m_sequence)
    {
        case 0:
        {
            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
            {
                QString mode = m_modes.value(data[i]);

                if (mode.isEmpty())
                    continue;

                m_endpoints.value(i + 1)->buffer().insert("dimmerMode", mode);
            }

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("dimmerFront", data[i] ? "trailing" : "leading");

            m_fullPoll = false;
            break;
        }

        case 2:
        {
            if (Modbus::parseReply(m_slaveId, Modbus::ReadCoilStatus, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("status", data[i] ? "on" : "off");

            memcpy(m_output, data, sizeof(m_output));
            break;
        }

        case 3:
        {
            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert("level", round(data[i] * 2.55));

            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBUps::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose battety(new SensorObject("battery")), batteryStatus(new SensorObject("batteryStatus")), temperature(new SensorObject("temperature")), temperatureStatus(new SensorObject("temperatureStatus")), inputVoltage(new SensorObject("inputVoltage")), outputVoltage(new SensorObject("outputVoltage")), batteryVoltage(new SensorObject("batteryVoltage")), batteryCurrent(new SensorObject("batteryCurrent")), chargeCurrent(new SensorObject("chargeCurrent")), dischargeCurrent(new SensorObject("dischargeCurrent")), operationMode(new SelectObject("operationMode")), outputVoltageLimit(new NumberObject("outputVoltageLimit")), chargeCurrentLimit(new NumberObject("chargeCurrentLimit"));

    m_type = "wbUps";
    m_description = "Wiren Board WB-UPS v3 Backup Power Supply";

    battety->setParent(endpoint.data());
    endpoint->exposes().append(battety);

    batteryStatus->setParent(endpoint.data());
    endpoint->exposes().append(batteryStatus);

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    temperatureStatus->setParent(endpoint.data());
    endpoint->exposes().append(temperatureStatus);

    inputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(inputVoltage);

    outputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltage);

    batteryVoltage->setParent(endpoint.data());
    endpoint->exposes().append(batteryVoltage);

    batteryCurrent->setParent(endpoint.data());
    endpoint->exposes().append(batteryCurrent);

    chargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrent);

    dischargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(dischargeCurrent);

    operationMode->setParent(endpoint.data());
    endpoint->exposes().append(operationMode);

    outputVoltageLimit->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltageLimit);

    chargeCurrentLimit->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrentLimit);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);

    m_options.insert("batteryStatus",      QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:battery-charging"}});
    m_options.insert("temperatureStatus",  QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:thermometer"}});

    m_options.insert("inputVoltage",       exposeOptions.value("voltage"));
    m_options.insert("outputVoltage",      exposeOptions.value("voltage"));
    m_options.insert("batteryVoltage",     exposeOptions.value("voltage"));

    m_options.insert("batteryCurrent",     exposeOptions.value("current"));
    m_options.insert("chargeCurrent",      exposeOptions.value("current"));
    m_options.insert("dischargeCurrent",   exposeOptions.value("current"));

    m_options.insert("operationMode",      QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"auto", "manual"}}, {"icon", "mdi:cog"}});
    m_options.insert("outputVoltageLimit", QMap <QString, QVariant> {{"type", "number"}, {"min", 9}, {"max", 25.6}, {"step", 0.1}, {"unit", "V"}, {"icon", "mdi:sine-wave"}});
    m_options.insert("chargeCurrentLimit", QMap <QString, QVariant> {{"type", "number"}, {"min", 0.3}, {"max", 2}, {"step", 0.1}, {"unit", "A"}, {"icon", "mdi:current-ac"}});
}

void WirenBoard::WBUps::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"operationMode", "outputVoltageLimit", "chargeCurrentLimit"};
    int index = actions.indexOf(name);

    switch (index)
    {
        case 0: // operationMode
        {
            quint16 value = data.toString() == "manual" ? 1 : 0;
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0010, value));
            break;
        }

        case 1: // outputVoltageLimit
        case 2: // chargeCurrentLimit
        {
            quint16 value = static_cast <quint16> (data.toDouble() * 1000);
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, index == 1 ? 0x0011 : 0x0012, value));
            break;
        }

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBUps::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBUps::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0010, 3);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x0000, 10);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::WBUps::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[3];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("operationMode",      data[0] ? "manual" : "auto");
            m_endpoints.value(0)->buffer().insert("outputVoltageLimit", data[1] / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrentLimit", data[2] / 1000.0);

            m_fullPoll = false;
            break;
        }

        case 1:
        {
            quint16 data[10];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            switch (data[0])
            {
                case 0: m_endpoints.value(0)->buffer().insert("batteryStatus", "charge"); break;
                case 1: m_endpoints.value(0)->buffer().insert("batteryStatus", "charged"); break;
                case 2: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharge"); break;
                case 3: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharged"); break;
                case 4: m_endpoints.value(0)->buffer().insert("batteryStatus", "alarm"); break;
            }

            switch (data[1])
            {
                case 0: m_endpoints.value(0)->buffer().insert("temperatureStatus", "normal"); break;
                case 1: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overcool"); break;
                case 2: m_endpoints.value(0)->buffer().insert("temperatureStatus", "cool"); break;
                case 3: m_endpoints.value(0)->buffer().insert("temperatureStatus", "heat"); break;
                case 4: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overheat"); break;
            }

            m_endpoints.value(0)->buffer().insert("inputVoltage",     data[2] / 1000.0);
            m_endpoints.value(0)->buffer().insert("outputVoltage",    data[3] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryVoltage",   data[4] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryCurrent",   static_cast <qint16> (data[5]) / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrent",    data[6] / 1000.0);
            m_endpoints.value(0)->buffer().insert("dischargeCurrent", data[7] / 1000.0);
            m_endpoints.value(0)->buffer().insert("battery",          data[8] / 100.0);
            m_endpoints.value(0)->buffer().insert("temperature",      data[9] / 100.0);
            break;
        }
    }

    m_sequence++;
}
