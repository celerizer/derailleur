#ifndef KIRBY_AIR_RIDE_H
#define KIRBY_AIR_RIDE_H

#include "DolphinGuest.h"
#include <string>

class KirbyAirRide : public DolphinGuest
{
  Q_OBJECT

public:
  KirbyAirRide(QRetro *sharedCore, QObject *parent = nullptr);

  const char *name() const override { return "Kirby Air Ride"; }
  dr_guest id() const override { return DR_GUEST_KIRBYAIRRIDE; }

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

  DrRetro *m_retro = nullptr;
  std::string m_corePath;
  std::string m_discPath;
  std::string m_statePath;
  int m_minigameFrames = 0;
  bool m_finishPending = false;
  dr_player_t m_players[4] = {};
};

#endif
