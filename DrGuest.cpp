#include "DrGuest.h"

dr_error DrGuest::setPlayer(unsigned index, const dr_player_t &player)
{
  if (index >= 4) return DR_ERR_INVALID_PARAMETER;
  setPlayerCharacter(index, player.character);
  setPlayerControlPort(index, player.control_port);
  setPlayerControlType(index, player.control_type);
  setPlayerDifficulty(index, player.difficulty);
  setPlayerTeam(index, player.team);
  return DR_OK;
}

dr_error DrGuest::setPlayerCharacter(unsigned index, dr_character character)
{
  if (index >= 4 || character >= DR_CHARACTER_SIZE) return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerCharacter(index, character);
}

dr_error DrGuest::setPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index >= 4 || control_port == DR_CONTROL_PORT_INVALID || control_port >= DR_CONTROL_PORT_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerControlPort(index, control_port);
}

dr_error DrGuest::setPlayerControlType(unsigned index, dr_control_type control_type)
{
  if (index >= 4 || control_type == DR_CONTROL_TYPE_INVALID || control_type >= DR_CONTROL_TYPE_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerControlType(index, control_type);
}

dr_error DrGuest::setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  if (index >= 4 || difficulty >= DR_DIFFICULTY_SIZE) return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerDifficulty(index, difficulty);
}

dr_error DrGuest::setPlayerTeam(unsigned index, dr_team team)
{
  if (index >= 4 || team >= DR_TEAM_SIZE) return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerTeam(index, team);
}

dr_error DrGuest::read8(uint8_t *out, size_t addr, bool big_endian)
{
  (void)big_endian;
  return memory().readValue<uint8_t>(out, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::read16(uint16_t *out, size_t addr, bool big_endian)
{
  if (!memory().readValue<uint16_t>(out, addr)) return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian) *out = qbswap(*out);
  return DR_OK;
}

dr_error DrGuest::read32(uint32_t *out, size_t addr, bool big_endian)
{
  if (!memory().readValue<uint32_t>(out, addr)) return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian) *out = qbswap(*out);
  return DR_OK;
}

dr_error DrGuest::write8(uint8_t val, size_t addr, bool big_endian)
{
  (void)big_endian;
  return memory().writeValue<uint8_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::write16(uint16_t val, size_t addr, bool big_endian)
{
  if (big_endian) val = qbswap(val);
  return memory().writeValue<uint16_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::write32(uint32_t val, size_t addr, bool big_endian)
{
  if (big_endian) val = qbswap(val);
  return memory().writeValue<uint32_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

void DrGuest::log(unsigned level, const char *message)
{
  emit logMessage(level, QString::fromUtf8(message));
}

void DrGuest::startMinigame()
{
  m_minigameActive = true;
}

void DrGuest::finishMinigame()
{
  if (m_minigameActive)
  {
    m_minigameActive = false;
    emit minigameFinished();
  }
}
