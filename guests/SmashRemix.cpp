#include "SmashRemix.h"

#include <QApplication>

static const dr_mp_minigame_t SR_MINIGAMES[] = {
  { "Remix Free-for-all", DR_MINIGAME_4P,     0x00, 0xFF, DR_NO_QUIRKS },
  { "Remix Battle",       DR_MINIGAME_BATTLE, 0x00, 0xFF, DR_NO_QUIRKS },

  { "Remix Team Battle", DR_MINIGAME_2V2, 0x01, 0xFF, DR_NO_QUIRKS },

  { "Remix Giant Battle", DR_MINIGAME_1V3, 0x02, 0xFF, DR_NO_QUIRKS },
  { "Remix Tiny Battle",  DR_MINIGAME_1V3, 0x03, 0xFF, DR_NO_QUIRKS },
  { "Remix Golden Gun",   DR_MINIGAME_1V3, 0x04, 0xFF, DR_NO_QUIRKS },

  { "Remix PKMN", DR_MINIGAME_4P, 0x05, 0xFF, DR_NO_QUIRKS },

  { "Remix Duel", DR_MINIGAME_DUEL, 0x00, 0xFF, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

/* Whether the game is in team battle mode */
static const size_t SR_GAME_TYPE_ADDR = 0x800a4d0a;
static const size_t SR_STAGE_ADDR = 0x800a4d09;

static const size_t SR_CHARACTER_ADDR[4] = { 0x800a4d2b, 0x800a4d9f, 0x800a4e13, 0x800a4e87 };

/* Player type for this slot: 0 = human, 1 = CPU, 2 = inactive (no player) */
static const size_t SR_PLAYER_TYPE_ADDR[4] = { 0x800a4d2a, 0x800a4d9e, 0x800a4e12, 0x800a4e86 };

static const size_t SR_DIFFICULTY_ADDR[4] = { 0x800a4d28, 0x800a4dac, 0x800a4e10, 0x800a4e84 };

static const size_t SR_COLOR_ADDR[4] = { 0x800a4d2e, 0x800a4da2, 0x800a4e16, 0x800a4e8a };

static const size_t SR_TEAM_ADDR_1[4] = { 0x800a4d2d, 0x800a4da1, 0x800a4e15, 0x800a4e89 };
static const size_t SR_TEAM_ADDR_2[4] = { 0x800a4d2c, 0x800a4da0, 0x800a4e14, 0x800a4e88 };

static const size_t SR_STOCKS_ADDR[4] = { 0x800a4d33, 0x800a4da7, 0x800a4e1b, 0x800a4e8f };

/* The controller port for this player, or 4 if CPU-controlled */
static const size_t SR_PORT_ADDR[4] = { 0x800a4d32, 0x800a4da6, 0x800a4e1a, 0x800a4e8e };

/* The color drawn behind the percentage, should match SR_PORT_ADDR */
static const size_t SR_PORT_COLOR_ADDR[4] = { 0x800a4d30, 0x800a4da4, 0x800a4e18, 0x800a4e8c };

/* The size of the player, a u32. 0=normal, 1=giant, 2=tiny */
static const size_t SR_SIZE_ADDR_1[4] = { 0x80502fac, 0x80502fb0, 0x80502fb4, 0x80502fb8 };
static const size_t SR_SIZE_ADDR_2[4] = { 0x80502fbc, 0x80502fc0, 0x80502fc4, 0x80502fc8 };

/* The item the player starts with */
static const size_t SR_START_ITEM_ADDR[4] = { 0x80453748, 0x8045374c, 0x80453750, 0x80453754 };

/* The item the player can spawn by taunting */
static const size_t SR_TAUNT_ITEM_ADDR[4] = { 0x804539a8, 0x804539ac, 0x804539b0, 0x804539b4 };

typedef enum
{
  SR_ITEM_NONE        = 0,
  SR_ITEM_BEAM_SWORD  = 1,
  SR_ITEM_HOMERUN_BAT = 2,
  SR_ITEM_FAN         = 3,
  SR_ITEM_STAR_ROD    = 4,
  SR_ITEM_RAY_GUN     = 5,
  SR_ITEM_FIRE_FLOWER = 6,
  SR_ITEM_HAMMER      = 7,
  SR_ITEM_MS_BOMB     = 8,
  SR_ITEM_BOBOMB      = 9,
  SR_ITEM_BUMPER      = 10,
  SR_ITEM_GREEN_SHELL = 11,
  SR_ITEM_RED_SHELL   = 12,
  SR_ITEM_POKEBALL    = 13,
  SR_ITEM_SPINY_SHELL = 14,
  SR_ITEM_DEKU_NUT    = 15,
  SR_ITEM_PITFALL     = 16,
  SR_ITEM_GOLDEN_GUN  = 17,
  SR_ITEM_MR_SATURN   = 18,
  SR_ITEM_RANDOM      = 19
} sr_item;

typedef struct
{
  dr_character character;
  unsigned character_value;
  unsigned color_value;
} sr_character_t;

static const sr_character_t SR_CHARACTER_ID[] = {
  { DR_CHARACTER_MARIO, 0x00, 0x00 },
  { DR_CHARACTER_LUIGI, 0x04, 0x00 },
  { DR_CHARACTER_PEACH, 0x49, 0x00 },
  { DR_CHARACTER_YOSHI, 0x06, 0x00 },
  { DR_CHARACTER_WARIO, 0x21, 0x00 },
  { DR_CHARACTER_DONKEY_KONG, 0x02, 0x00 },

  { DR_CHARACTER_WALUIGI, 0x04, 0x04 }, // Luigi (purple)
  { DR_CHARACTER_DAISY, 0x49, 0x01 }, // Peach (yellow)
};

void SmashRemix::run(void)
{
  if (!m_minigame || !m_minigameActive)
    return;

  int8_t stocks[4];
  for (unsigned i = 0; i < 4; i++)
  {
    if (m_retro->reads8(&stocks[i], SR_STOCKS_ADDR[i]) != DR_OK)
      return;
  }

  if (!m_winners && !m_finishCountdown)
  {
    if (m_minigame->type == DR_MINIGAME_DUEL)
    {
      for (unsigned slot = 0; slot < 4; slot++)
      {
        int idx = m_slotToIndex[slot];
        if (idx < 0 || stocks[slot] >= 0)
          continue;
        for (unsigned wslot = 0; wslot < 4; wslot++)
        {
          int widx = m_slotToIndex[wslot];
          if (widx >= 0 && wslot != slot && stocks[wslot] >= 0)
          {
            m_winners |= (1u << widx);
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[wslot]))));
          }
        }
        m_finishCountdown = 120;
        break;
      }
    }
    else if (m_minigame->type == DR_MINIGAME_BATTLE)
    {
      for (unsigned slot = 0; slot < 4; slot++)
      {
        int idx = m_slotToIndex[slot];
        if (idx < 0)
          continue;
        if (m_prevStocks[slot] == -2)
        {
          m_prevStocks[slot] = stocks[slot];
          continue;
        }
        if (m_prevStocks[slot] >= 0 && stocks[slot] < 0 && m_placement[idx] == -1)
        {
          m_placement[idx] = 3 - (int)m_eliminationCount++;
          log(DR_LOG_INFO,
            qPrintable(QString("%1 eliminated (%2th)").arg(
              dr_character_name(m_slotCharacters[slot])).arg(m_placement[idx] + 1)));
        }
        m_prevStocks[slot] = stocks[slot];
      }
      if (m_eliminationCount >= 3)
      {
        for (unsigned slot = 0; slot < 4; slot++)
        {
          int idx = m_slotToIndex[slot];
          if (idx >= 0 && m_placement[idx] == -1)
          {
            m_placement[idx] = 0;
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
          }
        }
        m_finishCountdown = 120;
      }
    }
    else
    {
      unsigned total_stocks = 0;
      for (unsigned i = 0; i < 4; i++)
        total_stocks += (unsigned)(stocks[i] + 1);

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
  }

  if (m_finishCountdown > 0 && --m_finishCountdown == 0)
    finishMinigame();
}

SmashRemix::SmashRemix(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  QString gamePath = dr_roms_directory() + "/smashremix.z64";
  QRetro *core = new QRetro();
  core->setSavingEnabled(false);
  if (!core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!core->loadContent(gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(core, true);

  /* Apply base N64 remaps. Additionally... */
  m_retro->applyN64Remaps();

  for (unsigned i = 0; i < 4; i++)
  {
    /* ...map all D-pad directions to L (taunt) */
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_SELECT);

    /* ...map R2 to Z (shield) */
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_R2, RETRO_DEVICE_ID_JOYPAD_L2);
  }
}

void SmashRemix::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

const dr_mp_minigame_t *SmashRemix::minigames() const
{
  return SR_MINIGAMES;
}

void SmashRemix::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_winners = 0;
  m_finishCountdown = 0;
  m_eliminationCount = 0;
  for (unsigned i = 0; i < 4; i++)
    m_players[i] = {};
  for (unsigned i = 0; i < 4; i++)
  {
    m_slotToIndex[i] = -1;
    m_prevStocks[i] = -2;
    m_placement[i] = -1;
  }
  core()->unserializeFromFile(dr_state_directory() + "/smashremix.state.zip");

  /* Use a random stage from the original stage list */
  unsigned long rc = dr_rand_count();
  uint8_t stage = dr_rand() % 9;
  log(DR_LOG_INFO, qPrintable(QString("SR stage=%1 randcount=%2").arg(stage).arg(rc)));
  m_retro->writeu8(stage, SR_STAGE_ADDR);

  /* Enable team battle for the 2v2 and 1v3 minigames */
  bool teamBattle = (minigame->type == DR_MINIGAME_2V2 || minigame->type == DR_MINIGAME_1V3);
  m_retro->writeu8(teamBattle ? 1 : 0, SR_GAME_TYPE_ADDR);

  log(DR_LOG_INFO, qPrintable(QString("Smash Remix starting!")));

  startMinigame();
}

dr_minigame_result_t SmashRemix::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index >= 4)
    return result;

  if (m_minigame && m_minigame->type == DR_MINIGAME_BATTLE)
  {
    int place = m_placement[index];
    if (place >= 0 && place <= 3)
      result.coins = (unsigned)place;
  }
  else if (m_winners & (1u << index))
  {
    result.coins = 10;
  }

  return result;
}

void SmashRemix::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];

    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;

    /** @todo this needs to get changed if we change how duel teams are set */
    if (p.team_type == DR_TEAM_TYPE_INVALID)
      continue;

    unsigned slot = p.control_port - DR_CONTROL_PORT_P1;
    m_slotToIndex[slot] = i;
    m_slotCharacters[slot] = p.character;

    bool isBot = (p.control_type == DR_CONTROL_TYPE_CPU);
    m_retro->writeu8(isBot ? 1 : 0, SR_PLAYER_TYPE_ADDR[slot]);

    for (const auto &entry : SR_CHARACTER_ID)
    {
      if (entry.character == p.character)
      {
        m_retro->writeu8(entry.character_value, SR_CHARACTER_ADDR[slot]);
        m_retro->writeu8(entry.color_value, SR_COLOR_ADDR[slot]);
        m_retro->writeu8(isBot ? 4 : slot, SR_PORT_ADDR[slot]);
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
    m_retro->writeu8(
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
    m_retro->writeu8(color, SR_PORT_COLOR_ADDR[slot]);
    m_retro->writeu8(team, SR_TEAM_ADDR_1[slot]);
    m_retro->writeu8(team, SR_TEAM_ADDR_2[slot]);

    uint8_t size = 0; // normal
    if (m_minigame->minigame_id == 0x02) // Giant Battle: solo is giant
      size = (p.team_type == DR_TEAM_TYPE_1V3_SOLO) ? 1 : 0;
    else if (m_minigame->minigame_id == 0x03) // Tiny Battle: group is tiny
      size = (p.team_type == DR_TEAM_TYPE_1V3_SOLO) ? 0 : 2;
    m_retro->writeu32(size, SR_SIZE_ADDR_1[slot]);
    m_retro->writeu32(size, SR_SIZE_ADDR_2[slot]);

    /* Items: none by default. */
    sr_item startItem = SR_ITEM_NONE;
    sr_item tauntItem = SR_ITEM_NONE;
    if (m_minigame->minigame_id == 0x04 && p.team_type == DR_TEAM_TYPE_1V3_SOLO)
    {
      /* Remix Golden Gun: arm the 1v3 solo player with a golden gun in both. */
      startItem = SR_ITEM_GOLDEN_GUN;
      tauntItem = SR_ITEM_GOLDEN_GUN;
    }
    else if (m_minigame->minigame_id == 0x05)
    {
      /* Remix Pokemon: everyone starts with a pokeball (no taunt item). */
      startItem = SR_ITEM_POKEBALL;
    }
    m_retro->writeu32(startItem, SR_START_ITEM_ADDR[slot]);
    m_retro->writeu32(tauntItem, SR_TAUNT_ITEM_ADDR[slot]);
  }

  unsigned activeSlots = 0;
  for (unsigned slot = 0; slot < 4; slot++)
    if (m_slotToIndex[slot] >= 0)
      activeSlots++;

  unsigned expectedPlayers = (m_minigame->type == DR_MINIGAME_DUEL) ? 2 : 4;
  if (activeSlots >= expectedPlayers)
  {
    for (unsigned slot = 0; slot < 4; slot++)
      if (m_slotToIndex[slot] == -1)
      {
        /* Mark the slot totally inactive (2), not just out of stocks — otherwise
         * a slot the savestate had populated (e.g. slot 1 when the duelists land
         * on slots 0 and 2) still spawns a stale player. */
        m_retro->writeu8(2, SR_PLAYER_TYPE_ADDR[slot]);
        m_retro->writes8(-1, SR_STOCKS_ADDR[slot]);
      }
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
  return DR_OK;
}

dr_error SmashRemix::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  m_players[index].difficulty = difficulty;
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
