#include "SmashRemix.h"

static const dr_mp_minigame_t SR_MINIGAMES[] = {
  { "Smash: Free-for-all", DR_MINIGAME_4P, 0x00, 0xFF, DR_NO_QUIRKS },

  { "Smash: Team Battle", DR_MINIGAME_2V2, 0x01, 0xFF, DR_NO_QUIRKS },

  { "Smash: Giant Battle", DR_MINIGAME_1V3, 0x02, 0xFF, DR_NO_QUIRKS },
  { "Smash: Tiny Battle", DR_MINIGAME_1V3, 0x03, 0xFF, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
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

static const sr_character_t SR_CHARACTER_ID[] = {
  { DR_CHARACTER_MARIO, 0x00, 0x00 }, { DR_CHARACTER_LUIGI, 0x04, 0x00 },
  { DR_CHARACTER_PEACH, 0x49, 0x00 }, { DR_CHARACTER_YOSHI, 0x06, 0x00 },
  { DR_CHARACTER_WARIO, 0x21, 0x00 }, { DR_CHARACTER_DONKEY_KONG, 0x02, 0x00 },

  { DR_CHARACTER_WALUIGI, 0x04, 0x04 }, // Luigi (purple)
  { DR_CHARACTER_DAISY, 0x49, 0x01 }, // Peach (yellow)
};

void SmashRemix::run(void)
{
  if (!m_minigame || !m_minigameActive)
    return;

  int8_t stocks[4];
  unsigned total_stocks = 0;

  for (unsigned i = 0; i < 4; i++)
  {
    if (reads8(&stocks[i], SR_STOCKS_ADDR[i]) != DR_OK)
      return;
    total_stocks += (unsigned)(stocks[i] + 1);
  }

  if (!m_winners && !m_finishCountdown)
  {
    if (total_stocks == 0)
    {
      log(DR_LOG_INFO, "It's a draw!");
      m_finishCountdown = 120;
    }
    else if (m_minigame->type == DR_MINIGAME_4P)
    {
      if (total_stocks == 1)
      {
        for (unsigned slot = 0; slot < 4; slot++)
        {
          if (stocks[slot] >= 0)
          {
            m_winners |= (1u << m_slotToIndex[slot]);
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
            break;
          }
        }
        m_finishCountdown = 120;
      }
    }
    else
    {
      /* Team game (2v2 or 1v3): a side wins when the opposing side has no stocks */
      unsigned teamStocks[2] = {};

      for (unsigned slot = 0; slot < 4; slot++)
      {
        int idx = m_slotToIndex[slot];
        if (idx >= 0 && m_players[idx].team_id < 2)
          teamStocks[m_players[idx].team_id] += (unsigned)(stocks[slot] + 1);
      }

      int winningTeam = -1;
      if (teamStocks[0] == 0 && teamStocks[1] > 0)
        winningTeam = 1;
      else if (teamStocks[1] == 0 && teamStocks[0] > 0)
        winningTeam = 0;

      if (winningTeam >= 0)
      {
        for (unsigned slot = 0; slot < 4; slot++)
        {
          int idx = m_slotToIndex[slot];
          if (idx >= 0 && (int)m_players[idx].team_id == winningTeam)
          {
            m_winners |= (1u << idx);
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
          }
        }
        m_finishCountdown = 120;
      }
    }
  }

  if (m_finishCountdown > 0 && --m_finishCountdown == 0)
    finishMinigame();
}

SmashRemix::SmashRemix(QObject *parent)
  : DrGuest(parent)
{
  m_core = new QRetro();
  m_ownCore = true;
  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  QString gamePath = dr_roms_directory() + "/smashremix.z64";
  if (!m_core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!m_core->loadContent(gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(gamePath)));
    m_valid = false;
  }
}

const dr_mp_minigame_t *SmashRemix::minigames() const
{
  return SR_MINIGAMES;
}

void SmashRemix::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_winners = 0;
  m_finishCountdown = 0;
  for (unsigned i = 0; i < 4; i++)
    m_prevStocks[i] = 0xFF;
  m_core->unserializeFromFile(dr_state_directory() + "/smashremix.state.zip");

  /* Use a random stage from the original stage list */
  m_core->memory().writeValue<uint8_t>(rand() % 9, SR_STAGE_ADDR);

  /* Enable team battle for the 2v2 and 1v3 minigames */
  bool teamBattle = (minigame->type == DR_MINIGAME_2V2 || minigame->type == DR_MINIGAME_1V3);
  m_core->memory().writeValue<uint8_t>(teamBattle ? 1 : 0, SR_GAME_TYPE_ADDR);

  log(DR_LOG_INFO, qPrintable(QString("Smash Remix starting!")));
  startMinigame();
}

dr_minigame_result_t SmashRemix::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index < 4 && (m_winners & (1u << index)))
    result.coins = 10;

  return result;
}

void SmashRemix::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;

    unsigned slot = p.control_port - DR_CONTROL_PORT_P1;
    m_slotToIndex[slot] = i;
    m_slotCharacters[slot] = p.character;

    bool isBot = (p.control_type == DR_CONTROL_TYPE_CPU);
    m_core->memory().writeValue<uint8_t>(isBot ? 1 : 0, SR_IS_BOT_ADDR[slot]);

    for (const auto &entry : SR_CHARACTER_ID)
    {
      if (entry.character == p.character)
      {
        writeu8(entry.character_value, SR_CHARACTER_ADDR[slot]);
        writeu8(entry.color_value, SR_COLOR_ADDR[slot]);
        writeu8(isBot ? 4 : slot, SR_PORT_ADDR[slot]);
        break;
      }
    }

    uint8_t difficulty;
    switch (p.difficulty)
    {
    case DR_DIFFICULTY_VERY_EASY:
      difficulty = 1;
      break;
    case DR_DIFFICULTY_EASY:
      difficulty = 3;
      break;
    case DR_DIFFICULTY_NORMAL:
      difficulty = 5;
      break;
    case DR_DIFFICULTY_HARD:
      difficulty = 7;
      break;
    case DR_DIFFICULTY_VERY_HARD:
      difficulty = 9;
      break;
    default:
      difficulty = 5;
      break;
    }
    m_core->memory().writeValue<uint8_t>(
      static_cast<uint8_t>(difficulty), SR_DIFFICULTY_ADDR[slot]);

    uint8_t color, team;
    if (m_minigame->type == DR_MINIGAME_1V3 || m_minigame->type == DR_MINIGAME_2V2)
    {
      switch (p.team_id)
      {
      case 0:
        color = 0x00;
        team = 0x00;
        break;
      case 1:
        color = 0x01;
        team = 0x01;
        break;
      default:
        color = 0x04;
        team = 0x00;
        break;
      }
    }
    else
    {
      /* Free-for-all mini-game; set color based on slot, or gray for CPU */
      if (p.control_type == DR_CONTROL_TYPE_CPU)
        color = 0x04;
      else
        color = slot;
      team = i;
    }
    m_core->memory().writeValue<uint8_t>(color, SR_PORT_COLOR_ADDR[slot]);
    m_core->memory().writeValue<uint8_t>(team, SR_TEAM_ADDR_1[slot]);
    m_core->memory().writeValue<uint8_t>(team, SR_TEAM_ADDR_2[slot]);

    uint8_t size;
    bool tinyBattle = (m_minigame->minigame_id == 0x03);
    if (p.team_type == DR_TEAM_TYPE_1V3_SOLO)
      size = tinyBattle ? 0 : 1; // Tiny: solo=normal, Giant: solo=giant
    else
      size = tinyBattle ? 2 : 0; // Tiny: group=tiny,  Giant: group=normal
    m_core->memory().writeValue<uint8_t>(size, SR_SIZE_ADDR_1[slot]);
    m_core->memory().writeValue<uint8_t>(size, SR_SIZE_ADDR_2[slot]);
  }
}

dr_error SmashRemix::doSetPlayerCharacter(unsigned index, dr_character character)
{
  m_players[index].character = character;
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  m_players[index].control_port = control_port;
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  m_players[index].control_type = control_type;
  applyPlayers();
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  m_players[index].difficulty = difficulty;
  applyPlayers();
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  m_players[index].team_color = color;
  m_players[index].team_type = type;
  m_players[index].team_id = team_id;
  applyPlayers();
  return DR_OK;
}
