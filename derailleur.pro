QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

CONFIG += QRETRO_CONFIG_NO_MIDI

include(QRetro/QRetro.pri)

SOURCES += \
    DrGuest.cpp \
    DrOverlay.cpp \
    DrGuestList.cpp \
    DrLogger.cpp \
    DrParty.cpp \
    guests/MarioKart64.cpp \
    guests/MarioParty4.cpp \
    guests/MarioParty5.cpp \
    guests/MarioParty6.cpp \
    guests/MarioParty7.cpp \
    guests/MarioPartyGcn.cpp \
    guests/SmashRemix.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrCommon.h \
    DrOverlay.h \
    DrGuest.h \
    DrGuestList.h \
    DrLogger.h \
    guests/MarioKart64.h \
    guests/MarioParty4.h \
    guests/MarioParty5.h \
    guests/MarioParty6.h \
    guests/MarioParty7.h \
    guests/MarioPartyGcn.h \
    guests/SmashRemix.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
