#ifndef DR_GUEST_BANJO_TOOIE_H
#define DR_GUEST_BANJO_TOOIE_H

#include "../DrGuest.h"

class BanjoTooie : public DrGuest
{
  Q_OBJECT

public:
  BanjoTooie(QObject *parent = nullptr);
  const char *name() const override { return "Banjo-Tooie"; }
  dr_guest id() const override { return DR_GUEST_BANJOTOOIE; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

  /// Reads the per-player scores and returns a bitmask of the winning player(s)
  /// (most points; ties share the win).
  unsigned computeWinners();

  DrRetro *m_retro = nullptr;
  dr_player_t m_players[4] = {};
  int m_slotToIndex[4] = { 0, 1, 2, 3 }; // in-game slot (controller port) -> board index
  int m_minigameFrames = 0;

  /// The heap pointers have been seen non-null at least once; only after that
  /// does them going null mean the mini-game finished (see run).
  bool m_pointersSeen = false;
  int m_aPressDelay = 0;   // frames after the write before forcing the P1 A press
  int m_aReleaseDelay = 0; // frames to hold the forced P1 A press before releasing
};

#endif
