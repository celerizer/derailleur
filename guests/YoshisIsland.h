#ifndef DR_GUEST_YOSHIS_ISLAND_H
#define DR_GUEST_YOSHIS_ISLAND_H

#include "../DrGuest.h"

class YoshisIsland : public DrGuest
{
  Q_OBJECT

public:
  YoshisIsland(QObject *parent = nullptr);

  const char *name() const override { return "Yoshi's Island"; }
  dr_guest id() const override { return DR_GUEST_YOSHISISLAND; }
  dr_core coreId() const override { return DR_CORE_SNES9X; }
  const char *rom() const override { return "Super Mario World 2 - Yoshi's Island (USA) (Rev 1).sfc"; }

  bool usesWarmup() const override { return false; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
};

#endif
