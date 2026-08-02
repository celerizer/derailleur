#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include "DrRetro.h"
#include <cstdio>
#include <string>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>

struct DrMinigameGroup
{
  const char *name;
  QList<const dr_mp_minigame_t *> minigames;
};

/// The full setup for one minigame launch, delivered to a guest in a single
/// call: which minigame, its type, and all four players. Extend with board
/// context as guests need it.
struct DrGameData
{
  const dr_mp_minigame_t *minigame = nullptr;
  dr_minigame_type type = DR_MINIGAME_INVALID;
  dr_player_t players[4] = {};
};

class DrGuest : public QObject
{
  Q_OBJECT

public:
  DrGuest(QObject *parent = nullptr)
    : QObject(parent)
  {
  }

  virtual QRetro *core() const { return m_retro ? m_retro->core() : nullptr; }
  virtual bool isValid() const { return m_valid && (!m_retro || m_retro->isValid()); }
  virtual unsigned warmupFrames() const { return 30; }

  /// Whether this guest is booted and warmed up at startup. Guests that defer
  /// loading their content until launch (the common case) return false so they
  /// are neither started nor added to the warmup queue; the base then boots them
  /// on first launch (see applyGameData). Only cores that must be set up ahead of
  /// time (e.g. the shared Dolphin core, for state/disc swaps) keep this true.
  virtual bool usesWarmup() const { return true; }

  /// Content path a deferred guest loads on its first launch. Ignored by warmed
  /// guests (usesWarmup() == true). Defaults to the core's own game path (set via
  /// DrRetro::init()).
  virtual std::string gamePath() const { return m_retro ? m_retro->gamePath() : std::string{}; }

  /// Path to the guest's core library. When set, the base dlopens it lazily on the
  /// first launch (see loadCore / applyGameData) instead of at construction, so
  /// cores are only opened once actually booted into. Empty = no own core (e.g. a
  /// Dolphin sub-game that shares CoreDolphin's core). Defaults to DrRetro::init()'s.
  virtual std::string corePath() const { return m_retro ? m_retro->corePath() : std::string{}; }

  /// Loads the guest's core, once, before its first content load. The default
  /// dlopens corePath(); override for cores that need custom loading (e.g.
  /// CoreDolphin patches its library first). Returns false if the core can't load.
  virtual bool loadCore();

  /// Frames to let the core boot after the deferred loadContent before the base
  /// invokes doApplyGameData(). Override per core as needed.
  virtual unsigned bootFrames() const { return 30; }

  /// Whether the deferred doApplyGameData() must run on the GUI thread rather than
  /// the core's timing thread. Most guests apply on the timing thread (safe for
  /// state loads + memory writes); return true only if doApplyGameData does GUI
  /// work like show()/waitFrames()/processEvents() (e.g. CoreDolphin's disc swap).
  virtual bool applyOnGuiThread() const { return false; }

  /// Default: connect run() to the core's frameBegin and start it. Guests that own
  /// a core (via DrRetro::init) need nothing more; override for special cases.
  virtual void startCore();
  virtual void pause() { if (m_retro) m_retro->pause(); }
  virtual void unpause() { if (m_retro) m_retro->unpause(); }
  virtual QWidget *createWidget(QWidget *parent)
  {
    m_container = QWidget::createWindowContainer(core(), parent);

    /* Accept keyboard focus without needing to click */
    m_container->setFocusPolicy(Qt::StrongFocus);

    return m_container;
  }

  void tick()
  {
    if (m_finishCountdown > 0 && --m_finishCountdown == 0)
    {
      finishMinigame();
      return;
    }
    run();
  }

  virtual dr_minigame_result_t minigameResult(unsigned index) = 0;
  virtual const dr_mp_minigame_t *minigames() const = 0;
  virtual const char *name(void) const = 0;
  virtual dr_guest id(void) const { return DR_GUEST_INVALID; }

  /// The core this guest runs on, its ROM filename (under the roms dir), and its
  /// savestate base name (dr_state_directory()/<state>.state.zip). An owned-core
  /// guest overrides these; the constructor feeds coreId()/rom() to DrRetro::init()
  /// and doApplyGameData() feeds state() to loadState().
  virtual dr_core coreId(void) const { return DR_CORE_INVALID; }
  virtual const char *rom(void) const { return ""; }
  virtual const char *state(void) const { return ""; }

  virtual QList<DrMinigameGroup> minigameGroups() const;
  void cancelMinigame() { m_minigameActive = false; }

  /// Applies a whole minigame launch — the chosen minigame and all four players
  /// — in one call. Sets m_minigame, applies emulation quirks, then dispatches to
  /// the guest's doApplyGameData(). Replaces the old per-variable
  /// setMinigame/setPlayer path.
  void applyGameData(const DrGameData &data);

protected:
  void log(unsigned level, const char *message)
  {
    /* Also echo to stderr: the logger isn't connected to guests until after they
     * are constructed, so construction-time messages (failed to load core / rom
     * not found / failed to load content) would otherwise be lost. */
    fprintf(stderr, "[guest] %s\n", message);
    emit logMessage(level, QString::fromUtf8(message));
  }
  void startMinigame();
  void finishMinigame();
  void finishMinigameInFrames(int frames) { m_finishCountdown = frames; }

  /// Loads the guest's savestate from dr_state_directory()/<name>.state.zip, logs
  /// the result, and returns whether it loaded. Called from doApplyGameData.
  bool loadState(const char *name);

  virtual void run() {}

  /// Guest hook: apply `data` (minigame + 4 players) to the core in whatever way
  /// it needs — write player memory, load a savestate, start the minigame, etc.
  /// For deferred guests this runs once the core has booted (bootFrames after the
  /// first launch); for warmed guests it runs immediately. See applyGameData().
  virtual void doApplyGameData(const DrGameData &data) = 0;

  /// Deferred-guest hook run at launch time, *before* loadContent/startCore on the
  /// first launch (and before the re-apply countdown on later launches). Use it for
  /// anything that must precede the core booting — e.g. laying down hires-texture
  /// data the core reads at content load. No-op by default; never called for warmed
  /// guests (they are already booted).
  virtual void onBeforeBoot(const DrGameData &data) { (void)data; }

  bool m_valid = true;
  bool m_minigameActive = false;
  int m_finishCountdown = 0;
  int m_minigameFrameCount = 0;    // frames elapsed in the current minigame
  bool m_frameHookInstalled = false;
  const dr_mp_minigame_t *m_minigame = nullptr;

  /* Core wrapper, shared by every guest that owns a core (set up via
   * DrRetro::init()). Dolphin sub-games leave it pointing at a shared core. */
  DrRetro *m_retro = nullptr;

  /* The four players for the current launch, copied from the launch data by
   * applyGameData() before doApplyGameData()/onBeforeBoot() run. */
  dr_player_t m_players[4] = {};

  /// Resize the core's window to fill its container. Must run on the GUI thread;
  /// the base queues it after a deferred boot (the core sizes itself to the game's
  /// native resolution when it boots, so this makes the widget fill the area).
  void resizeToContainer();

  /* Deferred-boot state (see applyGameData). Protected so guests' run() can react
   * to the boot window (e.g. mute while m_stateLoadCountdown > 0). */
  bool m_started = false;           // content loaded + core booted
  int m_stateLoadCountdown = 0;     // frames until doApplyGameData() is invoked
  DrGameData m_pendingData;         // launch data awaiting the boot countdown
  bool m_deferHookInstalled = false;
  QWidget *m_container = nullptr;    // window container from createWidget()

signals:
  void logMessage(unsigned level, const QString &message);
  void minigameStarted();
  void minigameFinished();
  /// A minigame was aborted (e.g. the global stuck-minigame timeout) rather than
  /// completing normally. Return to the board without writing results.
  void minigameCanceled();
};

#endif
