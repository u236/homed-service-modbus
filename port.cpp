#include <QEventLoop>
#include <QSerialPort>
#include "logger.h"
#include "device.h"
#include "port.h"

PortThread::PortThread(quint8 portId, const QString &portName, DeviceList *devices) : QThread(nullptr), m_portId(portId), m_portName(portName), m_devices(devices)
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

    m_serial->setBaudRate(device->baudRate());
    m_serial->write(request);

    timer.setSingleShot(true);
    timer.start(100); // TODO: use config here?
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
        logWarning << "Can't open" << m_portName;
        return;
    }

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
    m_receiveTimer->start(20); // TODO: use timeout from device settings?
}

void PortThread::readyRead(void)
{
    m_replyData = m_serial->readAll();
    emit replyReceived();
}

void PortThread::poll(void)
{
    QByteArray request;

    for (int i = 0; i < m_devices->count(); i++)
    {
        const Device &device = m_devices->at(i);
        bool force = false;

        if (device->portId() != m_portId)
            continue;

        while (!device->actionQueue().isEmpty())
        {
            sendRequest(device, device->actionQueue().dequeue());
            force = true;
        }

        if (!force && device->pollTime() + device->pollInterval() > QDateTime::currentMSecsSinceEpoch())
            continue;

        device->startPoll();

        while (!(request = device->pollRequest()).isEmpty())
        {
            sendRequest(device, request);

            if (device->errorCount())
                break;

            device->parseReply(m_replyData);
        }
    }
}
