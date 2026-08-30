#ifndef DR_HOST_MARIO_PARTY_6_H
#define DR_HOST_MARIO_PARTY_6_H

#include "MarioPartyGcnHost.h"

class MarioParty6Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty6Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY6; }
};

#endif
