#include "MarioPartyN64.h"

MarioPartyN64::MarioPartyN64(const MpN64Config &config, QWindow *parent)
  : DrGuest(parent), m_config(config)
{
  loadCore(m_config.core);
  loadContent(m_config.game);

  connect(this, &QRetro::frameBegin, this, [this]() {
    if (m_minigameWriteFrames > 0) {
      writeu8(static_cast<uint8_t>(m_minigame->minigame_id), m_config.minigame_addr);
      m_minigameWriteFrames--;
    }

    uint8_t val;
    if (readu8(&val, m_config.scene_addr) == DR_OK && val != m_lastScene) {
      log(DR_LOG_INFO, qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg(val, 2, 16, QChar('0'))));
      m_lastScene = val;
      if (val == m_config.scene_miniresults && m_minigameActive)
        finishMinigame();
    }
  }, Qt::DirectConnection);
}

const dr_mp_minigame_t* MarioPartyN64::minigames() const
{
  return m_config.minigames;
}

void MarioPartyN64::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  (void)minigame;
  unserializeFromFile(m_config.state);
  m_lastScene           = -1;
  m_minigameWriteFrames = 120;
  startMinigame();
}

dr_minigame_result_t MarioPartyN64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4) {
    uint8_t coins;
    if (readu8(&coins, m_config.result_addr[index]) == DR_OK)
      result.coins = coins;
  }
  return result;
}

dr_error MarioPartyN64::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return writeu8(m_config.character_ids[character], m_config.character_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return writeu8(static_cast<uint8_t>(control_port - 1), m_config.controller_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint8_t current, is_bot;

  is_bot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;

  if (readu8(&current, m_config.bot_addr[index]) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  else
    return writeu8((current & ~0x01) | is_bot, m_config.bot_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  uint8_t mp_difficulty;

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
  case DR_DIFFICULTY_VERY_HARD:
    mp_difficulty = 0x02;
    break;
  default:
    mp_difficulty = 0x01;
  }

  return writeu8(mp_difficulty, m_config.difficulty_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  (void)color; (void)type;
  return writeu8(static_cast<uint8_t>(team_id), m_config.team_addr[index]);
}
