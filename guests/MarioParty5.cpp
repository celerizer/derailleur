#include "MarioParty5.h"

#include <cstring>

// DONKEY_KONG maps to 0x08 (Boo) as MP5 has no DK.
static const uint16_t MP5_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x06, 0x05,
};

static const dr_mp_minigame_t MP5_MINIGAMES[] = {
  // 4-Player
  { "Coney Island", DR_MINIGAME_4P, 0x00, 0x0F, DR_NO_QUIRKS },
  { "Ground Pound Down", DR_MINIGAME_4P, 0x01, 0x10, DR_NO_QUIRKS },
  { "Chimp Chase", DR_MINIGAME_4P, 0x02, 0x11, DR_NO_QUIRKS },
  { "Chomp Romp", DR_MINIGAME_4P, 0x03, 0x12, DR_NO_QUIRKS },
  { "Pushy Penguins", DR_MINIGAME_4P, 0x04, 0x13, DR_QUIRK_EFB_TO_TEXTURE },
  { "Leaf Leap", DR_MINIGAME_4P, 0x05, 0x14, DR_NO_QUIRKS },
  { "Night Light Fright", DR_MINIGAME_4P, 0x06, 0x15, DR_NO_QUIRKS },
  { "Pop-Star Piranhas", DR_MINIGAME_4P, 0x07, 0x16, DR_NO_QUIRKS },
  { "Mazed & Confused", DR_MINIGAME_4P, 0x08, 0x17, DR_NO_QUIRKS },
  { "Dinger Derby", DR_MINIGAME_4P, 0x09, 0x18, DR_NO_QUIRKS },
  { "Hydrostars", DR_MINIGAME_4P, 0x0A, 0x19, DR_NO_QUIRKS },
  { "Later Skater", DR_MINIGAME_4P, 0x0B, 0x1A, DR_NO_QUIRKS },
  { "Will Flower", DR_MINIGAME_4P, 0x0C, 0x1B, DR_NO_QUIRKS },
  { "Triple Jump", DR_MINIGAME_4P, 0x0D, 0x1C, DR_NO_QUIRKS },
  { "Hotel Goomba", DR_MINIGAME_4P, 0x0E, 0x1D, DR_NO_QUIRKS },
  { "Coin Cache", DR_MINIGAME_4P, 0x0F, 0x1E, DR_NO_QUIRKS },
  { "Vicious Vending", DR_MINIGAME_4P, 0x17, 0x26, DR_NO_QUIRKS },
  { "Flower Shower", DR_MINIGAME_4P, 0x3E, 0x4A, DR_NO_QUIRKS },
  { "Dodge Bomb", DR_MINIGAME_4P, 0x3F, 0x4B, DR_NO_QUIRKS },
  { "Fish Upon a Star", DR_MINIGAME_4P, 0x40, 0x4C, DR_NO_QUIRKS },
  { "Rumble Fumble", DR_MINIGAME_4P, 0x41, 0x4D, DR_NO_QUIRKS },
  { "Frozen Frenzy", DR_MINIGAME_4P, 0x4B, 0x57, DR_NO_QUIRKS },
  { "Fish Sticks", DR_MINIGAME_4P, 0x4E, 0x59, DR_NO_QUIRKS },

  // 1-vs-3
  { "Flatiator", DR_MINIGAME_1V3, 0x10, 0x1F, DR_NO_QUIRKS },
  { "Squared Away", DR_MINIGAME_1V3, 0x11, 0x20, DR_NO_QUIRKS },
  { "Mario Mechs", DR_MINIGAME_1V3, 0x12, 0x21, DR_NO_QUIRKS },
  { "Revolving Fire", DR_MINIGAME_1V3, 0x13, 0x22, DR_NO_QUIRKS },
  { "Heat Stroke", DR_MINIGAME_1V3, 0x15, 0x24, DR_NO_QUIRKS },
  { "Beam Team", DR_MINIGAME_1V3, 0x16, 0x25, DR_NO_QUIRKS },
  { "Big Top Drop", DR_MINIGAME_1V3, 0x18, 0x27, DR_NO_QUIRKS },
  { "Quilt for Speed", DR_MINIGAME_1V3, 0x42, 0x4E, DR_NO_QUIRKS },
  { "Tube It or Lose It", DR_MINIGAME_1V3, 0x43, 0x4F, DR_NO_QUIRKS },
  { "Mathletes", DR_MINIGAME_1V3, 0x44, 0x50, DR_NO_QUIRKS },
  { "Fight Cards", DR_MINIGAME_1V3, 0x45, 0x51, DR_NO_QUIRKS },
  { "Curvy Curbs", DR_MINIGAME_1V3, 0x4C, 0x58, DR_NO_QUIRKS },

  // 2-vs-2
  { "Clock Stoppers", DR_MINIGAME_2V2, 0x14, 0x23, DR_NO_QUIRKS },
  { "Defuse or Lose", DR_MINIGAME_2V2, 0x19, 0x28, DR_NO_QUIRKS },
  { "ID UFO", DR_MINIGAME_2V2, 0x1A, 0x29, DR_NO_QUIRKS },
  { "Mario Can-Can", DR_MINIGAME_2V2, 0x1B, 0x2A, DR_NO_QUIRKS },
  { "Handy Hoppers", DR_MINIGAME_2V2, 0x1C, 0x2B, DR_NO_QUIRKS },
  { "Berry Basket", DR_MINIGAME_2V2, 0x1D, 0x2C, DR_NO_QUIRKS },
  { "Bus Buffer", DR_MINIGAME_2V2, 0x1E, 0x2D, DR_NO_QUIRKS },
  { "Rumble Ready", DR_MINIGAME_2V2, 0x1F, 0x2E, DR_NO_QUIRKS },
  { "Submarathon", DR_MINIGAME_2V2, 0x20, 0x2F, DR_NO_QUIRKS },
  { "Manic Mallets", DR_MINIGAME_2V2, 0x21, 0x30, DR_NO_QUIRKS },
  { "Panic Pinball", DR_MINIGAME_2V2, 0x49, 0x55, DR_NO_QUIRKS },
  { "Banking Coins", DR_MINIGAME_2V2, 0x4A, 0x56, DR_NO_QUIRKS },

  // Battle
  { "Astro-Logical", DR_MINIGAME_BATTLE, 0x22, 0x31, DR_NO_QUIRKS },
  { "Bill Blasters", DR_MINIGAME_BATTLE, 0x23, 0x32, DR_NO_QUIRKS },
  { "Tug-o-Dorrie", DR_MINIGAME_BATTLE, 0x24, 0x33, DR_NO_QUIRKS },
  { "Twist 'n' Out", DR_MINIGAME_BATTLE, 0x25, 0x34, DR_NO_QUIRKS },
  { "Lucky Lineup", DR_MINIGAME_BATTLE, 0x26, 0x35, DR_NO_QUIRKS },
  { "Random Ride", DR_MINIGAME_BATTLE, 0x27, 0x36, DR_NO_QUIRKS },

  // Duel
  { "Shock Absorbers", DR_MINIGAME_DUEL, 0x28, 0x37, DR_NO_QUIRKS },
  { "Countdown Pound", DR_MINIGAME_DUEL, 0x29, 0x38, DR_NO_QUIRKS },
  { "Whomp Maze", DR_MINIGAME_DUEL, 0x2A, 0x39, DR_NO_QUIRKS },
  { "Shy Guy Showdown", DR_MINIGAME_DUEL, 0x2B, 0x3A, DR_NO_QUIRKS },
  { "Button Mashers", DR_MINIGAME_DUEL, 0x2C, 0x3B, DR_NO_QUIRKS },
  { "Get a Rope", DR_MINIGAME_DUEL, 0x2D, 0x3C, DR_NO_QUIRKS },
  { "Pump 'n' Jump", DR_MINIGAME_DUEL, 0x2E, 0x3D, DR_NO_QUIRKS },
  { "Head Waiter", DR_MINIGAME_DUEL, 0x2F, 0x3E, DR_NO_QUIRKS },
  { "Blown Away", DR_MINIGAME_DUEL, 0x30, 0x3F, DR_NO_QUIRKS },
  { "Merry Poppings", DR_MINIGAME_DUEL, 0x31, 0x40, DR_NO_QUIRKS },
  { "Pound Peril", DR_MINIGAME_DUEL, 0x32, 0x41, DR_NO_QUIRKS },
  { "Piece Out", DR_MINIGAME_DUEL, 0x33, 0x42, DR_NO_QUIRKS },
  { "Bound of Music", DR_MINIGAME_DUEL, 0x34, 0x43, DR_NO_QUIRKS },
  { "Wind Wavers", DR_MINIGAME_DUEL, 0x35, 0x44, DR_NO_QUIRKS },
  { "Sky Survivor", DR_MINIGAME_DUEL, 0x36, 0x45, DR_NO_QUIRKS },

  // Bowser minigames (not selectable)
  { "Rain of Fire", DR_MINIGAME_INVALID, 0x3A, 0x46, DR_NO_QUIRKS },
  { "Cage-in Cookin'", DR_MINIGAME_INVALID, 0x3B, 0x47, DR_NO_QUIRKS },
  { "Scaldin' Cauldron", DR_MINIGAME_INVALID, 0x3C, 0x48, DR_NO_QUIRKS },

  // DK minigames (not selectable)
  { "Banana Punch", DR_MINIGAME_INVALID, 0x46, 0x52, DR_NO_QUIRKS },
  { "Da Vine Climb", DR_MINIGAME_INVALID, 0x47, 0x53, DR_NO_QUIRKS },
  { "Mass A-peel", DR_MINIGAME_INVALID, 0x48, 0x54, DR_NO_QUIRKS },

  // Story / Bonus (not selectable)
  { "Frightmare", DR_MINIGAME_SPECIAL, 0x3D, 0x49, DR_NO_QUIRKS },
  { "Beach Volleyball", DR_MINIGAME_SPECIAL, 0x4D, 0x0B, DR_NO_QUIRKS },
  { "Ice Hockey", DR_MINIGAME_SPECIAL, 0x4F, 0x5A, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

static MpGcnConfig buildConfig()
{
  MpGcnConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 5 (USA).rvz").toStdString();
  config.state = (dr_state_directory() + "/mp5.state.zip").toStdString();

  config.scene_miniexplain = 0x07;
  config.scene_miniresults = 0x6b;

  config.scene_addr = 0x80288860;
  config.minigame_addr = 0x8022A4C4;

  const size_t character_addr[4]    = { 0x8022a048, 0x8022a052, 0x8022a05c, 0x8022a066 };
  const size_t controller_addr[4]   = { 0x8022a04a, 0x8022a054, 0x8022a05e, 0x8022a068 };
  const size_t difficulty_addr[4]   = { 0x8022a04c, 0x8022a056, 0x8022a060, 0x8022a06a };
  const size_t team_addr[4]         = { 0x8022a04e, 0x8022a058, 0x8022a062, 0x8022a06c };
  const size_t bot_addr[4]          = { 0x8022a050, 0x8022a05a, 0x8022a064, 0x8022a06e };
  const size_t bonus_result_addr[4] = { 0x8022a09a, 0x8022a1a2, 0x8022a2aa, 0x8022a3b2 };
  const size_t result_addr[4]       = { 0x8022a09c, 0x8022a1a4, 0x8022a2ac, 0x8022a3b4 };
  memcpy(config.character_addr, character_addr, sizeof(character_addr));
  memcpy(config.controller_addr, controller_addr, sizeof(controller_addr));
  memcpy(config.difficulty_addr, difficulty_addr, sizeof(difficulty_addr));
  memcpy(config.team_addr, team_addr, sizeof(team_addr));
  memcpy(config.bot_addr, bot_addr, sizeof(bot_addr));
  memcpy(config.bonus_result_addr, bonus_result_addr, sizeof(bonus_result_addr));
  memcpy(config.result_addr, result_addr, sizeof(result_addr));

  config.character_ids = MP5_CHARACTER_IDS;
  config.minigames = MP5_MINIGAMES;

  return config;
}

MarioParty5::MarioParty5(QRetro *sharedCore, QObject *parent)
  : MarioPartyGcn(buildConfig(), sharedCore, parent)
{
}
