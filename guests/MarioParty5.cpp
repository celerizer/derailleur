#include "MarioParty5.h"

static const uint8_t k_characterIds[DR_CHARACTER_SIZE] =
{
  [DR_CHARACTER_INVALID]     = 0xFF,

  [DR_CHARACTER_MARIO]       = 0x00,
  [DR_CHARACTER_LUIGI]       = 0x01,
  [DR_CHARACTER_PEACH]       = 0x02,
  [DR_CHARACTER_YOSHI]       = 0x03,
  [DR_CHARACTER_WARIO]       = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x08, // Use Boo instead
  [DR_CHARACTER_WALUIGI]     = 0x06,
  [DR_CHARACTER_DAISY]       = 0x05,
};

static const dr_mp_minigame_t k_minigames[] =
{
  // 4-Player
  {"Coney Island",        DR_MINIGAME_4P,      0x00, 0x0F},
  {"Ground Pound Down",   DR_MINIGAME_4P,      0x01, 0x10},
  {"Chimp Chase",         DR_MINIGAME_4P,      0x02, 0x11},
  {"Chomp Romp",          DR_MINIGAME_4P,      0x03, 0x12},
  {"Pushy Penguins",      DR_MINIGAME_4P,      0x04, 0x13},
  {"Leaf Leap",           DR_MINIGAME_4P,      0x05, 0x14},
  {"Night Light Fright",  DR_MINIGAME_4P,      0x06, 0x15},
  {"Pop-Star Piranhas",   DR_MINIGAME_4P,      0x07, 0x16},
  {"Mazed & Confused",    DR_MINIGAME_4P,      0x08, 0x17},
  {"Dinger Derby",        DR_MINIGAME_4P,      0x09, 0x18},
  {"Hydrostars",          DR_MINIGAME_4P,      0x0A, 0x19},
  {"Later Skater",        DR_MINIGAME_4P,      0x0B, 0x1A},
  {"Will Flower",         DR_MINIGAME_4P,      0x0C, 0x1B},
  {"Triple Jump",         DR_MINIGAME_4P,      0x0D, 0x1C},
  {"Hotel Goomba",        DR_MINIGAME_4P,      0x0E, 0x1D},
  {"Coin Cache",          DR_MINIGAME_4P,      0x0F, 0x1E},
  {"Vicious Vending",     DR_MINIGAME_4P,      0x17, 0x26},
  {"Flower Shower",       DR_MINIGAME_4P,      0x3E, 0x4A},
  {"Dodge Bomb",          DR_MINIGAME_4P,      0x3F, 0x4B}, // ?
  {"Fish Upon a Star",    DR_MINIGAME_4P,      0x40, 0x4C},
  {"Rumble Fumble",       DR_MINIGAME_4P,      0x41, 0x4D},
  {"Frozen Frenzy",       DR_MINIGAME_4P,      0x4B, 0x57},
  {"Fish Sticks",         DR_MINIGAME_4P,      0x4E, 0x59}, // ?
  // 1-vs-3
  {"Flatiator",           DR_MINIGAME_1V3,     0x10, 0x1F},
  {"Squared Away",        DR_MINIGAME_1V3,     0x11, 0x20},
  {"Mario Mechs",         DR_MINIGAME_1V3,     0x12, 0x21},
  {"Revolving Fire",      DR_MINIGAME_1V3,     0x13, 0x22},
  {"Heat Stroke",         DR_MINIGAME_1V3,     0x15, 0x24},
  {"Beam Team",           DR_MINIGAME_1V3,     0x16, 0x25},
  {"Big Top Drop",        DR_MINIGAME_1V3,     0x18, 0x27},
  {"Quilt for Speed",     DR_MINIGAME_1V3,     0x42, 0x4E},
  {"Tube It or Lose It",  DR_MINIGAME_1V3,     0x43, 0x4F},
  {"Mathletes",           DR_MINIGAME_1V3,     0x44, 0x50},
  {"Fight Cards",         DR_MINIGAME_1V3,     0x45, 0x51},
  {"Curvy Curbs",         DR_MINIGAME_1V3,     0x4C, 0x58},
  // 2-vs-2
  {"Clock Stoppers",      DR_MINIGAME_2V2,     0x14, 0x23},
  {"Defuse or Lose",      DR_MINIGAME_2V2,     0x19, 0x28},
  {"ID UFO",              DR_MINIGAME_2V2,     0x1A, 0x29},
  {"Mario Can-Can",       DR_MINIGAME_2V2,     0x1B, 0x2A},
  {"Handy Hoppers",       DR_MINIGAME_2V2,     0x1C, 0x2B},
  {"Berry Basket",        DR_MINIGAME_2V2,     0x1D, 0x2C},
  {"Bus Buffer",          DR_MINIGAME_2V2,     0x1E, 0x2D},
  {"Rumble Ready",        DR_MINIGAME_2V2,     0x1F, 0x2E},
  {"Submarathon",         DR_MINIGAME_2V2,     0x20, 0x2F},
  {"Manic Mallets",       DR_MINIGAME_2V2,     0x21, 0x30},
  {"Panic Pinball",       DR_MINIGAME_2V2,     0x49, 0x55},
  {"Banking Coins",       DR_MINIGAME_2V2,     0x4A, 0x56},
  // Battle
  {"Astro-Logical",       DR_MINIGAME_BATTLE,  0x22, 0x31},
  {"Bill Blasters",       DR_MINIGAME_BATTLE,  0x23, 0x32},
  {"Tug-o-Dorrie",        DR_MINIGAME_BATTLE,  0x24, 0x33},
  {"Twist 'n' Out",       DR_MINIGAME_BATTLE,  0x25, 0x34},
  {"Lucky Lineup",        DR_MINIGAME_BATTLE,  0x26, 0x35},
  {"Random Ride",         DR_MINIGAME_BATTLE,  0x27, 0x36},
  // Duel
  {"Shock Absorbers",     DR_MINIGAME_DUEL,    0x28, 0x37},
  {"Countdown Pound",     DR_MINIGAME_DUEL,    0x29, 0x38},
  {"Whomp Maze",          DR_MINIGAME_DUEL,    0x2A, 0x39},
  {"Shy Guy Showdown",    DR_MINIGAME_DUEL,    0x2B, 0x3A},
  {"Button Mashers",      DR_MINIGAME_DUEL,    0x2C, 0x3B},
  {"Get a Rope",          DR_MINIGAME_DUEL,    0x2D, 0x3C},
  {"Pump 'n' Jump",       DR_MINIGAME_DUEL,    0x2E, 0x3D},
  {"Head Waiter",         DR_MINIGAME_DUEL,    0x2F, 0x3E},
  {"Blown Away",          DR_MINIGAME_DUEL,    0x30, 0x3F},
  {"Merry Poppings",      DR_MINIGAME_DUEL,    0x31, 0x40},
  {"Pound Peril",         DR_MINIGAME_DUEL,    0x32, 0x41},
  {"Piece Out",           DR_MINIGAME_DUEL,    0x33, 0x42},
  {"Bound of Music",      DR_MINIGAME_DUEL,    0x34, 0x43},
  {"Wind Wavers",         DR_MINIGAME_DUEL,    0x35, 0x44},
  {"Sky Survivor",        DR_MINIGAME_DUEL,    0x36, 0x45},
  // Bowser minigames (not selectable)
  {"Rain of Fire",        DR_MINIGAME_INVALID, 0x3A, 0x46},
  {"Cage-in Cookin'",     DR_MINIGAME_INVALID, 0x3B, 0x47},
  {"Scaldin' Cauldron",   DR_MINIGAME_INVALID, 0x3C, 0x48},
  // DK minigames (not selectable)
  {"Banana Punch",        DR_MINIGAME_INVALID, 0x46, 0x52},
  {"Da Vine Climb",       DR_MINIGAME_INVALID, 0x47, 0x53},
  {"Mass A-peel",         DR_MINIGAME_INVALID, 0x48, 0x54},
  // Story / Bonus (not selectable)
  {"Frightmare",          DR_MINIGAME_INVALID, 0x3D, 0x49},
  {"Beach Volleyball",    DR_MINIGAME_INVALID, 0x4D, 0x0B},
  {"Ice Hockey",          DR_MINIGAME_INVALID, 0x4F, 0x5A},
  {nullptr,               DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

static const MpGcnConfig k_config =
{
  .core  = "/media/keith/devtools/libretro/cores/dolphin_libretro.so",
  .game  = "/media/keith/devtools/libretro/roms/Mario Party 5 (USA).rvz",
  .state = "/media/keith/devtools/libretro/state/mp5.state.zip",

  .scene_miniexplain = 0x0007,
  .scene_miniresults = 0x006b,

  .scene_addr    = 0x80288862,
  .minigame_addr = 0x8022A4C4,

  // 8022a090 p1 coins
  // mg star
  // coin star?
  // coin star again?
  // 8022a198 p2 coins
  // 8022a2a0 p3 coins
  // 8022a3a8 p4 coins

  .character_addr  = { 0x8022a048, 0x8022a052, 0x8022a05c, 0x8022a066 },
  .controller_addr = { 0x8022a04a, 0x8022a054, 0x8022a05e, 0x8022a068 },
  .difficulty_addr = { 0x8022a04c, 0x8022a056, 0x8022a060, 0x8022a06a },
  .team_addr       = { 0x8022a04e, 0x8022a058, 0x8022a062, 0x8022a06c },
  .bot_addr        = { 0x8022a050, 0x8022a05a, 0x8022a064, 0x8022a06e },

  .result_addr     = { 0x8022a09c, 0x8022a1a4, 0x8022a2ac, 0x8022a3b4 },

  .character_ids = k_characterIds,
  .minigames     = k_minigames,
};

MarioParty5::MarioParty5(QWindow *parent) : MarioPartyGcn(k_config, parent)
{
}
