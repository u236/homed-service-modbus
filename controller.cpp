#include "controller.h"
#include "device.h"
#include "logger.h"

Controller::Controller(const QString &configFile) : HOMEd(configFile), m_timer(new QTimer(this)), m_devices(new DeviceList(getConfig(), this)), m_events(QMetaEnum::fromType <Event> ())
{
    QList <QString> keys = getConfig()->allKeys();

    logInfo << "Starting version" << SERVICE_VERSION;
    logInfo << "Configuration file is" << getConfig()->fileName();

    m_timer->setSingleShot(true);

    connect(m_timer, &QTimer::timeout, this, &Controller::updateProperties);
    connect(m_devices, &DeviceList::statusUpdated, this, &Controller::statusUpdated);

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

    m_names = getConfig()->value("mqtt/names", false).toBool();
    m_haStatus = getConfig()->value("homeassistant/status", "homeassistant/status").toString();
}

void Controller::publishExposes(DeviceObject *device, bool remove)
{
    device->publishExposes(this, device->address(), device->address().replace('.', '_'), remove);

    if (remove)
        return;

    m_timer->start(UPDATE_PROPERTIES_DELAY);
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
            mqttPublish(mqttTopic("device/modbus/%1").arg(m_names ? device->name() : device->address()), QJsonObject(), true);
            remove = true;
            break;

        case Event::added:
        case Event::updated:

            if (device->availability() != Availability::Unknown)
                mqttPublish(mqttTopic("device/modbus/%1").arg(m_names ? device->name() : device->address()), {{"status", device->availability() == Availability::Online ? "online" : "offline"}}, true);

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
    logInfo << "MQTT connected";

    mqttSubscribe(mqttTopic("command/modbus"));
    mqttSubscribe(mqttTopic("td/modbus/#"));

    if (getConfig()->value("homeassistant/enabled", false).toBool())
        mqttSubscribe(m_haStatus);

    for (int i = 0; i < m_devices->count(); i++)
        publishExposes(m_devices->at(i).data());

    m_devices->store();
}

void Controller::mqttReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString subTopic = topic.name().replace(mqttTopic(), QString());
    QJsonObject json = QJsonDocument::fromJson(message).object();

    if (subTopic == "command/modbus")
    {
        QString action = json.value("action").toString();

        if (action == "updateDevice")
        {
            int index = -1;
            QJsonObject data = json.value("data").toObject();
            QString name = data.value("name").toString().trimmed();
            Device device = m_devices->byName(json.value("device").toString(), &index), other = m_devices->byName(name);

            if (device != other && !other.isNull())
            {
                logWarning << "Device" << name << "update failed, name already in use";
                publishEvent(name, Event::nameDuplicate);
                return;
            }

            if (!device.isNull())
                deviceEvent(device.data(), Event::aboutToRename);

            device = m_devices->parse(data);

            if (device.isNull())
            {
                logWarning << "Device" << name << "update failed, data is incomplete";
                publishEvent(name, Event::incorrectData);
                return;
            }

            if (index < 0)
            {
                m_devices->append(device);
                logInfo << "Device" << device->name() << "successfully added";
                deviceEvent(device.data(), Event::added);
            }
            else
            {
                m_devices->replace(index, device);
                logInfo << "Device" << device->name() << "successfully updated";
                deviceEvent(device.data(), Event::updated);
            }

            connect(device.data(), &DeviceObject::endpointUpdated, this, &Controller::endpointUpdated);
        }
        else if (action == "removeDevice")
        {
            int index = -1;
            const Device &device = m_devices->byName(json.value("device").toString(), &index);

            if (index < 0)
                return;

            m_devices->removeAt(index);
            logInfo << "Device" << device->name() << "removed";
            deviceEvent(device.data(), Event::removed);
        }
        else
            return;

        m_devices->store(true);
    }
    else if (subTopic.startsWith("td/modbus/"))
    {
        QList <QString> list = subTopic.split('/');
        Device device = m_devices->byName(list.value(2));

        if (device.isNull())
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
    mqttPublish(mqttTopic("device/modbus/%1").arg(m_names ? device->name() : device->address()), {{"status", status}}, true);
    logInfo << "Device" << device->name() << "is" << status;
}

void Controller::updateProperties(void)
{
    for (int i = 0; i < m_devices->count(); i++)
    {
        const Device &device = m_devices->at(i);

        for (auto it = device->endpoints().begin(); it != device->endpoints().end(); it++)
            endpointUpdated(device.data(), it.key());
    }
}

void Controller::endpointUpdated(DeviceObject *device, quint8 endpointId)
{
    Endpoint endpoint = device->endpoints().value(endpointId);
    QString topic = mqttTopic("fd/modbus/%1").arg(m_names ? device->name() : device->address());

    if (endpointId)
        topic.append(QString("/%1").arg(endpointId));

    mqttPublish(topic, QJsonObject::fromVariantMap(endpoint->status()));
}

void Controller::statusUpdated(const QJsonObject &json)
{
    mqttPublish(mqttTopic("status/modbus"), json, true);
}
