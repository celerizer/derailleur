#ifndef DR_GUEST_MARIO_KART_64_H
#define DR_GUEST_MARIO_KART_64_H

#include "../DrGuest.h"

class MarioKart64 : public DrGuest
{
  Q_OBJECT

public:
  MarioKart64(QObject *parent = nullptr);
  const char *name() const override { return "Mario Kart 64"; }
  dr_guest id() const override { return DR_GUEST_MARIOKART64; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void run() override;
  DrRetro *m_retro = nullptr;
  dr_character m_characters[4] = {};
  dr_control_port m_ports[4] = { DR_CONTROL_PORT_P1, DR_CONTROL_PORT_P2, DR_CONTROL_PORT_P3,
    DR_CONTROL_PORT_P4 };
  dr_control_type m_controlTypes[4] = {};
  int m_slotToIndex[4] = { -1, -1, -1, -1 };
  int m_lapsFreezeFrames = 0;
  int m_finishCountdown = -1;
  int m_winnerIndex = -1;
};

#endif
