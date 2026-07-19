#ifndef DR_GUEST_MARIO_PARTY_N64_H
#define DR_GUEST_MARIO_PARTY_N64_H

#include "../DrGuest.h"
#include <string>

struct MpN64Config
{
  std::string core;
  std::string game;
  std::string state;

  int scene_miniexplain;
  int scene_miniresults;

  size_t scene_addr;
  size_t minigame_addr;

  size_t controller_addr[4];
  size_t difficulty_addr[4];
  size_t team_addr[4];
  size_t bot_addr[4];
  size_t character_addr[4];
  size_t bonus_result_addr[4];
  size_t result_addr[4];

  const uint8_t *character_ids;
  const dr_mp_minigame_t *minigames;
};

class MarioPartyN64 : public DrGuest
{
  Q_OBJECT

public:
  MarioPartyN64(const MpN64Config &config, QObject *parent = nullptr);
  ~MarioPartyN64() override;

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  bool usesWarmup() const override { return false; }
  std::string gamePath() const override { return m_config.game; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  DrRetro *m_retro = nullptr;
  MpN64Config m_config;
  int16_t m_lastScene = -1;
  int m_minigameFrames = 0;
};

#endif
