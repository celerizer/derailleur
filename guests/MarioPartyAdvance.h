#ifndef DR_GUEST_MARIO_PARTY_ADVANCE_H
#define DR_GUEST_MARIO_PARTY_ADVANCE_H

#include "../DrGuest.h"

class MarioPartyAdvance : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyAdvance(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party Advance"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTYADVANCE; }
  dr_core coreId() const override { return DR_CORE_MGBA; }
  const char *rom() const override { return "Mario Party Advance (USA).gba"; }
  const char *state() const override { return "mpadvance"; }

  bool usesWarmup() const override { return false; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void run4pPinball();
  void run() override;

  bool m_gameStarted = false;
  unsigned m_winners = 0;
  unsigned m_EndWaitFrames = 0;
};

#endif
