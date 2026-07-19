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
  /* Flycast is a heavy GL core and crashes if preloaded, so it boots lazily on
   * the first launch (see doApplyGameData) rather than warming up at startup. */
  bool usesWarmup(void) const override { return false; }
  std::string gamePath(void) const override { return m_gamePath.toStdString(); }
  unsigned bootFrames(void) const override { return 32; }
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
  QString m_gamePath;
  dr_player_t m_players[4] = {};
  bool m_waitingForReady = false; /* setup done, holding overlay until ready flag */
  int m_alphaSortDelay = 0;    /* frames until accurate alpha sorting is set */
  bool m_firstBoot = true;     /* first-boot one-shot: arm accurate alpha sorting */
  int m_minigameFrames = 0;
  uint8_t m_lastEndFlag = 0;   /* previous frame's player-0 flags byte */
  unsigned m_endFlashes = 0;   /* rising edges into 0x89 seen this mini-game */
  int m_endDelay = 0;          /* frames left to wait after the last end flash */
};

#endif
