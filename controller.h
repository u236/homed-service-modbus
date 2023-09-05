#ifndef CONTROLLER_H
#define CONTROLLER_H

#define SERVICE_VERSION             "1.0.0"
#define UPDATE_PROPERTIES_DELAY     1000

#include <QMetaEnum>
#include "homed.h"
#include "port.h"

class Controller : public HOMEd
{
    Q_OBJECT

public:

    Controller(const QString &configFile);

    enum class Event
    {
        nameDuplicate,
        incompleteData,
        aboutToRename,
        added,
        updated,
        removed
    };

    Q_ENUM(Event)

private:

    QTimer *m_timer;
    DeviceList *m_devices;
    QMap <quint8, Port> m_ports;

    QMetaEnum m_events;

    bool m_names;
    QString m_haStatus;

    void publishExposes(DeviceObject *device, bool remove = false);
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
