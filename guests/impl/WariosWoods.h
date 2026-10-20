#ifndef DR_GUEST_WARIOS_WOODS_H
#define DR_GUEST_WARIOS_WOODS_H

#include "../DrGuestNES.h"

class WariosWoods : public DrGuestNES
{
  Q_OBJECT

public:
  WariosWoods(QObject *parent = nullptr);

  const char *name() const override { return "Wario's Woods"; }
  dr_guest id() const override { return DR_GUEST_WARIOSWOODS; }
  dr_core coreId() const override { return DR_CORE_FCEUMM; }
  const char *rom() const override { return "Wario's Woods (USA).nes"; }
  const char *state() const override { return "wariowoods"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  dr_sync_validator_t syncValidator(void) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  int m_endFrames = 0;

  /* Which board player index is 1P (left) or 2P (right) */
  int m_slotToPlayer[2] = { -1, -1 };

  /* Toad cap spot color written for each side, every frame */
  uint8_t m_spots[2] = { 0x16, 0x16 };
};

#endif
