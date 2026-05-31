#ifndef DR_HOST_MARIO_PARTY_2_HOST_H
#define DR_HOST_MARIO_PARTY_2_HOST_H

#include "../DrHost.h"
#include <array>
#include <string>

class MarioParty2Host : public DrHost
{
  Q_OBJECT

public:
  explicit MarioParty2Host(QObject *parent = nullptr);
  void injectMinigameTitles(const std::array<DrMinigameCandidate, 5> &candidates) override;
  bool onMiniexplainDetected(dr_minigame_type type, int16_t minigameId,
    const std::array<dr_player_t, 4> &players, const std::array<bool, 4> &playerValid) override;
  void writeMinigameNames();

private:
  std::array<std::string, 5> m_candidateNames;
};

#endif
