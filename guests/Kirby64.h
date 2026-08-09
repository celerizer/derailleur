#ifndef DR_GUEST_KIRBY_64_H
#define DR_GUEST_KIRBY_64_H

#include "../DrGuest.h"

class Kirby64 : public DrGuest
{
  Q_OBJECT

public:
  Kirby64(QObject *parent = nullptr);
  const char *name() const override { return "Kirby 64: The Crystal Shards"; }
  dr_guest id() const override { return DR_GUEST_KIRBY64; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Kirby 64 - The Crystal Shards (USA).z64"; }
  const char *state() const override { return "kirby64"; }

  bool usesWarmup() const override { return false; }
  unsigned bootFrames() const override { return 16; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  void onBeforeBoot(const DrGameData &data) override;
  void writePlayerIcons(const DrGameData &data);

  int m_slotToIndex[4] = { 0, 1, 2, 3 }; // in-game slot -> board player index
  int m_minigameFrames = 0;
  int m_winnerIndex = -1;
  int m_finishCountdown = -1;

  int m_aPressDelay = 0;   // frames after load before forcing a P1 A press (0 = idle)
  int m_aReleaseDelay = 0; // frames to hold the forced A before releasing + starting
};

#endif
