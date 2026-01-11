#include <QtEndian>
#include "expose.h"
#include "peacefair.h"

void Peacefair::PZEM0x4::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), frequency(new SensorObject("frequency"));

    m_type = "pzem0x4";
    m_description = "Peacefair PZEM-004T, PZEM-014 or PZEM-016 Energy Meter";

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

    frequency->setMultiple(true);
    frequency->setParent(endpoint.data());
    endpoint->exposes().append(frequency);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);
}

void Peacefair::PZEM0x4::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Peacefair::PZEM0x4::pollRequest(void)
{
    if (!m_sequence)
        return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 8);

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Peacefair::PZEM0x4::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[8];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("voltage",   data[0] / 10.0);
            m_endpoints.value(0)->buffer().insert("current",   static_cast <double> (static_cast <quint32> (data[1]) | static_cast <quint32> (data[2]) << 16) / 1000);
            m_endpoints.value(0)->buffer().insert("power",     static_cast <double> (static_cast <quint32> (data[3]) | static_cast <quint32> (data[4]) << 16) / 10);
            m_endpoints.value(0)->buffer().insert("energy",    static_cast <double> (static_cast <quint32> (data[5]) | static_cast <quint32> (data[6]) << 16) / 1000);
            m_endpoints.value(0)->buffer().insert("frequency", data[7] / 10.0);
            break;
        }
    }

    m_sequence++;
}

void Peacefair::PZEM6l24::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "pzem6l24";
    m_description = "Peacefair PZEM-6L24 Energy Meter";

    for (quint8 i = 0; i <= 3; i++)
    {
        Endpoint endpoint(new EndpointObject(i, device));

        if (i)
        {
            Expose voltage(new SensorObject("voltage")), current(new SensorObject("current")), power(new SensorObject("power")), energy(new SensorObject("energy")), angle(new SensorObject("angle")), frequency(new SensorObject("frequency"));

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

            frequency->setMultiple(true);
            frequency->setParent(endpoint.data());
            endpoint->exposes().append(frequency);
        }
        else
        {
            Expose totalPower(new SensorObject("totalPower")), totalEnergy(new SensorObject("totalEnergy"));

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
}

void Peacefair::PZEM6l24::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Peacefair::PZEM6l24::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 12);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x000E, 6);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0028, 6);
        case 3: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0020, 2);
        case 4: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x003A, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Peacefair::PZEM6l24::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[12];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < 3; i++)
            {
                auto it = m_endpoints.find(i + 1);
                it.value()->buffer().insert("voltage",   qbswap(data[i]) / 10.0);
                it.value()->buffer().insert("current",   qbswap(data[i + 3]) / 100.0);
                it.value()->buffer().insert("frequency", qbswap(data[i + 6]) / 100.0);
                it.value()->buffer().insert("angle",     static_cast <qint16> (qbswap(data[i + 9])) / 100.0);
            }

            break;
        }

        case 1:
        case 2:
        {
            quint16 data[6];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (int i = 0; i < 3; i++)
                m_endpoints.value(i + 1)->buffer().insert(m_sequence == 1 ? "power" : "energy", static_cast <double> (static_cast <quint32> (qbswap(data[i * 2])) | static_cast <quint32> (qbswap(data[i * 2 + 1])) << 16) / 10);

            break;
        }

        case 3:
        case 4:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert(m_sequence == 3 ? "totalPower" : "totalEnergy", static_cast <double> (static_cast <quint32> (qbswap(data[0])) | static_cast <quint32> (qbswap(data[1])) << 16) / 10);
            break;
        }
    }

    m_sequence++;
}
