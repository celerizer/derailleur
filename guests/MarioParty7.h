#ifndef MARIO_PARTY_7_H
#define MARIO_PARTY_7_H

#include "MarioPartyGcn.h"

class MarioParty7 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty7(QRetro *sharedCore, QObject *parent = nullptr);
  const char* name() const override { return "Mario Party 7"; }
};

#endif
