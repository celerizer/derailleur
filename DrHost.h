#ifndef DR_HOST_H
#define DR_HOST_H

#include "DrRetro.h"
#include <array>
#include <string>

class DrGuest;

struct dr_scene_range_t
{
  uint8_t min;
  uint8_t max;
};

struct DrHostConfig
{
  std::string core;
  std::string game;
  uint8_t scene_miniexplain;
  uint8_t scene_miniresults;
  uint8_t scene_miniresults_battle;
  size_t scene_addr;
  dr_scene_range_t scene_board_ranges[4];
  unsigned scene_board_range_count;
  dr_scene_range_t scene_duel_board_range;
  size_t character_addr[4];
  size_t controller_addr[4];
  size_t difficulty_addr[4];
  size_t team_addr[4];
  size_t bot_addr[4];
  size_t result_addr[4];
  size_t bonus_result_addr[4];
  size_t panel_color_addr[4];
  const dr_character *char_to_dr;
  unsigned char_to_dr_size;
  const dr_difficulty *diff_to_dr;
  unsigned diff_to_dr_size;
  size_t battle_addr;
  const dr_team_color *panel_color_to_dr;
  unsigned panel_color_to_dr_size;
  size_t minigame_type_addr;
  const dr_minigame_type *minigame_type_to_dr;
  unsigned minigame_type_to_dr_size;
};

class DrHost : public DrRetro
{
  Q_OBJECT

public:
  explicit DrHost(const DrHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest);

signals:
  void minigameRequested(
    dr_minigame_type type, std::array<dr_player_t, 4> players, std::array<bool, 4> playerValid);

private:
  void writeBattleCoins();

  DrHostConfig m_config;
  int m_writing = 0;
  int m_clearCountdown = 0;
  uint8_t m_lastScene = 0xFF;
  uint8_t m_lastBoardScene = 0;
  uint8_t m_resultsScene = 0;
  uint8_t m_lastMinigameType = 0xFF;
};

#endif
