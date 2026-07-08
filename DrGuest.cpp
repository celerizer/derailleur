#include "DrGuest.h"


QList<DrMinigameGroup> DrGuest::minigameGroups() const
{
  DrMinigameGroup group;
  group.name = name();
  for (const dr_mp_minigame_t *mg = minigames(); mg->name; mg++)
    group.minigames.append(mg);
  return { group };
}

void DrGuest::applyGameData(const DrGameData &data)
{
  if (!core())
    return;
  m_minigame = data.minigame;

  /* Apply emulation quirks for the minigame */
  const dr_mp_minigame_t *minigame = data.minigame;
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

  doApplyGameData(data);
}

void DrGuest::startMinigame()
{
  m_finishCountdown = 0;
  m_minigameActive = true;
  emit minigameStarted();
}

void DrGuest::finishMinigame()
{
  if (m_minigameActive)
  {
    m_minigameActive = false;
    emit minigameFinished();
  }
}
