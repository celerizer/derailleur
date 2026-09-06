#ifndef DR_GUEST_MARIO_PARTY_2_H
#define DR_GUEST_MARIO_PARTY_2_H

#include "MarioPartyN64.h"

class MarioParty2 : public MarioPartyN64
{
  Q_OBJECT

public:
  MarioParty2(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 2"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY2; }

  /// Duels score off the game's own winner value rather than the coin fields.
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  /// Duel mini-games are built around the players in slots 0 and 2.
  void remapSlots(void) override;
};

#endif
