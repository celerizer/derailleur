#ifndef MARIO_PARTY_4_H
#define MARIO_PARTY_4_H

#include "../DrGuest.h"

class MarioParty4 : public DrGuest
{
  Q_OBJECT

public:
  MarioParty4(QWindow *parent = nullptr) : DrGuest(parent) {}

  dr_minigame_result_t minigameResult(unsigned index) const override;

  dr_error setPlayerCharacter(unsigned index, dr_character character) override;
  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error setPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error setPlayerTeam(unsigned index, dr_team team) override;
};

#endif
