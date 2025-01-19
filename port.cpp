#include <QEventLoop>
#include <QSerialPort>
#include "logger.h"
#include "device.h"
#include "port.h"

PortThread::PortThread(quint8 portId, const QString &portName, bool debug, DeviceList *devices) : QThread(nullptr), m_portId(portId), m_portName(portName), m_debug(debug), m_devices(devices)
{
    connect(this, &PortThread::started, this, &PortThread::threadStarted);
    connect(this, &PortThread::finished, this, &PortThread::threadFinished);

    moveToThread(this);
    start();
}

PortThread::~PortThread(void)
{
    quit();
    wait();
}

void PortThread::sendRequest(const Device &device, const QByteArray &request)
{
    Availability availability = device->availability();
    QEventLoop loop;
    QTimer timer;

    connect(this, &PortThread::replyReceived, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_replyData.clear();
    m_replyTimeout = device->replyTimeout();

    m_serial->setBaudRate(device->baudRate());
    m_serial->write(request);

    logDebug(m_debug) << "Port" << m_portId << "serial data sent:" << request.toHex(':');

    timer.setSingleShot(true);
    timer.start(device->requestTimeout());

    loop.exec();

    if (!timer.isActive())
        device->increaseErrorCount();
    else
        device->resetErrorCount();

    device->setAvailability(device->errorCount() > 2 ? Availability::Offline : Availability::Online);

    if (availability == device->availability())
        return;

    emit updateAvailability(device.data());
}

void PortThread::threadStarted(void)
{
    m_serial = new QSerialPort(m_portName, this);
    m_receiveTimer = new QTimer;
    m_pollTimer = new QTimer;

    connect(m_serial, &QSerialPort::readyRead, this, &PortThread::startTimer);
    connect(m_receiveTimer, &QTimer::timeout, this, &PortThread::readyRead);
    connect(m_pollTimer, &QTimer::timeout, this, &PortThread::poll);

    if (!m_serial->open(QIODevice::ReadWrite))
    {
        logWarning << "Port" << m_portId << "can't open" << m_serial->portName();
        return;
    }

    logInfo << "Port" << m_portId << "successfully opened" << m_serial->portName();
    m_receiveTimer->setSingleShot(true);
    m_pollTimer->start(1);
}

void PortThread::threadFinished(void)
{
    m_pollTimer->stop();
    m_serial->close();
}

void PortThread::startTimer(void)
{
    m_replyData.append(m_serial->readAll());
    m_receiveTimer->start(m_replyTimeout);
}

void PortThread::readyRead(void)
{
    logDebug(m_debug) << "Port" << m_portId << "serial data received:" << m_replyData.toHex(':');
    emit replyReceived();
}

void PortThread::poll(void)
{
    QByteArray request;

    for (int i = 0; i < m_devices->count(); i++)
    {
        const Device &device = m_devices->at(i);

        if (device->portId() != m_portId || !device->active())
            continue;

        if (!device->actionQueue().isEmpty())
        {
            QByteArray request = device->actionQueue().dequeue();

            while (device->availability() != Availability::Offline)
            {
                sendRequest(device, request);

                if (device->errorCount())
                    continue;

                break;
            }

            device->resetPollTime();
            continue;
        }

        if (device->pollTime() + device->pollInterval() > QDateTime::currentMSecsSinceEpoch())
            continue;

        device->startPoll();
        request = device->pollRequest();

        if (request.isEmpty())
            continue;

        sendRequest(device, request);

        if (device->errorCount())
            continue;

        device->parseReply(m_replyData);
    }
}
