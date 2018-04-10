TEMPLATE = app
CONFIG += console
CONFIG -= qt

CONFIG += static
QMAKE_CFLAGS += -std=c99

#CONFIG += USE_ARM_PLATFORM

HEADERS += \
    bits.h \
    crc.h \
    Gb28181PsMux.h \
    psmux.h \
    psmuxcommon.h \
    psmuxdef.h \
    psmuxstream.h \
    PsMuxStreamType.h
    

SOURCES += \
    Gb28181PsMux.cpp \
    psmux.cpp \
    PsMuxExample.cpp \
    psmuxstream.cpp
    
