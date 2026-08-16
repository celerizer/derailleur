#ifndef DR_HOST_MARIO_PARTY_4_H
#define DR_HOST_MARIO_PARTY_4_H

#include "MarioPartyGcnHost.h"

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
  explicit MarioParty4Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY4; }
};

#endif
