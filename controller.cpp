#include "controller.h"
#include "device.h"
#include "logger.h"

Controller::Controller(const QString &configFile) : HOMEd(configFile), m_timer(new QTimer(this)), m_devices(new DeviceList(getConfig(), this)), m_commands(QMetaEnum::fromType <Command> ()), m_events(QMetaEnum::fromType <Event> ())
{
    QList <QString> keys = getConfig()->allKeys();

    logInfo << "Starting version" << SERVICE_VERSION;
    logInfo << "Configuration file is" << getConfig()->fileName();

    m_haPrefix = getConfig()->value("homeassistant/prefix", "homeassistant").toString();
    m_haStatus = getConfig()->value("homeassistant/status", "homeassistant/status").toString();
    m_haEnabled = getConfig()->value("homeassistant/enabled", false).toBool();

    connect(m_timer, &QTimer::timeout, this, &Controller::updateProperties);
    connect(m_devices, &DeviceList::statusUpdated, this, &Controller::statusUpdated);

    m_timer->setSingleShot(true);

    m_devices->setNames(getConfig()->value("mqtt/names", false).toBool());
    m_devices->init();

    for (int i = 0; i < m_devices->count(); i++)
    {
        const Device &device = m_devices->at(i);
        connect(device.data(), &DeviceObject::endpointUpdated, this, &Controller::endpointUpdated);
    }

    for (int i = 0; i < keys.count(); i++)
    {
        const QString &key = keys.at(i);

        if (QRegExp("^port-\\d+/port$").exactMatch(key))
        {
            quint8 id = static_cast <quint8> (key.split('/').value(0).split('-').value(1).toInt());
            Port port(new PortThread(id, getConfig()->value(key).toString(), m_devices));
            connect(port.data(), &PortThread::updateAvailability, this, &Controller::updateAvailability);
            m_ports.insert(id, port);
        }
    }
}

void Controller::publishExposes(DeviceObject *device, bool remove)
{
    device->publishExposes(this, device->address(), QString("%1_%2").arg(uniqueId(), device->address().replace('.', '_')), m_haPrefix, m_haEnabled, m_devices->names(), remove);

    if (remove)
        return;

    m_timer->start(UPDATE_PROPERTIES_DELAY);
}

void Controller::publishProperties(const Device &device)
{
    for (auto it = device->endpoints().begin(); it != device->endpoints().end(); it++)
        endpointUpdated(device.data(), it.key());
}

void Controller::publishEvent(const QString &name, Event event)
{
    mqttPublish(mqttTopic("event/modbus"), {{"device", name}, {"event", m_events.valueToKey(static_cast <int> (event))}});
}

void Controller::deviceEvent(DeviceObject *device, Event event)
{
    bool check = true, remove = false;

    switch (event)
    {
        case Event::aboutToRename:
        case Event::removed:
            mqttPublish(mqttTopic("device/modbus/%1").arg(m_devices->names() ? device->name() : device->address()), QJsonObject(), true);
            remove = true;
            break;

        case Event::added:
        case Event::updated:

            if (device->availability() != Availability::Unknown)
                mqttPublish(mqttTopic("device/modbus/%1").arg(m_devices->names() ? device->name() : device->address()), {{"status", device->availability() == Availability::Online ? "online" : "offline"}}, true);

            break;

        default:
            check = false;
            break;
    }

    if (check)
        publishExposes(device, remove);

    publishEvent(device->name(), event);
}

void Controller::mqttConnected(void)
{
    mqttSubscribe(mqttTopic("command/modbus"));
    mqttSubscribe(mqttTopic("td/modbus/#"));

    for (int i = 0; i < m_devices->count(); i++)
        publishExposes(m_devices->at(i).data());

    if (m_haEnabled)
    {
        mqttPublishDiscovery("Modbus", SERVICE_VERSION, m_haPrefix);
        mqttSubscribe(m_haStatus);
    }

    m_devices->store();
    mqttPublishStatus();
}

void Controller::mqttReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString subTopic = topic.name().replace(mqttTopic(), QString());
    QJsonObject json = QJsonDocument::fromJson(message).object();

    if (subTopic == "command/modbus")
    {
        switch (static_cast <Command> (m_commands.keyToValue(json.value("action").toString().toUtf8().constData())))
        {
            case Command::restartService:
            {
                logWarning << "Restart request received...";
                mqttPublish(mqttTopic("command/custom"), QJsonObject(), true);
                QCoreApplication::exit(EXIT_RESTART);
                break;
            }

            case Command::updateDevice:
            {
                int index = -1;
                QJsonObject data = json.value("data").toObject();
                QString name = data.value("name").toString().trimmed();
                Device device = m_devices->byName(json.value("device").toString(), &index), other = m_devices->byName(name);

                if (device != other && !other.isNull())
                {
                    logWarning << device << "update failed, name already in use";
                    publishEvent(name, Event::nameDuplicate);
                    break;
                }

                if (!device.isNull() && device->name() != name)
                    deviceEvent(device.data(), Event::aboutToRename);

                device = m_devices->parse(data);

                if (device.isNull())
                {
                    logWarning << device << "update failed, data is incomplete";
                    publishEvent(name, Event::incompleteData);
                    break;
                }

                if (index >= 0)
                {
                    m_devices->replace(index, device);
                    logInfo << device << "successfully updated";
                    deviceEvent(device.data(), Event::updated);
                }
                else
                {
                    m_devices->append(device);
                    logInfo << device << "successfully added";
                    deviceEvent(device.data(), Event::added);
                }

                connect(device.data(), &DeviceObject::endpointUpdated, this, &Controller::endpointUpdated);
                m_devices->store(true);
                break;
            }

            case Command::removeDevice:
            {
                int index = -1;
                const Device &device = m_devices->byName(json.value("device").toString(), &index);

                if (index >= 0)
                {
                    m_devices->removeAt(index);
                    logInfo << device << "removed";
                    deviceEvent(device.data(), Event::removed);
                    m_devices->store(true);
                }

                break;
            }

            case Command::getProperties:
            {
                Device device = m_devices->byName(json.value("device").toString());

                if (!device.isNull())
                    publishProperties(device);

                break;
            }
        }
    }
    else if (subTopic.startsWith("td/modbus/"))
    {
        QList <QString> list = subTopic.split('/');
        Device device = m_devices->byName(list.value(2));

        if (device.isNull() || !device->active())
            return;

        for (auto it = json.begin(); it != json.end(); it++)
        {
            if (!it.value().toVariant().isValid())
                continue;

            device->enqueueAction(static_cast <quint8> (list.value(3).toInt()), it.key(), it.value().toVariant());
        }
    }
    else if (topic.name() == m_haStatus)
    {
        if (message != "online")
            return;

        m_timer->start(UPDATE_PROPERTIES_DELAY);
    }
}

void Controller::updateAvailability(DeviceObject *device)
{
    QString status = device->availability() == Availability::Online ? "online" : "offline";
    mqttPublish(mqttTopic("device/modbus/%1").arg(m_devices->names() ? device->name() : device->address()), {{"status", status}}, true);
    logInfo << "Device" << device->name() << "is" << status;
}

void Controller::updateProperties(void)
{
    for (int i = 0; i < m_devices->count(); i++)
        publishProperties(m_devices->at(i));
}

void Controller::endpointUpdated(DeviceObject *device, quint8 endpointId)
{
    Endpoint endpoint = device->endpoints().value(endpointId);

    if (!endpoint->status().isEmpty())
    {
        QString topic = mqttTopic("fd/modbus/%1").arg(m_devices->names() ? device->name() : device->address());

        if (endpointId)
            topic.append(QString("/%1").arg(endpointId));

        mqttPublish(topic, QJsonObject::fromVariantMap(endpoint->status()));
    }
}

void Controller::statusUpdated(const QJsonObject &json)
{
    mqttPublish(mqttTopic("status/modbus"), json, true);
}
