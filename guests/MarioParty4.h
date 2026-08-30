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

  /// MP4's Bowser mini-games score the other way round to every other type, so
  /// the result comes back flipped.
  dr_minigame_result_t minigameResult(unsigned index) override;
};

#endif
