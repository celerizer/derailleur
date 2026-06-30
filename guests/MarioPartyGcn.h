#ifndef DR_GUEST_MARIO_PARTY_GCN_H
#define DR_GUEST_MARIO_PARTY_GCN_H

#include "DolphinGuest.h"
#include <string>

struct MpGcnConfig
{
  std::string core;
  std::string game;
  std::string state;

  int scene_miniexplain;
  int scene_miniresults;

  size_t scene_addr;
  size_t minigame_addr;

  size_t character_addr[4];
  size_t controller_addr[4];
  size_t difficulty_addr[4];
  size_t team_addr[4];
  size_t bot_addr[4];

  /// The secondary coin amount gained from a mini-game
  size_t bonus_result_addr[4];

  /// The primary coin amount gained from a mini-game, usually 0 or 10
  size_t result_addr[4];

  const uint16_t *character_ids;
  const dr_mp_minigame_t *minigames;
};

class MarioPartyGcn : public DolphinGuest
{
  Q_OBJECT

public:
  MarioPartyGcn(const MpGcnConfig &config, QRetro *sharedCore, QObject *parent = nullptr);

  std::string corePath() const override { return m_config.core; }
  std::string discPath() const override { return m_config.game; }
  std::string statePath() const override { return m_config.state; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

  const MpGcnConfig &config() const { return m_config; }

protected:
  void run() override;
  void applyPlayers();
  DrRetro *m_retro = nullptr;
  MpGcnConfig m_config;
  int32_t m_lastScene = -1;
  int m_minigameFrames = 0;

  dr_player_t m_players[4] = {};
  int m_slotOf[4] = { 0, 1, 2, 3 }; // board player index -> game slot
};

#endif
