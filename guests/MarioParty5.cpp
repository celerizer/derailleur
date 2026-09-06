#include "MarioParty5.h"

#include <cstring>

/* Mario Party 5's playable roster, in native id order. */
typedef enum
{
  MP5_CHARACTER_MARIO = 0x0,
  MP5_CHARACTER_LUIGI = 0x1,
  MP5_CHARACTER_PEACH = 0x2,
  MP5_CHARACTER_YOSHI = 0x3,
  MP5_CHARACTER_WARIO = 0x4,
  MP5_CHARACTER_DAISY = 0x5,
  MP5_CHARACTER_WALUIGI = 0x6,
  MP5_CHARACTER_TOAD = 0x7,
  MP5_CHARACTER_BOO = 0x8,
  MP5_CHARACTER_KOOPA_KID = 0x9,
  MP5_CHARACTER_KOOPA_KID_R = 0xA,
  MP5_CHARACTER_KOOPA_KID_G = 0xB,
  MP5_CHARACTER_KOOPA_KID_B = 0xC
} mp5_character;

static dr_character_id_t mp5_char_from_dr(dr_character character)
{
  switch (character)
  {
  /* Supported characters */
  case DR_CHARACTER_MARIO:
    return { MP5_CHARACTER_MARIO, true };
  case DR_CHARACTER_LUIGI:
    return { MP5_CHARACTER_LUIGI, true };
  case DR_CHARACTER_PEACH:
    return { MP5_CHARACTER_PEACH, true };
  case DR_CHARACTER_YOSHI:
    return { MP5_CHARACTER_YOSHI, true };
  case DR_CHARACTER_WARIO:
    return { MP5_CHARACTER_WARIO, true };
  case DR_CHARACTER_DAISY:
    return { MP5_CHARACTER_DAISY, true };
  case DR_CHARACTER_WALUIGI:
    return { MP5_CHARACTER_WALUIGI, true };
  case DR_CHARACTER_TOAD:
    return { MP5_CHARACTER_TOAD, true };
  case DR_CHARACTER_BOO:
    return { MP5_CHARACTER_BOO, true };
  case DR_CHARACTER_KOOPA_KID:
    return { MP5_CHARACTER_KOOPA_KID, true };
  case DR_CHARACTER_KOOPA_KID_R:
    return { MP5_CHARACTER_KOOPA_KID_R, true };
  case DR_CHARACTER_KOOPA_KID_G:
    return { MP5_CHARACTER_KOOPA_KID_G, true };
  case DR_CHARACTER_KOOPA_KID_B:
    return { MP5_CHARACTER_KOOPA_KID_B, true };

  /* Character replacements */
  case DR_CHARACTER_DONKEY_KONG:
    return { MP5_CHARACTER_BOO, false };
  case DR_CHARACTER_TOADETTE:
    return { MP5_CHARACTER_PEACH, false };
  case DR_CHARACTER_BIRDO:
    return { MP5_CHARACTER_YOSHI, false };
  case DR_CHARACTER_DRY_BONES:
    return { MP5_CHARACTER_BOO, false };
  case DR_CHARACTER_BLOOPER:
    return { MP5_CHARACTER_PEACH, false };
  case DR_CHARACTER_HAMMER_BRO:
    return { MP5_CHARACTER_MARIO, false };
  default:
    return { MP5_CHARACTER_MARIO, false };
  }
}

static const dr_mp_minigame_t MP5_MINIGAMES[] = {
  // 4-Player
  { "Coney Island", DR_MINIGAME_4P, 0x00, 0x0F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ground Pound Down", DR_MINIGAME_4P, 0x01, 0x10, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Chimp Chase", DR_MINIGAME_4P, 0x02, 0x11, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Chomp Romp", DR_MINIGAME_4P, 0x03, 0x12, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pushy Penguins", DR_MINIGAME_4P, 0x04, 0x13, DR_QUIRK_EFB_TO_TEXTURE, DR_NO_FLAGS },
  { "Leaf Leap", DR_MINIGAME_4P, 0x05, 0x14, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Night Light Fright", DR_MINIGAME_4P, 0x06, 0x15, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pop-Star Piranhas", DR_MINIGAME_4P, 0x07, 0x16, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mazed & Confused", DR_MINIGAME_4P, 0x08, 0x17, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dinger Derby", DR_MINIGAME_4P, 0x09, 0x18, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hydrostars", DR_MINIGAME_4P, 0x0A, 0x19, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Later Skater", DR_MINIGAME_4P, 0x0B, 0x1A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Will Flower", DR_MINIGAME_4P, 0x0C, 0x1B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Triple Jump", DR_MINIGAME_4P, 0x0D, 0x1C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hotel Goomba", DR_MINIGAME_4P, 0x0E, 0x1D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Coin Cache", DR_MINIGAME_4P, 0x0F, 0x1E, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Vicious Vending", DR_MINIGAME_4P, 0x17, 0x26, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Flower Shower", DR_MINIGAME_4P, 0x3E, 0x4A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dodge Bomb", DR_MINIGAME_4P, 0x3F, 0x4B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fish Upon a Star", DR_MINIGAME_4P, 0x40, 0x4C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rumble Fumble", DR_MINIGAME_4P, 0x41, 0x4D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Frozen Frenzy", DR_MINIGAME_4P, 0x4B, 0x57, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fish Sticks", DR_MINIGAME_4P, 0x4E, 0x59, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 1-vs-3
  { "Flatiator", DR_MINIGAME_1V3, 0x10, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Squared Away", DR_MINIGAME_1V3, 0x11, 0x20, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mario Mechs", DR_MINIGAME_1V3, 0x12, 0x21, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Revolving Fire", DR_MINIGAME_1V3, 0x13, 0x22, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Heat Stroke", DR_MINIGAME_1V3, 0x15, 0x24, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beam Team", DR_MINIGAME_1V3, 0x16, 0x25, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Big Top Drop", DR_MINIGAME_1V3, 0x18, 0x27, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Quilt for Speed", DR_MINIGAME_1V3, 0x42, 0x4E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tube It or Lose It", DR_MINIGAME_1V3, 0x43, 0x4F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mathletes", DR_MINIGAME_1V3, 0x44, 0x50, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Fight Cards", DR_MINIGAME_1V3, 0x45, 0x51, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Curvy Curbs", DR_MINIGAME_1V3, 0x4C, 0x58, DR_NO_QUIRKS, DR_NO_FLAGS },

  // 2-vs-2
  { "Clock Stoppers", DR_MINIGAME_2V2, 0x14, 0x23, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Defuse or Lose", DR_MINIGAME_2V2, 0x19, 0x28, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "ID UFO", DR_MINIGAME_2V2, 0x1A, 0x29, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mario Can-Can", DR_MINIGAME_2V2, 0x1B, 0x2A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Handy Hoppers", DR_MINIGAME_2V2, 0x1C, 0x2B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Berry Basket", DR_MINIGAME_2V2, 0x1D, 0x2C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bus Buffer", DR_MINIGAME_2V2, 0x1E, 0x2D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rumble Ready", DR_MINIGAME_2V2, 0x1F, 0x2E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Submarathon", DR_MINIGAME_2V2, 0x20, 0x2F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Manic Mallets", DR_MINIGAME_2V2, 0x21, 0x30, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Panic Pinball", DR_MINIGAME_2V2, 0x49, 0x55, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Banking Coins", DR_MINIGAME_2V2, 0x4A, 0x56, DR_NO_QUIRKS, DR_FLAG_LUCKY },

  // Battle
  { "Astro-Logical", DR_MINIGAME_BATTLE, 0x22, 0x31, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bill Blasters", DR_MINIGAME_BATTLE, 0x23, 0x32, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tug-o-Dorrie", DR_MINIGAME_BATTLE, 0x24, 0x33, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Twist 'n' Out", DR_MINIGAME_BATTLE, 0x25, 0x34, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lucky Lineup", DR_MINIGAME_BATTLE, 0x26, 0x35, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Random Ride", DR_MINIGAME_BATTLE, 0x27, 0x36, DR_NO_QUIRKS, DR_NO_FLAGS },

  // Duel
  { "Shock Absorbers", DR_MINIGAME_DUEL, 0x28, 0x37, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Countdown Pound", DR_MINIGAME_DUEL, 0x29, 0x38, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Whomp Maze", DR_MINIGAME_DUEL, 0x2A, 0x39, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shy Guy Showdown", DR_MINIGAME_DUEL, 0x2B, 0x3A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Button Mashers", DR_MINIGAME_DUEL, 0x2C, 0x3B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Get a Rope", DR_MINIGAME_DUEL, 0x2D, 0x3C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pump 'n' Jump", DR_MINIGAME_DUEL, 0x2E, 0x3D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Head Waiter", DR_MINIGAME_DUEL, 0x2F, 0x3E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Blown Away", DR_MINIGAME_DUEL, 0x30, 0x3F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Merry Poppings", DR_MINIGAME_DUEL, 0x31, 0x40, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pound Peril", DR_MINIGAME_DUEL, 0x32, 0x41, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Piece Out", DR_MINIGAME_DUEL, 0x33, 0x42, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bound of Music", DR_MINIGAME_DUEL, 0x34, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wind Wavers", DR_MINIGAME_DUEL, 0x35, 0x44, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sky Survivor", DR_MINIGAME_DUEL, 0x36, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },

  // Bowser minigames
  { "Rain of Fire", DR_MINIGAME_BOWSER, 0x3A, 0x46, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cage-in Cookin'", DR_MINIGAME_BOWSER, 0x3B, 0x47, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Scaldin' Cauldron", DR_MINIGAME_BOWSER, 0x3C, 0x48, DR_NO_QUIRKS, DR_NO_FLAGS },

  // DK minigames
  { "Banana Punch", DR_MINIGAME_DK, 0x46, 0x52, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Da Vine Climb", DR_MINIGAME_DK, 0x47, 0x53, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mass A-peel", DR_MINIGAME_DK, 0x48, 0x54, DR_NO_QUIRKS, DR_NO_FLAGS },

  // Story / Bonus
  { "Frightmare", DR_MINIGAME_SPECIAL, 0x3D, 0x49, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beach Volleyball", DR_MINIGAME_SPECIAL, 0x4D, 0x0B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ice Hockey", DR_MINIGAME_SPECIAL, 0x4F, 0x5A, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

static MpGcnConfig buildConfig()
{
  MpGcnConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 5 (USA)").toStdString();
  config.state = (dr_state_directory() + "/mp5.state.zip").toStdString();

  config.scene_miniexplain = 0x07;
  config.scene_miniresults = 0x6b;

  config.scene = { 0x80288860, DR_VALUE_TYPE_S32 };
  config.minigame = { 0x8022A4C4, DR_VALUE_TYPE_S16 };

  const size_t character_addr[4]    = { 0x8022a048, 0x8022a052, 0x8022a05c, 0x8022a066 };
  const size_t controller_addr[4]   = { 0x8022a04a, 0x8022a054, 0x8022a05e, 0x8022a068 };
  const size_t difficulty_addr[4]   = { 0x8022a04c, 0x8022a056, 0x8022a060, 0x8022a06a };
  const size_t team_addr[4]         = { 0x8022a04e, 0x8022a058, 0x8022a062, 0x8022a06c };
  const size_t bot_addr[4]          = { 0x8022a050, 0x8022a05a, 0x8022a064, 0x8022a06e };
  const size_t bonus_result_addr[4] = { 0x8022a09a, 0x8022a1a2, 0x8022a2aa, 0x8022a3b2 };
  const size_t result_addr[4]       = { 0x8022a09c, 0x8022a1a4, 0x8022a2ac, 0x8022a3b4 };

  for (unsigned i = 0; i < 4; i++)
  {
    config.character[i] = { character_addr[i], DR_VALUE_TYPE_U16 };
    config.controller[i] = { controller_addr[i], DR_VALUE_TYPE_U16 };
    config.difficulty[i] = { difficulty_addr[i], DR_VALUE_TYPE_U16 };
    config.team[i] = { team_addr[i], DR_VALUE_TYPE_U16 };
    config.bot[i] = { bot_addr[i], DR_VALUE_TYPE_U16 };
    config.bonus_result[i] = { bonus_result_addr[i], DR_VALUE_TYPE_U16 };
    config.result[i] = { result_addr[i], DR_VALUE_TYPE_U16 };
  }

  config.char_from_dr = mp5_char_from_dr;
  config.roster_size = 13;
  config.minigames = MP5_MINIGAMES;

  return config;
}

MarioParty5::MarioParty5(QRetro *sharedCore, QObject *parent)
  : MarioPartyGcn(buildConfig(), sharedCore, parent)
{
}
