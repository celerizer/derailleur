#ifndef DR_GUEST_MARIO_PARTY_N64_H
#define DR_GUEST_MARIO_PARTY_N64_H

#include "../DrGuest.h"
#include <string>

struct MpN64Config
{
  std::string core;
  std::string game;
  std::string state;

  int scene_miniexplain[2]; // two because MP2 has two overlays for this
  int scene_miniresults;

  size_t scene_addr;
  size_t minigame_addr;

  /// The game's RNG state, reseeded as each mini-game starts. 0 = don't touch it.
  size_t rng_addr;

  size_t controller_addr[4];
  size_t difficulty_addr[4];
  size_t team_addr[4];
  size_t bot_addr[4];
  size_t character_addr[4];
  size_t bonus_result_addr[4];
  size_t result_addr[4];

  /// Board coins and stars, copied in from the host so the mini-game shows what
  /// the board has. Read at the width each game declares (stars are a byte in
  /// some, a halfword in others; coins are signed). 0 address = don't write.
  dr_value_t coins[4];
  dr_value_t stars[4];

  /// The battle pot global, copied in from the host so the battle results pay out
  /// what the board actually collected. 0 address = the game has no battles (MP1).
  dr_value_t battle_pot;

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
  std::string corePath() const override { return m_config.core; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  /// Reseeds the game's RNG from dr_rand, so a mini-game replayed from the same
  /// savestate doesn't play out identically. No-op without a configured address.
  void seedRng();
  void doApplyGameData(const DrGameData &data) override;
  DrRetro *m_retro = nullptr;
  MpN64Config m_config;
  int16_t m_lastScene = -1;
  int m_minigameFrames = 0;
};

#endif
