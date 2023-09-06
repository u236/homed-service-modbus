include(../homed-common/homed-common.pri)
include(../homed-common/homed-endpoint.pri)

HEADERS += \
    controller.h \
    device.h \
    devices/native.h \
    devices/wirenboard.h \
    modbus.h \
    port.h

SOURCES += \
    controller.cpp \
    device.cpp \
    devices/native.cpp \
    devices/wirenboard.cpp \
    modbus.cpp \
    port.cpp

QT += serialport
