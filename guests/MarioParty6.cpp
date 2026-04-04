#include "MarioParty6.h"

static const uint16_t k_characterIds[DR_CHARACTER_SIZE] =
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
  {"Smashdance",           DR_MINIGAME_4P,      0x00, 0x06, DR_NO_QUIRKS},
  {"Odd Card Out",         DR_MINIGAME_4P,      0x01, 0x07, DR_NO_QUIRKS},
  {"Freeze Frame",         DR_MINIGAME_4P,      0x02, 0x08, DR_NO_QUIRKS},
  {"What Goes Up...",      DR_MINIGAME_4P,      0x03, 0x09, DR_NO_QUIRKS},
  {"Granite Getaway",      DR_MINIGAME_4P,      0x04, 0x0A, DR_NO_QUIRKS},
  {"Circuit Maximus",      DR_MINIGAME_4P,      0x05, 0x0B, DR_NO_QUIRKS},
  {"Catch You Letter",     DR_MINIGAME_4P,      0x06, 0x0C, DR_NO_QUIRKS},
  {"Snow Whirled",         DR_MINIGAME_4P,      0x07, 0x0D, DR_NO_QUIRKS},
  {"Daft Rafts",           DR_MINIGAME_4P,      0x08, 0x0E, DR_NO_QUIRKS},
  {"Tricky Tires",         DR_MINIGAME_4P,      0x09, 0x0F, DR_NO_QUIRKS},
  {"Treasure Trawlers",    DR_MINIGAME_4P,      0x0A, 0x10, DR_NO_QUIRKS},
  {"Memory Lane",          DR_MINIGAME_4P,      0x0B, 0x11, DR_NO_QUIRKS},
  {"Mowtown",              DR_MINIGAME_4P,      0x0C, 0x12, DR_QUIRK_SAFE_TEXTURE_CACHE},
  {"Cannonball Fun",       DR_MINIGAME_4P,      0x0D, 0x13, DR_NO_QUIRKS},
  {"Note to Self",         DR_MINIGAME_4P,      0x0E, 0x14, DR_NO_QUIRKS},
  {"Same Is Lame",         DR_MINIGAME_4P,      0x0F, 0x15, DR_NO_QUIRKS},
  {"Lift Leapers",         DR_MINIGAME_4P,      0x11, 0x17, DR_NO_QUIRKS},
  {"Blooper Scooper",      DR_MINIGAME_4P,      0x12, 0x18, DR_NO_QUIRKS},
  {"Trap Ease Artist",     DR_MINIGAME_4P,      0x13, 0x19, DR_NO_QUIRKS},
  {"Pokey Punch-out",      DR_MINIGAME_4P,      0x14, 0x1A, DR_NO_QUIRKS},
  {"Money Belt",           DR_MINIGAME_4P,      0x15, 0x1B, DR_NO_QUIRKS},
  {"Sunday Drivers",       DR_MINIGAME_4P,      0x2F, 0x35, DR_NO_QUIRKS},
  {"Throw Me a Bone",      DR_MINIGAME_4P,      0x31, 0x37, DR_NO_QUIRKS},
  // 1-vs-3
  {"Cash Flow",            DR_MINIGAME_1V3,     0x16, 0x1C, DR_NO_QUIRKS},
  {"Sink or Swim",         DR_MINIGAME_1V3,     0x18, 0x1E, DR_NO_QUIRKS},
  {"Snow Brawl",           DR_MINIGAME_1V3,     0x19, 0x1F, DR_NO_QUIRKS},
  {"Ball Dozers",          DR_MINIGAME_1V3,     0x1A, 0x20, DR_NO_QUIRKS},
  {"Surge and Destroy",    DR_MINIGAME_1V3,     0x1B, 0x21, DR_NO_QUIRKS},
  {"Pop Star",             DR_MINIGAME_1V3,     0x1C, 0x22, DR_NO_QUIRKS},
  {"Stage Fright",         DR_MINIGAME_1V3,     0x1D, 0x23, DR_NO_QUIRKS},
  {"Conveyor Bolt",        DR_MINIGAME_1V3,     0x1E, 0x24, DR_NO_QUIRKS},
  {"Crate and Peril",      DR_MINIGAME_1V3,     0x1F, 0x25, DR_NO_QUIRKS},
  {"Ray of Fright",        DR_MINIGAME_1V3,     0x20, 0x26, DR_NO_QUIRKS},
  {"Dust 'til Dawn",       DR_MINIGAME_1V3,     0x21, 0x27, DR_NO_QUIRKS},
  // 2-vs-2
  {"Garden Grab",          DR_MINIGAME_2V2,     0x22, 0x28, DR_NO_QUIRKS},
  {"Pixel Perfect",        DR_MINIGAME_2V2,     0x23, 0x29, DR_NO_QUIRKS},
  {"Slot Trot",            DR_MINIGAME_2V2,     0x24, 0x2A, DR_NO_QUIRKS},
  {"Gondola Glide",        DR_MINIGAME_2V2,     0x25, 0x2B, DR_NO_QUIRKS},
  {"Light Breeze",         DR_MINIGAME_2V2,     0x26, 0x2C, DR_NO_QUIRKS},
  {"Body Builder",         DR_MINIGAME_2V2,     0x27, 0x2D, DR_NO_QUIRKS},
  {"Mole-it!",             DR_MINIGAME_2V2,     0x28, 0x2E, DR_NO_QUIRKS},
  {"Cashapult",            DR_MINIGAME_2V2,     0x29, 0x2F, DR_NO_QUIRKS},
  {"Jump the Gun",         DR_MINIGAME_2V2,     0x2A, 0x30, DR_NO_QUIRKS},
  {"Rocky Road",           DR_MINIGAME_2V2,     0x2B, 0x31, DR_NO_QUIRKS},
  {"Clean Team",           DR_MINIGAME_2V2,     0x2C, 0x32, DR_NO_QUIRKS},
  {"Burnstile",            DR_MINIGAME_2V2,     0x43, 0x46, DR_NO_QUIRKS},
  // Battle
  {"Hyper Sniper",         DR_MINIGAME_BATTLE,  0x2D, 0x33, DR_NO_QUIRKS},
  {"Insectiride",          DR_MINIGAME_BATTLE,  0x2E, 0x34, DR_NO_QUIRKS},
  {"Stamp By Me",          DR_MINIGAME_BATTLE,  0x30, 0x36, DR_NO_QUIRKS},
  {"Wrasslin' Rapids",     DR_MINIGAME_BATTLE,  0x3F, 0x45, DR_NO_QUIRKS},
  {"Strawberry Shortfuse", DR_MINIGAME_BATTLE,  0x4F, 0x55, DR_NO_QUIRKS},
  {"Control Shtick",       DR_MINIGAME_BATTLE,  0x50, 0x56, DR_NO_QUIRKS},
  // Duel
  {"Light Up My Night",    DR_MINIGAME_DUEL,    0x10, 0x16, DR_NO_QUIRKS},
  {"Cog Jog",              DR_MINIGAME_DUEL,    0x17, 0x1D, DR_NO_QUIRKS},
  {"Black Hole Boogie",    DR_MINIGAME_DUEL,    0x32, 0x38, DR_NO_QUIRKS},
  {"Full Tilt",            DR_MINIGAME_DUEL,    0x33, 0x39, DR_NO_QUIRKS},
  {"Sumo of Doom-o",       DR_MINIGAME_DUEL,    0x34, 0x3A, DR_NO_QUIRKS},
  {"O-Zone",               DR_MINIGAME_DUEL,    0x35, 0x3B, DR_NO_QUIRKS},
  {"Pitifall",             DR_MINIGAME_DUEL,    0x36, 0x3C, DR_NO_QUIRKS},
  {"Mass Meteor",          DR_MINIGAME_DUEL,    0x37, 0x3D, DR_NO_QUIRKS},
  {"Lunar-tics",           DR_MINIGAME_DUEL,    0x38, 0x3E, DR_NO_QUIRKS},
  {"T Minus Five",         DR_MINIGAME_DUEL,    0x39, 0x3F, DR_NO_QUIRKS},
  {"Asteroad Rage",        DR_MINIGAME_DUEL,    0x3A, 0x40, DR_NO_QUIRKS},
  {"Boo'd Off the Stage",  DR_MINIGAME_DUEL,    0x3B, 0x41, DR_NO_QUIRKS},
  {"Boonanza!",            DR_MINIGAME_DUEL,    0x3C, 0x42, DR_NO_QUIRKS},
  {"Trick or Tree",        DR_MINIGAME_DUEL,    0x3D, 0x43, DR_NO_QUIRKS},
  {"Something's Amist",    DR_MINIGAME_DUEL,    0x3E, 0x44, DR_NO_QUIRKS},
  // Mic minigames (not selectable)
  {"Verbal Assault",       DR_MINIGAME_INVALID, 0x40, 0x49, DR_NO_QUIRKS},
  {"Shoot Yer Mouth Off",  DR_MINIGAME_INVALID, 0x41, 0x4A, DR_NO_QUIRKS},
  {"Talkie Walkie",        DR_MINIGAME_INVALID, 0x42, 0x4B, DR_NO_QUIRKS},
  {"Word Herd",            DR_MINIGAME_INVALID, 0x44, 0x47, DR_NO_QUIRKS},
  {"Fruit Talktail",       DR_MINIGAME_INVALID, 0x45, 0x48, DR_NO_QUIRKS},
  // Bowser minigames (not selectable)
  {"Pit Boss",             DR_MINIGAME_INVALID, 0x46, 0x4C, DR_NO_QUIRKS},
  {"Dizzy Rotisserie",     DR_MINIGAME_INVALID, 0x47, 0x4D, DR_NO_QUIRKS},
  {"Dark 'n Crispy",       DR_MINIGAME_INVALID, 0x48, 0x4E, DR_NO_QUIRKS},
  // DK minigames (not selectable)
  {"Tally Me Banana",      DR_MINIGAME_INVALID, 0x49, 0x4F, DR_NO_QUIRKS},
  {"Banana Shake",         DR_MINIGAME_INVALID, 0x4A, 0x50, DR_NO_QUIRKS},
  {"Pier Factor",          DR_MINIGAME_INVALID, 0x4B, 0x51, DR_NO_QUIRKS},
  // Rare minigames (not selectable)
  {"Seer Terror",          DR_MINIGAME_INVALID, 0x4C, 0x52, DR_NO_QUIRKS},
  {"Block Star",           DR_MINIGAME_INVALID, 0x4D, 0x53, DR_NO_QUIRKS},
  {"Lab Brats",            DR_MINIGAME_INVALID, 0x4E, 0x54, DR_NO_QUIRKS},
  {"Dunk Bros.",           DR_MINIGAME_INVALID, 0x51, 0x57, DR_NO_QUIRKS},
  {nullptr,                DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS},
};

static const MpGcnConfig k_config =
{
  .core  = "/media/keith/devtools/libretro/cores/dolphin_libretro.so",
  .game  = "/media/keith/devtools/libretro/roms/Mario Party 6 (USA).rvz",
  .state = "/media/keith/devtools/libretro/state/mp6.state.zip",

  .scene_miniexplain = 0x04,
  .scene_miniresults = 0x71,

  .scene_addr    = 0x802C0254,
  .minigame_addr = 0x80265BA8,

  .character_addr  = { 0x80265728, 0x80265732, 0x8026573c, 0x80265746 },
  .controller_addr = { 0x8026572a, 0x80265734, 0x8026573e, 0x80265748 },
  .difficulty_addr = { 0x8026572c, 0x80265736, 0x80265740, 0x8026574a },
  .team_addr       = { 0x8026572e, 0x80265738, 0x80265742, 0x8026574c },
  .bot_addr        = { 0x80265730, 0x8026573a, 0x80265744, 0x8026574e },

  // player struct 2 -- size 0x108
  //.coin_addr       = { 0x8026576c, 0x80265874, 0x8026597c, 0x80265a84 },
  //.game_star_addr   = { 0x8026576e, 0x80265876, 0x8026597e, 0x80265a86 },
  // coin star
  // coin star again?
  // idk
  //.bonus_result_addr     = { 0x80265776, 0x00000000, 0x00000000, 0x00000000 },
  .result_addr     = { 0x80265778, 0x80265880, 0x80265988, 0x80265a90 },

  //.stars_addr     = { 0x80265780, 0x00000000, 0x00000000, 0x00000000 },
  //.max_stars?_addr     = { 0x80265782, 0x00000000, 0x00000000, 0x00000000 },

  .character_ids = k_characterIds,
  .minigames     = k_minigames,
};

MarioParty6::MarioParty6(QRetro *sharedCore, QObject *parent) : MarioPartyGcn(k_config, sharedCore, parent)
{
}
