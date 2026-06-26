#include "DrGuest.h"


QList<DrMinigameGroup> DrGuest::minigameGroups() const
{
  DrMinigameGroup group;
  group.name = name();
  for (const dr_mp_minigame_t *mg = minigames(); mg->name; mg++)
    group.minigames.append(mg);
  return { group };
}

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

dr_error DrGuest::setPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  if (index >= 4 || color >= DR_TEAM_COLOR_SIZE || type >= DR_TEAM_TYPE_SIZE)
    return DR_ERR_INVALID_PARAMETER;
  return doSetPlayerTeam(index, color, type, team_id);
}

void DrGuest::setMinigame(const dr_mp_minigame_t *minigame)
{
  if (!core())
    return;
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
    core()->options()->setOptionValue("dolphin_texture_cache_accuracy", option_value);

    /// Graphics > Hacks > Skip EFB Copy to RAM
    /// All the negative negation makes this look like a typo, but it's not
    if (minigame->quirks.dolphin.needs_efb_to_texture)
    {
      log(DR_LOG_INFO, "Mini-game requires EFB copy to RAM, enabling...");
      option_value = "disabled";
    }
    else
      option_value = "enabled";
    core()->options()->setOptionValue("dolphin_efb_to_texture", option_value);

    /// Graphics > Settings > Internal Resolution
    /// @todo Return to user's preferred setting when minigame is finished
    if (minigame->quirks.dolphin.needs_native_resolution)
    {
      log(DR_LOG_INFO, "Mini-game requires native resolution, enabling...");
      option_value = "1";
    }
    else
      option_value = "1";
    core()->options()->setOptionValue("dolphin_efb_scale", option_value);
  }

  doSetMinigame(minigame);
}

void DrGuest::startMinigame()
{
  m_finishCountdown = 0;
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
