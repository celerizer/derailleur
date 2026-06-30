#include "MarioPartyGcn.h"

MarioPartyGcn::MarioPartyGcn(const MpGcnConfig &config, QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_config(config)
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioPartyGcn::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioPartyGcn::run()
{
  m_retro->tickFrameWrites();

  if (m_minigameActive)
    m_minigameFrames++;

  int32_t val;
  if (m_retro->reads32(&val, m_config.scene_addr) == DR_OK && val != m_lastScene)
  {
    log(DR_LOG_INFO, qPrintable(QString("%1 scene: 0x%2").arg(name()).arg(val, 4, 16, QChar('0'))));
    m_lastScene = val;
    if (m_minigameActive && m_minigameFrames >= 60 &&
        val != m_config.scene_miniexplain && val != m_minigame->scene_id &&
        val != -1)
    {
      log(DR_LOG_INFO, qPrintable(QString("finishing minigame on scene 0x%1").arg(val, 4, 16, QChar('0'))));
      finishMinigame();
    }
  }
}

const dr_mp_minigame_t *MarioPartyGcn::minigames() const
{
  return m_config.minigames;
}

void MarioPartyGcn::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_lastScene = -1;
  m_minigameFrames = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = {};
    m_slotOf[i] = static_cast<int>(i);
  }
  int16_t id = static_cast<int16_t>(minigame->minigame_id);
  m_retro->writeForFrames(m_config.minigame_addr, &id, sizeof(id), 120);
  startMinigame();
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4)
  {
    unsigned slot = static_cast<unsigned>(m_slotOf[index]);
    uint16_t coins;
    if (m_retro->readu16(&coins, m_config.result_addr[slot]) == DR_OK)
      result.coins = coins;
    if (m_retro->readu16(&coins, m_config.bonus_result_addr[slot]) == DR_OK)
      result.bonus_coins = coins;
  }
  return result;
}

/* The per-field setters only buffer; doSetPlayerTeam (the last call DrGuest makes
 * per player) flushes everything through applyPlayers so the duel slot remap can
 * see all four players' team types at once. */
dr_error MarioPartyGcn::doSetPlayerCharacter(unsigned index, dr_character character)
{
  if (index < 4)
    m_players[index].character = character;
  return DR_OK;
}

dr_error MarioPartyGcn::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index < 4)
    m_players[index].control_port = control_port;
  return DR_OK;
}

dr_error MarioPartyGcn::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  if (index < 4)
    m_players[index].control_type = control_type;
  return DR_OK;
}

dr_error MarioPartyGcn::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  if (index < 4)
    m_players[index].difficulty = difficulty;
  return DR_OK;
}

dr_error MarioPartyGcn::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  if (index < 4)
  {
    m_players[index].team_color = color;
    m_players[index].team_type = type;
    m_players[index].team_id = team_id;
  }
  applyPlayers();
  return DR_OK;
}

void MarioPartyGcn::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
    m_slotOf[i] = static_cast<int>(i);

  /* Mario Party 4 duel mini-games from story mode always use first two player indices */
  if (id() == DR_GUEST_MARIOPARTY4 && m_minigame && m_minigame->type == DR_MINIGAME_DUEL)
  {
    int slot = 0;
    for (unsigned i = 0; i < 4; i++)
      if (m_players[i].team_type != DR_TEAM_TYPE_INVALID)
        m_slotOf[i] = slot++;
    for (unsigned i = 0; i < 4; i++)
      if (m_players[i].team_type == DR_TEAM_TYPE_INVALID)
        m_slotOf[i] = slot++;
  }

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    unsigned slot = static_cast<unsigned>(m_slotOf[i]);

    if (p.character < DR_CHARACTER_SIZE)
      m_retro->writeu16(m_config.character_ids[p.character], m_config.character_addr[slot]);

    if (p.control_port != DR_CONTROL_PORT_INVALID && p.control_port < DR_CONTROL_PORT_SIZE)
      m_retro->writeu16(static_cast<uint16_t>(p.control_port - 1), m_config.controller_addr[slot]);

    uint16_t current = 0;
    uint16_t is_bot = (p.control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
    if (m_retro->readu16(&current, m_config.bot_addr[slot]) == DR_OK)
      m_retro->writeu16((current & ~0x01) | is_bot, m_config.bot_addr[slot]);

    uint16_t mp_difficulty;
    switch (p.difficulty)
    {
    case DR_DIFFICULTY_VERY_EASY:
    case DR_DIFFICULTY_EASY:
      mp_difficulty = 0x00;
      break;
    case DR_DIFFICULTY_NORMAL:
      mp_difficulty = 0x01;
      break;
    case DR_DIFFICULTY_HARD:
      mp_difficulty = 0x02;
      break;
    case DR_DIFFICULTY_VERY_HARD:
      mp_difficulty = 0x03;
      break;
    default:
      mp_difficulty = 0x01;
      break;
    }
    m_retro->writeu16(mp_difficulty, m_config.difficulty_addr[slot]);

    m_retro->writeu16(static_cast<uint16_t>(p.team_id), m_config.team_addr[slot]);
  }
}
