#ifndef DR_GUEST_MARIO_KART_64_H
#define DR_GUEST_MARIO_KART_64_H

#include "../DrGuest.h"

class MarioKart64 : public DrGuest
{
  Q_OBJECT

public:
  MarioKart64(QWindow *parent = nullptr);
  const char* name() const override { return "Mario Kart 64"; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t* minigames() const override;
  void setMinigame(unsigned id) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(unsigned index, dr_team team) override;

private:
  dr_character    m_characters[4] = {};
  dr_control_port m_ports[4]      = { DR_CONTROL_PORT_P1, DR_CONTROL_PORT_P2,
                                      DR_CONTROL_PORT_P3, DR_CONTROL_PORT_P4 };
  dr_control_type m_controlTypes[4] = {};
  int             m_slotToIndex[4]   = { -1, -1, -1, -1 };
  int             m_lapsFreezeFrames = 0;
  int             m_finishCountdown  = -1;
  int             m_winnerIndex      = -1;
};

#endif
