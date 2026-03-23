#include "MarioPartyGcn.h"

MarioPartyGcn::MarioPartyGcn(const MpGcnConfig &config, QWindow *parent)
  : DrGuest(parent), m_config(config)
{
  loadCore(m_config.core);
  loadContent(m_config.game);

  connect(this, &QRetro::frameBegin, this, [this, last = uint8_t(0xFF)]() mutable {
    if (m_minigameWriteFrames > 0) {
      write8(m_minigameId, m_config.minigame_addr);
      --m_minigameWriteFrames;
    }

    uint8_t val;
    if (read8(&val, m_config.scene_addr) == DR_OK && val != last) {
      log(DR_LOG_INFO, qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg(val, 2, 16, QChar('0'))));
      last = val;
      if (val == m_config.scene_miniresults)
        emit minigameFinished();
    }
  }, Qt::DirectConnection);
}

const dr_mp_minigame_t* MarioPartyGcn::minigames() const
{
  return m_config.minigames;
}

void MarioPartyGcn::setMinigame(unsigned id)
{
  unserializeFromFile(m_config.state);
  m_minigameId          = id;
  m_minigameWriteFrames = 60 * 20;
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4) {
    uint8_t coins;
    if (read8(&coins, m_config.result_addr[index]) == DR_OK)
      result.coins = coins;
  }
  return result;
}

dr_error MarioPartyGcn::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return write8(m_config.character_ids[character], m_config.character_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return write8(static_cast<uint8_t>(control_port - 1), m_config.controller_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint8_t current;
  if (read8(&current, m_config.bot_addr[index]) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  uint8_t isBot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
  return write8((current & ~0x01) | isBot, m_config.bot_addr[index]);
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
  return write8(mp_difficulty, m_config.difficulty_addr[index]);
}

dr_error MarioPartyGcn::doSetPlayerTeam(unsigned index, dr_team team)
{
  return write8((team == DR_TEAM_RED) ? 1 : 0, m_config.team_addr[index]);
}
