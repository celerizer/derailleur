#ifndef DR_GUEST_MARIO_GOLF_H
#define DR_GUEST_MARIO_GOLF_H

#include "../DrGuest.h"

class MarioGolf : public DrGuest
{
  Q_OBJECT

public:
  MarioGolf(QObject *parent = nullptr);

  const char *name() const override { return "Mario Golf"; }
  dr_guest id() const override { return DR_GUEST_MARIOGOLF; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Mario Golf (USA).z64"; }
  const char *state() const override { return "mariogolf"; }

  bool usesWarmup() const override { return false; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  int m_lastTurn = -1;                       // last-seen "whose turn" value; drives golf mode
  int m_slotToIndex[4] = { -1, -1, -1, -1 }; // in-game player slot -> board player index
  unsigned m_winners = 0;                    // bitmask of winning board player indices
  bool m_finishPending = false;
  uint32_t m_prevShots[4] = {};              // last-frame strokes per slot (for logging)
  bool m_prevSunk[4] = {};                   // last-frame sunk flag per slot (for logging)
  int m_holeStrokes[4] = { -1, -1, -1, -1 }; // latched strokes at hole-out per slot, else -1
  int m_prevPar = -1;                        // last-logged par (diagnostic)
  int m_portLogDelay = -1;                    // TEST (remove): frames until the +5s port log
  int m_startupDelay = 0;                    // frames to wait after loadState before the A press
  int m_aReleaseDelay = 0;                   // frames until the forced P1 A press releases
};

#endif
