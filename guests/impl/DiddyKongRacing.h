#ifndef DR_GUEST_DIDDY_KONG_RACING_H
#define DR_GUEST_DIDDY_KONG_RACING_H

#include "../DrGuestN64.h"

class DiddyKongRacing : public DrGuestN64
{
  Q_OBJECT

public:
  DiddyKongRacing(QObject *parent = nullptr);
  const char *name() const override { return "Diddy Kong Racing"; }
  dr_guest id() const override { return DR_GUEST_DIDDYKONGRACING; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64"; }
  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  void doApplyGameData(const DrGameData &data) override;

private:
  void run() override;
  bool racerAddress(unsigned slot, uint32_t *address);
  void launch();
  int m_frames = 0;
  int m_readyFrames = 0;
  int m_course = -1;
  int m_winnerIndex = -1;
  bool m_launchPending = false;
  bool m_viewportReset = false;
  bool m_racersReady = false;
  unsigned m_humanCount = 0;
  int m_slotToIndex[4] = {};
};

#endif
