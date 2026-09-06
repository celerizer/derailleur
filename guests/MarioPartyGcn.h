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

  dr_value_t scene;
  dr_value_t minigame;

  dr_value_t character[4];
  dr_value_t controller[4];
  dr_value_t difficulty[4];
  dr_value_t team[4];
  dr_value_t bot[4];

  /// The secondary coin amount gained from a mini-game
  dr_value_t bonus_result[4];

  /// The primary coin amount gained from a mini-game, usually 0 or 10
  dr_value_t result[4];

  /// Board coins and stars, copied in from the host so the mini-game shows what
  /// the board has. Read at the width each game declares. 0 address = don't write.
  dr_value_t coins[4];
  dr_value_t stars[4];

  /// dr_character -> the game's own character id, flagged with whether the game
  /// really has that character or is only lending a slot (see dr_character_id_t).
  dr_character_id_t (*char_from_dr)(dr_character character);

  /// How many characters the game has, numbered from 0. Bounds the search for a
  /// free slot when a stand-in's preferred character is already taken.
  unsigned roster_size;
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
  void doApplyGameData(const DrGameData &data) override;


  const MpGcnConfig &config() const { return m_config; }

protected:
  void run() override;
  void applyPlayers();
  DrRetro *m_retro = nullptr;
  MpGcnConfig m_config;
  int32_t m_lastScene = -1;
  int m_minigameFrames = 0;

  int m_slotOf[4] = { 0, 1, 2, 3 }; // board player index -> game slot
};

#endif
