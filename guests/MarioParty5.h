#ifndef MARIO_PARTY_5_H
#define MARIO_PARTY_5_H

#include "MarioPartyGcn.h"

class MarioParty5 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty5(QRetro *sharedCore, QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 5"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY5; }
};

#endif
