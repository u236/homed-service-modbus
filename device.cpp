#include "devices/native.h"
#include "devices/wirenboard.h"
#include "controller.h"
#include "device.h"
#include "logger.h"

void DeviceObject::updateEndpoints(void)
{
    for (int i = 0; i < m_endpoints.count(); i++)
    {
        auto it = m_endpoints.find(i);

        if (it.value()->status() == it.value()->buffer())
            continue;

        it.value()->status() = it.value()->buffer();
        emit endpointUpdated(this, it.key());
    }
}

DeviceList::DeviceList(QSettings *config, QObject *parent) : QObject(parent), m_timer(new QTimer(this)), m_sync(false)
{
    m_types =
    {
        "homedRelayController",
        "homedSwitchController",
        "wbMap3e",
        "wbMap12h"
    };

    m_file.setFileName(config->value("device/database", "/opt/homed-modbus/database.json").toString());

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
        case 2: device = Device(new WirenBoard::WBMap3e(portId, slaveId, baudRate, pollInterval, name)); break;
        case 3: device = Device(new WirenBoard::WBMap12h(portId, slaveId, baudRate, pollInterval, name)); break;
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
        device->init(device);
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
        QJsonObject json = {{"type", device->type()}, {"portId", device->portId()}, {"slaveId", device->slaveId()}, {"baudRate", device->baudRate()}, {"pollInterval", device->pollInterval()}, {"name", device->name()}, {"active", device->active()}, {"cloud", device->cloud()}, {"discovery", device->discovery()}};

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
