#ifndef CONTROLLER_H
#define CONTROLLER_H

#define SERVICE_VERSION     "1.0.0"

#include "homed.h"
#include "port.h"

class Controller : public HOMEd
{
    Q_OBJECT

public:

    Controller(const QString &configFile);

private:

    bool m_names;
    QMap <quint8, Port> m_ports;

    Device findDevice(const QString &deviceName);
    QList <Device> m_devices;

private slots:

    void mqttConnected(void) override;
    void mqttReceived(const QByteArray &message, const QMqttTopicName &topic) override;

    void updateAvailability(DeviceObject *device);
    void endpointUpdated(quint8 endpointId);

};

#endif
