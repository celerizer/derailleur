#include "MarioKart64.h"

#define N64_CORE "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so"
#define N64_GAME "/media/keith/devtools/libretro/roms/Mario Kart 64 (USA).z64"
#define N64_STATE "/media/keith/devtools/libretro/state/mk64.state.zip"

static const size_t MK64_MENU_CHAR_ADDR[4] = { 0x8018EDE7, 0x8018EDE6, 0x8018EDE5, 0x8018EDE4 };
static const size_t MK64_REAL_CHAR_ADDR[4] = { 0x800e86ab, 0x800e86aa, 0x800e86a9, 0x800e86a8 };

static const size_t MK64_COURSE_ADDR = 0x8018EE08;
static const size_t MK64_CUP_ADDR = 0x8018EE0A;
static const size_t MK64_TRACK_ADDR = 0x800dc5a2;

static const size_t MK64_LAPS_ADDR[4] = { 0x80164390, 0x80164394, 0x80164398, 0x8016439C };

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

static const uint8_t MK64_TRACK_ID[] = {
  0x08, // 0x00 Luigi Raceway
  0x09, // 0x01 Moo Moo Farm
  0x06, // 0x02 Koopa Troopa Beach
  0x0B, // 0x03 Kalimari Desert
  0x0A, // 0x04 Toad's Turnpike
  0x05, // 0x05 Frappe Snowland
  0x01, // 0x06 Choco Mountain
  0x00, // 0x07 Mario Raceway
  0x0E, // 0x08 Wario Stadium
  0x0C, // 0x09 Sherbet Land
  0x07, // 0x0A Royal Raceway
  0x02, // 0x0B Bowser's Castle
  0x12, // 0x0C D.K.'s Jungle Parkway
  0x04, // 0x0D Yoshi Valley
  0x03, // 0x0E Banshee Boardwalk
  0x0D, // 0x0F Rainbow Road
};

typedef struct
{
  dr_character character;
  unsigned menu_value;
  unsigned real_value;
} mk64_character_t;

static const mk64_character_t MK64_CHARACTER_ID[] =
{
  { DR_CHARACTER_MARIO, 0x01, 0x00 },
  { DR_CHARACTER_LUIGI, 0x02, 0x01 },
  { DR_CHARACTER_PEACH, 0x03, 0x06 },
  { DR_CHARACTER_YOSHI, 0x05, 0x02 },
  { DR_CHARACTER_WARIO, 0x07, 0x05 },
  { DR_CHARACTER_DONKEY_KONG, 0x06, 0x04 },

  { DR_CHARACTER_DAISY, 0x04, 0x03 }, // Toad
  { DR_CHARACTER_WALUIGI, 0x08, 0x07 }, // Bowser
};

MarioKart64::MarioKart64(QWindow *parent) : DrGuest(parent)
{
  loadCore(N64_CORE);
  loadContent(N64_GAME);

  connect(this, &QRetro::frameBegin, this, [this]() {
    if (m_lapsFreezeFrames > 0) {
      --m_lapsFreezeFrames;
      for (unsigned i = 0; i < 4; i++)
        memory().writeValue<uint32_t>(1, MK64_LAPS_ADDR[i]);
    }

    if (m_winnerIndex == -1) {
      for (unsigned i = 0; i < 4; i++) {
        uint32_t laps;
        if (memory().readValue<uint32_t>(&laps, MK64_LAPS_ADDR[i]) && laps >= 3) {
          m_winnerIndex = m_slotToIndex[i];
          m_finishCountdown = 240;
          break;
        }
      }
    } else if (m_finishCountdown > 0) {
      if (--m_finishCountdown == 0)
        emit minigameFinished();
    }
  }, Qt::DirectConnection);
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

  if (id < sizeof(MK64_TRACK_ID) / sizeof(*MK64_TRACK_ID))
    memory().writeValue<uint8_t>(MK64_TRACK_ID[id], MK64_TRACK_ADDR);

  m_lapsFreezeFrames = 300;
  m_finishCountdown  = -1;
  m_winnerIndex      = -1;
  for (unsigned i = 0; i < 4; i++)
    m_slotToIndex[i] = -1;
}

dr_minigame_result_t MarioKart64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if ((int)index == m_winnerIndex)
    result.coins = 10;

  return result;
}

dr_error MarioKart64::doSetPlayerCharacter(unsigned index, dr_character character)
{
  m_characters[index] = character;

  for (const auto &entry : MK64_CHARACTER_ID) {
    if (entry.character == character) {
      unsigned slot = m_ports[index] - DR_CONTROL_PORT_P1;
      memory().writeValue<uint8_t>(entry.menu_value, MK64_MENU_CHAR_ADDR[slot]);
      memory().writeValue<uint8_t>(entry.real_value, MK64_REAL_CHAR_ADDR[slot]);
      break;
    }
  }

  return DR_OK;
}

dr_error MarioKart64::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  m_ports[index] = control_port;
  m_slotToIndex[control_port - DR_CONTROL_PORT_P1] = (int)index;
  doSetPlayerCharacter(index, m_characters[index]);
  return DR_OK;
}

dr_error MarioKart64::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  m_controlTypes[index] = control_type;

  unsigned humanCount = 0;
  for (unsigned i = 0; i < 4; i++)
    if (m_controlTypes[i] == DR_CONTROL_TYPE_HUMAN)
      humanCount++;
  //memory().writeValue<uint8_t>(static_cast<uint8_t>(humanCount), MK64_NUMBER_PLAYERS_ADDR);

  return DR_OK;
}

/* No difficulty settings */
dr_error MarioKart64::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  (void)index;
  (void)difficulty;
  return DR_OK;
}

/* All minigames are 4P, so no teams */
dr_error MarioKart64::doSetPlayerTeam(unsigned index, dr_team team)
{
  (void)index;
  (void)team;
  return DR_OK;
}
