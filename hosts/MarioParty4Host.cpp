#include "MarioParty4Host.h"

#include <asm/gecko/mp4.h>

#include <QRetroDirectories.h>

static DrGcnHostConfig makeConfig()
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 4 (USA) (Rev 1).rvz").toStdString();

  config.cheats.cave = MP4_CAVE;
  config.cheats.cave_addr = MP4_CAVE_ADDR;
  config.cheats.cave_size = MP4_CAVE_SIZE;
  config.cheats.cheat_board = MP4_HOOK_BOARD;

  return config;
}

MarioParty4Host::MarioParty4Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
}
