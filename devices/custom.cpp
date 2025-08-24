#include <QtEndian>
#include "custom.h"
#include "modbus.h"

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

void Custom::Controller::init(const Device &, const QMap <QString, QVariant> &)
{
    m_type = "customController";
    m_description = "Custom Modbus Controller";
    m_types = {"bool", "value", "enum"};
}

void Custom::Controller::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    for (int i = 0; i < m_items.count(); i++)
    {
        const Item &item = m_items.at(i);
        quint16 count = item->count(), buffer[4], payload[4];
        QVariant value;

        if (item->expose() != name || (item->registerType() != RegisterType::coil && item->registerType() != RegisterType::holding))
            continue;

        if (!item->read())
            m_endpoints.find(0).value()->buffer().insert(name, data);

        switch (m_types.indexOf(item->type()))
        {
            case 0: value = data.toBool() ? 0x01 : 0x00; break; // bool
            case 1: value = data.toDouble() * item->divider(); break; // value

            case 2: // enum
            {
                QString action = data.toString();
                QVariant option = m_options.value(item->expose()).toMap().value("enum");
                int index = -1;

                if (name.split('_').value(0) == "status" && action == "toggle")
                    action = m_endpoints.find(0).value()->buffer().value(name).toString() == "on" ? "off" : "on";

                switch (option.type())
                {
                    case QVariant::Map:
                    {
                        QMap <QString, QVariant> map = option.toMap();

                        for (auto it = map.begin(); it != map.end(); it++)
                        {
                            if (it.value() != action)
                                continue;

                            index = it.key().toInt();
                            break;
                        }

                        break;
                    }

                    case QVariant::List: index = option.toList().indexOf(action); break;
                    default: break;
                }

                if (index < 0)
                    return;

                value = index;
                break;
            }
        }

        if (item->registerType() == RegisterType::coil)
        {
            m_actionQueue.enqueue(Modbus::makeRequest(m_slaveId, Modbus::WriteSingleCoil, item->address(), value.toInt() ? 0xFF00 : 0x0000));
            return;
        }

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

        m_actionQueue.enqueue(count > 1 ? Modbus::makeRequest(m_slaveId, Modbus::WriteMultipleRegisters, item->address(), count, payload) : Modbus::makeRequest(m_slaveId, Modbus::WriteSingleRegister, item->address(), payload[0]));
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
    while (m_sequence < m_items.count() && !m_items.at(m_sequence)->read())
        m_sequence++;

    if (m_sequence < m_items.count())
    {
        const Item &item = m_items.at(m_sequence);

        switch (item->registerType())
        {
            case RegisterType::coil:     return Modbus::makeRequest(m_slaveId, Modbus::ReadCoilStatus,       item->address(), 1);
            case RegisterType::discrete: return Modbus::makeRequest(m_slaveId, Modbus::ReadInputStatus,      item->address(), 1);
            case RegisterType::holding:  return Modbus::makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, item->address(), item->count());
            case RegisterType::input:    return Modbus::makeRequest(m_slaveId, Modbus::ReadInputRegisters,   item->address(), item->count());
        }
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Custom::Controller::parseReply(const QByteArray &reply)
{
    const Item &item = m_items.at(m_sequence++);
    Modbus::FunctionCode function;
    quint16 count = item->count(), buffer[8], payload[4];
    QVariant value;

    switch (item->registerType())
    {
        case RegisterType::coil:     function = Modbus::ReadCoilStatus; break;
        case RegisterType::discrete: function = Modbus::ReadInputStatus; break;
        case RegisterType::holding:  function = Modbus::ReadHoldingRegisters; break;
        case RegisterType::input:    function = Modbus::ReadInputRegisters; break;
    }

    if (Modbus::parseReply(m_slaveId, function, reply, buffer) != Modbus::ReplyStatus::Ok)
        return;

    if (item->registerType() == RegisterType::holding || item->registerType() == RegisterType::input)
    {
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
    }
    else
        value = buffer[0];

    if (!value.isValid())
        return;

    switch (m_types.indexOf(item->type()))
    {
        case 0: m_endpoints.find(0).value()->buffer().insert(item->expose(), value.toInt() ? true : false); break; // bool
        case 1: m_endpoints.find(0).value()->buffer().insert(item->expose(), value.toDouble() / item->divider()); break; // value

        case 2: // enum
        {
            QVariant option = m_options.value(item->expose()).toMap().value("enum");

            switch (option.type())
            {
                case QVariant::Map:  m_endpoints.find(0).value()->buffer().insert(item->expose(), option.toMap().value(QString::number(value.toInt()))); break;
                case QVariant::List: m_endpoints.find(0).value()->buffer().insert(item->expose(), option.toList().value(value.toInt())); break;
                default: break;
            }

            break;
        }
    }
}
