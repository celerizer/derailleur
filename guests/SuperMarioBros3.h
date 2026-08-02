#ifndef DR_GUEST_SUPER_MARIO_BROS_3_H
#define DR_GUEST_SUPER_MARIO_BROS_3_H

#include "../DrGuest.h"

/*
 * Super Mario Bros. 3 (NES) -- skeleton guest, hosted by its own fceumm core.
 *
 * TODO: fill in the mini-game table, the per-player/setup memory addresses,
 *       doApplyGameData (load a savestate + write the selected mini-game and
 *       players), finish detection in run(), and minigameResult.
 */
class SuperMarioBros3 : public DrGuest
{
  Q_OBJECT

public:
  SuperMarioBros3(QObject *parent = nullptr);

  const char *name() const override { return "Super Mario Bros. 3"; }
  dr_guest id() const override { return DR_GUEST_SUPERMARIOBROS3; }
  dr_core coreId() const override { return DR_CORE_FCEUMM; }
  const char *rom() const override { return "Super Mario Bros. 3 (USA) (Rev 1).nes"; }
  const char *state() const override { return "supermariobros3"; }

  bool usesWarmup() const override { return false; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  /* Which board player index fills each Vs. sprite slot (0 = Mario, 1 = Luigi),
   * so the winner byte can be mapped back to a player. */
  int m_slotToPlayer[2] = { -1, -1 };
};

#endif
