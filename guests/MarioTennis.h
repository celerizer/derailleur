#ifndef DR_GUEST_MARIO_TENNIS_H
#define DR_GUEST_MARIO_TENNIS_H

#include "../DrGuest.h"

class MarioTennis : public DrGuest
{
  Q_OBJECT

public:
  MarioTennis(QObject *parent = nullptr);
  const char *name() const override { return "Mario Tennis"; }
  dr_guest id() const override { return DR_GUEST_MARIOTENNIS; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Mario Tennis (USA).z64"; }
  const char *state() const override { return "mariotennis"; }

  bool usesWarmup() const override { return false; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void applyTeams();
  void run();

  unsigned m_winners = 0;
  int m_finishCountdown = 0;
  int m_allCpuFrames = 0;
};

#endif
