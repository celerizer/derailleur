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
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

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
