#include "MarioParty1.h"

#include <cstring>

#include <QRetroDirectories.h>

static const uint8_t MP1_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
};

static const dr_mp_minigame_t MP1_MINIGAMES[] = {
  { "Memory Match", DR_MINIGAME_1P, 0x00, 0x00, DR_NO_QUIRKS },
  // 01 chance time -- unused
  { "Slot Machine", DR_MINIGAME_1P, 0x02, 0x02, DR_NO_QUIRKS },
  { "Buried Treasure", DR_MINIGAME_4P, 0x03, 0x03, DR_NO_QUIRKS },
  { "Treasure Divers", DR_MINIGAME_4P, 0x04, 0x04, DR_NO_QUIRKS },
  { "Shell Game", DR_MINIGAME_1P, 0x05, 0x05, DR_NO_QUIRKS },
  // { "Same Game", DR_MINIGAME_1P, 0x06, 0x06, DR_NO_QUIRKS },
  { "Hot Bob-omb", DR_MINIGAME_4P, 0x07, 0x07, DR_NO_QUIRKS },
  // 08 yoshis tongue meeting -- unused
  { "Pipe Maze", DR_MINIGAME_1V3, 0x09, 0x09, DR_NO_QUIRKS },
  { "Ghost Guess", DR_MINIGAME_1P, 0x0A, 0x0A, DR_NO_QUIRKS },
  { "Musical Mushroom", DR_MINIGAME_4P, 0x0B, 0x0B, DR_NO_QUIRKS },
  { "Pedal Power", DR_MINIGAME_1P, 0x0C, 0x0C, DR_NO_QUIRKS },
  { "Crazy Cutter", DR_MINIGAME_4P, 0x0D, 0x0D, DR_NO_QUIRKS },
  { "Face Lift", DR_MINIGAME_4P, 0x0E, 0x0E, DR_NO_QUIRKS },
  { "Whack-a-Plant", DR_MINIGAME_1P, 0x0F, 0x0F, DR_NO_QUIRKS },

  { "Bash 'n' Cash", DR_MINIGAME_1V3, 0x10, 0x10, DR_NO_QUIRKS },
  { "Bowl Over", DR_MINIGAME_1V3, 0x11, 0x11, DR_NO_QUIRKS },
  { "Ground Pound", DR_MINIGAME_1P, 0x12, 0x12, DR_NO_QUIRKS },
  { "Balloon Burst", DR_MINIGAME_4P, 0x13, 0x13, DR_NO_QUIRKS },
  { "Coin Block Blitz", DR_MINIGAME_4P, 0x14, 0x14, DR_NO_QUIRKS },
  { "Coin Block Bash", DR_MINIGAME_1V3, 0x15, 0x15, DR_NO_QUIRKS },
  { "Skateboard Scamper", DR_MINIGAME_4P, 0x16, 0x16, DR_NO_QUIRKS },
  { "Box Mountain Mayhem", DR_MINIGAME_4P, 0x17, 0x17, DR_NO_QUIRKS },
  { "Platform Peril", DR_MINIGAME_4P, 0x18, 0x18, DR_NO_QUIRKS },
  { "Teetering Towers", DR_MINIGAME_1P, 0x19, 0x19, DR_NO_QUIRKS },
  { "Mushroom Mix-up", DR_MINIGAME_4P, 0x1A, 0x1A, DR_NO_QUIRKS },
  { "Bumper Ball Maze 1", DR_MINIGAME_INVALID, 0x1B, 0x34, DR_NO_QUIRKS },
  { "Grab Bag", DR_MINIGAME_4P, 0x1C, 0x1C, DR_NO_QUIRKS },
  { "Bobsled Run", DR_MINIGAME_2V2, 0x1D, 0x1D, DR_NO_QUIRKS },
  { "Bumper Balls", DR_MINIGAME_4P, 0x1E, 0x1E, DR_NO_QUIRKS },
  { "Tightrope Treachery", DR_MINIGAME_1V3, 0x1F, 0x1F, DR_NO_QUIRKS },

  { "Knock Block Tower", DR_MINIGAME_1P, 0x20, 0x20, DR_NO_QUIRKS },
  { "Tipsy Tourney", DR_MINIGAME_4P, 0x21, 0x21, DR_NO_QUIRKS },
  { "Bombs Away", DR_MINIGAME_4P, 0x22, 0x22, DR_NO_QUIRKS },
  { "Crane Game", DR_MINIGAME_1V3, 0x23, 0x23, DR_NO_QUIRKS },
  { "Bumper Ball Maze 2", DR_MINIGAME_INVALID, 0x24, 0x34, DR_NO_QUIRKS },
  { "Slot Car Derby", DR_MINIGAME_4P, 0x25, 0x25, DR_NO_QUIRKS },
  { "Mario Bandstand", DR_MINIGAME_4P, 0x26, 0x26, DR_NO_QUIRKS },
  { "Desert Dash", DR_MINIGAME_2V2, 0x27, 0x27, DR_NO_QUIRKS },
  { "Shy Guy Says", DR_MINIGAME_4P, 0x28, 0x28, DR_NO_QUIRKS },
  { "Limbo Dance", DR_MINIGAME_1P, 0x29, 0x29, DR_NO_QUIRKS },
  { "Bombsketball", DR_MINIGAME_2V2, 0x2A, 0x2A, DR_NO_QUIRKS },
  { "Cast Aways", DR_MINIGAME_4P, 0x2B, 0x2B, DR_NO_QUIRKS },
  { "Key-pa-way", DR_MINIGAME_4P, 0x2C, 0x2C, DR_NO_QUIRKS },
  { "Running of the Bulb", DR_MINIGAME_4P, 0x2D, 0x2D, DR_NO_QUIRKS },
  { "Hot Rope Jump", DR_MINIGAME_4P, 0x2E, 0x2E, DR_NO_QUIRKS },
  { "Handcar Havoc", DR_MINIGAME_2V2, 0x2F, 0x2F, DR_NO_QUIRKS },

  // 30 unused
  { "Deep Sea Divers", DR_MINIGAME_2V2, 0x31, 0x30, DR_NO_QUIRKS },
  { "Piranha's Pursuit", DR_MINIGAME_1V3, 0x32, 0x31, DR_NO_QUIRKS },
  { "Tug o' War", DR_MINIGAME_1V3, 0x33, 0x32, DR_NO_QUIRKS },
  { "Paddle Battle", DR_MINIGAME_1V3, 0x34, 0x33, DR_NO_QUIRKS },
  { "Bumper Ball Maze 3", DR_MINIGAME_INVALID, 0x35, 0x34, DR_NO_QUIRKS },
  { "Coin Shower Flower", DR_MINIGAME_1V3, 0x36, 0x24, DR_NO_QUIRKS },
  { "Hammer Drop", DR_MINIGAME_4P, 0x37, 0x1B, DR_NO_QUIRKS },
  // 38 unused

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

static MpN64Config buildConfig()
{
  MpN64Config config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString();
  config.state = (dr_state_directory() + "/mp1.state.zip").toStdString();

  config.scene_miniexplain = 0x6f;
  config.scene_miniresults = 0x7c;

  // Hardware addresses (accessed wordflipped via DrRetroN64).
  config.scene_addr = 0x800C596C;    // u16
  config.minigame_addr = 0x800ED5DE; // u16

  const size_t controller_addr[4] = { 0x800f32b3, 0x800f32e3, 0x800f3313, 0x800f3343 };
  const size_t difficulty_addr[4] = { 0x800f32b2, 0x800f32e2, 0x800f3312, 0x800f3342 };
  const size_t team_addr[4]       = { 0x800f32b0, 0x800f32e0, 0x800f3310, 0x800f3340 };
  const size_t bot_addr[4]        = { 0x800f32b7, 0x800f32e7, 0x800f3317, 0x800f3347 };
  const size_t character_addr[4]  = { 0x800f32b4, 0x800f32e4, 0x800f3314, 0x800f3344 };
  const size_t result_addr[4]     = { 0x800f32ba, 0x800f32ea, 0x800f331a, 0x800f334a }; // u16
  memcpy(config.controller_addr, controller_addr, sizeof(controller_addr));
  memcpy(config.difficulty_addr, difficulty_addr, sizeof(difficulty_addr));
  memcpy(config.team_addr, team_addr, sizeof(team_addr));
  memcpy(config.bot_addr, bot_addr, sizeof(bot_addr));
  memcpy(config.character_addr, character_addr, sizeof(character_addr));
  memcpy(config.result_addr, result_addr, sizeof(result_addr));
  // bonus_result_addr: MP1 doesn't separate this (left zero)

  config.character_ids = MP1_CHARACTER_IDS;
  config.minigames = MP1_MINIGAMES;

  return config;
}

MarioParty1::MarioParty1(QObject *parent)
  : MarioPartyN64(buildConfig(), parent)
{
}
