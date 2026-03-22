#include "MarioParty4.h"

#define MP4_CORE "/media/keith/devtools/libretro/cores/dolphin_libretro.so"
#define MP4_GAME "/media/keith/devtools/libretro/roms/Mario Party 4 (USA) (Rev 1).rvz"
#define MP4_STATE "/media/keith/devtools/libretro/state/mp4.state"

#define MP4_SCENE_ADDR        0x801d3ce3
#define MP4_MINIGAME_ADDR     0x8018FD2d
#define MP4_SCENE_MINIEXPLAIN 0x03
#define MP4_SCENE_MINIRESULTS 0x54

static const size_t MP4_CHARACTER_ADDR[4]  = { 0x8018fc11, 0x8018fc1b, 0x8018fc25, 0x8018fc2f };
static const size_t MP4_CONTROLLER_ADDR[4] = { 0x8018fc13, 0x8018fc1d, 0x8018fc27, 0x8018fc31 };
static const size_t MP4_DIFFICULTY_ADDR[4] = { 0x8018fc15, 0x8018fc1f, 0x8018fc29, 0x8018fc33 };
static const size_t MP4_BOT_ADDR[4]        = { 0x8018fc19, 0x8018fc23, 0x8018fc2d, 0x8018fc37 };
static const size_t MP4_RESULT_ADDR[4]     = { 0x8018fc61, 0x8018fc91, 0x8018fcc1, 0x8018fcf1 };

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

const dr_mp_minigame_t* MarioParty4::minigames() const
{
  return k_minigames;
}

static const uint8_t MP4_CHARACTER_ID[DR_CHARACTER_SIZE] = {
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0x00,
  [DR_CHARACTER_LUIGI]       = 0x01,
  [DR_CHARACTER_PEACH]       = 0x02,
  [DR_CHARACTER_YOSHI]       = 0x03,
  [DR_CHARACTER_WARIO]       = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
  [DR_CHARACTER_DAISY]       = 0x07,
  [DR_CHARACTER_WALUIGI]     = 0x06,
};

static const uint8_t MP4_DIFFICULTY_ID[DR_DIFFICULTY_SIZE] = {
  [DR_DIFFICULTY_INVALID]   = 0xFF,
  [DR_DIFFICULTY_VERY_EASY] = 0x00,
  [DR_DIFFICULTY_EASY]      = 0x01,
  [DR_DIFFICULTY_NORMAL]    = 0x02,
  [DR_DIFFICULTY_HARD]      = 0x03,
  [DR_DIFFICULTY_VERY_HARD] = 0x04,
};

MarioParty4::MarioParty4(QWindow *parent) : DrGuest(parent)
{
  loadCore(MP4_CORE);
  loadContent(MP4_GAME);

  connect(this, &QRetro::frameBegin, this, [this, last = uint8_t(0xFF)]() mutable {
    if (frames() <= 120)
      return;

    if (m_minigameWriteFrames > 0) {
      memory().writeValue<uint8_t>(m_minigameId, MP4_MINIGAME_ADDR);
      --m_minigameWriteFrames;
    }

    uint8_t val;
    if (memory().readValue<uint8_t>(&val, MP4_SCENE_ADDR) && val != last) {
      printf("MP4_SCENE_ADDR: 0x%02X\n", val);
      last = val;
      if (val == MP4_SCENE_MINIRESULTS)
        emit minigameFinished();
    }
  }, Qt::DirectConnection);
}

void MarioParty4::setMinigame(unsigned id)
{
  unserializeFromFile(MP4_STATE);
  m_minigameId = id;
  m_minigameWriteFrames = 60 * 5;
}

dr_minigame_result_t MarioParty4::minigameResult(unsigned index)
{
  dr_minigame_result_t result = {};

  if (index < 4)
    memory().readValue<uint8_t>((uint8_t*)&result.coins, MP4_RESULT_ADDR[index]);

  return result;
}

dr_error MarioParty4::setPlayerCharacter(unsigned index, dr_character character)
{
  if (index >= 4 || character >= DR_CHARACTER_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  memory().writeValue<uint8_t>(MP4_CHARACTER_ID[character], MP4_CHARACTER_ADDR[index]);

  return DR_OK;
}

dr_error MarioParty4::setPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;
  
  memory().writeValue<uint8_t>(static_cast<uint8_t>(control_port), MP4_CONTROLLER_ADDR[index]);
  
  return DR_OK;
}

dr_error MarioParty4::setPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint8_t current;

  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;
  else if (memory().readValue<uint8_t>(&current, MP4_BOT_ADDR[index]))
  {
    uint8_t isBot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
    memory().writeValue<uint8_t>((current & ~0x01) | isBot, MP4_BOT_ADDR[index]);
  }

  return DR_OK;
}

dr_error MarioParty4::setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  if (index >= 4 || difficulty >= DR_DIFFICULTY_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  memory().writeValue<uint8_t>(MP4_DIFFICULTY_ID[difficulty], MP4_DIFFICULTY_ADDR[index]);

  return DR_OK;
}

dr_error MarioParty4::setPlayerTeam(unsigned index, dr_team team)
{
  (void)index;
  (void)team;
  return DR_ERR_INVALID_PARAMETER;
}
