#ifndef DR_GUEST_SONIC_SECRET_RINGS_H
#define DR_GUEST_SONIC_SECRET_RINGS_H

#include "../DrGuestWii.h"

class SonicSecretRings : public DrGuestWii
{
  Q_OBJECT

public:
  SonicSecretRings(QObject *parent = nullptr);

  const char *name() const override { return "Sonic and the Secret Rings"; }
  dr_guest id() const override { return DR_GUEST_SONICSECRETRINGS; }
  const char *state(void) const override { return "sonicsecretrings"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  bool m_finishScheduled = false;
  /// Mini-game state from the previous frame, to catch ending -> finished
  int64_t m_lastState = 0;
  /// Each slot's Party points when the state was loaded
  uint32_t m_startPoints[4] = { 0, 0, 0, 0 };
};

#endif
