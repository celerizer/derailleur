#include "DrGuest.h"

dr_error DrGuest::setPlayer(unsigned index, const dr_player_t &player)
{
  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;
  else
  {
    setPlayerCharacter(index, player.character);
    setPlayerControlPort(index, player.control_port);
    setPlayerControlType(index, player.control_type);
    setPlayerDifficulty(index, player.difficulty);
    setPlayerTeam(index, player.team_color, player.team_type, player.team_id);

    return DR_OK;
  }
}

dr_error DrGuest::setPlayerCharacter(unsigned index, dr_character character)
{
  if (index >= 4 || character >= DR_CHARACTER_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  else
    return doSetPlayerCharacter(index, character);
}

dr_error DrGuest::setPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index >= 4 || control_port == DR_CONTROL_PORT_INVALID || control_port >= DR_CONTROL_PORT_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  else
    return doSetPlayerControlPort(index, control_port);
}

dr_error DrGuest::setPlayerControlType(unsigned index, dr_control_type control_type)
{
  if (index >= 4 || control_type == DR_CONTROL_TYPE_INVALID || control_type >= DR_CONTROL_TYPE_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  else
    return doSetPlayerControlType(index, control_type);
}

dr_error DrGuest::setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  if (index >= 4 || difficulty >= DR_DIFFICULTY_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  else
    return doSetPlayerDifficulty(index, difficulty);
}

dr_error DrGuest::setPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  if (index >= 4 || color >= DR_TEAM_COLOR_SIZE || type >= DR_TEAM_TYPE_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerTeam(index, color, type, team_id);
}

dr_error DrGuest::readu8(uint8_t *out, size_t addr, bool big_endian)
{
  (void)big_endian;
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  return m_core->memory().readValue<uint8_t>(out, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::reads8(int8_t *out, size_t addr, bool big_endian)
{
  return readu8(reinterpret_cast<uint8_t*>(out), addr, big_endian);
}

dr_error DrGuest::readu16(uint16_t *out, size_t addr, bool big_endian)
{
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  if (!m_core->memory().readValue<uint16_t>(out, addr))
    return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian)
    *out = qbswap(*out);
  return DR_OK;
}

dr_error DrGuest::reads16(int16_t *out, size_t addr, bool big_endian)
{
  return readu16(reinterpret_cast<uint16_t*>(out), addr, big_endian);
}

dr_error DrGuest::readu32(uint32_t *out, size_t addr, bool big_endian)
{
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  if (!m_core->memory().readValue<uint32_t>(out, addr))
    return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian)
    *out = qbswap(*out);
  return DR_OK;
}

dr_error DrGuest::reads32(int32_t *out, size_t addr, bool big_endian)
{
  return readu32(reinterpret_cast<uint32_t*>(out), addr, big_endian);
}

dr_error DrGuest::writeu8(uint8_t val, size_t addr, bool big_endian)
{
  (void)big_endian;
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  return m_core->memory().writeValue<uint8_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::writes8(int8_t val, size_t addr, bool big_endian)
{
  return writeu8(static_cast<uint8_t>(val), addr, big_endian);
}

dr_error DrGuest::writeu16(uint16_t val, size_t addr, bool big_endian)
{
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian)
    val = qbswap(val);
  return m_core->memory().writeValue<uint16_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::writes16(int16_t val, size_t addr, bool big_endian)
{
  return writeu16(static_cast<uint16_t>(val), addr, big_endian);
}

dr_error DrGuest::writeu32(uint32_t val, size_t addr, bool big_endian)
{
  if (!m_core) return DR_ERR_MEMORY_ACCESS_CORE;
  if (big_endian)
    val = qbswap(val);
  return m_core->memory().writeValue<uint32_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrGuest::writes32(int32_t val, size_t addr, bool big_endian)
{
  return writeu32(static_cast<uint32_t>(val), addr, big_endian);
}

void DrGuest::writeForFrames(size_t addr, const void *value, unsigned bytes, bool big_endian, int frames)
{
  m_frameWrites.append({addr, QByteArray(static_cast<const char *>(value), bytes), big_endian, frames});
}

void DrGuest::tickFrameWrites(void)
{
  for (auto it = m_frameWrites.begin(); it != m_frameWrites.end(); ) {
    switch (it->data.size()) {
    case 1: writeu8(*reinterpret_cast<const uint8_t *>(it->data.constData()), it->addr, it->big_endian); break;
    case 2: writeu16(*reinterpret_cast<const uint16_t *>(it->data.constData()), it->addr, it->big_endian); break;
    case 4: writeu32(*reinterpret_cast<const uint32_t *>(it->data.constData()), it->addr, it->big_endian); break;
    }
    if (--it->frames <= 0)
      it = m_frameWrites.erase(it);
    else
      ++it;
  }
}

void DrGuest::log(unsigned level, const char *message)
{
  emit logMessage(level, QString::fromUtf8(message));
}

void DrGuest::setMinigame(const dr_mp_minigame_t *minigame)
{
  if (!m_core) return;
  m_minigame = minigame;

  /* Apply emulation quirks for the minigame */
  if (minigame)
  {
    const char *option_value = nullptr;

    /// Graphics > Settings > Texture Cache Accuracy
    /// 128=Normal, 512=Fast, 0=Safe
    if (minigame->quirks.dolphin.needs_safe_texture_cache)
    {
      log(DR_LOG_INFO, "Mini-game requires safe texture cache, enabling...");
      option_value = "0";
    }
    else
      option_value = "128";
    m_core->options()->setOptionValue("dolphin_texture_cache_accuracy", option_value);

    /// Graphics > Hacks > Skip EFB Copy to RAM
    /// All the negative negation makes this look like a typo, but it's not
    if (minigame->quirks.dolphin.needs_efb_to_texture)
    {
      log(DR_LOG_INFO, "Mini-game requires EFB copy to RAM, enabling...");
      option_value = "disabled";
    }
    else
      option_value = "enabled";
    m_core->options()->setOptionValue("dolphin_efb_to_texture", option_value);

    /// Graphics > Settings > Internal Resolution
    /// @todo Return to user's preferred setting when minigame is finished
    if (minigame->quirks.dolphin.needs_native_resolution)
    {
      log(DR_LOG_INFO, "Mini-game requires native resolution, enabling...");
      option_value = "1";
    }
    else
      option_value = "1";
    m_core->options()->setOptionValue("dolphin_efb_scale", option_value);
  }

  doSetMinigame(minigame);
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
