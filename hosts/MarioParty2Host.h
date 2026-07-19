#ifndef DR_HOST_MARIO_PARTY_2_HOST_H
#define DR_HOST_MARIO_PARTY_2_HOST_H

#include "../MarioPartyN64Host.h"

class MarioParty2Host : public MarioPartyN64Host
{
  Q_OBJECT

public:
  explicit MarioParty2Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY2; }
};

#endif
