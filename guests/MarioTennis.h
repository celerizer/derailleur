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
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

private:
  void applyTeams();
  void run();

  DrRetro *m_retro = nullptr;
  dr_player_t m_players[4] = {};
  unsigned m_winners = 0;
  int m_finishCountdown = 0;
};

#endif
