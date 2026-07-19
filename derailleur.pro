QT       += core gui network
QT       += gui-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Capture the git commit at qmake time so netplay peers can verify they are
# running the same build. Re-run qmake to refresh it. Passed as a bare token and
# stringized in code to avoid cross-platform quote-escaping issues.
GIT_HASH = $$system(git -C $$PWD rev-parse --short HEAD)
isEmpty(GIT_HASH): GIT_HASH = unknown
DEFINES += DR_GIT_HASH=$$GIT_HASH

DEFINES += QRETRO_INTERNAL

CONFIG += QRETRO_CONFIG_NO_GAMEPAD
CONFIG += QRETRO_CONFIG_NO_LOCATION
CONFIG += QRETRO_CONFIG_NO_MIDI
CONFIG += QRETRO_CONFIG_NO_SENSORS
CONFIG += QRETRO_CONFIG_NO_SYSTEMINFO

include(QRetro/QRetro.pri)

SOURCES += \
    DrCommon.cpp \
    DrDebug.cpp \
    DrDownloader.cpp \
    DrGuest.cpp \
    DrHost.cpp \
    MarioPartyN64Host.cpp \
    DrInputStore.cpp \
    DrMinigameFilter.cpp \
    DrNetplay.cpp \
    DrNetplayWidget.cpp \
    DrOverlay.cpp \
    DrToolWindow.cpp \
    DrGuestList.cpp \
    DrLogger.cpp \
    DrRetro.cpp \
    QRetroInputBackendShared.cpp \
    hosts/MarioParty1Host.cpp \
    hosts/MarioParty2Host.cpp \
    hosts/MarioParty3Host.cpp \
    guests/BanjoTooie.cpp \
    guests/CoreDolphin.cpp \
    guests/Kirby64.cpp \
    guests/KirbyAirRide.cpp \
    guests/MarioKart64.cpp \
    guests/MarioKartDoubleDash.cpp \
    guests/MarioParty1.cpp \
    guests/MarioParty2.cpp \
    guests/MarioParty3.cpp \
    guests/MarioPartyAdvance.cpp \
    guests/MarioPartyE.cpp \
    guests/MarioParty4.cpp \
    guests/MarioPartyN64.cpp \
    guests/MarioParty5.cpp \
    guests/MarioParty6.cpp \
    guests/MarioParty7.cpp \
    guests/MarioParty9.cpp \
    guests/MarioPartyGcn.cpp \
    guests/MarioTennis.cpp \
    guests/PokemonStadium2.cpp \
    guests/SmashRemix.cpp \
    guests/SonicShuffle.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrCommon.h \
    DrDebug.h \
    DrDownloader.h \
    DrGuest.h \
    DrGuestList.h \
    DrHost.h \
    MarioPartyN64Host.h \
    DrInputStore.h \
    DrMinigameFilter.h \
    DrNetplay.h \
    DrNetplayWidget.h \
    DrLogger.h \
    DrOverlay.h \
    DrToolWindow.h \
    DrRetro.h \
    QRetroInputBackendShared.h \
    hosts/MarioParty1Host.h \
    hosts/MarioParty2Host.h \
    hosts/MarioParty3Host.h \
    guests/BanjoTooie.h \
    guests/CoreDolphin.h \
    guests/DolphinGuest.h \
    guests/Kirby64.h \
    guests/KirbyAirRide.h \
    guests/MarioKart64.h \
    guests/MarioKartDoubleDash.h \
    guests/MarioParty1.h \
    guests/MarioParty2.h \
    guests/MarioParty3.h \
    guests/MarioPartyAdvance.h \
    guests/MarioPartyE.h \
    guests/MarioParty4.h \
    guests/MarioPartyN64.h \
    guests/MarioParty5.h \
    guests/MarioParty6.h \
    guests/MarioParty7.h \
    guests/MarioParty9.h \
    guests/MarioPartyGcn.h \
    guests/MarioTennis.h \
    guests/PokemonStadium2.h \
    guests/SmashRemix.h \
    guests/SonicShuffle.h \
    mainwindow.h

RESOURCES += assets.qrc

win32:exists($$PWD/assets/icon.ico) {
    RC_ICONS = assets/icon.ico
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
