#ifndef CUSTOM_H
#define CUSTOM_H

#include <QMetaEnum>
#include "device.h"

namespace Custom
{
    Q_NAMESPACE

    enum class RegisterType
    {
        input,
        holding
    };

    enum class DataType
    {
        i16,
        u16,
        i32,
        u32,
        i64,
        u64,
        f32,
        f64
    };

    enum class ByteOrder
    {
        be,
        le,
        mixed
    };

    class ItemObject;
    typedef QSharedPointer <ItemObject> Item;

    class ItemObject
    {

    public:

        ItemObject(const QString &name, const QString &type, quint16 address, RegisterType registerType, DataType dataType, ByteOrder order, double divider) :
            m_name(name), m_type(type), m_address(address), m_registerType(registerType), m_dataType(dataType), m_order(order), m_divider(divider > 0 ? divider : 1) {}

        inline QString name(void) { return m_name; };
        inline QString type(void) { return m_type; };
        inline quint16 address(void) { return m_address; };
        inline RegisterType registerType(void) { return m_registerType; };
        inline DataType dataType(void) { return m_dataType; };
        inline ByteOrder byteOrder(void) { return m_order; };
        inline double divider(void) { return m_divider; };

        quint16 count(void);

    private:

        QString m_name, m_type;
        quint16 m_address;
        RegisterType m_registerType;
        DataType m_dataType;
        ByteOrder m_order;
        double m_divider;

    };

    class Controller : public DeviceObject
    {

    public:

        Controller(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name, const QJsonArray &items, const QJsonObject &options);

        inline QJsonObject options(void) { return m_options; }
        QJsonArray items(void);

        void init(const Device &device) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        QMetaEnum m_registerTypes, m_dataTypes, m_byteOrders;
        QJsonObject m_options;

        QList <QString> m_types;
        QList <Item> m_items;

    };

    Q_ENUM_NS(RegisterType)
    Q_ENUM_NS(DataType)
    Q_ENUM_NS(ByteOrder)
}

#endif
