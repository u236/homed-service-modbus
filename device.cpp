#include "devices/custom.h"
#include "devices/native.h"
#include "devices/neptun.h"
#include "devices/other.h"
#include "devices/r4pin08.h"
#include "devices/wirenboard.h"
#include "controller.h"
#include "device.h"
#include "expose.h"
#include "logger.h"

void DeviceObject::updateOptions(const QMap <QString, QVariant> &exposeOptions)
{
    for (auto it = m_endpoints.begin(); it != m_endpoints.end(); it++)
    {
        for (int i = 0; i < it.value()->exposes().count(); i++)
        {
            const QString &name = it.value()->exposes().at(i)->name();

            if (m_options.contains(name) || !exposeOptions.contains(name))
                continue;

            m_options.insert(name, exposeOptions.value(name));
        }
    }
}

void DeviceObject::updateEndpoints(void)
{
    for (auto it = m_endpoints.begin(); it != m_endpoints.end(); it++)
    {
        if (it.value()->status() == it.value()->buffer())
            continue;

        it.value()->status() = it.value()->buffer();
        emit endpointUpdated(this, it.key());
    }
}

DeviceList::DeviceList(QSettings *config, QObject *parent) : QObject(parent), m_timer(new QTimer(this)), m_registerTypes(QMetaEnum::fromType <Custom::RegisterType> ()), m_dataTypes(QMetaEnum::fromType <Custom::DataType> ()), m_byteOrders(QMetaEnum::fromType <Custom::ByteOrder> ()), m_sync(false)
{
    QFile file(config->value("device/expose", "/usr/share/homed-common/expose.json").toString());

    ExposeObject::registerMetaTypes();

    m_file.setFileName(config->value("device/database", "/opt/homed-modbus/database.json").toString());

    if (file.open(QFile::ReadOnly))
    {
        m_exposeOptions = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
        file.close();
    }

    m_types =
    {
        "customController",
        "homedRelayController",
        "homedSwitchController",
        "r4pin08m0",
        "r4pin08m1",
        "r4pin08m2",
        "r4pin08m3",
        "r4pin08m4",
        "wbMap3e",
        "wbMap6s",
        "wbMap12e",
        "wbMap12h",
        "wbMrm2",
        "wbMr3",
        "wbMr6",
        "wbMr6p",
        "wbUps",
        "neptunSmartPlus",
        "jth2d1",
        "t13"
    };

    m_specialExposes =
    {
        "light",
        "switch",
        "cover",
        "lock",
        "thermostat"
    };

    connect(m_timer, &QTimer::timeout, this, &DeviceList::writeDatabase);
    m_timer->setSingleShot(true);
}

DeviceList::~DeviceList(void)
{
    m_sync = true;
    writeDatabase();
}

void DeviceList::init(void)
{
    QJsonObject json;

    if (!m_file.open(QFile::ReadOnly))
        return;

    json = QJsonDocument::fromJson(m_file.readAll()).object();
    unserialize(json.value("devices").toArray());
    m_file.close();
}

void DeviceList::store(bool sync)
{
    m_sync = sync;
    m_timer->start(STORE_DATABASE_DELAY);
}

Device DeviceList::byName(const QString &name, int *index)
{
    for (int i = 0; i < count(); i++)
    {
        if (at(i)->address() != name && at(i)->name() != name)
            continue;

        if (index)
            *index = i;

        return at(i);
    }

    return Device();
}

Device DeviceList::parse(const QJsonObject &json)
{
    QString name = mqttSafe(json.value("name").toString());
    quint8 portId = static_cast <quint8> (json.value("portId").toInt()), slaveId = static_cast <quint8> (json.value("slaveId").toInt());
    quint32 baudRate = json.value("baudRate").toInt(), pollInterval = json.value("pollInterval").toInt(), requestTimeout = json.value("requestTimeout").toInt(1000), replyTimeout = json.value("replyTimeout").toInt(20);
    Device device;

    if (name.isEmpty() || !portId || !slaveId || !baudRate)
        return device;

    switch (m_types.indexOf(json.value("type").toString()))
    {
        case 0:  device = Device(new Custom::Controller(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 1:  device = Device(new Native::RelayController(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 2:  device = Device(new Native::SwitchController(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 3:  device = Device(new R4PIN08::M0(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 4:  device = Device(new R4PIN08::M1(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 5:  device = Device(new R4PIN08::M2(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 6:  device = Device(new R4PIN08::M3(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 7:  device = Device(new R4PIN08::M4(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 8:  device = Device(new WirenBoard::WBMap3e(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 9:  device = Device(new WirenBoard::WBMap6s(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 10: device = Device(new WirenBoard::WBMap12e(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 11: device = Device(new WirenBoard::WBMap12h(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 12: device = Device(new WirenBoard::WBMrm2(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 13: device = Device(new WirenBoard::WBMr3(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 14: device = Device(new WirenBoard::WBMr6(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 15: device = Device(new WirenBoard::WBMr6p(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 16: device = Device(new WirenBoard::WBUps(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 17: device = Device(new Neptun::SmartPlus(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 18: device = Device(new Other::JTH2D1(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
        case 19: device = Device(new Other::T13(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name)); break;
    }

    if (!device.isNull())
    {
        if (json.contains("active"))
            device->setActive(json.value("active").toBool());

        if (json.contains("discovery"))
            device->setDiscovery(json.value("discovery").toBool());

        if (json.contains("cloud"))
            device->setCloud(json.value("cloud").toBool());

        device->setNote(json.value("note").toString());
        device->init(device, m_exposeOptions);

        if (device->type() == "customController")
        {
            QJsonArray items = json.value("items").toArray();
            Custom::Controller* controller = reinterpret_cast <Custom::Controller*> (device.data());
            Endpoint endpoint(new EndpointObject(DEFAULT_ENDPOINT, device));

            controller->options() = json.value("options").toObject().toVariantMap();
            controller->endpoints().insert(endpoint->id(), endpoint);

            for (auto it = items.begin(); it != items.end(); it++)
            {
                QJsonObject item = it->toObject();
                QString exposeName = item.value("expose").toString(), itemName = exposeName.split('_').value(0);
                QMap <QString, QVariant> option = m_exposeOptions.value(itemName).toMap();
                Expose expose;
                int type;

                if (exposeName.isEmpty())
                    continue;

                if (controller->options().contains(exposeName))
                    option.insert(controller->options().value(exposeName).toMap());
                else if (controller->options().contains(itemName))
                    option.insert(controller->options().value(itemName).toMap());

                if (!option.isEmpty())
                    controller->options().insert(exposeName, option);

                type = QMetaType::type(QString(m_specialExposes.contains(itemName) ? itemName : option.value("type").toString()).append("Expose").toUtf8());

                expose = Expose(type ? reinterpret_cast <ExposeObject*> (QMetaType::create(type)) : new ExposeObject(exposeName));
                expose->setName(exposeName);
                expose->setParent(endpoint.data());

                controller->items().append(Custom::Item(new Custom::ItemObject(exposeName, item.value("type").toString("value"), static_cast <quint16> (item.value("address").toInt()), static_cast <Custom::RegisterType> (m_registerTypes.keyToValue(item.value("registerType").toString("input").toUtf8().constData())), static_cast <Custom::DataType> (m_dataTypes.keyToValue(item.value("dataType").toString("u16").toUtf8().constData())), static_cast <Custom::ByteOrder> (m_byteOrders.keyToValue(item.value("byteOrder").toString("be").toUtf8().constData())), item.value("divider").toDouble(1), item.value("read").toBool(true))));
                endpoint->exposes().append(expose);
            }
        }
    }

    return device;
}

void DeviceList::unserialize(const QJsonArray &devices)
{
    quint16 count = 0;

    for (auto it = devices.begin(); it != devices.end(); it++)
    {
        QJsonObject json = it->toObject();
        Device device;

        if (!byName(json.value("name").toString()).isNull())
            continue;

        device = parse(json);

        if (device.isNull())
            continue;

        append(device);
        count++;
    }

    if (count)
        logInfo << count << "devices loaded";
}

QJsonArray DeviceList::serialize(void)
{
    QJsonArray array;

    for (int i = 0; i < count(); i++)
    {
        const Device &device = at(i);
        QJsonObject json = {{"type", device->type()}, {"portId", device->portId()}, {"slaveId", device->slaveId()}, {"baudRate", device->baudRate()}, {"pollInterval", device->pollInterval()}, {"requestTimeout", device->requestTimeout()}, {"replyTimeout", device->replyTimeout()}, {"name", device->name()}, {"active", device->active()}, {"cloud", device->cloud()}, {"discovery", device->discovery()}};

        if (device->type() == "customController")
        {
            Custom::Controller* controller = reinterpret_cast <Custom::Controller*> (device.data());
            QJsonArray items;
            QJsonObject options;

            for (int i = 0; i < controller->items().count(); i++)
            {
                const Custom::Item &item = controller->items().at(i);
                QJsonObject json = {{"expose", item->expose()}, {"type", item->type()}, {"address", item->address()}, {"registerType", m_registerTypes.valueToKey(static_cast <int> (item->registerType()))}, {"read", item->read()}};

                if (item->registerType() == Custom::RegisterType::holding || item->registerType() == Custom::RegisterType::input)
                {
                    json.insert("dataType", m_dataTypes.valueToKey(static_cast <int> (item->dataType())));
                    json.insert("byteOrder", m_byteOrders.valueToKey(static_cast <int> (item->byteOrder())));
                    json.insert("divider", item->divider());
                }

                items.append(json);
            }

            for (auto it = controller->options().begin(); it != controller->options().end(); it++)
            {
                QString expose = it.key().split('_').value(0);
                QMap <QString, QVariant> option, map;

                if (it.value().type() != QVariant::Map)
                {
                    options.insert(it.key(), QJsonValue::fromVariant(it.value()));
                    continue;
                }

                option = m_exposeOptions.value(expose).toMap();
                map = it.value().toMap();

                for (auto it = option.begin(); it != option.end(); it++)
                    if (map.value(it.key()) == it.value())
                        map.remove(it.key());

                if (map.isEmpty())
                    continue;

                options.insert(it.key(), QJsonObject::fromVariantMap(map));
            }

            if (!items.isEmpty())
                json.insert("items", items);

            if (!options.isEmpty())
                json.insert("options", options);
        }

        if (!device->note().isEmpty())
            json.insert("note", device->note());

        array.append(json);
    }

    return array;
}

void DeviceList::writeDatabase(void)
{
    QJsonObject json = {{"devices", serialize()}, {"names", m_names}, {"timestamp", QDateTime::currentSecsSinceEpoch()}, {"version", SERVICE_VERSION}};
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    bool check = true;

    emit statusUpdated(json);

    if (!m_sync)
        return;

    m_sync = false;

    if (!m_file.open(QFile::WriteOnly))
    {
        logWarning << "Database not stored, file" << m_file.fileName() << "open error:" << m_file.errorString();
        return;
    }

    if (m_file.write(data) != data.length())
    {
        logWarning << "Database not stored, file" << m_file.fileName() << "write error";
        check = false;
    }

    m_file.close();

    if (!check)
        return;

    system("sync");
}
