#include "MarioParty4.h"

static const uint16_t k_characterIds[DR_CHARACTER_SIZE] =
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
  {"Manta Rings",              DR_MINIGAME_4P,      0x00, 0x09, DR_NO_QUIRKS },
  {"Slime Time",               DR_MINIGAME_4P,      0x01, 0x0A, DR_NO_QUIRKS },
  {"Booksquirm",               DR_MINIGAME_4P,      0x02, 0x0B, DR_NO_QUIRKS },
  {"Trace Race",               DR_MINIGAME_BATTLE,  0x03, 0x0C, DR_QUIRK_SAFE_TEXTURE_CACHE },
  {"Mario Medley",             DR_MINIGAME_4P,      0x04, 0x0D, DR_NO_QUIRKS},
  {"Avalanche!",               DR_MINIGAME_4P,      0x05, 0x0E, DR_NO_QUIRKS},
  {"Domination",               DR_MINIGAME_4P,      0x06, 0x0F, DR_NO_QUIRKS},
  {"Paratrooper Plunge",       DR_MINIGAME_4P,      0x07, 0x10, DR_NO_QUIRKS},
  {"Toad's Quick Draw",        DR_MINIGAME_4P,      0x08, 0x11, DR_NO_QUIRKS},
  {"Three Throw",              DR_MINIGAME_4P,      0x09, 0x12, DR_NO_QUIRKS},
  {"Photo Finish",             DR_MINIGAME_4P,      0x0A, 0x13, DR_NO_QUIRKS},
  {"Mr. Blizzard's Brigade",   DR_MINIGAME_4P,      0x0B, 0x14, DR_NO_QUIRKS},
  {"Bob-omb Breakers",         DR_MINIGAME_4P,      0x0C, 0x15, DR_NO_QUIRKS},
  {"Long Claw of the Law",     DR_MINIGAME_4P,      0x0D, 0x16, DR_NO_QUIRKS},
  {"Stamp Out!",               DR_MINIGAME_4P,      0x0E, 0x17, DR_NO_QUIRKS},
  {"Candlelight Fright",       DR_MINIGAME_1V3,     0x0F, 0x18, DR_NO_QUIRKS},
  {"Makin' Waves",             DR_MINIGAME_1V3,     0x10, 0x19, DR_NO_QUIRKS},
  {"Hide and Go BOOM!",        DR_MINIGAME_1V3,     0x11, 0x1A, DR_NO_QUIRKS},
  {"Tree Stomp",               DR_MINIGAME_1V3,     0x12, 0x1B, DR_NO_QUIRKS},
  {"Fish n' Drips",            DR_MINIGAME_1V3,     0x13, 0x1C, DR_NO_QUIRKS},
  {"Hop or Pop",               DR_MINIGAME_1V3,     0x14, 0x1D, DR_NO_QUIRKS},
  {"Money Belts",              DR_MINIGAME_1V3,     0x15, 0x1E, DR_NO_QUIRKS},
  {"GOOOOOOOAL!!",             DR_MINIGAME_1V3,     0x16, 0x1F, DR_NO_QUIRKS},
  {"Blame it on the Crane",    DR_MINIGAME_1V3,     0x17, 0x20, DR_NO_QUIRKS},
  {"The Great Deflate",        DR_MINIGAME_2V2,     0x18, 0x21, DR_NO_QUIRKS},
  {"Revers-a-Bomb",            DR_MINIGAME_2V2,     0x19, 0x22, DR_NO_QUIRKS},
  {"Right Oar Left?",          DR_MINIGAME_2V2,     0x1A, 0x23, DR_NO_QUIRKS},
  {"Cliffhangers",             DR_MINIGAME_2V2,     0x1B, 0x24, DR_NO_QUIRKS},
  {"Team Treasure Trek",       DR_MINIGAME_2V2,     0x1C, 0x25, DR_NO_QUIRKS},
  {"Pair-a-sailing",           DR_MINIGAME_2V2,     0x1D, 0x26, DR_NO_QUIRKS},
  {"Order Up",                 DR_MINIGAME_2V2,     0x1E, 0x27, DR_NO_QUIRKS},
  {"Dungeon Duos",             DR_MINIGAME_2V2,     0x1F, 0x28, DR_NO_QUIRKS},
  {"Beach Volley Folley",      DR_MINIGAME_DUEL,    0x20, 0x29, DR_NO_QUIRKS},
  {"Cheep Cheep Sweep",        DR_MINIGAME_2V2,     0x21, 0x2A, DR_NO_QUIRKS},
  {"Darts of Doom",            DR_MINIGAME_BATTLE,  0x22, 0x2B, DR_NO_QUIRKS},
  {"Fruits of Doom",           DR_MINIGAME_BATTLE,  0x23, 0x2C, DR_NO_QUIRKS},
  {"Balloon of Doom",          DR_MINIGAME_BATTLE,  0x24, 0x2D, DR_NO_QUIRKS},
  {"Chain Chomp Fever",        DR_MINIGAME_BATTLE,  0x25, 0x2E, DR_NO_QUIRKS},
  {"Paths of Peril",           DR_MINIGAME_BATTLE,  0x26, 0x2F, DR_QUIRK_EFB_TO_TEXTURE },
  {"Bowser's Bigger Blast",    DR_MINIGAME_BATTLE,  0x27, 0x30, DR_NO_QUIRKS},
  {"Butterfly Blitz",          DR_MINIGAME_BATTLE,  0x28, 0x31, DR_NO_QUIRKS},
  {"Barrel Baron",             DR_MINIGAME_DUEL,    0x29, 0x32, DR_NO_QUIRKS},
  {"Mario Speedwagons",        DR_MINIGAME_4P,      0x2A, 0x33, DR_NO_QUIRKS},
  /* 0x2B unknown */
  {"Bowser Bop",               DR_MINIGAME_ITEM,    0x2C, 0x35, DR_NO_QUIRKS},
  {"Mystic Match 'Em",         DR_MINIGAME_ITEM,    0x2D, 0x36, DR_NO_QUIRKS},
  {"Archaeologuess",           DR_MINIGAME_ITEM,    0x2E, 0x37, DR_NO_QUIRKS},
  {"Goomba's Chip Flip",       DR_MINIGAME_ITEM,    0x2F, 0x38, DR_NO_QUIRKS},
  {"Kareening Koopas",         DR_MINIGAME_ITEM,    0x30, 0x39, DR_NO_QUIRKS},
  {"The Final Battle!",        DR_MINIGAME_INVALID, 0x31, 0x3A, DR_NO_QUIRKS},
  {"Jigsaw Jitters",           DR_MINIGAME_INVALID, 0xFF, 0x3B, DR_NO_QUIRKS},
  {"Challenge Booksquirm",     DR_MINIGAME_INVALID, 0xFF, 0x3C, DR_NO_QUIRKS},
  {"Rumble Fishing",           DR_MINIGAME_BATTLE,  0xFF, 0x3D, DR_NO_QUIRKS},
  {"Take a Breather",          DR_MINIGAME_4P,      0xFF, 0x3E, DR_NO_QUIRKS},
  {"Bowser Wrestling",         DR_MINIGAME_DUEL,    0xFF, 0x3F, DR_NO_QUIRKS},
  {"Mushroom Medic",           DR_MINIGAME_ITEM,    0xFF, 0x41, DR_NO_QUIRKS},
  {"Doors of Doom",            DR_MINIGAME_ITEM,    0xFF, 0x42, DR_NO_QUIRKS},
  {"Bob-omb X-ing",            DR_MINIGAME_ITEM,    0xFF, 0x43, DR_NO_QUIRKS},
  {"Goomba Stomp",             DR_MINIGAME_ITEM,    0xFF, 0x44, DR_NO_QUIRKS},
  {"Panel Panic",              DR_MINIGAME_ITEM,    0xFF, 0x45, DR_NO_QUIRKS},
  {nullptr,                    DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS},
};

static const MpGcnConfig k_config =
{
  .core  = "/media/keith/devtools/libretro/cores/dolphin_libretro.so",
  .game  = "/media/keith/devtools/libretro/roms/Mario Party 4 (USA) (Rev 1).rvz",
  .state = "/media/keith/devtools/libretro/state/mp4.state.zip",

  .scene_miniexplain = 0x03,
  .scene_miniresults = 0x54,

  .scene_addr    = 0x801d3ce0,
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

MarioParty4::MarioParty4(QRetro *sharedCore, QObject *parent) : MarioPartyGcn(k_config, sharedCore, parent)
{
}
