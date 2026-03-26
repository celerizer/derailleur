#include "MarioParty4.h"

static const uint8_t k_characterIds[DR_CHARACTER_SIZE] =
{
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0x00,
  [DR_CHARACTER_LUIGI]       = 0x01,
  [DR_CHARACTER_PEACH]       = 0x02,
  [DR_CHARACTER_YOSHI]       = 0x03,
  [DR_CHARACTER_WARIO]       = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
  [DR_CHARACTER_WALUIGI]     = 0x07,
  [DR_CHARACTER_DAISY]       = 0x06,
};

static const dr_mp_minigame_t k_minigames[] =
{
  {"Manta Rings",              DR_MINIGAME_4P,      0x00, 0x09},
  {"Slime Time",               DR_MINIGAME_4P,      0x01, 0x0A},
  {"Booksquirm",               DR_MINIGAME_4P,      0x02, 0x0B},
  {"Trace Race",               DR_MINIGAME_BATTLE,  0x03, 0x0C},
  {"Mario Medley",             DR_MINIGAME_4P,      0x04, 0x0D},
  {"Avalanche!",               DR_MINIGAME_4P,      0x05, 0x0E},
  {"Domination",               DR_MINIGAME_4P,      0x06, 0x0F},
  {"Paratrooper Plunge",       DR_MINIGAME_4P,      0x07, 0x10},
  {"Toad's Quick Draw",        DR_MINIGAME_4P,      0x08, 0x11},
  {"Three Throw",              DR_MINIGAME_4P,      0x09, 0x12},
  {"Photo Finish",             DR_MINIGAME_4P,      0x0A, 0x13},
  {"Mr. Blizzard's Brigade",   DR_MINIGAME_4P,      0x0B, 0x14},
  {"Bob-omb Breakers",         DR_MINIGAME_4P,      0x0C, 0x15},
  {"Long Claw of the Law",     DR_MINIGAME_4P,      0x0D, 0x16},
  {"Stamp Out!",               DR_MINIGAME_4P,      0x0E, 0x17},
  {"Candlelight Fright",       DR_MINIGAME_1V3,     0x0F, 0x18},
  {"Makin' Waves",             DR_MINIGAME_1V3,     0x10, 0x19},
  {"Hide and Go BOOM!",        DR_MINIGAME_1V3,     0x11, 0x1A},
  {"Tree Stomp",               DR_MINIGAME_1V3,     0x12, 0x1B},
  {"Fish n' Drips",            DR_MINIGAME_1V3,     0x13, 0x1C},
  {"Hop or Pop",               DR_MINIGAME_1V3,     0x14, 0x1D},
  {"Money Belts",              DR_MINIGAME_1V3,     0x15, 0x1E},
  {"GOOOOOOOAL!!",             DR_MINIGAME_1V3,     0x16, 0x1F},
  {"Blame it on the Crane",    DR_MINIGAME_1V3,     0x17, 0x20},
  {"The Great Deflate",        DR_MINIGAME_2V2,     0x18, 0x21},
  {"Revers-a-Bomb",            DR_MINIGAME_2V2,     0x19, 0x22},
  {"Right Oar Left?",          DR_MINIGAME_2V2,     0x1A, 0x23},
  {"Cliffhangers",             DR_MINIGAME_2V2,     0x1B, 0x24},
  {"Team Treasure Trek",       DR_MINIGAME_2V2,     0x1C, 0x25},
  {"Pair-a-sailing",           DR_MINIGAME_2V2,     0x1D, 0x26},
  {"Order Up",                 DR_MINIGAME_2V2,     0x1E, 0x27},
  {"Dungeon Duos",             DR_MINIGAME_2V2,     0x1F, 0x28},
  {"Beach Volley Folley",      DR_MINIGAME_DUEL,    0x20, 0x29},
  {"Cheep Cheep Sweep",        DR_MINIGAME_2V2,     0x21, 0x2A},
  {"Darts of Doom",            DR_MINIGAME_BATTLE,  0x22, 0x2B},
  {"Fruits of Doom",           DR_MINIGAME_BATTLE,  0x23, 0x2C},
  {"Balloon of Doom",          DR_MINIGAME_BATTLE,  0x24, 0x2D},
  {"Chain Chomp Fever",        DR_MINIGAME_BATTLE,  0x25, 0x2E},
  {"Paths of Peril",           DR_MINIGAME_BATTLE,  0x26, 0x2F},
  {"Bowser's Bigger Blast",    DR_MINIGAME_BATTLE,  0x27, 0x30},
  {"Butterfly Blitz",          DR_MINIGAME_BATTLE,  0x28, 0x31},
  {"Barrel Baron",             DR_MINIGAME_DUEL,    0x29, 0x32},
  {"Mario Speedwagons",        DR_MINIGAME_4P,      0x2A, 0x33},
  /* 0x2B unknown */
  {"Bowser Bop",               DR_MINIGAME_ITEM,    0x2C, 0x35},
  {"Mystic Match 'Em",         DR_MINIGAME_ITEM,    0x2D, 0x36},
  {"Archaeologuess",           DR_MINIGAME_ITEM,    0x2E, 0x37},
  {"Goomba's Chip Flip",       DR_MINIGAME_ITEM,    0x2F, 0x38},
  {"Kareening Koopas",         DR_MINIGAME_ITEM,    0x30, 0x39},
  {"The Final Battle!",        DR_MINIGAME_INVALID, 0x31, 0x3A},
  {"Jigsaw Jitters",           DR_MINIGAME_INVALID, 0xFF, 0x3B},
  {"Challenge Booksquirm",     DR_MINIGAME_INVALID, 0xFF, 0x3C},
  {"Rumble Fishing",           DR_MINIGAME_BATTLE,  0xFF, 0x3D},
  {"Take a Breather",          DR_MINIGAME_4P,      0xFF, 0x3E},
  {"Bowser Wrestling",         DR_MINIGAME_DUEL,    0xFF, 0x3F},
  {"Mushroom Medic",           DR_MINIGAME_ITEM,    0xFF, 0x41},
  {"Doors of Doom",            DR_MINIGAME_ITEM,    0xFF, 0x42},
  {"Bob-omb X-ing",            DR_MINIGAME_ITEM,    0xFF, 0x43},
  {"Goomba Stomp",             DR_MINIGAME_ITEM,    0xFF, 0x44},
  {"Panel Panic",              DR_MINIGAME_ITEM,    0xFF, 0x45},
  {nullptr,                    DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

static const MpGcnConfig k_config =
{
  .core  = "/media/keith/devtools/libretro/cores/dolphin_libretro.so",
  .game  = "/media/keith/devtools/libretro/roms/Mario Party 4 (USA) (Rev 1).rvz",
  .state = "/media/keith/devtools/libretro/state/mp4.state.zip",

  .scene_miniexplain = 0x0003,
  .scene_miniresults = 0x0054,

  .scene_addr    = 0x801d3ce2,
  .minigame_addr = 0x8018fd2c,

  .character_addr  = { 0x8018fc10, 0x8018fc1a, 0x8018fc24, 0x8018fc2e },
  .controller_addr = { 0x8018fc12, 0x8018fc1c, 0x8018fc26, 0x8018fc30 },
  .difficulty_addr = { 0x8018fc14, 0x8018fc1e, 0x8018fc28, 0x8018fc32 },
  .team_addr       = { 0x8018fc16, 0x8018fc20, 0x8018fc2a, 0x8018fc34 },
  .bot_addr        = { 0x8018fc18, 0x8018fc22, 0x8018fc2c, 0x8018fc36 },

  .result_addr     = { 0x8018fc60, 0x8018fc90, 0x8018fcc0, 0x8018fcf0 },

  .character_ids = k_characterIds,
  .minigames     = k_minigames,
};

MarioParty4::MarioParty4(QWindow *parent) : MarioPartyGcn(k_config, parent)
{
}
