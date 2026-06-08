#ifndef DR_GUEST_MARIO_PARTY_ADVANCE_H
#define DR_GUEST_MARIO_PARTY_ADVANCE_H

#include "../DrGuest.h"

class MarioPartyAdvance : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyAdvance(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party Advance"; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;
};

#endif
