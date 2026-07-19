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

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  bool usesWarmup() const override { return false; }
  std::string gamePath() const override { return m_gamePath; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void applyTeams();
  void run();

  DrRetro *m_retro = nullptr;
  std::string m_gamePath;
  dr_player_t m_players[4] = {};
  unsigned m_winners = 0;
  int m_finishCountdown = 0;
  int m_allCpuFrames = 0;
};

#endif
