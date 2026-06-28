#ifndef MARIO_PARTY_4_H
#define MARIO_PARTY_4_H

#include "MarioPartyGcn.h"

class MarioParty4 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty4(QRetro *sharedCore, QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 4"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY4; }
};

#endif
