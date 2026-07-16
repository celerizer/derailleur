#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include "DrRetro.h"
#include <cstdio>
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

  virtual QRetro *core() const { return nullptr; }
  virtual bool isValid() const { return m_valid; }
  virtual unsigned warmupFrames() const { return 30; }

  /// Whether this guest is booted and warmed up at startup. Guests that defer
  /// loading their content until launch (e.g. PokemonStadium2) return false so
  /// they are neither started nor added to the warmup queue.
  virtual bool usesWarmup() const { return true; }

  virtual void startCore() {}
  virtual void pause() {}
  virtual void unpause() {}
  virtual QWidget *createWidget(QWidget *parent)
  {
    QWidget *container = QWidget::createWindowContainer(core(), parent);

    /* Accept keyboard focus without needing to click */
    container->setFocusPolicy(Qt::StrongFocus);

    return container;
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

  virtual void run() {}

  /// Guest hook: apply `data` (minigame + 4 players) to the core in whatever way
  /// it needs — write player memory, load a savestate, start the minigame, etc.
  /// Called by applyGameData() after m_minigame and quirks are set.
  virtual void doApplyGameData(const DrGameData &data) = 0;

  bool m_valid = true;
  bool m_minigameActive = false;
  int m_finishCountdown = 0;
  int m_minigameFrameCount = 0;    // frames elapsed in the current minigame
  bool m_frameHookInstalled = false;
  const dr_mp_minigame_t *m_minigame = nullptr;

signals:
  void logMessage(unsigned level, const QString &message);
  void minigameStarted();
  void minigameFinished();
  /// A minigame was aborted (e.g. the global stuck-minigame timeout) rather than
  /// completing normally. Return to the board without writing results.
  void minigameCanceled();
};

#endif
