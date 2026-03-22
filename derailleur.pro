QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

CONFIG += QRETRO_CONFIG_NO_MIDI

include(QRetro/QRetro.pri)

SOURCES += \
    DrGuest.cpp \
    DrParty.cpp \
    guests/MarioParty4.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrCommon.h \
    DrGuest.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
