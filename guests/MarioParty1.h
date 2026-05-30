#ifndef DR_GUEST_MARIO_PARTY_1_H
#define DR_GUEST_MARIO_PARTY_1_H

#include "MarioPartyN64.h"

class MarioParty1 : public MarioPartyN64
{
  Q_OBJECT

public:
  MarioParty1(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 1"; }
};

#endif
