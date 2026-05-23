#include "MarioPartyGcn.h"

MarioPartyGcn::MarioPartyGcn(const MpGcnConfig &config, QRetro *sharedCore, QObject *parent)
  : DrGuest(sharedCore, parent), m_config(config)
{
  connect(m_core, &QRetro::frameBegin, this, [this]() {
    tickFrameWrites();
    if (!m_minigameActive)
      return;

    int32_t val;
    if (reads32(&val, m_config.scene_addr) == DR_OK && val != m_lastScene) {
      log(DR_LOG_INFO, qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg(val, 4, 16, QChar('0'))));
      m_lastScene = val;
      if (val == m_config.scene_miniresults)
        finishMinigame();
    }
  }, Qt::DirectConnection);
}

const dr_mp_minigame_t* MarioPartyGcn::minigames() const
{
  return m_config.minigames;
}

void MarioPartyGcn::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_lastScene  = -1;
  uint16_t id = minigame->minigame_id;
  writeForFrames(m_config.minigame_addr, &id, sizeof(id), false, 120);
  startMinigame();
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4) {
    uint16_t coins;
    if (readu16(&coins, m_config.result_addr[index]) == DR_OK)
      result.coins = coins;
    if (readu16(&coins, m_config.bonus_result_addr[index]) == DR_OK)
      result.bonus_coins = coins;
  }
  return result;
}

dr_error MarioPartyGcn::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return writeu16(m_config.character_ids[character], m_config.character_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return writeu16(static_cast<uint16_t>(control_port - 1), m_config.controller_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint16_t current, is_bot;

  /**
   * Whether the player is a bot seems to always be the least significant bit
   * of this value
   */
  is_bot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;

  if (readu16(&current, m_config.bot_addr[index]) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  else
    return writeu16((current & ~0x01) | is_bot, m_config.bot_addr[index]);
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

  return writeu16(mp_difficulty, m_config.difficulty_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  (void)color; (void)type;
  return writeu16(static_cast<uint16_t>(team_id), m_config.team_addr[index]);
}
