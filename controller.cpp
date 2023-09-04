#include "controller.h"
#include "device.h"
#include "logger.h"

Controller::Controller(const QString &configFile) : HOMEd(configFile)
{
    QList <QString> keys = getConfig()->allKeys();

    m_names = getConfig()->value("mqtt/names", false).toBool();

    for (int i = 0; i < keys.count(); i++)
    {
        const QString &key = keys.at(i);

        if (QRegExp("^port-\\d+/port$").exactMatch(key))
        {
            quint8 id = static_cast <quint8> (key.split('/').value(0).split('-').value(1).toInt());
            Port port(new PortThread(id, getConfig()->value(key).toString(), m_devices));
            connect(port.data(), &PortThread::updateAvailability, this, &Controller::updateAvailability);
            m_ports.insert(id, port);

            //

            if (id != 1)
                continue;

            {
                Device device(new Devices::SwitchController(1, 11, 115200, 0, "Switch Controller"));
                connect(device.data(), &DeviceObject::endpointUpdated, this, &Controller::endpointUpdated);
                device->init(device);
                m_devices.append(device);
            }

            {
                Device device(new Devices::RelayController(1, 12, 115200, 2000, "Relay Controller"));
                connect(device.data(), &DeviceObject::endpointUpdated, this, &Controller::endpointUpdated);
                device->init(device);
                m_devices.append(device);
            }

            //
        }
    }
}

Device Controller::findDevice(const QString &deviceName)
{
    for (int i = 0; i < m_devices.count(); i++)
    {
        const Device &device = m_devices.at(i);

        if (device->name() != deviceName && device->address() != deviceName)
            continue;

        return device;
    }

    return Device();
}

void Controller::mqttConnected(void)
{
    logInfo << "MQTT connected";
    mqttSubscribe("homed/td/modbus/#");

    // if (getConfig()->value("homeassistant/enabled", false).toBool())
    //     mqttSubscribe(m_haStatus);

    for (int i = 0; i < m_devices.count(); i++)
    {
        const Device &device = m_devices.at(i);
        device->publishExposes(this, device->address(), device->address().replace('.', '_'));
    }
}

void Controller::mqttReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString subTopic = topic.name().replace(mqttTopic(), QString());
    QJsonObject json = QJsonDocument::fromJson(message).object();

    if (subTopic.startsWith("td/modbus/"))
    {
        QList <QString> list = subTopic.split('/');
        Device device = findDevice(list.value(2));

        if (device.isNull())
            return;

        for (auto it = json.begin(); it != json.end(); it++)
        {
            if (!it.value().toVariant().isValid())
                continue;

            device->enqueueAction(static_cast <quint8> (list.value(3).toInt()), it.key(), it.value().toVariant());
        }
    }
}

void Controller::updateAvailability(DeviceObject *device)
{
    QString status = device->availability() == Availability::Online ? "online" : "offline";
    mqttPublish(mqttTopic("device/modbus/%1").arg(m_names ? device->name() : device->address()), {{"status", status}}, true);
    logInfo << "Device" << device->name() << "is" << status;
}

void Controller::endpointUpdated(quint8 endpointId)
{
    DeviceObject *device = reinterpret_cast <DeviceObject*> (sender());
    Endpoint endpoint = device->endpoints().value(endpointId);
    QString topic = mqttTopic("fd/modbus/%1").arg(m_names ? device->name() : device->address());

    if (endpointId)
        topic.append(QString("/%1").arg(endpointId));

    mqttPublish(topic, QJsonObject::fromVariantMap(endpoint->status()));
}
