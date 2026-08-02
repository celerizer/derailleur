#ifndef DR_GUEST_MARIO_PARTY_E_H
#define DR_GUEST_MARIO_PARTY_E_H

#include "../DrGuest.h"

class MarioPartyE : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyE(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party-e"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTYE; }
  dr_core coreId() const override { return DR_CORE_MGBA; }
  const char *rom() const override { return "e-Reader (USA).gba"; }

  bool usesWarmup() const override { return false; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

  void doApplyGameData(const DrGameData &data) override;

protected:
  void run() override;

private:
  void runTimeBombTicks();

  unsigned m_winners = 0;
};

#endif
