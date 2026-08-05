#ifndef MARIO_KART_DOUBLE_DASH_H
#define MARIO_KART_DOUBLE_DASH_H

#include "DolphinGuest.h"
#include <string>

class MarioKartDoubleDash : public DolphinGuest
{
  Q_OBJECT

public:
  MarioKartDoubleDash(QRetro *sharedCore, QObject *parent = nullptr);

  const char *name() const override { return "Mario Kart: Double Dash!!"; }
  dr_guest id() const override { return DR_GUEST_MARIOKARTDOUBLEDASH; }

  std::string corePath() const override { return m_corePath; }
  std::string discPath() const override { return m_discPath; }
  std::string statePath() const override { return m_statePath; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;


private:
  void applyPlayers();
  void pressA();
  void advanceSetup();

  DrRetro *m_retro = nullptr;
  std::string m_corePath;
  std::string m_discPath;
  std::string m_statePath;
  int m_minigameFrames = 0;
  bool m_finishPending = false;

  /* Post-load menu sequence (see doApplyGameData/advanceSetup). */
  int m_setupStep = 0;
  int m_stepDelay = 0;
  int m_aReleaseDelay = 0;
  int m_cup = 0;
  int m_track = 0;
};

#endif
