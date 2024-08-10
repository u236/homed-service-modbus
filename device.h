#ifndef DEVICE_H
#define DEVICE_H

#define STORE_DATABASE_DELAY    20

#include <QFile>
#include <QQueue>
#include "endpoint.h"

class EndpointObject : public AbstractEndpointObject
{

public:

    EndpointObject(quint8 id, const Device &device) : AbstractEndpointObject(id, device) {}

    inline QMap <QString, QVariant> &status(void) { return m_status; }
    inline QMap <QString, QVariant> &buffer(void) { return m_buffer; }

private:

    QMap <QString, QVariant> m_status, m_buffer;

};

class DeviceObject : public AbstractDeviceObject
{
    Q_OBJECT

public:

    DeviceObject(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, const QString &name) :
        AbstractDeviceObject(name), m_portId(portId), m_slaveId(slaveId), m_baudRate(baudRate), m_pollInterval(pollInterval), m_pollTime(0), m_errorCount(0), m_sequence(0), m_polling(false), m_fullPoll(true) {}

    virtual void init(const Device &) {}
    virtual void enqueueAction(quint8, const QString &, const QVariant &) {}
    virtual void startPoll(void) = 0;

    virtual QByteArray pollRequest(void) = 0;
    virtual void parseReply(const QByteArray &) = 0;

    inline QString type(void) { return m_type; }
    inline QString address(void) { return QString("%1.%2").arg(m_portId).arg(m_slaveId); }

    inline quint8 portId(void) { return m_portId; }
    inline quint8 slaveId(void) { return m_slaveId; }
    inline qint32 baudRate(void) { return m_baudRate; }
    inline qint32 pollInterval(void) { return m_pollInterval; }

    inline qint64 pollTime(void) { return m_pollTime; }
    inline void resetPollTime(void) { m_pollTime = 0; }

    inline quint32 errorCount(void) { return m_errorCount; }
    inline void increaseErrorCount(void) { m_errorCount++; }
    inline void resetErrorCount(void) { m_errorCount = 0; }

    inline QQueue <QByteArray> &actionQueue(void) { return m_actionQueue; }

protected:

    QString m_type, m_address;

    quint8 m_portId, m_slaveId;
    qint32 m_baudRate, m_pollInterval;

    qint64 m_pollTime;
    quint32 m_errorCount;

    quint8 m_sequence;
    bool m_polling, m_fullPoll;
    QQueue <QByteArray> m_actionQueue;

    void updateEndpoints(void);

signals:

    void endpointUpdated(DeviceObject *device, quint8 endpointId);

};

class DeviceList : public QObject, public QList <Device>
{
    Q_OBJECT

public:

    DeviceList(QSettings *config, QObject *parent);
    ~DeviceList(void);

    inline bool names(void) { return m_names; }
    inline void setNames(bool value) { m_names = value; }

    void init(void);
    void store(bool sync = false);

    Device byName(const QString &name, int *index = nullptr);
    Device parse(const QJsonObject &json);

private:

    QTimer *m_timer;

    QList <QString> m_types;
    QFile m_file;
    bool m_names, m_sync;

    void unserialize(const QJsonArray &devices);
    QJsonArray serialize(void);

private slots:

    void writeDatabase(void);

signals:

    void statusUpdated(const QJsonObject &json);

};

#endif
