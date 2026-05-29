#ifndef DR_GUEST_MARIO_PARTY_2_H
#define DR_GUEST_MARIO_PARTY_2_H

#include "MarioPartyN64.h"

class MarioParty2 : public MarioPartyN64
{
  Q_OBJECT

public:
  MarioParty2(QObject *parent = nullptr);
  const char* name() const override { return "Mario Party 2"; }
};

#endif
