#ifndef DR_GUEST_SMASH_REMIX_H
#define DR_GUEST_SMASH_REMIX_H

#include "../DrGuest.h"

class SmashRemix : public DrGuest
{
  Q_OBJECT

public:
  SmashRemix(QObject *parent = nullptr);
  const char *name() const override { return "Smash Remix"; }
  dr_guest id() const override { return DR_GUEST_SMASHREMIX; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "smashremix.z64"; }
  const char *state() const override { return "smashremix"; }

  bool usesWarmup() const override { return false; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void applyPlayers();
  void run(void);

  dr_character m_slotCharacters[4] = {};
  int m_slotToIndex[4] = { -1, -1, -1, -1 };
  int8_t m_prevStocks[4] = { -1, -1, -1, -1 };
  uint8_t m_winners = 0;
  int m_finishCountdown = 0;
  int m_placement[4] = { -1, -1, -1, -1 };
  unsigned m_eliminationCount = 0;
  int m_startDelay = 0; // frames to hold the overlay before starting the minigame
};

#endif
