#ifndef DEVICES_WB_SENSOR_H
#define DEVICES_WB_SENSOR_H

#define WBMSW_OCCUPANCY_THRESHOLD           200
#define WBMSW_OCCUPANCY_TIMEOUT             60

#include "device.h"

namespace WirenBoard
{
    class WBM1w2 : public DeviceObject
    {

    public:

        WBM1w2(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        quint16 m_singleClick[2], m_doubleClick[2];

    };

    class WBMs : public DeviceObject
    {

    public:

        WBMs(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    };

    class WBMsw : public DeviceObject
    {

    public:

        WBMsw(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name), m_timer(new QTimer(this)), m_time(0) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        QTimer *m_timer;

        bool m_output[3];
        qint64 m_time;

    private slots:

        void update(void);

    };

    class WBMai6 : public DeviceObject
    {

    public:

        WBMai6(quint8 portId, quint8 slaveId, quint32 baudRate, quint32 pollInterval, quint32 requestTimeout, quint32 replyTimeout, const QString &name) :
            DeviceObject(portId, slaveId, baudRate, pollInterval, requestTimeout, replyTimeout, name) {}

        void init(const Device &device, const QMap <QString, QVariant> &exposeOptions) override;
        void enqueueAction(quint8 endpointId, const QString &name, const QVariant &data) override;
        void startPoll(void) override;

        QByteArray pollRequest(void) override;
        void parseReply(const QByteArray &reply) override;

    private:

        struct Settings
        {
            quint16 type;
            double  inputMultiplier;
            double  valueMultiplier;
            bool    nChannel;
        };

        Settings m_pChannel[6];
        Settings m_nChannel[6];

        QList <QString> m_types;
        QList <Settings> m_settings;

    };
}

#endif
