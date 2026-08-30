#ifndef DR_HOST_MARIO_PARTY_5_H
#define DR_HOST_MARIO_PARTY_5_H

#include "MarioPartyGcnHost.h"

class MarioParty5Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty5Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY5; }
};

#endif
