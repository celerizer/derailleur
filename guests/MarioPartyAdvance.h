#ifndef DR_GUEST_MARIO_PARTY_ADVANCE_H
#define DR_GUEST_MARIO_PARTY_ADVANCE_H

#include "../DrGuest.h"

class MarioPartyAdvance : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyAdvance(QObject *parent = nullptr);
  const char *name() const override { return "Mario Party Advance"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTYADVANCE; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void run4pPinball();
  void run() override;

  DrRetro *m_retro = nullptr;
  bool m_gameStarted = false;
  unsigned m_winners = 0;
  unsigned m_EndWaitFrames = 0;
};

#endif
