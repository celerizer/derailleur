#include "MarioParty1.h"

#include <QRetroDirectories.h>

static const uint8_t MP1_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  [DR_CHARACTER_INVALID] = 0xFF,
  [DR_CHARACTER_MARIO] = 0x00,
  [DR_CHARACTER_LUIGI] = 0x01,
  [DR_CHARACTER_PEACH] = 0x02,
  [DR_CHARACTER_YOSHI] = 0x03,
  [DR_CHARACTER_WARIO] = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
};

static const dr_mp_minigame_t MP1_MINIGAMES[] = {
  { "Memory Match", DR_MINIGAME_1P, 0x00, 0x00, DR_NO_QUIRKS },
  // 01 chance time -- unused
  { "Slot Machine", DR_MINIGAME_1P, 0x02, 0x02, DR_NO_QUIRKS },
  { "Buried Treasure", DR_MINIGAME_4P, 0x03, 0x03, DR_NO_QUIRKS },
  { "Treasure Divers", DR_MINIGAME_4P, 0x04, 0x04, DR_NO_QUIRKS },
  { "Shell Game", DR_MINIGAME_1P, 0x05, 0x05, DR_NO_QUIRKS },
  { "Same Game", DR_MINIGAME_1P, 0x06, 0x06, DR_NO_QUIRKS },
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
  { "Mario Bandstand", DR_MINIGAME_1V3, 0x26, 0x26, DR_NO_QUIRKS }, // categorized as 4p in MP1 but 1v3 works better
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
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString(),
    .state = (dr_state_directory() + "/mp1.state.zip").toStdString(),

    .scene_miniexplain = 0x6f,
    .scene_miniresults = 0x7c,

    .scene_addr = 0x800C596E,
    .minigame_addr = 0x800ED5DC,

    .controller_addr   = { 0x800f32b0, 0x800f32e0, 0x800f3310, 0x800f3340 },
    .difficulty_addr   = { 0x800f32b1, 0x800f32e1, 0x800f3311, 0x800f3341 },
    // another difficulty byte?
    .team_addr         = { 0x800f32b3, 0x800f32e3, 0x800f3313, 0x800f3343 },
    .bot_addr          = { 0x800f32b4, 0x800f32e4, 0x800f3314, 0x800f3344 },
    .character_addr    = { 0x800f32b7, 0x800f32e7, 0x800f3317, 0x800f3347 },
    .bonus_result_addr = { 0, 0, 0, 0 }, // MP1 doesn't separate this
    .result_addr       = { 0x800f32b8, 0x800f32e8, 0x800f3318, 0x800f3348 },

    .character_ids = MP1_CHARACTER_IDS,
    .minigames = MP1_MINIGAMES,
  };
}

MarioParty1::MarioParty1(QObject *parent)
  : MarioPartyN64(buildConfig(), parent)
{
}
