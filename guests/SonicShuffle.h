#ifndef DR_GUEST_SONIC_SHUFFLE_H
#define DR_GUEST_SONIC_SHUFFLE_H

#include "../DrGuest.h"

class SonicShuffle : public DrGuest
{
  Q_OBJECT

public:
  SonicShuffle(QObject *parent = nullptr);

  const char *name(void) const override { return "Sonic Shuffle"; }
  dr_guest id(void) const override { return DR_GUEST_SONICSHUFFLE; }

  QRetro *core(void) const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore(void) override;
  void pause(void) override { if (m_retro) m_retro->pause(); }
  void unpause(void) override { if (m_retro) m_retro->unpause(); }

  const dr_mp_minigame_t *minigames(void) const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run(void) override;
  void doApplyGameData(const DrGameData &data) override;

private:
  DrRetro *m_retro = nullptr;
  dr_player_t m_players[4] = {};
  int m_minigameFrames = 0;
};

#endif
