#ifndef DR_GUEST_SMASH_REMIX_H
#define DR_GUEST_SMASH_REMIX_H

#include "../DrGuest.h"

class SmashRemix : public DrGuest
{
  Q_OBJECT

public:
  SmashRemix(QWindow *parent = nullptr);

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t* minigames() const override;
  void setMinigame(unsigned id) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(unsigned index, dr_team team) override;

private:
  void applyPlayers();

  dr_character    m_characters[4]     = {};
  dr_control_port m_ports[4]          = {};
  dr_control_type m_controlTypes[4]   = {};
  dr_character    m_slotCharacters[4] = {};
  int             m_slotToIndex[4]    = { -1, -1, -1, -1 };
  uint8_t         m_prevStocks[4]     = { 0xFF, 0xFF, 0xFF, 0xFF };
  int             m_winnerIndex       = -1;
  int             m_finishCountdown   = 0;
};

#endif
