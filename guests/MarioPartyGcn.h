#ifndef DR_GUEST_MARIO_PARTY_GCN_H
#define DR_GUEST_MARIO_PARTY_GCN_H

#include "../DrGuest.h"
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

class MarioPartyGcn : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyGcn(const MpGcnConfig &config, QRetro *sharedCore, QObject *parent = nullptr);

  dr_minigame_result_t    minigameResult(unsigned index) override;
  const dr_mp_minigame_t* minigames() const override;
  void                    doSetMinigame(const dr_mp_minigame_t *minigame) override;

  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

  const MpGcnConfig &config() const { return m_config; }

private:
  void run() override;
  MpGcnConfig  m_config;
  int32_t      m_lastScene           = -1;
};

#endif
