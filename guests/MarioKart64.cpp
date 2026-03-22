#include "MarioKart64.h"

#define N64_CORE "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so"
#define N64_GAME "/media/keith/devtools/libretro/roms/Mario Kart 64 (USA).z64"
#define N64_STATE "/media/keith/devtools/libretro/state/mk64.state"

static const size_t MK64_CHARACTER_ADDR[4]  = { 0x8018EDE4, 0x8018EDE5, 0x8018EDE6, 0x8018EDE7 };

static const size_t MK64_COURSE_ADDR = 0x8018EE08;
static const size_t MK64_CUP_ADDR = 0x8018EE0A;

static const size_t MK64_NUMBER_PLAYERS_ADDR = 0x8018EDF0;

static const dr_mp_minigame_t k_minigames[] =
{
  {"Single Race: Luigi Raceway",        DR_MINIGAME_4P, 0x00, 0xFF},
  {"Single Race: Moo Moo Farm",         DR_MINIGAME_4P, 0x01, 0xFF},
  {"Single Race: Koopa Troopa Beach",   DR_MINIGAME_4P, 0x02, 0xFF},
  {"Single Race: Kalimari Desert",      DR_MINIGAME_4P, 0x03, 0xFF},
  {"Single Race: Toad's Turnpike",      DR_MINIGAME_4P, 0x04, 0xFF},
  {"Single Race: Frappe Snowland",      DR_MINIGAME_4P, 0x05, 0xFF},
  {"Single Race: Choco Mountain",       DR_MINIGAME_4P, 0x06, 0xFF},
  {"Single Race: Mario Raceway",        DR_MINIGAME_4P, 0x07, 0xFF},
  {"Single Race: Wario Stadium",        DR_MINIGAME_4P, 0x08, 0xFF},
  {"Single Race: Sherbet Land",         DR_MINIGAME_4P, 0x09, 0xFF},
  {"Single Race: Royal Raceway",        DR_MINIGAME_4P, 0x0A, 0xFF},
  {"Single Race: Bowser's Castle",      DR_MINIGAME_4P, 0x0B, 0xFF},
  {"Single Race: D.K.'s Jungle Parkway",DR_MINIGAME_4P, 0x0C, 0xFF},
  {"Single Race: Yoshi Valley",         DR_MINIGAME_4P, 0x0D, 0xFF},
  {"Single Race: Banshee Boardwalk",    DR_MINIGAME_4P, 0x0E, 0xFF},
  {"Single Race: Rainbow Road",         DR_MINIGAME_4P, 0x0F, 0xFF},
  {nullptr,                             DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

static const uint8_t MK64_CHARACTER_ID[DR_CHARACTER_SIZE] = {
  [DR_CHARACTER_INVALID]     = 0x00,

  [DR_CHARACTER_MARIO]       = 0x01,
  [DR_CHARACTER_LUIGI]       = 0x02,
  [DR_CHARACTER_PEACH]       = 0x03,
  [DR_CHARACTER_YOSHI]       = 0x05,
  [DR_CHARACTER_WARIO]       = 0x07,
  [DR_CHARACTER_DONKEY_KONG] = 0x06,

  [DR_CHARACTER_DAISY]       = 0x04, // Toad
  [DR_CHARACTER_WALUIGI]     = 0x08, // Bowser
};

MarioKart64::MarioKart64(QWindow *parent) : DrGuest(parent)
{
  loadCore(N64_CORE);
  loadContent(N64_GAME);
}

const dr_mp_minigame_t* MarioKart64::minigames() const
{
  return k_minigames;
}

void MarioKart64::setMinigame(unsigned id)
{
  unserializeFromFile(N64_STATE);
  memory().writeValue<uint8_t>(id % 4, MK64_COURSE_ADDR);
  memory().writeValue<uint8_t>(id / 4, MK64_CUP_ADDR);
}

dr_minigame_result_t MarioKart64::minigameResult(unsigned index)
{
  (void)index;
  return {};
}

dr_error MarioKart64::setPlayerCharacter(unsigned index, dr_character character)
{
  if (index >= 4 || character >= DR_CHARACTER_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  m_characters[index] = character;

  unsigned slot = m_ports[index] - DR_CONTROL_PORT_P1;
  memory().writeValue<uint8_t>(MK64_CHARACTER_ID[character], MK64_CHARACTER_ADDR[slot]);

  return DR_OK;
}

dr_error MarioKart64::setPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index >= 4 || control_port == DR_CONTROL_PORT_INVALID || control_port >= DR_CONTROL_PORT_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  m_ports[index] = control_port;

  unsigned slot = control_port - DR_CONTROL_PORT_P1;
  memory().writeValue<uint8_t>(MK64_CHARACTER_ID[m_characters[index]], MK64_CHARACTER_ADDR[slot]);

  return DR_OK;
}

dr_error MarioKart64::setPlayerControlType(unsigned index, dr_control_type control_type)
{
  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;

  m_controlTypes[index] = control_type;

  unsigned humanCount = 0;
  for (unsigned i = 0; i < 4; i++)
    if (m_controlTypes[i] == DR_CONTROL_TYPE_HUMAN)
      humanCount++;
  memory().writeValue<uint8_t>(static_cast<uint8_t>(humanCount), MK64_NUMBER_PLAYERS_ADDR);

  return DR_OK;
}

/* No difficulty settings */
dr_error MarioKart64::setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  (void)index;
  (void)difficulty;
  return DR_OK;
}

/* All minigames are 4P, so no teams */
dr_error MarioKart64::setPlayerTeam(unsigned index, dr_team team)
{
  (void)index;
  (void)team;
  return DR_OK;
}
