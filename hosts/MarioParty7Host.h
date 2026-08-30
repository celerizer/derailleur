#ifndef DR_HOST_MARIO_PARTY_7_H
#define DR_HOST_MARIO_PARTY_7_H

#include "MarioPartyGcnHost.h"

class MarioParty7Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty7Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY7; }
};

#endif
