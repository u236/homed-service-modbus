include(../homed-common/homed-common.pri)
include(../homed-common/homed-endpoint.pri)

HEADERS += \
    devices/common.h \
    controller.h \
    device.h \
    modbus.h \
    port.h

SOURCES += \
    devices/common.cpp \
    controller.cpp \
    device.cpp \
    modbus.cpp \
    port.cpp

QT += serialport
