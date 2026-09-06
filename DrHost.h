#ifndef DR_HOST_H
#define DR_HOST_H

#include "DrGuest.h"
#include "DrRetro.h"
#include <QStringList>
#include <array>

struct DrMinigameCandidate
{
  DrGuest *guest;
  const dr_mp_minigame_t *minigame;
};

typedef struct
{
  /// The type of mini-game being rolled in the roulette, doubling as a signal
  /// that a mini-game is being rolled for at all.
  /// The board context game will write to this value when the mini-game
  /// roulette appears, using the game's internal mini-game type.
  /// derailleur will monitor this value and reset it to -1 after the roulette
  /// lifecycle has completed.
  int8_t minigame_type;

  /// Game-specific. MP4 stamps the roulette position the hand landed on here,
  /// 1..N; every other game leaves it alone.
  int8_t reserved;

  /// Nonzero while the list being built is the single-player DK/Bowser set.
  /// Mario Party 7 only.
  int8_t single_player;

  /// Nonzero while the list being built is the mic set.
  /// Mario Party 6 and 7 only.
  int8_t mic;
} dr_host_state_t;

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
  virtual const std::array<DrMinigameCandidate, 5> &minigameCandidates(
    dr_minigame_type type, dr_mic_mode mic = DR_MIC_ANY) = 0;

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

  /// The console family this host runs on, handed to guests with the launch data
  /// so they can pick artwork that matches the board (see dr_player_icon_32px).
  virtual dr_host_platform platform(void) const { return DR_HOST_PLATFORM_INVALID; }

  /// Wildcard patterns selecting which files under the save directory are shipped
  /// to netplay clients at session start, so every peer plays off the host's save.
  virtual QStringList saveFilePatterns(void) const;

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

  /// Debug helper: read the four board slots' player setup (character, controller
  /// port, human/CPU, team and difficulty) out of the running host game. Fields the
  /// host doesn't track are left as they are. Returns false if this host can't read
  /// its player setup at all.
  virtual bool readPlayerSetup(DrPlayerArray &players) { (void)players; return false; }

  /// Debug helper: stamp `players` into the four board slots of the running host
  /// game, the inverse of readPlayerSetup. Only the fields readPlayerSetup returns
  /// are written. Returns false if this host can't write its player setup.
  virtual bool writePlayerSetup(const DrPlayerArray &players) { (void)players; return false; }

  /// The board's current RNG state, or 0 when this host has no RNG address
  /// configured. Netplay samples it every frame to spot a diverged peer.
  virtual uint32_t rngValue(void) { return 0; }

  /// Coins riding on the current battle mini-game, handed to the guest so it can
  /// show and pay out the same pot the board collected. 0 when the host has no
  /// battle mini-games (MP1) or nothing has been collected.
  virtual unsigned battlePot(void) { return 0; }

  /// The one line both hosts log a mini-game's per-player payout in, e.g.
  /// "Player 1 (Mario): 0 result + 0 bonus".
  static QString resultLogLine(unsigned index, dr_character character,
    const dr_minigame_result_t &result)
  {
    return QString("Player %1 (%2): %3 result + %4 bonus")
      .arg(index + 1)
      .arg(dr_character_name(character))
      .arg(result.coins)
      .arg(result.bonus_coins);
  }

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
