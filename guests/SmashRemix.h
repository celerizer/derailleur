#ifndef DR_GUEST_SMASH_REMIX_H
#define DR_GUEST_SMASH_REMIX_H

#include "../DrGuest.h"

class SmashRemix : public DrGuest
{
  Q_OBJECT

public:
  SmashRemix(QWindow *parent = nullptr);
  const char* name() const override { return "Smash Remix"; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t* minigames() const override;
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

private:
  void applyPlayers();
  void run(void);

  dr_player_t     m_players[4]        = {};
  dr_character    m_slotCharacters[4] = {};
  int             m_slotToIndex[4]    = { -1, -1, -1, -1 };
  uint8_t         m_prevStocks[4]     = { 0xFF, 0xFF, 0xFF, 0xFF };
  uint8_t         m_winners           = 0;
  int             m_finishCountdown   = 0;
};

#endif
