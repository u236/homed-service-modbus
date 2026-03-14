include(../homed-common/homed-color.pri)
include(../homed-common/homed-common.pri)
include(../homed-common/homed-endpoint.pri)
include(../homed-common/homed-parser.pri)

HEADERS += \
    controller.h \
    device.h \
    devices/custom.h \
    devices/eletechsup.h \
    devices/kincony.h \
    devices/native.h \
    devices/neptun.h \
    devices/other.h \
    devices/peacefair.h \
    devices/wb-common.h \
    devices/wb-dimmer.h \
    devices/wb-map.h \
    devices/wb-relay.h \
    devices/wb-sensor.h \
    modbus.h \
    port.h

SOURCES += \
    controller.cpp \
    device.cpp \
    devices/custom.cpp \
    devices/eletechsup.cpp \
    devices/kincony.cpp \
    devices/native.cpp \
    devices/neptun.cpp \
    devices/other.cpp \
    devices/peacefair.cpp \
    devices/wb-common.cpp \
    devices/wb-dimmer.cpp \
    devices/wb-map.cpp \
    devices/wb-relay.cpp \
    devices/wb-sensor.cpp \
    modbus.cpp \
    port.cpp

QT += serialport
