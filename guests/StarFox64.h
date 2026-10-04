#ifndef DR_GUEST_STAR_FOX_64_H
#define DR_GUEST_STAR_FOX_64_H

#include "../DrGuest.h"

class StarFox64 : public DrGuest
{
  Q_OBJECT

public:
  StarFox64(QObject *parent = nullptr);

  const char *name() const override { return "Star Fox 64"; }
  dr_guest id() const override { return DR_GUEST_STARFOX64; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Star Fox 64 (USA) (Rev 1).z64"; }
  const char *state() const override { return "starfox64"; }

  bool usesWarmup() const override { return false; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  void onBeforeBoot(const DrGameData &data) override;

private:
  /* Writes the character's head into in-game player slot `slot`'s hires icon. */
  void writePlayerIcon(unsigned slot, dr_character character);

  int m_minigameFrames = 0;
  int m_slotToIndex[4] = { -1, -1, -1, -1 }; /* in-game player slot -> board player index */
  int m_winnerIndex = -1;                    /* board player index of the winner, else -1 */
  int m_formOption = 0;                      /* starting vehicle to pick (sf64_form_option), 0 = none */
  bool m_shieldsPending = false;             /* Battle Royal: cut shields once play starts */
};

#endif
