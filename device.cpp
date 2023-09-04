
#include "devices/common.h"
#include "controller.h"
#include "device.h"
#include "logger.h"

DeviceList::DeviceList(QSettings *config, QObject *parent) : QObject(parent)
{
    m_file.setFileName(config->value("device/database", "/opt/homed-modbus/database.json").toString());
}

DeviceList::~DeviceList(void)
{
//    store(true);
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
    QList <QString> list = json.value("address").toString().split('.');
    QString type = json.value("type").toString(), name = json.value("name").toString();
    quint8 portId = static_cast <quint8> (list.value(0).toInt()), slaveId = static_cast <quint8> (list.value(1).toInt());
    quint32 baudRate = static_cast <quint32> (json.value("baudRate").toInt()), pollInterval = static_cast <quint32> (json.value("pollInterval").toInt());
    Device device;

    if (name.isEmpty() || !portId || !slaveId || !baudRate)
        return device;

    if (type == "homedRelayController")
        device = Device(new Devices::RelayController(portId, slaveId, baudRate, pollInterval, name));
    else if (type == "homedSwitchController")
        device = Device(new Devices::SwitchController(portId, slaveId, baudRate, pollInterval, name));

    if (!device.isNull())
        device->init(device);

    return device;
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
    QJsonObject json = {{"devices", serialize()}, {"timestamp", QDateTime::currentSecsSinceEpoch()}, {"version", SERVICE_VERSION}};
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    bool check = true;

    emit statusUpdated(json);

    if (!sync)
        return;

    if (!m_file.open(QFile::WriteOnly))
    {
        logWarning << "Database not stored, file" << m_file.fileName() << "open error:" << m_file.errorString();
        return;
    }

    if (m_file.write(data) != data.length())
    {
        logWarning << "Database not stored, file" << m_file.fileName() << "open error:" << m_file.errorString();
        check = false;
    }

    m_file.close();

    if (!check)
        return;

    system("sync");
}

void DeviceList::unserialize(const QJsonArray &devices)
{
    quint16 count = 0;

    for (auto it = devices.begin(); it != devices.end(); it++)
    {
        QJsonObject json = it->toObject();
        Device device = byName(json.value("name").toString());

        if (!device.isNull())
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
        array.append(QJsonObject {{"type", device->type()}, {"address", device->address()}, {"baudRate", QJsonValue::fromVariant(device->baudRate())}, {"pollInterval", QJsonValue::fromVariant(device->pollInterval())}, {"name", device->name()}});
    }

    return array;
}
