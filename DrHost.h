#ifndef DR_HOST_H
#define DR_HOST_H

#include "DrGuest.h"
#include "DrRetro.h"
#include <array>

struct DrMinigameCandidate
{
  DrGuest *guest;
  const dr_mp_minigame_t *minigame;
};
Q_DECLARE_METATYPE(DrMinigameCandidate)
Q_DECLARE_METATYPE(dr_minigame_type)
using DrPlayerArray = std::array<dr_player_t, 4>;
Q_DECLARE_METATYPE(DrPlayerArray)

/// Abstract base for a "host" game: a randomizer board that runs its own core,
/// asks for mini-game candidates, launches guest mini-games, and writes their
/// results back. All game-specific detection lives in subclasses (e.g.
/// MarioPartyN64Host); the rest of the program only ever talks to this interface.
class DrHost : public DrRetro
{
  Q_OBJECT

public:
  explicit DrHost(QObject *parent = nullptr);

  /// Which host game this is (used for netplay/session identity).
  virtual dr_game game(void) const = 0;

  /// Inject the five chosen mini-game candidates for the pending roulette.
  virtual void setCandidates(std::array<DrMinigameCandidate, 5> candidates) = 0;

  /// Write a finished guest mini-game's results back into the host.
  virtual void writeResults(DrGuest *guest) = 0;

  /// Zero every result (used when a mini-game is canceled rather than completed,
  /// so the board doesn't read stale coins from the previous one).
  virtual void clearResults(void) = 0;

  /// Debug helper: overwrite the board's current-turn counter. No-op if the host
  /// has no turn counter.
  virtual void setCurrentTurn(unsigned turn) { (void)turn; }

signals:
  /// The host is about to open its roulette and needs candidates of `type`.
  void candidatesNeeded(dr_minigame_type type);

  /// A guest mini-game should be launched for `candidate` with these players.
  void minigameRequested(DrMinigameCandidate candidate, DrPlayerArray players);
};

#endif
