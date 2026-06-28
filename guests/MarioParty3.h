#ifndef DR_GUEST_MARIO_PARTY_3_H
#define DR_GUEST_MARIO_PARTY_3_H

#include "MarioPartyN64.h"

class MarioParty3 : public MarioPartyN64
{
  Q_OBJECT

public:
  MarioParty3(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 3"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY3; }
};

#endif
