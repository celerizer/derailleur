#include "SmashRemix.h"

#define N64_CORE "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so"
#define N64_GAME "/media/keith/devtools/libretro/roms/smashremix.z64"
#define N64_STATE "/media/keith/devtools/libretro/state/smashremix.state.zip"

static const dr_mp_minigame_t k_minigames[] =
{
  {"Smash: Free-for-all", DR_MINIGAME_4P, 0x00, 0xFF},

  {"Smash: Team Battle", DR_MINIGAME_2V2, 0x01, 0xFF},

  {"Smash: Giant Battle", DR_MINIGAME_1V3, 0x02, 0xFF},

  {nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

/* Whether the game is in team battle mode */
static const size_t SR_GAME_TYPE_ADDR = 0x800a4d09;
static const size_t SR_STAGE_ADDR = 0x800a4d0a;

static const size_t SR_CHARACTER_ADDR[4] = { 0x800a4d28, 0x800a4d9c, 0x800a4e10, 0x800a4e84 };

/* If this value is 1, the player is CPU-controlled */
static const size_t SR_IS_BOT_ADDR[4] = { 0x800a4d29, 0x800a4d9d, 0x800a4e11, 0x800a4e85 };

static const size_t SR_DIFFICULTY_ADDR[4] = { 0x800a4d2b, 0x800a4daf, 0x800a4e13, 0x800a4e87 };

static const size_t SR_COLOR_ADDR[4] = { 0x800a4d2d, 0x800a4da1, 0x800a4e15, 0x800a4e89 };

static const size_t SR_TEAM_ADDR_1[4] = { 0x800a4d2e, 0x800a4da2, 0x800a4e16, 0x800a4e8a };
static const size_t SR_TEAM_ADDR_2[4] = { 0x800a4d2f, 0x800a4da3, 0x800a4e17, 0x800a4e8b };

static const size_t SR_STOCKS_ADDR[4] = { 0x800a4d30, 0x800a4da4, 0x800a4e18, 0x800a4e8c };

/* The controller port for this player, or 4 if CPU-controlled */
static const size_t SR_PORT_ADDR[4] = { 0x800a4d31, 0x800a4da5, 0x800a4e19, 0x800a4e8d };

/* The color drawn behind the percentage, should match SR_PORT_ADDR */
static const size_t SR_PORT_COLOR_ADDR[4] = { 0x800a4d33, 0x800a4da7, 0x800a4e1b, 0x800a4e8f };

/* The size of the player. 0=normal, 1=giant, 2=tiny */
static const size_t SR_SIZE_ADDR_1[4] = { 0x80502fac, 0x80502fb0, 0x80502fb4, 0x80502fb8 };
static const size_t SR_SIZE_ADDR_2[4] = { 0x80502fbc, 0x80502fc0, 0x80502fc4, 0x80502fc8 };

typedef struct
{
  dr_character character;
  unsigned character_value;
  unsigned color_value;
} sr_character_t;

static const sr_character_t SR_CHARACTER_ID[] =
{
  { DR_CHARACTER_MARIO,       0x00, 0x00 },
  { DR_CHARACTER_LUIGI,       0x04, 0x00 },
  { DR_CHARACTER_PEACH,       0x49, 0x00 },
  { DR_CHARACTER_YOSHI,       0x06, 0x00 },
  { DR_CHARACTER_WARIO,       0x21, 0x00 },
  { DR_CHARACTER_DONKEY_KONG, 0x02, 0x00 },

  { DR_CHARACTER_WALUIGI,     0x04, 0x04 }, // Luigi (purple)
  { DR_CHARACTER_DAISY,       0x49, 0x01 }, // Peach (yellow)
};

SmashRemix::SmashRemix(QWindow *parent) : DrGuest(parent)
{
  loadCore(N64_CORE);
  loadContent(N64_GAME);

  connect(this, &QRetro::frameBegin, this, [this]() mutable {
    uint8_t stocks[4];
    for (unsigned i = 0; i < 4; i++)
      if (!memory().readValue<uint8_t>(&stocks[i], SR_STOCKS_ADDR[i]))
        return;

    unsigned defeated = 0;
    int winner = -1;
    for (unsigned i = 0; i < 4; i++) {
      if (stocks[i] == 0xFF) {
        defeated++;
        if (m_prevStocks[i] != 0xFF)
          log(DR_LOG_INFO, qPrintable(QString("%1 was defeated").arg(dr_character_name(m_slotCharacters[i]))));
      } else {
        winner = i;
        if (m_prevStocks[i] != 0xFF && stocks[i] < m_prevStocks[i])
          log(DR_LOG_INFO, qPrintable(QString("%1 lost a stock (%2 remaining)").arg(dr_character_name(m_slotCharacters[i])).arg(stocks[i] + 1)));
      }
      m_prevStocks[i] = stocks[i];
    }

    if (defeated == 3 && winner != -1 && m_winnerIndex == -1) {
      m_winnerIndex = m_slotToIndex[winner];
      m_finishCountdown = 120;
      log(DR_LOG_INFO, qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[winner]))));
    }

    if (m_finishCountdown > 0 && --m_finishCountdown == 0)
      emit minigameFinished();
  }, Qt::DirectConnection);
}

const dr_mp_minigame_t* SmashRemix::minigames() const
{
  return k_minigames;
}

void SmashRemix::setMinigame(unsigned id)
{
  (void)id;
  m_winnerIndex     = -1;
  m_finishCountdown = 0;
  for (unsigned i = 0; i < 4; i++)
    m_prevStocks[i] = 0xFF;
  unserializeFromFile(N64_STATE);

  /* Use a random stage from the original stage list */
  memory().writeValue<uint8_t>(rand() % 9, SR_STAGE_ADDR);

  /* Enable team battle for the 2v2 and 1v3 minigames */
  if (id == 0x01 || id == 0x02)
    memory().writeValue<uint8_t>(1, SR_GAME_TYPE_ADDR);
  else
    memory().writeValue<uint8_t>(0, SR_GAME_TYPE_ADDR);

  log(DR_LOG_INFO, qPrintable(QString("Smash Remix starting!")));
}

dr_minigame_result_t SmashRemix::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if ((int)index == m_winnerIndex)
    result.coins = 10;

  return result;
}

void SmashRemix::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++) {
    if (m_ports[i] == DR_CONTROL_PORT_INVALID || m_ports[i] >= DR_CONTROL_PORT_SIZE)
      continue;

    unsigned slot = m_ports[i] - DR_CONTROL_PORT_P1;
    m_slotToIndex[slot]    = i;
    m_slotCharacters[slot] = m_characters[i];

    bool isBot = (m_controlTypes[i] == DR_CONTROL_TYPE_CPU);
    memory().writeValue<uint8_t>(isBot ? 1 : 0, SR_IS_BOT_ADDR[slot]);

    for (const auto &entry : SR_CHARACTER_ID) {
      if (entry.character == m_characters[i]) {
        memory().writeValue<uint8_t>(entry.character_value, SR_CHARACTER_ADDR[slot]);
        memory().writeValue<uint8_t>(entry.color_value,     SR_COLOR_ADDR[slot]);
        memory().writeValue<uint8_t>(m_controlTypes[i] == DR_CONTROL_TYPE_CPU ? 4 : slot, SR_PORT_ADDR[slot]);
        memory().writeValue<uint8_t>(m_controlTypes[i] == DR_CONTROL_TYPE_CPU ? 4 : slot, SR_PORT_COLOR_ADDR[slot]);
        break;
      }
    }
  }
}

dr_error SmashRemix::doSetPlayerCharacter(unsigned index, dr_character character)
{
  m_characters[index] = character;
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  m_ports[index] = control_port;
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  m_controlTypes[index] = control_type;
  applyPlayers();
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  unsigned smash_difficulty;

  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
    smash_difficulty = 1;
    break;
  case DR_DIFFICULTY_EASY:
    smash_difficulty = 3;
    break;
  case DR_DIFFICULTY_NORMAL:
    smash_difficulty = 5;
    break;
  case DR_DIFFICULTY_HARD:
    smash_difficulty = 7;
    break;
  case DR_DIFFICULTY_VERY_HARD:
    smash_difficulty = 9;
    break;
  default:
    smash_difficulty = 5;
    break;
  }

  memory().writeValue<uint8_t>(static_cast<uint8_t>(smash_difficulty), SR_DIFFICULTY_ADDR[index]);

  return DR_OK;
}

dr_error SmashRemix::doSetPlayerTeam(unsigned index, dr_team team)
{
  uint8_t teamValue;

  /* Green is 2, yellow is 3 */
  switch (team)
  {
  case DR_TEAM_BLUE:
    teamValue = 0x01;
    break;
  case DR_TEAM_RED:
    teamValue = 0x00;
    break;
  default:
    teamValue = 0x00;
    break;
  }

  memory().writeValue<uint8_t>(teamValue, SR_TEAM_ADDR_1[index]);
  memory().writeValue<uint8_t>(teamValue, SR_TEAM_ADDR_2[index]);

  return DR_OK;
}
