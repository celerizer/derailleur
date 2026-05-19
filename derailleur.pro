QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

include(QRetro/QRetro.pri)

SOURCES += \
    DrCommon.cpp \
    DrDebug.cpp \
    DrGuest.cpp \
    DrHost.cpp \
    DrOverlay.cpp \
    DrGuestList.cpp \
    DrLogger.cpp \
    DrParty.cpp \
    DrRetro.cpp \
    hosts/MarioParty3.cpp \
    guests/CoreDolphin.cpp \
    guests/MarioKart64.cpp \
    guests/MarioParty4.cpp \
    guests/MarioPartyN64.cpp \
    guests/MarioParty5.cpp \
    guests/MarioParty6.cpp \
    guests/MarioParty7.cpp \
    guests/MarioPartyGcn.cpp \
    guests/SmashRemix.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrCommon.h \
    DrDebug.h \
    DrGuest.h \
    DrGuestList.h \
    DrHost.h \
    DrLogger.h \
    DrOverlay.h \
    DrRetro.h \
    hosts/MarioParty3.h \
    guests/CoreDolphin.h \
    guests/MarioKart64.h \
    guests/MarioParty4.h \
    guests/MarioPartyN64.h \
    guests/MarioParty5.h \
    guests/MarioParty6.h \
    guests/MarioParty7.h \
    guests/MarioPartyGcn.h \
    guests/SmashRemix.h \
    mainwindow.h

RESOURCES += assets.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
