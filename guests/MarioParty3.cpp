#include "MarioParty3.h"

#include <cstring>

#include <QRetroDirectories.h>

/* Mario Party 3's playable roster, in native id order. */
typedef enum
{
  MP3_CHARACTER_MARIO = 0x0,
  MP3_CHARACTER_LUIGI = 0x1,
  MP3_CHARACTER_PEACH = 0x2,
  MP3_CHARACTER_YOSHI = 0x3,
  MP3_CHARACTER_WARIO = 0x4,
  MP3_CHARACTER_DONKEY_KONG = 0x5,
  MP3_CHARACTER_WALUIGI = 0x6,
  MP3_CHARACTER_DAISY = 0x7
} mp3_character;

static dr_character_id_t mp3_char_from_dr(dr_character character)
{
  switch (character)
  {
  /* Supported characters */
  case DR_CHARACTER_MARIO:
    return { MP3_CHARACTER_MARIO, true };
  case DR_CHARACTER_LUIGI:
    return { MP3_CHARACTER_LUIGI, true };
  case DR_CHARACTER_PEACH:
    return { MP3_CHARACTER_PEACH, true };
  case DR_CHARACTER_YOSHI:
    return { MP3_CHARACTER_YOSHI, true };
  case DR_CHARACTER_WARIO:
    return { MP3_CHARACTER_WARIO, true };
  case DR_CHARACTER_DONKEY_KONG:
    return { MP3_CHARACTER_DONKEY_KONG, true };
  case DR_CHARACTER_WALUIGI:
    return { MP3_CHARACTER_WALUIGI, true };
  case DR_CHARACTER_DAISY:
    return { MP3_CHARACTER_DAISY, true };

  /* Character replacements */
  case DR_CHARACTER_TOAD:
    return { MP3_CHARACTER_MARIO, false };
  case DR_CHARACTER_BOO:
    return { MP3_CHARACTER_DONKEY_KONG, false };
  case DR_CHARACTER_KOOPA_KID:
    return { MP3_CHARACTER_WARIO, false };
  case DR_CHARACTER_KOOPA_KID_R:
    return { MP3_CHARACTER_MARIO, false };
  case DR_CHARACTER_KOOPA_KID_G:
    return { MP3_CHARACTER_LUIGI, false };
  case DR_CHARACTER_KOOPA_KID_B:
    return { MP3_CHARACTER_WARIO, false };
  case DR_CHARACTER_TOADETTE:
    return { MP3_CHARACTER_PEACH, false };
  case DR_CHARACTER_BIRDO:
    return { MP3_CHARACTER_YOSHI, false };
  case DR_CHARACTER_DRY_BONES:
    return { MP3_CHARACTER_DONKEY_KONG, false };
  case DR_CHARACTER_BLOOPER:
    return { MP3_CHARACTER_PEACH, false };
  case DR_CHARACTER_HAMMER_BRO:
    return { MP3_CHARACTER_MARIO, false };
  default:
    return { MP3_CHARACTER_MARIO, false };
  }
}

static const dr_mp_minigame_t MP3_MINIGAMES[] =
{
  // 1v3
  { "Hand, Line and Sinker", DR_MINIGAME_1V3, 0x01, 0x01, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Coconut Conk", DR_MINIGAME_1V3, 0x02, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Spotlight Swim", DR_MINIGAME_1V3, 0x03, 0x03, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Boulder Ball", DR_MINIGAME_1V3, 0x04, 0x04, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crazy Cogs", DR_MINIGAME_1V3, 0x05, 0x05, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hide and Sneak", DR_MINIGAME_1V3, 0x06, 0x06, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ridiculous Relay", DR_MINIGAME_1V3, 0x07, 0x07, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Thwomp Pull", DR_MINIGAME_1V3, 0x08, 0x08, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "River Raiders", DR_MINIGAME_1V3, 0x09, 0x09, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Tidal Toss", DR_MINIGAME_1V3, 0x0A, 0x0A, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 2v2
  { "Eatsa Pizza", DR_MINIGAME_2V2, 0x0B, 0x0B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Baby Bowser Broadside", DR_MINIGAME_2V2, 0x0C, 0x0C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pump, Pump and Away", DR_MINIGAME_2V2, 0x0D, 0x0D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hyper Hydrants", DR_MINIGAME_2V2, 0x0E, 0x0E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Picking Panic", DR_MINIGAME_2V2, 0x0F, 0x0F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cosmic Coaster", DR_MINIGAME_2V2, 0x10, 0x10, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Puddle Paddle", DR_MINIGAME_2V2, 0x11, 0x11, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Etch 'n' Catch", DR_MINIGAME_2V2, 0x12, 0x12, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Log Jam", DR_MINIGAME_2V2, 0x13, 0x13, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Slot Synch", DR_MINIGAME_2V2, 0x14, 0x14, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 4p
  { "Treadmill Grill", DR_MINIGAME_4P, 0x15, 0x15, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Toadstool Titan", DR_MINIGAME_4P, 0x16, 0x16, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Aces High", DR_MINIGAME_4P, 0x17, 0x17, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bounce 'n' Trounce", DR_MINIGAME_4P, 0x18, 0x18, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ice Rink Risk", DR_MINIGAME_4P, 0x19, 0x19, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Locked Out", DR_MINIGAME_BATTLE, 0x1A, 0x1A, DR_NO_QUIRKS, DR_NO_FLAGS }, // yep
  { "Chip Shot Challenge", DR_MINIGAME_4P, 0x1B, 0x1B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Parasol Plummet", DR_MINIGAME_4P, 0x1C, 0x1C, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Messy Memory", DR_MINIGAME_4P, 0x1D, 0x1D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Picture Imperfect", DR_MINIGAME_4P, 0x1E, 0x1E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mario's Puzzle Party", DR_MINIGAME_4P, 0x1F, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "The Beat Goes On", DR_MINIGAME_4P, 0x20, 0x20, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "M.P.I.Q.", DR_MINIGAME_4P, 0x21, 0x21, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Curtain Call", DR_MINIGAME_4P, 0x22, 0x22, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Water Whirled", DR_MINIGAME_4P, 0x23, 0x23, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Frigid Bridges", DR_MINIGAME_4P, 0x24, 0x24, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Awful Tower", DR_MINIGAME_4P, 0x25, 0x25, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cheep Cheep Chase", DR_MINIGAME_4P, 0x26, 0x26, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pipe Cleaners", DR_MINIGAME_4P, 0x27, 0x27, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Snowball Summit", DR_MINIGAME_4P, 0x28, 0x28, DR_NO_QUIRKS, DR_NO_FLAGS },

  // battle
  { "All Fired Up", DR_MINIGAME_BATTLE, 0x29, 0x29, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stacked Deck", DR_MINIGAME_BATTLE, 0x2A, 0x2A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Three Door Monty", DR_MINIGAME_BATTLE, 0x2B, 0x2B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rockin' Raceway", DR_MINIGAME_4P, 0x2C, 0x2C, DR_NO_QUIRKS, DR_NO_FLAGS }, // yep
  { "Merry-Go-Chomp", DR_MINIGAME_BATTLE, 0x2D, 0x2D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Slap Down", DR_MINIGAME_BATTLE, 0x2E, 0x2E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Storm Chasers", DR_MINIGAME_BATTLE, 0x2F, 0x2F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Eye Sore", DR_MINIGAME_BATTLE, 0x30, 0x30, DR_NO_QUIRKS, DR_NO_FLAGS },

  // duel
  { "Vine With Me", DR_MINIGAME_DUEL, 0x31, 0x31, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Popgun Pick-Off", DR_MINIGAME_DUEL, 0x32, 0x32, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "End of the Line", DR_MINIGAME_DUEL, 0x33, 0x33, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bowser Toss", DR_MINIGAME_DUEL, 0x34, 0x34, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Baby Bowser Bonkers", DR_MINIGAME_DUEL, 0x35, 0x35, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Motor Rooter", DR_MINIGAME_DUEL, 0x36, 0x36, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Silly Screws", DR_MINIGAME_DUEL, 0x37, 0x37, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crowd Cover", DR_MINIGAME_DUEL, 0x38, 0x38, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tick Tock Hop", DR_MINIGAME_DUEL, 0x39, 0x39, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fowl Play", DR_MINIGAME_DUEL, 0x3A, 0x3A, DR_NO_QUIRKS, DR_NO_FLAGS },

  // item
  { "Winner's Wheel", DR_MINIGAME_ITEM, 0x3B, 0x3B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hey, Batter, Batter!", DR_MINIGAME_ITEM, 0x3C, 0x3C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bobbing Bow-loons", DR_MINIGAME_ITEM, 0x3D, 0x3D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dorrie Dip", DR_MINIGAME_ITEM, 0x3E, 0x3E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Swinging with Sharks", DR_MINIGAME_ITEM, 0x3F, 0x3F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Swing 'n' Swipe", DR_MINIGAME_ITEM, 0x40, 0x40, DR_NO_QUIRKS, DR_NO_FLAGS },

  // misc mini-games

  // 41 unused (chance time)
  { "Stardust Battle", DR_MINIGAME_SPECIAL, 0x42, 0x41, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Game Guy's Roulette", DR_MINIGAME_GAME_GUY, 0x43, 0x42, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Game Guy's Lucky 7", DR_MINIGAME_GAME_GUY, 0x44, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Game Guy's Magic Boxes", DR_MINIGAME_GAME_GUY, 0x45, 0x44, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Game Guy's Sweet Surprise", DR_MINIGAME_GAME_GUY, 0x46, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Dizzy Dinghies", DR_MINIGAME_SPECIAL, 0x47, 0x46, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mario's Puzzle Party Pro", DR_MINIGAME_SPECIAL, 0x48, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

static MpN64Config buildConfig()
{
  MpN64Config config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString();
  config.state = (dr_state_directory() + "/mp3.state.zip").toStdString();

  config.scene_miniexplain[0] = 0x70;
  config.scene_miniexplain[1] = 0x70;
  config.scene_miniresults = 0x71;

  config.scene = { 0x800ce202, DR_VALUE_TYPE_S16 };
  config.minigame = { 0x800cd069, DR_VALUE_TYPE_S16 };

  const size_t controller_addr[4]   = { 0x800d110a, 0x800d1142, 0x800d117a, 0x800d11b2 };
  const size_t difficulty_addr[4]   = { 0x800d1109, 0x800d1141, 0x800d1179, 0x800d11b1 };
  const size_t team_addr[4]         = { 0x800d1108, 0x800d1140, 0x800d1178, 0x800d11b0 };
  const size_t bot_addr[4]          = { 0x800d110c, 0x800d1144, 0x800d117c, 0x800d11b4 };
  const size_t character_addr[4]    = { 0x800d110b, 0x800d1143, 0x800d117b, 0x800d11b3 };
  const size_t bonus_result_addr[4] = { 0x800d110e, 0x800d1146, 0x800d117e, 0x800d11b6 }; // u16
  const size_t result_addr[4]       = { 0x800d1110, 0x800d1148, 0x800d1180, 0x800d11b8 }; // u16

  for (unsigned i = 0; i < 4; i++)
  {
    config.controller[i] = { controller_addr[i], DR_VALUE_TYPE_U8 };
    config.difficulty[i] = { difficulty_addr[i], DR_VALUE_TYPE_U8 };
    config.team[i] = { team_addr[i], DR_VALUE_TYPE_U8 };
    config.bot[i] = { bot_addr[i], DR_VALUE_TYPE_U8 };
    config.character[i] = { character_addr[i], DR_VALUE_TYPE_U8 };
    config.bonus_result[i] = { bonus_result_addr[i], DR_VALUE_TYPE_S16 };
    config.result[i] = { result_addr[i], DR_VALUE_TYPE_S16 };
  }

  config.coins[0] = { 0x800d1112, DR_VALUE_TYPE_U16 };
  config.coins[1] = { 0x800d114a, DR_VALUE_TYPE_U16 };
  config.coins[2] = { 0x800d1182, DR_VALUE_TYPE_U16 };
  config.coins[3] = { 0x800d11ba, DR_VALUE_TYPE_U16 };
  config.stars[0] = { 0x800d1116, DR_VALUE_TYPE_U8 };
  config.stars[1] = { 0x800d114e, DR_VALUE_TYPE_U8 };
  config.stars[2] = { 0x800d1186, DR_VALUE_TYPE_U8 };
  config.stars[3] = { 0x800d11be, DR_VALUE_TYPE_U8 };

  config.battle_pot = { 0x800cc698, DR_VALUE_TYPE_U16 };

  config.char_from_dr = mp3_char_from_dr;
  config.roster_size = 8;
  config.minigames = MP3_MINIGAMES;

  return config;
}

MarioParty3::MarioParty3(QObject *parent)
  : MarioPartyN64(buildConfig(), parent)
{
}
