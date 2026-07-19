#include "DrGuest.h"

#include <QRetro.h>

/* Global safety net: a minigame that runs this many frames without finishing is
 * almost certainly stuck, so cancel it and return to the board (~5 min @ 60fps). */
static constexpr int DR_MINIGAME_TIMEOUT_FRAMES = 60 * 60 * 5;

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

  /* Warmed guests (booted at startup, e.g. the Dolphin core) apply immediately.
   * Deferred guests load their content and boot on the first launch, then apply
   * once the core has run bootFrames() frames; subsequent launches re-apply on
   * the next frame. The boot countdown is driven off the core's own frames. */
  if (usesWarmup())
  {
    doApplyGameData(data);
    return;
  }

  m_pendingData = data;
  onBeforeBoot(data); /* pre-boot / pre-reload work (e.g. hires textures) */

  if (m_started)
  {
    m_stateLoadCountdown = 1; /* already booted; apply on the next frame */
    return;
  }

  m_started = true;
  if (!gamePath().empty())
    core()->loadContent(gamePath().c_str());
  startCore();

  if (!m_deferHookInstalled)
  {
    m_deferHookInstalled = true;
    connect(core(), &QRetro::frameBegin, this, [this]() {
      if (m_stateLoadCountdown <= 0)
        return;
      /* Mute through the boot window. On the first lazy launch mainwindow's mute
       * ran before the core booted (audio() was still null), so re-assert it here
       * each frame; the minigameStarted handler unmutes once the game starts. */
      if (auto *a = core()->audio())
        a->setMute(true);
      if (--m_stateLoadCountdown != 0)
        return;
      /* The hook runs on the timing thread. Guests that do GUI work in
       * doApplyGameData (show/waitFrames/processEvents) must be marshaled to the
       * GUI thread; the rest apply directly (safe for state loads + memory). The
       * container nudge always runs on the GUI thread, after the apply. */
      if (applyOnGuiThread())
        QMetaObject::invokeMethod(this, [this]() {
          doApplyGameData(m_pendingData);
          resizeToContainer();
        }, Qt::QueuedConnection);
      else
      {
        doApplyGameData(m_pendingData);
        QMetaObject::invokeMethod(this, [this]() { resizeToContainer(); },
          Qt::QueuedConnection);
      }
    }, Qt::DirectConnection);
  }
  m_stateLoadCountdown = bootFrames();
}

void DrGuest::resizeToContainer()
{
  if (m_container && core())
    core()->resize(m_container->width(), m_container->height());
}

void DrGuest::startMinigame()
{
  m_finishCountdown = 0;
  m_minigameFrameCount = 0;
  m_minigameActive = true;

  /* Install the stuck-minigame timeout once, on this guest's core. It fires every
   * frame but only counts while a minigame is active, so it's harmless otherwise.
   * Receiver is `this`, so it auto-disconnects when the guest is destroyed. */
  if (!m_frameHookInstalled && core())
  {
    m_frameHookInstalled = true;
    connect(core(), &QRetro::frameBegin, this, [this]() {
      if (m_minigameActive && ++m_minigameFrameCount >= DR_MINIGAME_TIMEOUT_FRAMES)
      {
        const char *mg = (m_minigame && m_minigame->name) ? m_minigame->name : "minigame";
        log(DR_LOG_WARN,
          qPrintable(QString("%1: \"%2\" ran %3 frames without finishing; aborting")
                       .arg(name()).arg(mg).arg(DR_MINIGAME_TIMEOUT_FRAMES)));
        cancelMinigame();
        emit minigameCanceled();
      }
    }, Qt::DirectConnection);
  }

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
