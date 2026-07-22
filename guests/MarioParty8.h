#ifndef DR_GUEST_MARIO_PARTY_8_H
#define DR_GUEST_MARIO_PARTY_8_H

#include "DolphinGuest.h"
#include <string>

class MarioParty8 : public DolphinGuest
{
  Q_OBJECT

public:
  MarioParty8(QRetro *sharedCore, QObject *parent = nullptr);

  const char *name() const override { return "Mario Party 8"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY8; }

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
  DrRetro *m_retro = nullptr;
  std::string m_corePath;
  std::string m_discPath;
  std::string m_statePath;

  dr_player_t m_players[4] = {};
  int m_minigameFrames = 0;
  int32_t m_lastScene = -1;
};

#endif
