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
  int16_t id = static_cast<int16_t>(minigame->minigame_id);
  m_retro->writeForFrames(m_config.minigame_addr, &id, sizeof(id), 120);
  startMinigame();
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4)
  {
    uint16_t coins;
    if (m_retro->readu16(&coins, m_config.result_addr[index]) == DR_OK)
      result.coins = coins;
    if (m_retro->readu16(&coins, m_config.bonus_result_addr[index]) == DR_OK)
      result.bonus_coins = coins;
  }
  return result;
}

dr_error MarioPartyGcn::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return m_retro->writeu16(m_config.character_ids[character], m_config.character_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return m_retro->writeu16(static_cast<uint16_t>(control_port - 1), m_config.controller_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint16_t current, is_bot;

  is_bot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;

  if (m_retro->readu16(&current, m_config.bot_addr[index]) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  else
    return m_retro->writeu16((current & ~0x01) | is_bot, m_config.bot_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  uint16_t mp_difficulty;

  switch (difficulty)
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
  }

  return m_retro->writeu16(mp_difficulty, m_config.difficulty_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  (void)color;
  (void)type;
  return m_retro->writeu16(static_cast<uint16_t>(team_id), m_config.team_addr[index]);
}
