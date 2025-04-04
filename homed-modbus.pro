include(../homed-common/homed-common.pri)
include(../homed-common/homed-endpoint.pri)

HEADERS += \
    controller.h \
    device.h \
    devices/custom.h \
    devices/native.h \
    devices/r4pin08.h \
    devices/wirenboard.h \
    modbus.h \
    port.h

SOURCES += \
    controller.cpp \
    device.cpp \
    devices/custom.cpp \
    devices/native.cpp \
    devices/r4pin08.cpp \
    devices/wirenboard.cpp \
    modbus.cpp \
    port.cpp

QT += serialport
