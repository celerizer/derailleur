#include "MarioPartyGcn.h"

MarioPartyGcn::MarioPartyGcn(const MpGcnConfig &config, QWindow *parent)
  : DrGuest(parent), m_config(config)
{
  loadCore(m_config.core);
  loadContent(m_config.game);

  connect(this, &QRetro::frameBegin, this, [this, last = uint16_t(0xFFFF)]() mutable {
    if (m_minigameWriteFrames > 0) {
      write16(m_minigameId, m_config.minigame_addr, true);
      --m_minigameWriteFrames;
    }

    uint16_t val;
    if (read16(&val, m_config.scene_addr, true) == DR_OK && val != last) {
      log(DR_LOG_INFO, qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg(val, 4, 16, QChar('0'))));
      last = val;
      if (val == m_config.scene_miniresults && m_minigameActive)
        finishMinigame();
    }
  }, Qt::DirectConnection);
}

const dr_mp_minigame_t* MarioPartyGcn::minigames() const
{
  return m_config.minigames;
}

void MarioPartyGcn::setMinigame(unsigned id)
{
  startMinigame();
  unserializeFromFile(m_config.state);
  m_minigameId          = id;
  m_minigameWriteFrames = 60 * 20;
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4) {
    uint16_t coins;
    if (read16(&coins, m_config.result_addr[index], true) == DR_OK)
      result.coins = coins;
  }
  return result;
}

dr_error MarioPartyGcn::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return write16(m_config.character_ids[character], m_config.character_addr[index], true);
}

dr_error MarioPartyGcn::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return write16(static_cast<uint16_t>(control_port - 1), m_config.controller_addr[index], true);
}

dr_error MarioPartyGcn::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint16_t current;
  if (read16(&current, m_config.bot_addr[index], true) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  uint16_t isBot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
  return write16((current & ~0x01) | isBot, m_config.bot_addr[index], true);
}

dr_error MarioPartyGcn::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  uint8_t mp_difficulty;
  switch (difficulty) {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:      mp_difficulty = 0x00; break;
  case DR_DIFFICULTY_NORMAL:    mp_difficulty = 0x01; break;
  case DR_DIFFICULTY_HARD:      mp_difficulty = 0x02; break;
  case DR_DIFFICULTY_VERY_HARD: mp_difficulty = 0x03; break;
  default:                      mp_difficulty = 0x00;
  }
  return write16(mp_difficulty, m_config.difficulty_addr[index], true);
}

dr_error MarioPartyGcn::doSetPlayerTeam(unsigned index, dr_team team)
{
  return write16((team == DR_TEAM_RED) ? 1 : 0, m_config.team_addr[index], true);
}
