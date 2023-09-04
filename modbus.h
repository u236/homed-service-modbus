#ifndef MODBUS_H
#define MODBUS_H

#include <QByteArray>

class Modbus
{

public:

    enum FunctionCode
    {
        ReadHoldingRegisters    = 0x03,
        ReadInputRegisters      = 0x04,
        WriteSingleCoil         = 0x05,
        WriteSingleRegister     = 0x06,
        WriteMultipleRegisters  = 0x10,
        ReportSlaveId           = 0x11
    };

    enum ExceptionCode
    {
        IllegalFunction         = 0x01,
        IllegalDataAddress      = 0x02,
        IllegalDataValue        = 0x03,
        SlaveDeviceFailure      = 0x04,
        SlaveDeviceBusy         = 0x06
    };

    enum ReplyStatus
    {
        Ok,
        WrongLength,
        WrongSlaveAddress,
        WrongFunctionCode,
        BadCRC,
        Exception
    };

    static QByteArray makeRequest(quint8 slaveAddress, FunctionCode functionCode, quint16 registerAddress = 0, quint16 registerValue = 0, quint16 *registerData = nullptr);
    static ReplyStatus parseReply(quint8 slaveAddress, FunctionCode functionCode, const QByteArray &reply, quint16 *registerData = nullptr);

private:

    static quint16 crc16(const QByteArray &data);

};

#endif
