#include "MarioParty6.h"

static const uint8_t k_characterIds[DR_CHARACTER_SIZE] =
{
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0xFF, // TODO
  [DR_CHARACTER_LUIGI]       = 0xFF, // TODO
  [DR_CHARACTER_PEACH]       = 0xFF, // TODO
  [DR_CHARACTER_YOSHI]       = 0xFF, // TODO
  [DR_CHARACTER_WARIO]       = 0xFF, // TODO
  [DR_CHARACTER_DONKEY_KONG] = 0xFF, // TODO
  [DR_CHARACTER_WALUIGI]     = 0xFF, // TODO
  [DR_CHARACTER_DAISY]       = 0xFF, // TODO
};

static const dr_mp_minigame_t k_minigames[] =
{
  // 4-Player
  {"Smashdance",           DR_MINIGAME_4P,      0x00, 0x06},
  {"Odd Card Out",         DR_MINIGAME_4P,      0x01, 0x07},
  {"Freeze Frame",         DR_MINIGAME_4P,      0x02, 0x08},
  {"What Goes Up...",      DR_MINIGAME_4P,      0x03, 0x09},
  {"Granite Getaway",      DR_MINIGAME_4P,      0x04, 0x0A},
  {"Circuit Maximus",      DR_MINIGAME_4P,      0x05, 0x0B},
  {"Catch You Letter",     DR_MINIGAME_4P,      0x06, 0x0C},
  {"Snow Whirled",         DR_MINIGAME_4P,      0x07, 0x0D},
  {"Daft Rafts",           DR_MINIGAME_4P,      0x08, 0x0E},
  {"Tricky Tires",         DR_MINIGAME_4P,      0x09, 0x0F},
  {"Treasure Trawlers",    DR_MINIGAME_4P,      0x0A, 0x10},
  {"Memory Lane",          DR_MINIGAME_4P,      0x0B, 0x11},
  {"Mowtown",              DR_MINIGAME_4P,      0x0C, 0x12},
  {"Cannonball Fun",       DR_MINIGAME_4P,      0x0D, 0x13},
  {"Note to Self",         DR_MINIGAME_4P,      0x0E, 0x14},
  {"Same Is Lame",         DR_MINIGAME_4P,      0x0F, 0x15},
  {"Lift Leapers",         DR_MINIGAME_4P,      0x11, 0x17},
  {"Blooper Scooper",      DR_MINIGAME_4P,      0x12, 0x18},
  {"Trap Ease Artist",     DR_MINIGAME_4P,      0x13, 0x19},
  {"Pokey Punch-out",      DR_MINIGAME_4P,      0x14, 0x1A},
  {"Money Belt",           DR_MINIGAME_4P,      0x15, 0x1B},
  {"Sunday Drivers",       DR_MINIGAME_4P,      0x2F, 0x35},
  {"Throw Me a Bone",      DR_MINIGAME_4P,      0x31, 0x37},
  // 1-vs-3
  {"Cash Flow",            DR_MINIGAME_1V3,     0x16, 0x1C},
  {"Sink or Swim",         DR_MINIGAME_1V3,     0x18, 0x1E},
  {"Snow Brawl",           DR_MINIGAME_1V3,     0x19, 0x1F},
  {"Ball Dozers",          DR_MINIGAME_1V3,     0x1A, 0x20},
  {"Surge and Destroy",    DR_MINIGAME_1V3,     0x1B, 0x21},
  {"Pop Star",             DR_MINIGAME_1V3,     0x1C, 0x22},
  {"Stage Fright",         DR_MINIGAME_1V3,     0x1D, 0x23},
  {"Conveyor Bolt",        DR_MINIGAME_1V3,     0x1E, 0x24},
  {"Crate and Peril",      DR_MINIGAME_1V3,     0x1F, 0x25},
  {"Ray of Fright",        DR_MINIGAME_1V3,     0x20, 0x26},
  {"Dust 'til Dawn",       DR_MINIGAME_1V3,     0x21, 0x27},
  // 2-vs-2
  {"Garden Grab",          DR_MINIGAME_2V2,     0x22, 0x28},
  {"Pixel Perfect",        DR_MINIGAME_2V2,     0x23, 0x29},
  {"Slot Trot",            DR_MINIGAME_2V2,     0x24, 0x2A},
  {"Gondola Glide",        DR_MINIGAME_2V2,     0x25, 0x2B},
  {"Light Breeze",         DR_MINIGAME_2V2,     0x26, 0x2C},
  {"Body Builder",         DR_MINIGAME_2V2,     0x27, 0x2D},
  {"Mole-it!",             DR_MINIGAME_2V2,     0x28, 0x2E},
  {"Cashapult",            DR_MINIGAME_2V2,     0x29, 0x2F},
  {"Jump the Gun",         DR_MINIGAME_2V2,     0x2A, 0x30},
  {"Rocky Road",           DR_MINIGAME_2V2,     0x2B, 0x31},
  {"Clean Team",           DR_MINIGAME_2V2,     0x2C, 0x32},
  {"Burnstile",            DR_MINIGAME_2V2,     0x43, 0x46},
  // Battle
  {"Hyper Sniper",         DR_MINIGAME_BATTLE,  0x2D, 0x33},
  {"Insectiride",          DR_MINIGAME_BATTLE,  0x2E, 0x34},
  {"Stamp By Me",          DR_MINIGAME_BATTLE,  0x30, 0x36},
  {"Wrasslin' Rapids",     DR_MINIGAME_BATTLE,  0x3F, 0x45},
  {"Strawberry Shortfuse", DR_MINIGAME_BATTLE,  0x4F, 0x55},
  {"Control Shtick",       DR_MINIGAME_BATTLE,  0x50, 0x56},
  // Duel
  {"Light Up My Night",    DR_MINIGAME_DUEL,    0x10, 0x16},
  {"Cog Jog",              DR_MINIGAME_DUEL,    0x17, 0x1D},
  {"Black Hole Boogie",    DR_MINIGAME_DUEL,    0x32, 0x38},
  {"Full Tilt",            DR_MINIGAME_DUEL,    0x33, 0x39},
  {"Sumo of Doom-o",       DR_MINIGAME_DUEL,    0x34, 0x3A},
  {"O-Zone",               DR_MINIGAME_DUEL,    0x35, 0x3B},
  {"Pitifall",             DR_MINIGAME_DUEL,    0x36, 0x3C},
  {"Mass Meteor",          DR_MINIGAME_DUEL,    0x37, 0x3D},
  {"Lunar-tics",           DR_MINIGAME_DUEL,    0x38, 0x3E},
  {"T Minus Five",         DR_MINIGAME_DUEL,    0x39, 0x3F},
  {"Asteroad Rage",        DR_MINIGAME_DUEL,    0x3A, 0x40},
  {"Boo'd Off the Stage",  DR_MINIGAME_DUEL,    0x3B, 0x41},
  {"Boonanza!",            DR_MINIGAME_DUEL,    0x3C, 0x42},
  {"Trick or Tree",        DR_MINIGAME_DUEL,    0x3D, 0x43},
  {"Something's Amist",    DR_MINIGAME_DUEL,    0x3E, 0x44},
  // Mic minigames (not selectable)
  {"Verbal Assault",       DR_MINIGAME_INVALID, 0x40, 0x49},
  {"Shoot Yer Mouth Off",  DR_MINIGAME_INVALID, 0x41, 0x4A},
  {"Talkie Walkie",        DR_MINIGAME_INVALID, 0x42, 0x4B},
  {"Word Herd",            DR_MINIGAME_INVALID, 0x44, 0x47},
  {"Fruit Talktail",       DR_MINIGAME_INVALID, 0x45, 0x48},
  // Bowser minigames (not selectable)
  {"Pit Boss",             DR_MINIGAME_INVALID, 0x46, 0x4C},
  {"Dizzy Rotisserie",     DR_MINIGAME_INVALID, 0x47, 0x4D},
  {"Dark 'n Crispy",       DR_MINIGAME_INVALID, 0x48, 0x4E},
  // DK minigames (not selectable)
  {"Tally Me Banana",      DR_MINIGAME_INVALID, 0x49, 0x4F},
  {"Banana Shake",         DR_MINIGAME_INVALID, 0x4A, 0x50},
  {"Pier Factor",          DR_MINIGAME_INVALID, 0x4B, 0x51},
  // Rare minigames (not selectable)
  {"Seer Terror",          DR_MINIGAME_INVALID, 0x4C, 0x52},
  {"Block Star",           DR_MINIGAME_INVALID, 0x4D, 0x53},
  {"Lab Brats",            DR_MINIGAME_INVALID, 0x4E, 0x54},
  {"Dunk Bros.",           DR_MINIGAME_INVALID, 0x51, 0x57},
  {nullptr,                DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

static const MpGcnConfig k_config =
{
  .core  = "", // TODO
  .game  = "", // TODO
  .state = "", // TODO

  .scene_miniexplain = 0x04,
  .scene_miniresults = 0x71,

  .scene_addr    = 0x802C0257,
  .minigame_addr = 0x80265BA9,

  .character_addr  = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .controller_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .difficulty_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .team_addr       = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .bot_addr        = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .result_addr     = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO

  .character_ids = k_characterIds,
  .minigames     = k_minigames,
};

MarioParty6::MarioParty6(QWindow *parent) : MarioPartyGcn(k_config, parent)
{
}
