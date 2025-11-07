#include <netinet/tcp.h>
#include <QEventLoop>
#include <QSerialPort>
#include "logger.h"
#include "device.h"
#include "port.h"

PortThread::PortThread(quint8 portId, const QString &portName, bool tcp, bool debug, DeviceList *devices) : QThread(nullptr), m_portId(portId), m_portName(portName), m_tcp(tcp), m_debug(debug), m_serialError(false), m_connected(false), m_devices(devices)
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

void PortThread::init(void)
{
    if (m_device == m_serial)
    {
        if (m_serial->isOpen())
            m_serial->close();

        if (!m_serial->open(QIODevice::ReadWrite))
        {
            logWarning << this << "can't open" << m_serial->portName();
            return;
        }

        logInfo << this << "serial port" << m_serial->portName() << "opened successfully";
        m_serial->clear();
        m_pollTimer->start(1);
    }
    else
    {
        if (m_adddress.isNull() && !m_port)
        {
            logWarning << this << "has invalid connection address or port number";
            return;
        }

        if (m_connected)
            m_socket->disconnectFromHost();

        m_socket->connectToHost(m_adddress, m_port);
    }
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

    if (m_device == m_serial)
        m_serial->setBaudRate(device->baudRate());

    m_device->write(request);
    logDebug(m_debug) << this << "serial data sent:" << request.toHex(':');

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

    if (device->availability() == Availability::Offline)
        device->resetPoll();

    emit updateAvailability(device.data());
}

void PortThread::threadStarted(void)
{
    m_serial = new QSerialPort(this);
    m_socket = new QTcpSocket(this);

    m_receiveTimer = new QTimer(this);
    m_resetTimer = new QTimer(this);
    m_pollTimer = new QTimer(this);

    if (!m_portName.startsWith("tcp://"))
    {
        m_device = m_serial;

        m_serial->setPortName(m_portName);
        m_serial->setDataBits(QSerialPort::Data8);
        m_serial->setParity(QSerialPort::NoParity);
        m_serial->setStopBits(QSerialPort::OneStop);

        connect(m_serial, &QSerialPort::errorOccurred, this, &PortThread::serialError);
    }
    else
    {
        QList <QString> list = QString(m_portName).remove("tcp://").split(':');

        m_device = m_socket;
        m_adddress = QHostAddress(list.value(0));
        m_port = static_cast <quint16> (list.value(1).toInt());

        connect(m_socket, &QTcpSocket::errorOccurred, this, &PortThread::socketError);
        connect(m_socket, &QTcpSocket::connected, this, &PortThread::socketConnected);
    }

    connect(m_device, &QSerialPort::readyRead, this, &PortThread::startTimer);
    connect(m_receiveTimer, &QTimer::timeout, this, &PortThread::readyRead);
    connect(m_resetTimer, &QTimer::timeout, this, &PortThread::reset);
    connect(m_pollTimer, &QTimer::timeout, this, &PortThread::poll);

    m_receiveTimer->setSingleShot(true);
    m_resetTimer->setSingleShot(true);

    init();
}

void PortThread::threadFinished(void)
{
    if (m_connected)
        m_socket->disconnectFromHost();
}

void PortThread::serialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::SerialPortError::NoError)
    {
        m_serialError = false;
        return;
    }

    if (!m_serialError)
        logWarning << this << "serial port error:" << error;

    m_resetTimer->start(RESET_TIMEOUT);
    m_pollTimer->stop();
    m_serialError = true;
}

void PortThread::socketError(QAbstractSocket::SocketError error)
{
    logWarning << this << "connection error:" << error;
    m_resetTimer->start(RESET_TIMEOUT);
    m_pollTimer->stop();
    m_connected = false;
}

void PortThread::socketConnected(void)
{
    int descriptor = m_socket->socketDescriptor(), keepAlive = 1, interval = 10, count = 3;

    setsockopt(descriptor, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
    setsockopt(descriptor, SOL_TCP, TCP_KEEPIDLE, &interval, sizeof(interval));
    setsockopt(descriptor, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(descriptor, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));

    logInfo << this << "successfully connected to" << QString("%1:%2").arg(m_adddress.toString()).arg(m_port);
    m_socket->readAll();
    m_pollTimer->start(1);
    m_connected = true;
}

void PortThread::startTimer(void)
{
    m_replyData.append(m_device->readAll());
    m_receiveTimer->start(m_replyTimeout);
}

void PortThread::readyRead(void)
{
    if (m_replyData.length() < 4)
        return;

    logDebug(m_debug) << this << "serial data received:" << m_replyData.toHex(':');
    emit replyReceived();
}

void PortThread::reset(void)
{
    init();
}

void PortThread::poll(void)
{
    QByteArray request;

    for (int i = 0; i < m_devices->count(); i++)
    {
        const Device &device = m_devices->at(i);

        if (device->portId() != m_portId || !device->active())
            continue;

        device->modbus()->setTcp(m_tcp);

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

            device->actionFinished();
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
