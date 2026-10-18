#include "DrHosts.h"

#include "impl/MarioParty1Host.h"
#include "impl/MarioParty2Host.h"
#include "impl/MarioParty3Host.h"
#include "impl/MarioParty4Host.h"
#include "impl/MarioParty5Host.h"
#include "impl/MarioParty6Host.h"
#include "impl/MarioParty7Host.h"
#include "impl/MarioParty8Host.h"
#include "impl/MarioPartyAdvanceHost.h"
#include "impl/MarioPartyDSHost.h"
#include "impl/SonicShuffleHost.h"

template <typename T>
static DrHost *dr_create_host(QObject *parent, const std::string &rom)
{
  return new T(parent, rom);
}

template <typename T>
static DrHost *dr_create_host_stock(QObject *parent, const std::string &)
{
  return new T(parent);
}

static const DrHostEntry DR_HOSTS[] = {
  { DR_GAME_MARIOPARTY1, "Mario Party", "marioparty1", "Mario Party (USA).z64",
    DR_CORE_MUPEN64PLUSNEXT, dr_create_host<MarioParty1Host> },
  { DR_GAME_MARIOPARTY2, "Mario Party 2", "marioparty2", "Mario Party 2 (USA).z64",
    DR_CORE_MUPEN64PLUSNEXT, dr_create_host<MarioParty2Host> },
  { DR_GAME_MARIOPARTY3, "Mario Party 3", "marioparty3", "Mario Party 3 (USA).z64",
    DR_CORE_MUPEN64PLUSNEXT, dr_create_host<MarioParty3Host> },
  { DR_GAME_MARIOPARTY4, "Mario Party 4", "marioparty4", "Mario Party 4 (USA) (Rev 1).rvz",
    DR_CORE_DOLPHIN, dr_create_host<MarioParty4Host> },
  { DR_GAME_MARIOPARTY5, "Mario Party 5", "marioparty5", "Mario Party 5 (USA).rvz",
    DR_CORE_DOLPHIN, dr_create_host<MarioParty5Host> },
  { DR_GAME_MARIOPARTY6, "Mario Party 6", "marioparty6", "Mario Party 6 (USA).rvz",
    DR_CORE_DOLPHIN, dr_create_host<MarioParty6Host> },
  { DR_GAME_MARIOPARTY7, "Mario Party 7", "marioparty7", "Mario Party 7 (USA) (Rev 1).rvz",
    DR_CORE_DOLPHIN, dr_create_host<MarioParty7Host> },
  { DR_GAME_MARIOPARTY8, "Mario Party 8", "marioparty8", "Mario Party 8 (USA, Asia) (Rev 2).rvz",
    DR_CORE_DOLPHIN, dr_create_host<MarioParty8Host> },
  { DR_GAME_MARIOPARTYDS, "Mario Party DS", "mariopartyds", "Mario Party DS (USA) (Rev 2).nds",
    DR_CORE_MELONDSDS, dr_create_host_stock<MarioPartyDSHost> },
  { DR_GAME_MARIOPARTYADVANCE, "Mario Party Advance", "mariopartyadvance", "Mario Party Advance (USA).gba",
    DR_CORE_MGBA, dr_create_host_stock<MarioPartyAdvanceHost> },
  { DR_GAME_SONICSHUFFLE, "Sonic Shuffle", nullptr, "Sonic Shuffle (USA).chd",
    DR_CORE_FLYCAST, dr_create_host_stock<SonicShuffleHost> },

  { DR_GAME_INVALID, nullptr, nullptr, nullptr, DR_CORE_INVALID, nullptr }
};

const DrHostEntry *dr_host_entries(void)
{
  return DR_HOSTS;
}

const DrHostEntry *dr_host_entry(dr_game game)
{
  for (const DrHostEntry *entry = DR_HOSTS; entry->game != DR_GAME_INVALID; entry++)
    if (entry->game == game)
      return entry;

  return nullptr;
}

QString dr_host_rom_path(const DrHostEntry *entry)
{
  return dr_roms_directory() + "/" + entry->rom;
}
