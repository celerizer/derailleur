#ifndef DR_HOST_MARIO_PARTY_4_H
#define DR_HOST_MARIO_PARTY_4_H

#include "MarioPartyGcnHost.h"
#include <string>

/**
 * Gecko codes:
 * 0209BC92 00000054 // ExecBattle skip mini-game
 * 020A218A 00000054 // ExecMGSetup skip mini-game
 * 0212EF12 00000012 // Force a sound group for results screen
 */
class MarioParty4Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty4Host(QObject *parent = nullptr, const std::string &game = {});
  dr_game game(void) const override { return DR_GAME_MARIOPARTY4; }

protected:
  /// MP4's board scores a Bowser mini-game the other way round to every other
  /// type, so the guest's result is flipped back before it is written.
  dr_minigame_result_t adjustResult(unsigned index, const dr_minigame_result_t &result) override;
};

#endif
