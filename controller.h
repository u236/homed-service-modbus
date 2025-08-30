#ifndef CONTROLLER_H
#define CONTROLLER_H

#define SERVICE_VERSION             "1.2.2"
#define UPDATE_PROPERTIES_DELAY     1000

#include <QMetaEnum>
#include "homed.h"
#include "port.h"

class Controller : public HOMEd
{
    Q_OBJECT

public:

    enum class Command
    {
        restartService,
        updateDevice,
        removeDevice,
        getProperties
    };

    enum class Event
    {
        nameDuplicate,
        incompleteData,
        aboutToRename,
        added,
        updated,
        removed
    };

    Controller(const QString &configFile);

    Q_ENUM(Command)
    Q_ENUM(Event)

private:

    QTimer *m_timer;
    DeviceList *m_devices;
    QMap <quint8, Port> m_ports;

    QMetaEnum m_commands, m_events;
    QString m_haPrefix, m_haStatus;
    bool m_haEnabled;

    void publishExposes(DeviceObject *device, bool remove = false);
    void publishProperties(const Device &device);
    void publishEvent(const QString &name, Event event);
    void deviceEvent(DeviceObject *device, Event event);

private slots:

    void mqttConnected(void) override;
    void mqttReceived(const QByteArray &message, const QMqttTopicName &topic) override;

    void updateAvailability(DeviceObject *device);
    void updateProperties(void);

    void endpointUpdated(DeviceObject *device, quint8 endpointId);
    void statusUpdated(const QJsonObject &json);

};

#endif
