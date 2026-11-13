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

GIT_HASH_FULL = $$system(git -C $$PWD rev-parse HEAD)
isEmpty(GIT_HASH_FULL): GIT_HASH_FULL = unknown
DEFINES += DR_GIT_HASH_FULL=$$GIT_HASH_FULL

DEFINES += QRETRO_INTERNAL

CONFIG += QRETRO_CONFIG_NO_GAMEPAD
CONFIG += QRETRO_CONFIG_NO_LOCATION
CONFIG += QRETRO_CONFIG_NO_MIDI
CONFIG += QRETRO_CONFIG_NO_SENSORS
CONFIG += QRETRO_CONFIG_NO_SYSTEMINFO

include(QRetro/QRetro.pri)

INCLUDEPATH += $$PWD

# Mario Party 64 unpacker/repacker
INCLUDEPATH += $$PWD/partystuffer/include
SOURCES += \
    partystuffer/src/audio.c \
    partystuffer/src/character_name.c \
    partystuffer/src/codec.c \
    partystuffer/src/engine.c \
    partystuffer/src/library.c \
    partystuffer/src/runtime.c \
    partystuffer/src/util.c
HEADERS += \
    partystuffer/include/partystuffer.h

SOURCES += \
    DrChallenge.cpp \
    DrDebug.cpp \
    DrPaths.cpp \
    DrRandom.cpp \
    DrSettings.cpp \
    DrSettingsPage.cpp \
    DrDownloader.cpp \
    DrGuest.cpp \
    DrHost.cpp \
    DrInputStore.cpp \
    DrMinigameFilter.cpp \
    DrNetplay.cpp \
    DrNetplayWidget.cpp \
    DrOverlay.cpp \
    DrToolWindow.cpp \
    DrGuestList.cpp \
    DrHostPicker.cpp \
    DrStartup.cpp \
    DrLogger.cpp \
    DrRetro.cpp \
    DrRetroDolphin.cpp \
    QRetroInputBackendShared.cpp \
    hosts/DrHosts.cpp \
    hosts/DrHostRetro.cpp \
    hosts/impl/MarioPartyN64Host.cpp \
    hosts/impl/MarioParty1Host.cpp \
    hosts/impl/MarioParty2Host.cpp \
    hosts/impl/MarioParty3Host.cpp \
    hosts/impl/MarioPartyGcnHost.cpp \
    hosts/impl/MarioParty4Host.cpp \
    hosts/impl/MarioParty5Host.cpp \
    hosts/impl/MarioParty6Host.cpp \
    hosts/impl/MarioParty7Host.cpp \
    hosts/impl/MarioParty8Host.cpp \
    hosts/impl/MarioPartyDSHost.cpp \
    hosts/impl/SonicShuffleHand.cpp \
    hosts/impl/SonicShuffleHost.cpp \
    guests/DrGuests.cpp \
    guests/DrGuestRetro.cpp \
    guests/DrGuestDolphin.cpp \
    guests/DrGuestGcn.cpp \
    guests/DrGuestWii.cpp \
    guests/impl/BanjoTooie.cpp \
    guests/impl/Kirby64.cpp \
    guests/impl/KirbyAirRide.cpp \
    guests/impl/MarioGolf.cpp \
    guests/impl/MarioKart64.cpp \
    guests/impl/MarioKartDoubleDash.cpp \
    guests/impl/MarioKartWii.cpp \
    guests/impl/MarioParty1.cpp \
    guests/impl/MarioParty2.cpp \
    guests/impl/MarioParty3.cpp \
    guests/impl/MarioPartyAdvance.cpp \
    guests/impl/MarioPartyE.cpp \
    guests/impl/MarioParty4.cpp \
    guests/impl/MarioPartyN64.cpp \
    guests/impl/MarioParty5.cpp \
    guests/impl/MarioParty6.cpp \
    guests/impl/MarioParty7.cpp \
    guests/impl/MarioParty8.cpp \
    guests/impl/MarioParty9.cpp \
    guests/impl/MarioPartyGcn.cpp \
    guests/impl/MarioTennis.cpp \
    guests/impl/MaryKateAndAshleySweet16LicensedToDrive.cpp \
    guests/impl/PokemonStadium.cpp \
    guests/impl/PokemonStadium2.cpp \
    guests/impl/SmashRemix.cpp \
    guests/impl/SonicShuffle.cpp \
    guests/impl/StarFox64.cpp \
    guests/impl/SuperMarioBros3.cpp \
    guests/impl/WiiParty.cpp \
    guests/impl/YoshisIsland.cpp \
    guests/impl/DonkeyKong64.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DrChallenge.h \
    DrCommon.h \
    DrDebug.h \
    DrPaths.h \
    DrRandom.h \
    DrSettings.h \
    DrSettingsPage.h \
    DrDownloader.h \
    DrGuest.h \
    DrGuestList.h \
    DrHostPicker.h \
    DrStartup.h \
    DrHost.h \
    DrInputStore.h \
    DrMinigameFilter.h \
    DrNetplay.h \
    DrNetplayWidget.h \
    DrLogger.h \
    DrOverlay.h \
    DrToolWindow.h \
    DrRetro.h \
    DrRetroDS.h \
    DrRetroDolphin.h \
    DrRetroGcn.h \
    DrRetroN64.h \
    DrRetroNES.h \
    DrRetroWii.h \
    QRetroInputBackendShared.h \
    hosts/DrHosts.h \
    hosts/DrHostDS.h \
    hosts/DrHostN64.h \
    hosts/DrHostRetro.h \
    hosts/impl/MarioPartyN64Host.h \
    hosts/impl/MarioParty1Host.h \
    hosts/impl/MarioParty2Host.h \
    hosts/impl/MarioParty3Host.h \
    hosts/impl/MarioPartyGcnHost.h \
    hosts/impl/MarioParty4Host.h \
    hosts/impl/MarioParty5Host.h \
    hosts/impl/MarioParty6Host.h \
    hosts/impl/MarioParty7Host.h \
    hosts/impl/MarioParty8Host.h \
    hosts/impl/MarioPartyDSHost.h \
    hosts/impl/SonicShuffleHand.h \
    hosts/impl/SonicShuffleHost.h \
    guests/DrGuests.h \
    guests/DrGuestN64.h \
    guests/DrGuestNES.h \
    guests/DrGuestRetro.h \
    guests/DrGuestDolphin.h \
    guests/DrGuestGcn.h \
    guests/DrGuestWii.h \
    guests/impl/BanjoTooie.h \
    guests/impl/Kirby64.h \
    guests/impl/KirbyAirRide.h \
    guests/impl/MarioGolf.h \
    guests/impl/MarioKart64.h \
    guests/impl/MarioKartDoubleDash.h \
    guests/impl/MarioKartWii.h \
    guests/impl/MarioParty1.h \
    guests/impl/MarioParty2.h \
    guests/impl/MarioParty3.h \
    guests/impl/MarioPartyAdvance.h \
    guests/impl/MarioPartyE.h \
    guests/impl/MarioParty4.h \
    guests/impl/MarioPartyN64.h \
    guests/impl/MarioParty5.h \
    guests/impl/MarioParty6.h \
    guests/impl/MarioParty7.h \
    guests/impl/MarioParty8.h \
    guests/impl/MarioParty9.h \
    guests/impl/MarioPartyGcn.h \
    guests/impl/MarioTennis.h \
    guests/impl/MaryKateAndAshleySweet16LicensedToDrive.h \
    guests/impl/PokemonStadium.h \
    guests/impl/PokemonStadium2.h \
    guests/impl/SmashRemix.h \
    guests/impl/SonicShuffle.h \
    guests/impl/StarFox64.h \
    guests/impl/SuperMarioBros3.h \
    guests/impl/WiiParty.h \
    guests/impl/YoshisIsland.h \
    guests/impl/DonkeyKong64.h \
    mainwindow.h

RESOURCES += assets.qrc

win32:exists($$PWD/assets/icon.ico) {
    RC_ICONS = assets/icon.ico
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
