QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

CONFIG += QRETRO_CONFIG_NO_MIDI

include(QRetro/QRetro.pri)

SOURCES += \
    DrGuest.cpp \
    DrGuestList.cpp \
    DrParty.cpp \
    guests/MarioKart64.cpp \
    guests/MarioParty4.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrCommon.h \
    DrGuest.h \
    DrGuestList.h \
    guests/MarioKart64.h \
    guests/MarioParty4.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
