#ifndef MARIO_PARTY_6_H
#define MARIO_PARTY_6_H

#include "MarioPartyGcn.h"

class MarioParty6 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty6(QWindow *parent = nullptr);
  const char* name() const override { return "Mario Party 6"; }
};

#endif
