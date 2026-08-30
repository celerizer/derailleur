#include "MarioParty2.h"

#include <cstring>

#include <QRetroDirectories.h>

/* Mario Party 2's playable roster, in native id order. */
typedef enum
{
  MP2_CHARACTER_MARIO = 0x0,
  MP2_CHARACTER_LUIGI = 0x1,
  MP2_CHARACTER_PEACH = 0x2,
  MP2_CHARACTER_YOSHI = 0x3,
  MP2_CHARACTER_WARIO = 0x4,
  MP2_CHARACTER_DONKEY_KONG = 0x5
} mp2_character;

static dr_character_id_t mp2_char_from_dr(dr_character character)
{
  switch (character)
  {
  /* Supported characters */
  case DR_CHARACTER_MARIO:
    return { MP2_CHARACTER_MARIO, true };
  case DR_CHARACTER_LUIGI:
    return { MP2_CHARACTER_LUIGI, true };
  case DR_CHARACTER_PEACH:
    return { MP2_CHARACTER_PEACH, true };
  case DR_CHARACTER_YOSHI:
    return { MP2_CHARACTER_YOSHI, true };
  case DR_CHARACTER_WARIO:
    return { MP2_CHARACTER_WARIO, true };
  case DR_CHARACTER_DONKEY_KONG:
    return { MP2_CHARACTER_DONKEY_KONG, true };

  /* Character replacements */
  case DR_CHARACTER_WALUIGI:
    return { MP2_CHARACTER_LUIGI, false };
  case DR_CHARACTER_DAISY:
    return { MP2_CHARACTER_PEACH, false };
  case DR_CHARACTER_TOAD:
    return { MP2_CHARACTER_MARIO, false };
  case DR_CHARACTER_BOO:
    return { MP2_CHARACTER_DONKEY_KONG, false };
  case DR_CHARACTER_KOOPA_KID:
    return { MP2_CHARACTER_WARIO, false };
  case DR_CHARACTER_KOOPA_KID_R:
    return { MP2_CHARACTER_MARIO, false };
  case DR_CHARACTER_KOOPA_KID_G:
    return { MP2_CHARACTER_LUIGI, false };
  case DR_CHARACTER_KOOPA_KID_B:
    return { MP2_CHARACTER_WARIO, false };
  case DR_CHARACTER_TOADETTE:
    return { MP2_CHARACTER_PEACH, false };
  case DR_CHARACTER_BIRDO:
    return { MP2_CHARACTER_YOSHI, false };
  case DR_CHARACTER_DRY_BONES:
    return { MP2_CHARACTER_DONKEY_KONG, false };
  case DR_CHARACTER_BLOOPER:
    return { MP2_CHARACTER_PEACH, false };
  case DR_CHARACTER_HAMMER_BRO:
    return { MP2_CHARACTER_MARIO, false };
  default:
    return { MP2_CHARACTER_MARIO, false };
  }
}

static const dr_mp_minigame_t MP2_MINIGAMES[] = {
  // item
  { "Bowser Slots", DR_MINIGAME_ITEM, 0x01, 0x01, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Roll Out the Barrels", DR_MINIGAME_ITEM, 0x02, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Coffin Congestion", DR_MINIGAME_ITEM, 0x03, 0x03, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hammer Slammer", DR_MINIGAME_ITEM, 0x04, 0x04, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Give Me a Brake!", DR_MINIGAME_ITEM, 0x05, 0x05, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mallet-Go-Round", DR_MINIGAME_ITEM, 0x06, 0x06, DR_NO_QUIRKS, DR_NO_FLAGS },

  // battle
  { "Grab Bag", DR_MINIGAME_BATTLE, 0x07, 0x07, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bumper Balloon Cars", DR_MINIGAME_BATTLE, 0x08, 0x09, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rakin' 'em In", DR_MINIGAME_BATTLE, 0x09, 0x0A, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 0A unused
  { "Day at the Races", DR_MINIGAME_BATTLE, 0x0B, 0x0B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Face Lift", DR_MINIGAME_BATTLE, 0x0C, 0x21, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crazy Cutters", DR_MINIGAME_BATTLE, 0x0D, 0x23, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hot Bob-omb", DR_MINIGAME_BATTLE, 0x0E, 0x0D, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 1v3
  { "Bowl Over", DR_MINIGAME_1V3, 0x0F, 0x0E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rainbow Run", DR_MINIGAME_1V3, 0x10, 0x0F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crane Game", DR_MINIGAME_1V3, 0x11, 0x10, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Move to the Music", DR_MINIGAME_1V3, 0x12, 0x11, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bob-omb Barrage", DR_MINIGAME_1V3, 0x13, 0x12, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Look Away", DR_MINIGAME_1V3, 0x14, 0x13, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shock, Drop or Roll", DR_MINIGAME_1V3, 0x15, 0x14, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lights Out", DR_MINIGAME_1V3, 0x16, 0x15, DR_QUIRK_NATIVE_BOUNDARIES, DR_NO_FLAGS },
  { "Filet Relay", DR_MINIGAME_1V3, 0x17, 0x16, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Archer-ival", DR_MINIGAME_1V3, 0x18, 0x17, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 19 unused

  // 2v2
  { "Toad Bandstand", DR_MINIGAME_2V2, 0x1A, 0x18, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bobsled Run", DR_MINIGAME_2V2, 0x1B, 0x19, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Handcar Havoc", DR_MINIGAME_2V2, 0x1C, 0x1A, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 1D unused
  { "Balloon Burst", DR_MINIGAME_2V2, 0x1E, 0x1B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sky Pilots", DR_MINIGAME_2V2, 0x1F, 0x1C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Speed Hockey", DR_MINIGAME_2V2, 0x20, 0x1D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cake Factory", DR_MINIGAME_2V2, 0x21, 0x1E, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 22 unused
  { "Dungeon Dash", DR_MINIGAME_2V2, 0x23, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Magnet Carta", DR_MINIGAME_2V2, 0x24, 0x20, DR_NO_QUIRKS, DR_FLAG_LUCKY },

  // 4p
  { "Lava Tile Isle", DR_MINIGAME_4P, 0x25, 0x08, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hot Rope Jump", DR_MINIGAME_4P, 0x26, 0x0C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shell Shocked", DR_MINIGAME_4P, 0x27, 0x22, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Toad in the Box", DR_MINIGAME_4P, 0x28, 0x24, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mecha-Marathon", DR_MINIGAME_4P, 0x29, 0x25, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Roll Call", DR_MINIGAME_4P, 0x2A, 0x26, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Abandon Ship", DR_MINIGAME_4P, 0x2B, 0x27, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Platform Peril", DR_MINIGAME_4P, 0x2C, 0x28, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Totem Pole Pound", DR_MINIGAME_4P, 0x2D, 0x29, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bumper Balls", DR_MINIGAME_4P, 0x2E, 0x2A, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 2F unused
  { "Bombs Away", DR_MINIGAME_4P, 0x30, 0x2B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tipsy Tourney", DR_MINIGAME_4P, 0x31, 0x2C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Honeycomb Havoc", DR_MINIGAME_4P, 0x32, 0x2D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hexagon Heat", DR_MINIGAME_4P, 0x33, 0x2E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Skateboard Scamper", DR_MINIGAME_4P, 0x34, 0x2F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Slot Car Derby", DR_MINIGAME_4P, 0x35, 0x30, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shy Guy Says", DR_MINIGAME_4P, 0x36, 0x31, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sneak 'n' Snore", DR_MINIGAME_4P, 0x37, 0x32, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 38 unused
  { "Driver's Ed", DR_MINIGAME_SPECIAL, 0x39, 0x33, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 3A Chance Time (scene 34)

  // duel
  /// @todo leaving these unsupported for now as they dont give results
  /*
  { "Quick Draw Corks", DR_MINIGAME_DUEL, 0x3B, 0x3F, DR_NO_QUIRKS },
  { "Saber Swipes", DR_MINIGAME_DUEL, 0x3C, 0x42, DR_NO_QUIRKS },
  { "Mushroom Brew", DR_MINIGAME_DUEL, 0x3D, 0x44, DR_NO_QUIRKS },
  { "Time Bomb", DR_MINIGAME_DUEL, 0x3E, 0x46, DR_NO_QUIRKS },
  { "Psychic Safari", DR_MINIGAME_DUEL, 0x3F, 0x48, DR_NO_QUIRKS },
  { "Rock, Paper, Mario", DR_MINIGAME_DUEL, 0x40, 0x4A, DR_NO_QUIRKS },
  */

  // leftovers
  { "Bowser's Big Blast", DR_MINIGAME_BATTLE, 0x41, 0x39, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Looney Lumberjacks", DR_MINIGAME_2V2, 0x42, 0x35, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Torpedo Targets", DR_MINIGAME_2V2, 0x43, 0x3A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Destruction Duet", DR_MINIGAME_2V2, 0x44, 0x3B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dizzy Dancing", DR_MINIGAME_4P, 0x45, 0x36, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tile Driver", DR_MINIGAME_4P, 0x46, 0x37, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Quicksand Cache", DR_MINIGAME_1V3, 0x47, 0x38, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Deep Sea Salvage", DR_MINIGAME_4P, 0x48, 0x3C, DR_NO_QUIRKS, DR_FLAG_LUCKY },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/// @todo mini-game variant is s16 at hardware 800cafee

static MpN64Config buildConfig()
{
  MpN64Config config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString();
  config.state = (dr_state_directory() + "/mp2.state.zip").toStdString();

  config.scene_miniexplain[0] = 0x5F;
  config.scene_miniexplain[1] = 0x60;
  config.scene_miniresults = 0x70;

  config.scene_addr = 0x800FA63E;    // u16
  config.minigame_addr = 0x800F93C8; // u16

  const size_t controller_addr[4]   = { 0x800fd2c3, 0x800fd2f7, 0x800fd32b, 0x800fd35f };
  const size_t difficulty_addr[4]   = { 0x800fd2c2, 0x800fd2f6, 0x800fd32a, 0x800fd35e };
  const size_t team_addr[4]         = { 0x800fd2c0, 0x800fd2f4, 0x800fd328, 0x800fd35c };
  const size_t bot_addr[4]          = { 0x800fd2c7, 0x800fd2fb, 0x800fd32f, 0x800fd363 };
  const size_t character_addr[4]    = { 0x800fd2c4, 0x800fd2f8, 0x800fd32c, 0x800fd360 };
  const size_t bonus_result_addr[4] = { 0x800fd2ca, 0x800fd2fe, 0x800fd332, 0x800fd366 }; // u16
  const size_t result_addr[4]       = { 0x800fd2cc, 0x800fd300, 0x800fd334, 0x800fd368 }; // u16
  memcpy(config.controller_addr, controller_addr, sizeof(controller_addr));
  memcpy(config.difficulty_addr, difficulty_addr, sizeof(difficulty_addr));
  memcpy(config.team_addr, team_addr, sizeof(team_addr));
  memcpy(config.bot_addr, bot_addr, sizeof(bot_addr));
  memcpy(config.character_addr, character_addr, sizeof(character_addr));
  memcpy(config.bonus_result_addr, bonus_result_addr, sizeof(bonus_result_addr));
  memcpy(config.result_addr, result_addr, sizeof(result_addr));

  config.coins[0] = { 0x800fd2c8, DR_VALUE_TYPE_U16 };
  config.coins[1] = { 0x800fd2fc, DR_VALUE_TYPE_U16 };
  config.coins[2] = { 0x800fd330, DR_VALUE_TYPE_U16 };
  config.coins[3] = { 0x800fd364, DR_VALUE_TYPE_U16 };
  config.stars[0] = { 0x800fd2ce, DR_VALUE_TYPE_U16 };
  config.stars[1] = { 0x800fd302, DR_VALUE_TYPE_U16 };
  config.stars[2] = { 0x800fd336, DR_VALUE_TYPE_U16 };
  config.stars[3] = { 0x800fd36a, DR_VALUE_TYPE_U16 };

  config.battle_pot = { 0x800f9208, DR_VALUE_TYPE_U16 };

  config.char_from_dr = mp2_char_from_dr;
  config.roster_size = 6;

  /* Shell Shocked runs Koopa Kid off character id 6; nothing else in the game does. */
  config.hidden.character = DR_CHARACTER_KOOPA_KID;
  config.hidden.native_id = 0x06;
  config.hidden.minigame_ids[0] = 0x27;
  config.hidden.minigame_ids[1] = -1;
  config.minigames = MP2_MINIGAMES;

  return config;
}

MarioParty2::MarioParty2(QObject *parent)
  : MarioPartyN64(buildConfig(), parent)
{
}
