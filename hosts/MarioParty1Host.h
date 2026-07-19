#ifndef DR_HOST_MARIO_PARTY_1_H
#define DR_HOST_MARIO_PARTY_1_H

#include "../MarioPartyN64Host.h"

class MarioParty1Host : public MarioPartyN64Host
{
  Q_OBJECT

public:
  explicit MarioParty1Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY1; }
};

#endif
