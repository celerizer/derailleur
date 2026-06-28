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

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override   { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

protected:
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;
  void run() override;

private:
  void runTimeBombTicks();

  DrRetro *m_retro = nullptr;
  unsigned m_winners = 0;
};

#endif
