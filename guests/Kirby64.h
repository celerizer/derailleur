#ifndef DR_GUEST_KIRBY_64_H
#define DR_GUEST_KIRBY_64_H

#include "../DrGuest.h"

/* Kirby 64: The Crystal Shards
 *
 * Skeleton guest for the game's three multiplayer sub-games:
 *   - 100-Yard Hop
 *   - Bumper Crop Bump
 *   - Checkerboard Chase
 *
 * TODO: fill in the hardware addresses, the minigame-select write, and the
 *       per-minigame winner detection in run(). Everything below is stubbed so
 *       the guest builds and boots but does not yet drive real gameplay.
 */
class Kirby64 : public DrGuest
{
  Q_OBJECT

public:
  Kirby64(QObject *parent = nullptr);
  const char *name() const override { return "Kirby 64: The Crystal Shards"; }
  dr_guest id() const override { return DR_GUEST_KIRBY64; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  bool usesWarmup() const override { return false; }
  std::string gamePath() const override { return m_gamePath.toStdString(); }
  unsigned bootFrames() const override { return 16; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  void onBeforeBoot(const DrGameData &data) override;
  void writePlayerIcons(const DrGameData &data);

  DrRetro *m_retro = nullptr;
  dr_player_t m_players[4] = {};
  int m_slotToIndex[4] = { 0, 1, 2, 3 }; // in-game slot -> board player index
  int m_minigameFrames = 0;
  int m_winnerIndex = -1;
  int m_finishCountdown = -1;

  QString m_gamePath;
  int m_aPressDelay = 0;   // frames after load before forcing a P1 A press (0 = idle)
  int m_aReleaseDelay = 0; // frames to hold the forced A before releasing + starting
};

#endif
