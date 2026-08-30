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

  /// dr_character -> the game's own character id, flagged with whether the game
  /// really has that character or is only lending a slot (see dr_character_id_t).
  dr_character_id_t (*char_from_dr)(dr_character character);

  /// How many characters the game has, numbered from 0. Bounds the search for a
  /// free slot when a stand-in's preferred character is already taken.
  unsigned roster_size;

  /// A handful of mini-games leave one more character playable than the board
  /// ever offers, under a native id the character select never writes: Toad in
  /// MP1's Slot Car Derby, Koopa Kid in MP2's Shell Shocked. A player who came
  /// in as that character on the host gets the native id here instead of their
  /// usual stand-in. DR_CHARACTER_INVALID (0) = this game has no such mini-game.
  struct
  {
    dr_character character;
    uint8_t native_id;

    /// The mini-game ids the character is playable in, -1 terminated.
    signed minigame_ids[8];
  } hidden;

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
  /// True if the hidden character is playable in `minigame` (see MpN64Config::hidden).
  bool hiddenCharacterPlayable(const dr_mp_minigame_t *minigame) const;
  /// Stamps the hidden character back into every slot in m_hiddenSlots.
  void writeHiddenCharacters(void);
  void doApplyGameData(const DrGameData &data) override;
  DrRetro *m_retro = nullptr;
  MpN64Config m_config;
  int16_t m_lastScene = -1;
  int m_minigameFrames = 0;
  /// Bitmask of the slots playing as the hidden character this mini-game.
  uint8_t m_hiddenSlots = 0;
};

#endif
