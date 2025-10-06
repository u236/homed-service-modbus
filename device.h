#ifndef DEVICE_H
#define DEVICE_H

#define DEFAULT_ENDPOINT        0
#define STORE_DATABASE_DELAY    20

#include <QFile>
#include <QMetaEnum>
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

    DeviceObject(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
        AbstractDeviceObject(name), m_portId(portId), m_slaveId(slaveId), m_baudRate(baudRate), m_pollInterval(pollInterval), m_requestTimeout(requestTimeout), m_replyTimeout(replyTimeout), m_pollTime(0), m_errorCount(0), m_sequence(0), m_polling(false), m_fullPoll(true) {}

    virtual void init(const Device &, const QMap <QString, QVariant> &) = 0;
    virtual void enqueueAction(quint8, const QString &, const QVariant &) {}
    virtual void actionFinished(void) {}
    virtual void startPoll(void) = 0;

    virtual QByteArray pollRequest(void) = 0;
    virtual void parseReply(const QByteArray &) = 0;

    inline QString type(void) { return m_type; }
    inline QString address(void) { return QString("%1.%2").arg(m_portId).arg(m_slaveId); }

    inline quint8 portId(void) { return m_portId; }
    inline quint8 slaveId(void) { return m_slaveId; }
    inline qint32 baudRate(void) { return m_baudRate; }
    inline qint32 pollInterval(void) { return m_pollInterval; }
    inline qint32 requestTimeout(void) { return m_requestTimeout; }
    inline qint32 replyTimeout(void) { return m_replyTimeout; }

    inline qint64 pollTime(void) { return m_pollTime; }
    inline void resetPollTime(void) { m_pollTime = 0; }

    inline quint32 errorCount(void) { return m_errorCount; }
    inline void increaseErrorCount(void) { m_errorCount++; }
    inline void resetErrorCount(void) { m_errorCount = 0; }

    inline void resetPoll(void) { m_polling = false; m_fullPoll = true; }
    inline QQueue <QByteArray> &actionQueue(void) { return m_actionQueue; }

protected:

    QString m_type, m_address;

    quint8 m_portId, m_slaveId;
    quint32 m_baudRate, m_pollInterval, m_requestTimeout, m_replyTimeout;

    qint64 m_pollTime;
    quint32 m_errorCount;

    quint8 m_sequence;
    bool m_polling, m_fullPoll;
    QQueue <QByteArray> m_actionQueue;

    void updateOptions(const QMap <QString, QVariant> &exposeOptions);
    void updateEndpoints(void);

signals:

    void deviceUpdated(DeviceObject *device);
    void endpointUpdated(DeviceObject *device, quint8 endpointId);

};

class DeviceList : public QObject, public QList <Device>
{
    Q_OBJECT

public:

    enum class DeviceType
    {
        customController,
        homedCommon,
        homedRelayController,
        homedSwitchController,
        n4dsa02,
        r4pin08m0,
        r4pin08m1,
        r4pin08m2,
        r4pin08m3,
        r4pin08m4,
        wbM1w2,
        wbCommon,
        wbMs,
        wbMsw,
        wbMai6,
        wbMap3ev,
        wbMap3e,
        wbMap6s,
        wbMap12e,
        wbMap12h,
        wbMrwm2,
        wbMrm2,
        wbMr3,
        wbMr6,
        wbMr6p,
        wbLed0,
        wbLed1,
        wbLed2,
        wbLed16,
        wbLed17,
        wbLed18,
        wbLed32,
        wbLed33,
        wbLed34,
        wbLed256,
        wbLed512,
        wbMdm,
        wbUps,
        neptunSmartPlus,
        jth2d1,
        t13,
        m0701s
    };

    DeviceList(QSettings *config, QObject *parent);
    ~DeviceList(void);

    inline bool names(void) { return m_names; }
    inline void setNames(bool value) { m_names = value; }

    void init(void);
    void store(bool sync = false);

    Device byName(const QString &name, int *index = nullptr);
    Device parse(const QJsonObject &json);

    Q_ENUM(DeviceType)

private:

    QTimer *m_timer;

    QMetaEnum m_deviceTypes, m_registerTypes, m_dataTypes, m_byteOrders;
    QFile m_file;
    bool m_names, m_sync;

    QMap <QString, QVariant> m_exposeOptions;
    QList <QString> m_specialExposes;

    void unserialize(const QJsonArray &devices);
    QJsonArray serialize(void);

private slots:

    void writeDatabase(void);

signals:

    void statusUpdated(const QJsonObject &json);

};

inline QDebug operator << (QDebug debug, DeviceObject *device) { return debug << "device" << device->name(); }
inline QDebug operator << (QDebug debug, const Device &device) { return debug << "device" << device->name(); }

#endif
