#ifndef DR_HOST_MARIO_PARTY_8_H
#define DR_HOST_MARIO_PARTY_8_H

#include "MarioPartyGcnHost.h"
#include <string>

class MarioParty8Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty8Host(QObject *parent = nullptr, const std::string &game = {});
  dr_game game(void) const override { return DR_GAME_MARIOPARTY8; }

  /// MP8's board never runs a mini-game results screen we can drive, so the
  /// guest reporting back is what ends the mini-game: the winnings are added
  /// straight onto the board's coin and Minigame Star totals.
  void writeResults(DrGuest *guest) override;

protected:
  /// Installs the roulette hook belonging to the board overlay the current scene
  /// id names, checking the site before writing (see MP8_ROULETTE_ORIG).
  void applyGameHooks(int32_t scene) override;

private:
  /// Splits `pot` between the four board slots by the placements they finished in
  /// (0 = 1st), writing each slot's winnings into `payout`. Returns the payout row
  /// used, or nullptr if the placements match no pattern (nothing is paid then).
  const struct mp8_battle_payout *battlePayout(const int placements[4], unsigned pot,
    int64_t payout[4]);

  /// Rolls the next pool and puts the host back to watching the board, standing
  /// in for the results-scene edge the other games return on.
  void returnToBoard(void);

  /// The scene the hooks were last checked for, so a board only logs once.
  int32_t m_HookedScene = -1;

  /// Frames spent on the current board overlay, counted up to the install delay.
  int m_BoardFrames = 0;

};

#endif
