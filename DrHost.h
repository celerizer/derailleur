#ifndef DR_HOST_H
#define DR_HOST_H

#include "DrRetro.h"
#include <array>
#include <string>

class DrGuest;

struct DrHostConfig
{
  std::string core;
  std::string game;
  uint8_t     scene_miniexplain;
  uint8_t     scene_miniresults;
  size_t      scene_addr;
  size_t      character_addr[4];
  size_t      controller_addr[4];
  size_t      difficulty_addr[4];
  size_t      team_addr[4];
  size_t      bot_addr[4];
  size_t      result_addr[4];
  size_t      panel_color_addr[4];
  const dr_character  *char_to_dr;
  unsigned             char_to_dr_size;
  const dr_difficulty *diff_to_dr;
  unsigned             diff_to_dr_size;
};

class DrHost : public DrRetro
{
  Q_OBJECT

public:
  explicit DrHost(const DrHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest);

signals:
  void minigameRequested(dr_minigame_type type,
                         std::array<dr_player_t, 4> players,
                         std::array<bool, 4>        playerValid);

private:
  DrHostConfig m_config;
  int          m_writing   = 0;
  uint8_t      m_lastScene = 0xFF;
};

#endif
