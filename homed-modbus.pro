include(../homed-common/homed-common.pri)
include(../homed-common/homed-endpoint.pri)

HEADERS += \
    controller.h \
    device.h \
    modbus.h \
    port.h

SOURCES += \
    controller.cpp \
    device.cpp \
    modbus.cpp \
    port.cpp

QT += serialport
