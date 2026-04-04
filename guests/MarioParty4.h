#ifndef MARIO_PARTY_4_H
#define MARIO_PARTY_4_H

#include "MarioPartyGcn.h"

class MarioParty4 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty4(QRetro *sharedCore, QObject *parent = nullptr);
  const char* name() const override { return "Mario Party 4"; }
};

#endif
