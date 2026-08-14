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

/// The program's mini-game pool, seen from a host game. It caches five rolled
/// candidates for every mini-game type; a host queries a type to read its five,
/// or asks for a fresh reroll of all of them. Implemented by DrGuestList.
///
/// The cache starts empty and is not filled until the first query (or reroll).
/// Because that first fill happens inside the host's lockstepped state machine,
/// every netplay peer rolls from the shared PRNG at the same logical point and
/// stays in sync -- no per-candidate network round-trip.
class DrMinigameSource
{
public:
  virtual ~DrMinigameSource() = default;

  /// The five cached candidates for `type`. Fills the cache on the first-ever
  /// query. Returns a zeroed array for an out-of-range type.
  virtual const std::array<DrMinigameCandidate, 5> &minigameCandidates(dr_minigame_type type) = 0;

  /// Re-roll the five cached candidates for every type.
  virtual void rerollMinigames(void) = 0;
};

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

  /// The pool a host queries for its mini-game candidates. Set once by the owner
  /// (MainWindow) after both the host and the guest list exist.
  void setMinigameSource(DrMinigameSource *source) { m_MinigameSource = source; }

  /// Write a finished guest mini-game's results back into the host.
  virtual void writeResults(DrGuest *guest) = 0;

  /// Zero every result (used when a mini-game is canceled rather than completed,
  /// so the board doesn't read stale coins from the previous one).
  virtual void clearResults(void) = 0;

  /// Debug helper: overwrite the board's current-turn counter. No-op if the host
  /// has no turn counter.
  virtual void setCurrentTurn(unsigned turn) { (void)turn; }

  /// Which of the four board players is the local human (0-3). In a netplay
  /// session this is our peer index; solo it stays 0. Used by hosts that show
  /// per-player private state (e.g. Sonic Shuffle's VMU hand).
  void setLocalPlayer(int index) { m_LocalPlayer = index; }
  int localPlayer(void) const { return m_LocalPlayer; }

protected:
  int m_LocalPlayer = 0;
  DrMinigameSource *m_MinigameSource = nullptr;

signals:
  /// A guest mini-game should be launched for `candidate` with these players.
  void minigameRequested(DrMinigameCandidate candidate, DrPlayerArray players);

  /// Request netplay "golf mode": `authorityPlayer` (a peer index) gets 0 input delay,
  /// everyone else `highDelay` frames. -1 disables it. See DrNetplay::setGolfMode.
  void golfModeRequested(int authorityPlayer, int highDelay);
};

#endif
