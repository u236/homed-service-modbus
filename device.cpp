#include "devices/native.h"
#include "controller.h"
#include "device.h"
#include "logger.h"

DeviceList::DeviceList(QSettings *config, QObject *parent) : QObject(parent)
{
    m_types =
    {
        "homedRelayController",
        "homedSwitchController"
    };

    m_file.setFileName(config->value("device/database", "/opt/homed-modbus/database.json").toString());
}

DeviceList::~DeviceList(void)
{
    store(true);
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
    QString name = json.value("name").toString();
    quint8 portId = static_cast <quint8> (json.value("portId").toInt()), slaveId = static_cast <quint8> (json.value("slaveId").toInt());
    quint32 baudRate = json.value("baudRate").toInt(), pollInterval = json.value("pollInterval").toInt();
    Device device;

    if (name.isEmpty() || !portId || !slaveId || !baudRate)
        return device;

    switch (m_types.indexOf(json.value("type").toString()))
    {
        case 0: device = Device(new Native::RelayController(portId, slaveId, baudRate, pollInterval, name)); break;
        case 1: device = Device(new Native::SwitchController(portId, slaveId, baudRate, pollInterval, name)); break;
    }

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
        array.append(QJsonObject {{"type", device->type()}, {"portId", device->portId()}, {"slaveId", device->slaveId()}, {"baudRate", device->baudRate()}, {"pollInterval", device->pollInterval()}, {"name", device->name()}});
    }

    return array;
}
