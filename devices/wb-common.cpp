#include "expose.h"
#include "wb-common.h"

void WirenBoard::Common::init(const Device &device, const QMap <QString, QVariant> &)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose slaveId(new NumberObject("slaveId")), baudRate(new SelectObject("baudRate")), serialNumber(new SensorObject("serialNumber"));

    m_type = "wbCommon";
    m_description = "Wiren Board Device Common Settings";

    slaveId->setParent(endpoint.data());
    endpoint->exposes().append(slaveId);

    baudRate->setParent(endpoint.data());
    endpoint->exposes().append(baudRate);

    serialNumber->setParent(endpoint.data());
    endpoint->exposes().append(serialNumber);

    m_endpoints.insert(0, endpoint);

    m_options.insert("slaveId",      QMap <QString, QVariant> {{"type", "number"}, {"min", 1}, {"max", 247}, {"collapse", "true"}, {"icon", "mdi:cog"}});
    m_options.insert("baudRate",     QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"}}, {"collapse", "true"}, {"icon", "mdi:cog"}});
    m_options.insert("serialNumber", QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:numeric"}});
}

void WirenBoard::Common::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"slaveId", "baudRate"};

    switch (actions.indexOf(name))
    {
        case 0: // slaveId
            m_pendingSlaveId = static_cast <quint8> (data.toInt());
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0080, static_cast <quint16> (m_pendingSlaveId)));
            break;

        case 1: // baudRate
            m_pendingBaudRate = static_cast <quint32> (data.toInt());
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x006E, static_cast <quint16> (m_pendingBaudRate / 100)));
            break;
    }
}

void WirenBoard::Common::actionFinished(void)
{
    bool check = false;

    if (m_pendingSlaveId && m_slaveId != m_pendingSlaveId)
    {
        m_slaveId = m_pendingSlaveId;
        check = true;
    }

    if (m_pendingBaudRate && m_baudRate != m_pendingBaudRate)
    {
        m_baudRate = m_pendingBaudRate;
        check = true;
    }

    if (!check)
        return;

    emit deviceUpdated(this);
}

void WirenBoard::Common::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = 0;
    m_polling = true;
}

QByteArray WirenBoard::Common::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0080, 1);
        case 1: return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x006E, 1);
        case 2: return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters,   0x010E, 2);
    }

    updateEndpoints();
    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;

    return QByteArray();
}

void WirenBoard::Common::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("slaveId", value);
            break;
        }

        case 1:
        {
            quint16 value;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, &value) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("baudRate", value * 100);
            break;
        }

        case 2:
        {
            quint16 data[2];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("serialNumber", Modbus::toUInt32BE(data));
            break;
        }
    }

    m_sequence++;
}

void WirenBoard::WBUps::init(const Device &device, const QMap <QString, QVariant> &exposeOptions)
{
    Endpoint endpoint(new EndpointObject(0, device));
    Expose battety(new SensorObject("battery")), batteryStatus(new SensorObject("batteryStatus")), temperature(new SensorObject("temperature")), temperatureStatus(new SensorObject("temperatureStatus")), action(new SensorObject("action")), inputVoltage(new SensorObject("inputVoltage")), outputVoltage(new SensorObject("outputVoltage")), batteryVoltage(new SensorObject("batteryVoltage")), batteryCurrent(new SensorObject("batteryCurrent")), chargeCurrent(new SensorObject("chargeCurrent")), dischargeCurrent(new SensorObject("dischargeCurrent")), operationMode(new SelectObject("operationMode")), outputVoltageLimit(new NumberObject("outputVoltageLimit")), chargeCurrentLimit(new NumberObject("chargeCurrentLimit"));

    m_type = "wbUps";
    m_description = "Wiren Board WB-UPS v3 Backup Power Supply";

    battety->setParent(endpoint.data());
    endpoint->exposes().append(battety);

    batteryStatus->setParent(endpoint.data());
    endpoint->exposes().append(batteryStatus);

    temperature->setParent(endpoint.data());
    endpoint->exposes().append(temperature);

    temperatureStatus->setParent(endpoint.data());
    endpoint->exposes().append(temperatureStatus);

    action->setParent(endpoint.data());
    endpoint->exposes().append(action);

    inputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(inputVoltage);

    outputVoltage->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltage);

    batteryVoltage->setParent(endpoint.data());
    endpoint->exposes().append(batteryVoltage);

    batteryCurrent->setParent(endpoint.data());
    endpoint->exposes().append(batteryCurrent);

    chargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrent);

    dischargeCurrent->setParent(endpoint.data());
    endpoint->exposes().append(dischargeCurrent);

    operationMode->setParent(endpoint.data());
    endpoint->exposes().append(operationMode);

    outputVoltageLimit->setParent(endpoint.data());
    endpoint->exposes().append(outputVoltageLimit);

    chargeCurrentLimit->setParent(endpoint.data());
    endpoint->exposes().append(chargeCurrentLimit);

    m_endpoints.insert(0, endpoint);
    updateOptions(exposeOptions);

    m_options.insert("batteryStatus",      QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:battery-charging"}});
    m_options.insert("temperatureStatus",  QMap <QString, QVariant> {{"type", "sensor"}, {"icon", "mdi:thermometer"}});
    m_options.insert("action",             QMap <QString, QVariant> {{"type", "sensor"}, {"enum", QList <QVariant> {"singleClick", "doubleClick"}}, {"icon", "mdi:gesture-double-tap"}});

    m_options.insert("inputVoltage",       exposeOptions.value("voltage"));
    m_options.insert("outputVoltage",      exposeOptions.value("voltage"));
    m_options.insert("batteryVoltage",     exposeOptions.value("voltage"));

    m_options.insert("batteryCurrent",     exposeOptions.value("current"));
    m_options.insert("chargeCurrent",      exposeOptions.value("current"));
    m_options.insert("dischargeCurrent",   exposeOptions.value("current"));

    m_options.insert("operationMode",      QMap <QString, QVariant> {{"type", "select"}, {"enum", QList <QVariant> {"auto", "manual"}}, {"icon", "mdi:cog"}});
    m_options.insert("outputVoltageLimit", QMap <QString, QVariant> {{"type", "number"}, {"min", 9}, {"max", 25.6}, {"step", 0.1}, {"unit", "V"}, {"icon", "mdi:sine-wave"}});
    m_options.insert("chargeCurrentLimit", QMap <QString, QVariant> {{"type", "number"}, {"min", 0.3}, {"max", 2}, {"step", 0.1}, {"unit", "A"}, {"icon", "mdi:current-ac"}});
}

void WirenBoard::WBUps::enqueueAction(quint8, const QString &name, const QVariant &data)
{
    QList <QString> actions = {"operationMode", "outputVoltageLimit", "chargeCurrentLimit"};
    int index = actions.indexOf(name);

    switch (index)
    {
        case 0: // operationMode
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, 0x0010, data.toString() == "manual" ? 0x0001 : 0x0000));
            break;

        case 1: // outputVoltageLimit
        case 2: // chargeCurrentLimit
            m_actionQueue.enqueue(m_modbus->makeRequest(m_slaveId, Modbus::WriteSingleRegister, index == 1 ? 0x0011 : 0x0012, static_cast <quint16> (data.toDouble() * 1000)));
            break;

        default:
            return;
    }

    m_fullPoll = true;
}

void WirenBoard::WBUps::startPoll(void)
{
    if (m_polling)
        return;

    m_sequence = m_fullPoll ? 0 : 1;
    m_polling = true;
}

QByteArray WirenBoard::WBUps::pollRequest(void)
{
    switch (m_sequence)
    {
        case 0:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadHoldingRegisters, 0x0010, 3);

        case 1:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, 0x0000, 10);

        case 2 ... 3:
            return m_modbus->makeRequest(m_slaveId, Modbus::ReadInputRegisters, m_sequence == 2 ? 0x01D0 : 0x01F0, 1);
    }

    updateEndpoints();
    m_endpoints.value(0)->buffer().remove("action");

    m_pollTime = QDateTime::currentMSecsSinceEpoch();
    m_polling = false;
    m_fullPoll = false;

    return QByteArray();
}

void WirenBoard::WBUps::parseReply(const QByteArray &reply)
{
    switch (m_sequence)
    {
        case 0:
        {
            quint16 data[3];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadHoldingRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            m_endpoints.value(0)->buffer().insert("operationMode",      data[0] ? "manual" : "auto");
            m_endpoints.value(0)->buffer().insert("outputVoltageLimit", data[1] / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrentLimit", data[2] / 1000.0);
            break;
        }

        case 1:
        {
            quint16 data[10];

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, data) != Modbus::ReplyStatus::Ok)
                break;

            switch (data[0])
            {
                case 0: m_endpoints.value(0)->buffer().insert("batteryStatus", "charge"); break;
                case 1: m_endpoints.value(0)->buffer().insert("batteryStatus", "charged"); break;
                case 2: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharge"); break;
                case 3: m_endpoints.value(0)->buffer().insert("batteryStatus", "discharged"); break;
                case 4: m_endpoints.value(0)->buffer().insert("batteryStatus", "alarm"); break;
            }

            switch (data[1])
            {
                case 0: m_endpoints.value(0)->buffer().insert("temperatureStatus", "normal"); break;
                case 1: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overcool"); break;
                case 2: m_endpoints.value(0)->buffer().insert("temperatureStatus", "cool"); break;
                case 3: m_endpoints.value(0)->buffer().insert("temperatureStatus", "heat"); break;
                case 4: m_endpoints.value(0)->buffer().insert("temperatureStatus", "overheat"); break;
            }

            m_endpoints.value(0)->buffer().insert("inputVoltage",     data[2] / 1000.0);
            m_endpoints.value(0)->buffer().insert("outputVoltage",    data[3] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryVoltage",   data[4] / 1000.0);
            m_endpoints.value(0)->buffer().insert("batteryCurrent",   static_cast <qint16> (data[5]) / 1000.0);
            m_endpoints.value(0)->buffer().insert("chargeCurrent",    data[6] / 1000.0);
            m_endpoints.value(0)->buffer().insert("dischargeCurrent", data[7] / 1000.0);
            m_endpoints.value(0)->buffer().insert("battery",          data[8] / 100.0);
            m_endpoints.value(0)->buffer().insert("temperature",      data[9] / 100.0);
            break;
        }

        case 2 ... 3:
        {
            quint16 value, &counter = m_sequence == 2 ? m_singleClick : m_doubleClick;

            if (m_modbus->parseReply(m_slaveId, Modbus::ReadInputRegisters, reply, &value) != Modbus::ReplyStatus::Ok || counter == value)
                break;

            if (!m_fullPoll)
                m_endpoints.value(0)->buffer().insert("action", m_sequence == 2 ? "singleClick" : "doubleClick");

            counter = value;
            break;
        }
    }

    m_sequence++;
}
