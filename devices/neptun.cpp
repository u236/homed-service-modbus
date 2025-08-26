#include "expose.h"
#include "modbus.h"
#include "neptun.h"

void Neptun::SmartPlus::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    m_type = "neptunSmartPlus";
    m_description = "Neptun Smart+ Controller";

    for (quint8 i = 0; i <= 10; i++)
    {
        Endpoint endpoint;
        quint8 endpointId;

        switch (i)
        {
            case 0 ... 2: endpointId = i;  break;
            case 3:       endpointId = 11; break;
            case 4:       endpointId = 12; break;
            case 5:       endpointId = 21; break;
            case 6:       endpointId = 22; break;
            case 7:       endpointId = 31; break;
            case 8:       endpointId = 32; break;
            case 9:       endpointId = 41; break;
            case 10:      endpointId = 42; break;
        }

        endpoint = Endpoint(new EndpointObject(endpointId, device));

        if (!endpointId)
        {
            Expose enableGroups(new ToggleObject("enableGroups")), lostProtection(new ToggleObject("lostProtection")), pairingMode(new ToggleObject("pairingMode")), childLock(new ToggleObject("childLock")), cleaningMode(new ToggleObject("cleaningMode")), waterLeak(new BinaryObject("waterLeak")), waterLeakList(new SensorObject("waterLeakList")), batteryLow(new BinaryObject("batteryLow")), batteryLowList(new SensorObject("batteryLowList")), sensorLost(new BinaryObject("sensorLost")), sensorLostList(new SensorObject("sensorLostList"));

            enableGroups->setParent(endpoint.data());
            endpoint->exposes().append(enableGroups);

            lostProtection->setParent(endpoint.data());
            endpoint->exposes().append(lostProtection);

            pairingMode->setParent(endpoint.data());
            endpoint->exposes().append(pairingMode);

            childLock->setParent(endpoint.data());
            endpoint->exposes().append(childLock);

            cleaningMode->setParent(endpoint.data());
            endpoint->exposes().append(cleaningMode);

            waterLeak->setParent(endpoint.data());
            endpoint->exposes().append(waterLeak);

            waterLeakList->setParent(endpoint.data());
            endpoint->exposes().append(waterLeakList);

            batteryLow->setParent(endpoint.data());
            endpoint->exposes().append(batteryLow);

            batteryLowList->setParent(endpoint.data());
            endpoint->exposes().append(batteryLowList);

            sensorLost->setParent(endpoint.data());
            endpoint->exposes().append(sensorLost);

            sensorLostList->setParent(endpoint.data());
            endpoint->exposes().append(sensorLostList);
        }
        else if (endpointId <= 2)
        {
            Expose lock(new LockObject), waterLeak(new BinaryObject("waterLeak"));

            lock->setMultiple(true);
            lock->setParent(endpoint.data());
            endpoint->exposes().append(lock);

            waterLeak->setMultiple(true);
            waterLeak->setParent(endpoint.data());
            endpoint->exposes().append(waterLeak);
        }
        else
        {
            Expose volume(new NumberObject("volume"));

            volume->setMultiple(true);
            volume->setParent(endpoint.data());
            endpoint->exposes().append(volume);
        }

        m_endpoints.insert(endpointId, endpoint);
    }

    updateOptions(exposeOptions);

    m_options.insert("volume",         QMap <QString, QVariant> {{"type", "number"}, {"class", "volume"}, {"state", "total_increasing"}, {"min", 0}, {"max", 1000000000}, {"unit", "L"}});
    m_options.insert("enableGroups",   QMap <QString, QVariant> {{"type", "toggle"}, {"icon", "mdi:source-branch"}});
    m_options.insert("lostProtection", QMap <QString, QVariant> {{"type", "toggle"}, {"icon", "mdi:leak-off"}});
    m_options.insert("pairingMode",    QMap <QString, QVariant> {{"type", "toggle"}, {"icon", "mdi:leak"}});
    m_options.insert("cleaningMode",   QMap <QString, QVariant> {{"type", "toggle"}, {"control", true}, {"icon", "mdi:vacuum"}});

    m_options.insert("lock",           "valve");
}

void Neptun::SmartPlus::enqueueAction(quint8 endpointId, const QString &name, const QVariant &data)
{
    quint16 status = m_status;

    if (endpointId <= 2)
    {
        QList <QString> list = {"status", "enableGroups", "lostProtection", "pairingMode", "childLock", "cleaningMode"};

        switch (list.indexOf(name))
        {
            case 0: // status
            {
                QList <QString> list = {"on", "off", "toggle"};
                quint16 mask = 1 << (endpointId + 7);

                switch (list.indexOf(data.toString()))
                {
                    case 0: status |=  mask; break;
                    case 1: status &= ~mask; break;
                    case 2: status ^=  mask; break;
                }

                break;
            }

            case 1: status = data.toBool() ? status | 0x0400 : status & 0xFBFF; break; // enableGroups
            case 2: status = data.toBool() ? status | 0x0800 : status & 0xF7FF; break; // lostProtection
            case 3: status = data.toBool() ? status | 0x0080 : status & 0xFF7F; break; // pairingMode
            case 4: status = data.toBool() ? status | 0x1000 : status & 0xEFFF; break; // childLock
            case 5: status = data.toBool() ? status | 0x0001 : status & 0xFFFE; break; // cleaningMode
        }

        if (status != m_status)
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0000, status));

        return;
    }

    if (name == "volume")
    {
        quint32 value = static_cast <quint32> (data.toInt());
        quint16 address;

        switch (endpointId)
        {
            case 11: address = 0x006B; break;
            case 12: address = 0x006D; break;
            case 21: address = 0x006F; break;
            case 22: address = 0x0071; break;
            case 31: address = 0x0073; break;
            case 32: address = 0x0075; break;
            case 41: address = 0x0077; break;
            case 42: address = 0x0079; break;
            default: return;
        }

        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, address, static_cast <quint16> (value >> 16)));
        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, address + 1, static_cast <quint16> (value)));
    }
}

void Neptun::SmartPlus::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Neptun::SmartPlus::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0000, 1);
        case 1: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0039, 16);
        case 2: return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x006B, 16);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Neptun::SmartPlus::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok || m_status == value)
                break;

            m_endpoints.value(0)->buffer().insert("enableGroups",   value & 0x0400 ? true : false);
            m_endpoints.value(0)->buffer().insert("lostProtection", value & 0x0800 ? true : false);
            m_endpoints.value(0)->buffer().insert("pairingMode",    value & 0x0080 ? true : false);
            m_endpoints.value(0)->buffer().insert("childLock",      value & 0x1000 ? true : false);
            m_endpoints.value(0)->buffer().insert("cleaningMode",   value & 0x0001 ? true : false);
            m_endpoints.value(0)->buffer().insert("batteryLow",     value & 0x0008 ? true : false);
            m_endpoints.value(0)->buffer().insert("sensorLost",     value & 0x0010 ? true : false);
            m_endpoints.value(1)->buffer().insert("waterLeak",      value & 0x0002 ? true : false);
            m_endpoints.value(1)->buffer().insert("status",         value & 0x0100 ? "on" : "off");
            m_endpoints.value(2)->buffer().insert("waterLeak",      value & 0x0004 ? true : false);
            m_endpoints.value(2)->buffer().insert("status",         value & 0x0200 ? "on" : "off");

            m_status = value;
            break;
        }

        case 1:
        {
            QList <QString> waterLeak, batteryLow, sensorLost;
            quint16 data[16];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (quint8 i = 0; i < 16; i++)
            {
                QString id = QString::number(i + 1);

                if (data[i] & 0x0001)
                    waterLeak.append(id);

                if (data[i] & 0x0002)
                    batteryLow.append(id);

                if (data[i] & 0x0004)
                    sensorLost.append(id);
            }

            m_endpoints.value(0)->buffer().insert("waterLeak",      waterLeak.isEmpty() ? false : true);
            m_endpoints.value(0)->buffer().insert("waterLeakList",  waterLeak.isEmpty() ? "-" : waterLeak.join(", "));
            m_endpoints.value(0)->buffer().insert("batteryLowList", batteryLow.isEmpty() ? "-" : batteryLow.join(", "));
            m_endpoints.value(0)->buffer().insert("sensorLostList", sensorLost.isEmpty() ? "-" : sensorLost.join(", "));
            break;
        }

        case 2:
        {
            quint16 data[16];

            if (Modbus::parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            for (qint8 i = 0; i < 8; i++)
            {
                quint8 endpointId;

                switch (i)
                {
                    case 0: endpointId = 11; break;
                    case 1: endpointId = 12; break;
                    case 2: endpointId = 21; break;
                    case 3: endpointId = 22; break;
                    case 4: endpointId = 31; break;
                    case 5: endpointId = 32; break;
                    case 6: endpointId = 41; break;
                    case 7: endpointId = 42; break;
                }

                m_endpoints.value(endpointId)->buffer().insert("volume", static_cast <quint32> (data[i * 2] << 16 | data[i * 2 + 1]));
            }
        }
    }

    m_sequence++;
}
