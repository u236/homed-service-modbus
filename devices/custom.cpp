#include <QtEndian>
#include "custom.h"
#include "modbus.h"
#include "logger.h" // TODO: remove me

quint16 Custom::ItemObject::count(void)
{
    switch (m_dataType)
    {
        case DataType::i32:
        case DataType::u32:
        case DataType::f32:
            return 2;

        case DataType::i64:
        case DataType::u64:
        case DataType::f64:
            return 4;

        default:
            return 1;
    }
}

void Custom::Controller::init(const Device &device)
{
    m_type = "customController";
    m_description = "HOMEd Modbus Custom Controller";
    m_types = {"bool", "value", "enum"};
    m_endpoints.insert(0, Endpoint(new EndpointObject(0, device)));
}

void Custom::Controller::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    for (int i = 0; i < m_items.count(); i++)
    {
        const Item &item = m_items.at(i);
        quint16 count = item->count(), buffer[4], payload[4];
        QVariant value;

        switch (m_types.indexOf(item->type()))
        {
            case 0: value = data.toBool() ? 0x01 : 0x00; break; // bool
            case 1: value = data.toDouble() * item->divider(); break; // value

            // case 2: // enum
            // {
            //     int index = enumIndex(m_name, data.toString());
            //
            //     if (index < 0)
            //         return QByteArray();
            //
            //     value = index;
            //     break;
            // }
        }

        if (item->expose() != name || item->registerType() != RegisterType::holding)
            continue;

        switch (item->dataType())
        {
            case DataType::f32:
            {
                float number = qToLittleEndian(value.toFloat());
                memcpy(buffer, &number, sizeof(number));
                break;
            }

            case DataType::f64:
            {
                double number = qToLittleEndian(value.toFloat());
                memcpy(buffer, &number, sizeof(number));
                break;
            }

            default:
            {
                qint64 number = qToLittleEndian <qint64> (value.toDouble());
                memcpy(buffer, &number, count * 2);
                break;
            }
        }

        for (int i = 0; i < count; i++)
        {
            switch (item->byteOrder())
            {
                case ByteOrder::be:    payload[i] = qFromLittleEndian(buffer[count - i - 1]); break;
                case ByteOrder::le:    payload[i] = qFromBigEndian(buffer[i]); break;
                case ByteOrder::mixed: payload[i] = qFromLittleEndian(buffer[i]); break;
            }
        }

        m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteMultipleRegisters, item->address(), count, payload));
        return;
    }
}

void Custom::Controller::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray Custom::Controller::pollRequest(void)
{
    if (m_sequence < m_items.count())
    {
        const Item &item = m_items.at(m_sequence);
        return Modbus::makeRequest(m_slaveId, item->registerType() == RegisterType::holding ? Modbus::ReadHoldingRegisters : Modbus::ReadInputRegisters, item->address(), item->count());
    }

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    return QByteArray();
}

void Custom::Controller::parseReply(const QByteArray &reply)
{
    const Item &item = m_items.at(m_sequence);
    quint16 count = item->count(), buffer[4], payload[4];
    QVariant value;

    if (Modbus::parseReply(m_slaveId, item->registerType() == RegisterType::holding ? Modbus::ReadHoldingRegisters : Modbus::ReadInputRegisters, reply, buffer) != Modbus::ReplyStatus::Ok)
        return;

    for (int i = 0; i < count; i++)
    {
        switch (item->byteOrder())
        {
            case ByteOrder::be:    payload[i] = qToBigEndian(buffer[i]); break;
            case ByteOrder::le:    payload[i] = qToLittleEndian(buffer[count - i - 1]); break;
            case ByteOrder::mixed: payload[i] = qToBigEndian(buffer[count - i - 1]); break;
        }
    }

    switch (item->dataType())
    {
        case DataType::i16: value = qFromBigEndian <qint16>  (*(reinterpret_cast <qint16*>  (payload))); break;
        case DataType::u16: value = qFromBigEndian <quint16> (*(reinterpret_cast <quint16*> (payload))); break;
        case DataType::i32: value = qFromBigEndian <qint32>  (*(reinterpret_cast <qint32*>  (payload))); break;
        case DataType::u32: value = qFromBigEndian <quint32> (*(reinterpret_cast <quint32*> (payload))); break;
        case DataType::i64: value = qFromBigEndian <qint64>  (*(reinterpret_cast <qint64*>  (payload))); break;
        case DataType::u64: value = qFromBigEndian <quint64> (*(reinterpret_cast <quint64*> (payload))); break;
        case DataType::f32: value = qFromBigEndian <float>   (*(reinterpret_cast <float*>   (payload))); break;
        case DataType::f64: value = qFromBigEndian <double>  (*(reinterpret_cast <double*>  (payload))); break;
    }

    switch (m_types.indexOf(item->type()))
    {
        case 0: m_endpoints.find(0).value()->buffer().insert(item->expose(), value.toInt() ? true : false); break;       // bool
        case 1: m_endpoints.find(0).value()->buffer().insert(item->expose(), value.toDouble() / item->divider()); break; // value
        // case 2: m_endpoints.find(0).value()->buffer().insert(item->expose(), enumValue(m_name, value.toInt()); break;   // enum
    }

    m_sequence++;

    if (m_sequence < m_items.count())
        return;

    updateEndpoints();
}
