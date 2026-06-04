#ifndef DR_HOST_H
#define DR_HOST_H

#include "DrGuest.h"
#include "DrRetro.h"
#include <array>
#include <string>

struct DrMinigameCandidate
{
  DrGuest *guest;
  const dr_mp_minigame_t *minigame;
};
Q_DECLARE_METATYPE(DrMinigameCandidate)
Q_DECLARE_METATYPE(dr_minigame_type)
using DrPlayerArray = std::array<dr_player_t, 4>;
using DrPlayerValidArray = std::array<bool, 4>;
Q_DECLARE_METATYPE(DrPlayerArray)
Q_DECLARE_METATYPE(DrPlayerValidArray)

struct dr_scene_range_t
{
  uint8_t min;
  uint8_t max;
};

typedef enum
{
  DR_HOST_STATE_INVALID = 0,

  /// Waiting for the current scene to be a valid board scene.
  DR_HOST_STATE_BEFORE_BOARD,

  /// The host game is currently on the board or results.
  /// The emulator will monitor the mini-game type to see when the roulette is about to start.
  DR_HOST_STATE_BOARD,

  /// The mini-game type has been chosen, the roulette is about to open.
  /// The emulator will roll mini-game candidates and inject them into memory here.
  DR_HOST_STATE_BEFORE_ROULETTE,

  /// The roulette is choosing one of the mini-games.
  /// The emulator will wait for a mini-game to be chosen.
  DR_HOST_STATE_ROULETTE,

  DR_HOST_STATE_AFTER_ROULETTE,

  DR_HOST_STATE_MINIGAME,

  DR_HOST_STATE_SIZE
} dr_host_state;

struct DrHostConfig
{
  std::string core;
  std::string game;
  uint8_t scene_miniexplain[4];
  unsigned scene_miniexplain_count;
  uint8_t scene_miniresults;
  uint8_t scene_miniresults_battle;
  uint8_t scene_miniresults_duel;
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
  size_t next_scene_addr;
  size_t next_scene_modifier_addr;
  size_t minigame_id_addr;
  bool minigame_id_is_8bit;
  uint8_t minigame_blacklist[16];
  unsigned minigame_blacklist_count;

  const size_t *title_addrs;              // 6 addrs (5 slots + sentinel), nullptr = no injection
  uint8_t title_id_base;                  // first roulette ID (e.g. 0x25)
  uint8_t title_id_step;                  // step between IDs (e.g. 2); 0 = use slot_addrs instead
  uint8_t title_len_offset;              // added to nameLen when writing the length byte
  size_t (*title_addr_transform)(size_t); // byte-swap fn (e.g. n64ByteAddr), nullptr = identity
  const size_t *slot_addrs;              // 5 word-flipped RAM addrs holding per-slot minigame IDs
  size_t scene_trampoline_addr;          // packed word: upper half = scene, lower half = modifier; 0 = passthrough
};

class DrHost : public DrRetro
{
  Q_OBJECT

public:
  explicit DrHost(const DrHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest);
  void setCandidates(std::array<DrMinigameCandidate, 5> candidates);
  void startMinigame(unsigned index);

  void run(void);

  virtual void injectMinigameTitles(const std::array<DrMinigameCandidate, 5> &candidates);

  // Called when miniexplain is detected. Return true to suppress candidatesRequested.
  virtual bool onMiniexplainDetected(dr_minigame_type type, int16_t minigameId,
    const DrPlayerArray &players, const DrPlayerValidArray &playerValid)
  {
    if (m_config.title_addrs && m_config.title_id_step > 0)
    {
      for (unsigned k = 0; k < 5; k++)
        if (minigameId == (int16_t)(m_config.title_id_base + k * m_config.title_id_step))
        {
          if (m_config.next_scene_addr)
            m_pendingStartIndex = k;
          else
            startMinigame(k);
          return true;
        }
      return false;
    }
    if (m_config.title_addrs && m_config.slot_addrs)
    {
      for (unsigned k = 0; k < 5; k++)
      {
        uint8_t slotId = 0;
        readu8(&slotId, m_config.slot_addrs[k]);
        if (minigameId == (int16_t)slotId)
        {
          if (m_config.next_scene_addr)
            m_pendingStartIndex = k;
          else
            startMinigame(k);
          return true;
        }
      }
      return false;
    }
    if (m_config.minigame_type_addr)
    {
      if (m_config.next_scene_addr)
        m_pendingStartIndex = 0;
      else
        startMinigame(0);
      return true;
    }
    return false;
  }

signals:
  void candidatesNeeded(dr_minigame_type type);
  void candidatesRequested(
    dr_minigame_type type, DrPlayerArray players, DrPlayerValidArray playerValid);
  void minigameRequested(
    DrMinigameCandidate candidate, DrPlayerArray players, DrPlayerValidArray playerValid);

protected:
  bool initTitleSlots();
  void writeMinigameNames(const std::array<std::string, 5> &names);
  size_t m_titleSlotAddrs[5] = {};
  bool m_titleSlotsValid = false;

private:
  void readPlayers(dr_minigame_type type);
  void writeBattleCoins();

  DrHostConfig m_config;
  int m_writing = 0;
  uint8_t m_lastScene = 0xFF;
  int16_t m_lastMinigameId = -1;
  uint8_t m_lastBoardScene = 0;
  uint8_t m_resultsScene = 0;
  int16_t m_resultsModifier = 0;
  signed m_MinigameType = -1;
  signed m_SceneId = 0;
  uint8_t m_lastNextScene = 0xFF;
  dr_minigame_type m_pendingMgType = DR_MINIGAME_INVALID;
  uint8_t m_pendingStartIndex = 0;

  dr_host_state m_State = DR_HOST_STATE_INVALID;

  std::array<DrMinigameCandidate, 5> m_candidates = {};
  std::array<dr_player_t, 4> m_pendingPlayers = {};
  std::array<bool, 4> m_pendingPlayerValid = {};
};

#endif
