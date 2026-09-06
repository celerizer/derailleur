#ifndef MARIO_PARTY_7_H
#define MARIO_PARTY_7_H

#include "MarioPartyGcn.h"

class MarioParty7 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty7(QRetro *sharedCore, QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 7"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY7; }

  /// Settles the mic option before the mini-game starts.
  void doApplyGameData(const DrGameData &data) override;
};

#endif
