#include <QtEndian>
#include "custom.h"

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
            m_endpoints.value(0)->buffer().insert(name, data);

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
                    action = m_endpoints.value(0)->buffer().value(name).toString() == "on" ? "off" : "on";

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
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleCoil, item->address(), value.toInt() ? 0xFF00 : 0x0000));
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

        m_actionQueue.enqueue(count > 1 ? m_modbus->makeRequest(m_slaveId, Modbus::WriteMultipleRegisters, item->address(), count, payload) : m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, item->address(), payload[0]));
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
    if (m_sequence < m_blocks.count())
    {
        const Block &block = m_blocks.at(m_sequence);
        Modbus::FunctionCode function;

        switch (block.registerType)
        {
            case RegisterType::coil:     function = Modbus::ReadCoilStatus; break;
            case RegisterType::discrete: function = Modbus::ReadInputStatus; break;
            case RegisterType::holding:  function = Modbus::ReadHoldingRegisters; break;
            case RegisterType::input:    function = Modbus::ReadInputRegisters; break;
        }

        return m_modbus->makeRequest(m_slaveId, function, block.address, block.count);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void Custom::Controller::parseReply(const QByteArray &reply)
{
    const Block &block = m_blocks.at(m_sequence++);
    Modbus::FunctionCode function;
    quint16 buffer[MAX_REGISTERS * 2];

    switch (block.registerType)
    {
        case RegisterType::coil:     function = Modbus::ReadCoilStatus; break;
        case RegisterType::discrete: function = Modbus::ReadInputStatus; break;
        case RegisterType::holding:  function = Modbus::ReadHoldingRegisters; break;
        case RegisterType::input:    function = Modbus::ReadInputRegisters; break;
    }

    if (m_modbus->parseReply(m_slaveId, function, reply, buffer) != Modbus::ReplyStatus::Ok)
        return;

    for (int i = 0; i < block.items.count(); i++)
    {
        const Item &item = block.items.at(i);
        quint16 *data = buffer + item->address() - block.address, count = item->count(), payload[4];
        QVariant value;

        if (item->registerType() == RegisterType::holding || item->registerType() == RegisterType::input)
        {
            for (int j = 0; j < count; j++)
            {
                switch (item->byteOrder())
                {
                    case ByteOrder::be:    payload[j] = qToBigEndian(data[j]); break;
                    case ByteOrder::le:    payload[j] = qToLittleEndian(data[count - j - 1]); break;
                    case ByteOrder::mixed: payload[j] = qToBigEndian(data[count - j - 1]); break;
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
            value = data[0];

        if (!value.isValid())
            continue;

        switch (m_types.indexOf(item->type()))
        {
            case 0: m_endpoints.value(0)->buffer().insert(item->expose(), value.toInt() ? true : false); break; // bool
            case 1: m_endpoints.value(0)->buffer().insert(item->expose(), value.toDouble() / item->divider()); break; // value

            case 2: // enum
            {
                QVariant option = m_options.value(item->expose()).toMap().value("enum");

                switch (option.type())
                {
                    case QVariant::Map:  m_endpoints.value(0)->buffer().insert(item->expose(), option.toMap().value(QString::number(value.toInt()))); break;
                    case QVariant::List: m_endpoints.value(0)->buffer().insert(item->expose(), option.toList().value(value.toInt())); break;
                    default: break;
                }

                break;
            }
        }
    }
}

void Custom::Controller::arrangeBlocks(void)
{
    QList <Item> list;

    m_blocks.clear();

    for (int i = 0; i < m_items.count(); i++)
        if (m_items.at(i)->read())
            list.append(m_items.at(i));

    std::sort(list.begin(), list.end(), [] (const Item &a, const Item &b) { return a->registerType() != b->registerType() ? a->registerType() < b->registerType() : a->address() < b->address(); });

    for (int i = 0; i < list.count(); i++)
    {
        const Item &item = list.at(i);
        quint32 end = item->address() + item->count();

        if (!m_blocks.isEmpty())
        {
            Block &block = m_blocks.last();
            quint32 last = block.address + block.count, span = (end > last ? end : last) - block.address;

            if (block.registerType == item->registerType() && item->address() <= last && span <= m_maxRegisters)
            {
                if (end > last)
                    block.count = end - block.address;

                block.items.append(item);
                continue;
            }
        }

        m_blocks.append({item->registerType(), item->address(), item->count(), {item}});
    }
}
