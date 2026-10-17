#ifndef DR_GUEST_DK_JR_MATH_H
#define DR_GUEST_DK_JR_MATH_H

#include "../DrGuestNES.h"

class DkJrMath : public DrGuestNES
{
  Q_OBJECT

public:
  DkJrMath(QObject *parent = nullptr);

  const char *name() const override { return "Donkey Kong Jr. Math"; }
  dr_guest id() const override { return DR_GUEST_DKJRMATH; }
  dr_core coreId() const override { return DR_CORE_FCEUMM; }
  const char *rom() const override { return "Donkey Kong Jr. Math (USA, Europe).nes"; }
  const char *state() const override { return "dkjrmath"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  dr_sync_validator_t syncValidator(void) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  int m_endFrames = 0;

  /* Which NES side (0 = 1P, 1 = 2P) won the match, or -1 */
  int m_winnerSide = -1;

  /* Which board player index is 1P (left) or 2P (right) */
  int m_slotToPlayer[2] = { -1, -1 };

  /* Fur (main) color per side; overalls use one palette row darker */
  uint8_t m_fur[2] = { 0x06, 0x25 };
};

#endif
