#ifndef DR_MARIO_PARTY_N64_HOST_H
#define DR_MARIO_PARTY_N64_HOST_H

#include "DrHost.h"
#include <array>
#include <string>

/// One entry in a host's scene-name table. Tables are terminated by an entry
/// whose scene_id is -1 (scene_id is signed so the sentinel can't collide with
/// any real 0x00-0xFF scene).
struct dr_scene_name_t
{
  int scene_id;
  const char *name;
};

/// One element of the game's native scene stack (see DrHostConfig::scene_stack_addr).
struct mp64_overlay_t
{
  int32_t id;
  int16_t event;
  int16_t stat;
};

/// Returns the descriptive name for `scene_id` by scanning `scenes` until a
/// match or the -1 terminator, or nullptr if there is no table or no match.
static inline const char *dr_scene_name(const dr_scene_name_t *scenes, int scene_id)
{
  if (!scenes)
    return nullptr;
  for (; scenes->scene_id != -1; scenes++)
    if (scenes->scene_id == scene_id)
      return scenes->name;
  return nullptr;
}

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
  uint8_t scene_board_ids[16];
  unsigned scene_board_id_count;
  uint8_t scene_duel_board_ids[16];
  unsigned scene_duel_board_id_count;
  size_t character_addr[4];
  size_t controller_addr[4];
  size_t difficulty_addr[4];
  size_t team_addr[4];
  size_t bot_addr[4];
  size_t result_addr[4];
  size_t bonus_result_addr[4];
  size_t panel_color_addr[4];
  size_t coins_addr[4]; /* u16 current coins; 0 = not available */
  size_t stars_addr[4]; /* u8 current stars; 0 = not available */
  size_t mg_star_addr[4]; /* s16 mini-game star; 0 = not available */
  /* MP3 bandaid: MP3 sometimes fails to add mini-game winnings to the mini-game
   * star total on a normal (non-duel, non-battle) mini-game. When set, the host
   * checks a second after the results and adds the coins itself if it didn't. */
  bool fixup_mg_star;
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
  size_t minigame_id_addr;
  bool minigame_id_is_8bit;
  uint8_t minigame_blacklist[16];
  unsigned minigame_blacklist_count;

  /* Roulette-title injection. A cheat (cheat_title_hook) installs a trampoline that
   * redirects the game's title loader to read NUL-terminated glyph strings from a
   * flat block at title_block_addr, laid out type-major as [type][5 slots][32 bytes]:
   *   title_block_addr + (rawType * 5 + slot) * 32
   * The trampoline reads the game's own minigame-type byte, so nothing has to react
   * to the roll -- the host stamps every type's row up front on entering the board
   * (see rollAndStampTitles). 0 = no injection. */
  size_t title_block_addr;
  size_t title_type_addr_duel;            // duel-overlay type byte, stamped with duel candidates; 0 = none
  const char *cheat_title_hook;           // GS: trampoline + jal hook for the board title loader; nullptr = none
  const char *cheat_title_hook_duel;      // GS: same for a duel-mode overlay (MP3 name_81); nullptr = none
  /* GS codes that force the roulette to land on the slot index, so the chosen minigame
   * id read back is exactly the slot (0-4). Toggled by board/duel like the hooks above;
   * cheat_force_id_duel = nullptr for a game without a separate duel overlay. */
  const char *cheat_force_id;
  const char *cheat_force_id_duel;
  /* The game's native scene stack: a LIFO of 5 8-byte elements { s32 scene, s16 event,
   * s16 stat } at scene_stack_addr, with an s16 count at scene_stack_count_addr. The
   * game pops the top (index count-1) each transition; when the count hits 0 it infers
   * the next scene itself. We push a single results element to redirect after a
   * mini-game. Both 0 = host does not drive scenes (see setSceneQueue). */
  size_t scene_stack_addr;
  size_t scene_stack_count_addr;
  int16_t scene_stat_board;             // element `stat` for a board scene
  int16_t scene_stat_minigame;          // element `stat` for a mini-game / results scene
  int16_t scene_stat_duel;              // element `stat` for a duel-results scene
  size_t turn_total_addr;               // byte: total turn count; 0 = skip end-of-game check
  size_t turn_current_addr;             // byte: current turn count
  /* Inclusive scene-id range for item mini-games (played natively, one participant).
   * On entry the host grants netplay golf mode to that player; 0/0 = none. */
  uint8_t scene_item_first;
  uint8_t scene_item_last;
  uint8_t scene_board_results;          // scene forced when the game is over; 0 = passthrough
  uint8_t scene_last_five_turns;        // scene forced entering the last 5 turns; 0 = passthrough
  size_t scene_duel_slot0_addr;         // word-flipped RAM addr of duel board's first slot; 0 = use minigame_type_addr
  const char *cheat_regular_board;      // cheat code string for regular board roulette (nullptr = unused)
  const char *cheat_duel_board;         // cheat code string for duel board roulette (nullptr = unused)
  const dr_scene_name_t *scene_names;   // scene id -> name table (-1 terminated); nullptr = none
};

/// Mario Party 1-3 (Nintendo 64) host. Drives the board by pushing onto the game's
/// native scene stack, watches memory for the roulette, and injects mini-game titles.
/// Concrete games (MarioParty1/2/3Host) just supply a DrHostConfig and game().
class MarioPartyN64Host : public DrHost
{
  Q_OBJECT

public:
  explicit MarioPartyN64Host(const DrHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest) override;
  void clearResults() override;
  void setCurrentTurn(unsigned turn) override;
  void startMinigame(unsigned index);

  void run(void);

private:
  void readPlayers(dr_minigame_type type);
  /// Copies the five cached candidates for `type` into m_candidates (for launching the
  /// chosen guest). Does not reroll -- the pool is rolled by rollAndStampTitles.
  void rollCandidates(dr_minigame_type type);
  /// Rerolls the shared pool and stamps every mini-game type's five names into its row
  /// of the title block, so the roulette can read any row without reacting to the roll.
  void rollAndStampTitles(void);
  /// Encodes and writes one 5-slot title row (block index `row`) into the block.
  void stampTitleRow(unsigned row, const std::array<DrMinigameCandidate, 5> &candidates);
  void writeBattleCoins();
  /// Writes all 5 scene-stack elements from `overlays` and sets the element count.
  /// No-op when the stack isn't configured.
  void setSceneQueue(const mp64_overlay_t overlays[5], int overlay_count);

  DrHostConfig m_config;
  int m_writing = 0;
  uint8_t m_lastScene = 0xFF;
  int16_t m_lastMinigameId = -1;
  uint8_t m_lastBoardScene = 0;
  uint8_t m_resultsScene = 0;
  int16_t m_resultsModifier = 0;
  signed m_MinigameType = -1;
  bool m_isDuelBoard = false;
  uint8_t m_pendingStartIndex = 0;
  int m_startDelay = 0; // frames to spin after queuing the results scene before launching
  bool m_itemPending = false;
  bool m_itemSceneLeft = false;
  uint8_t m_itemChosenId = 0;

  /* MP3 mini-game star bandaid (see fixup_mg_star). Armed at writeResults; the
   * countdown runs down in run(), then adds coins for any player whose star total
   * did not move. */
  int m_mgStarFixupCountdown = 0;
  int16_t m_mgStarPrev[4] = {};
  int16_t m_mgStarAdd[4] = {};

  dr_host_state m_State = DR_HOST_STATE_INVALID;

  std::array<DrMinigameCandidate, 5> m_candidates = {};
  std::array<dr_player_t, 4> m_pendingPlayers = {};
};

#endif
