#include "MarioParty2.h"

#include <QRetroDirectories.h>

static const uint8_t MP2_CHARACTER_IDS[DR_CHARACTER_SIZE] =
{
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0x00,
  [DR_CHARACTER_LUIGI]       = 0x01,
  [DR_CHARACTER_PEACH]       = 0x02,
  [DR_CHARACTER_YOSHI]       = 0x03,
  [DR_CHARACTER_WARIO]       = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
};

static const dr_mp_minigame_t MP2_MINIGAMES[] =
{
  { "Bowser Slots", DR_MINIGAME_ITEM, 0x01, 0x01, DR_NO_QUIRKS },
  { "Roll Out the Barrels", DR_MINIGAME_ITEM, 0x02, 0x02, DR_NO_QUIRKS },
  { "Give Me a Brake!", DR_MINIGAME_ITEM, 0x05, 0x05, DR_NO_QUIRKS },
  { "Grab Bag", DR_MINIGAME_BATTLE, 0x07, 0x07, DR_NO_QUIRKS },
  { "Rakin' 'em In", DR_MINIGAME_BATTLE, 0x09, 0x0A, DR_NO_QUIRKS },
  // 0a unused
  { "Day at the Races", DR_MINIGAME_BATTLE, 0x0B, 0x0B, DR_NO_QUIRKS },
  { "Bowl Over", DR_MINIGAME_1V3, 0x0F, 0x0E, DR_NO_QUIRKS },

  { "Bob-omb Barrage", DR_MINIGAME_1V3, 0x13, 0x12, DR_NO_QUIRKS },
  { "Shock, Drop or Roll", DR_MINIGAME_1V3, 0x15, 0x14, DR_NO_QUIRKS },
  // 16
  // 17
  { "Archer-ival", DR_MINIGAME_1V3, 0x18, 0x17, DR_NO_QUIRKS },
  // 19 unused
  { "Toad Bandstand", DR_MINIGAME_2V2, 0x1A, 0x18, DR_NO_QUIRKS },
  // 1b
  { "Handcar Havoc", DR_MINIGAME_2V2, 0x1C, 0x1A, DR_NO_QUIRKS },
  // 1d unused
  { "Balloon Burst", DR_MINIGAME_2V2, 0x1E, 0x1B, DR_NO_QUIRKS },
  { "Sky Pilots", DR_MINIGAME_2V2, 0x1F, 0x1C, DR_NO_QUIRKS },

  { "Speed Hockey", DR_MINIGAME_2V2, 0x20, 0x1D, DR_NO_QUIRKS },
  { "Cake Factory", DR_MINIGAME_2V2, 0x21, 0x1E, DR_NO_QUIRKS },
  // 22 unused
  { "Dungeon Dash", DR_MINIGAME_2V2, 0x23, 0x1F, DR_NO_QUIRKS },
  // 24
  { "Lava Tile Isle", DR_MINIGAME_4P, 0x25, 0x08, DR_NO_QUIRKS },
  { "Hot Rope Jump", DR_MINIGAME_4P, 0x26, 0x0C, DR_NO_QUIRKS },
  { "Shell Shocked", DR_MINIGAME_4P, 0x27, 0x22, DR_NO_QUIRKS },
  { "Toad in the Box", DR_MINIGAME_4P, 0x28, 0x24, DR_NO_QUIRKS },
  { "Mecha-Marathon", DR_MINIGAME_4P, 0x29, 0x25, DR_NO_QUIRKS },
  { "Roll Call", DR_MINIGAME_4P, 0x2A, 0x26, DR_NO_QUIRKS },
  { "Abandon Ship", DR_MINIGAME_4P, 0x2B, 0x27, DR_NO_QUIRKS },
  { "Platform Peril", DR_MINIGAME_4P, 0x2C, 0x28, DR_NO_QUIRKS },
  { "Totem Pole Pound", DR_MINIGAME_4P, 0x2D, 0x29, DR_NO_QUIRKS },
  { "Bumper Balls", DR_MINIGAME_4P, 0x2E, 0x2A, DR_NO_QUIRKS },
  // 2f unused

  { "Bombs Away", DR_MINIGAME_4P, 0x30, 0x43, DR_NO_QUIRKS },
  { "Tipsy Tourney", DR_MINIGAME_4P, 0x31, 0x2C, DR_NO_QUIRKS },
  { "Hexagon Heat", DR_MINIGAME_4P, 0x33, 0x2E, DR_NO_QUIRKS },
  { "Skateboard Scamper", DR_MINIGAME_4P, 0x34, 0x2F, DR_NO_QUIRKS },
  { "Slot Car Derby", DR_MINIGAME_4P, 0x35, 0x30, DR_NO_QUIRKS },
  // 38 unused

  { "Bowser's Big Blast", DR_MINIGAME_BATTLE, 0x41, 0x39, DR_NO_QUIRKS },
  { "Looney Lumberjacks", DR_MINIGAME_2V2, 0x42, 0x35, DR_NO_QUIRKS },
  { "Quicksand Cache", DR_MINIGAME_1V3, 0x47, 0x38, DR_NO_QUIRKS },
  { "Dizzy Dancing", DR_MINIGAME_4P, 0x4F, 0x54, DR_NO_QUIRKS },


  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

static MpN64Config buildConfig()
{
  return {
    .core  = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game  = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString(),
    .state = (dr_state_directory() + "/mp2.state.zip").toStdString(),

    .scene_miniexplain = 0x5F,
    .scene_miniresults = 0x70,

    .scene_addr    = 0x800FA63C,
    .minigame_addr = 0x800F93CA,

    .character_addr  = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
    .controller_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
    .difficulty_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
    .team_addr       = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
    .bot_addr        = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO

    .result_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO

    .character_ids = MP2_CHARACTER_IDS,
    .minigames     = MP2_MINIGAMES,
  };
}

MarioParty2::MarioParty2(QObject *parent) : MarioPartyN64(buildConfig(), parent)
{
}
