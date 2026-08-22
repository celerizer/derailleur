#ifndef DR_MARIO_PARTY_GCN_HOST_H
#define DR_MARIO_PARTY_GCN_HOST_H

#include "../DrHost.h"
#include <string>
#include <vector>

typedef enum
{
  DR_GCN_HOST_STATE_INVALID = 0,
  DR_GCN_HOST_STATE_BEFORE_BOARD,
  DR_GCN_HOST_STATE_BOARD,
  DR_GCN_HOST_STATE_BEFORE_ROULETTE,
  DR_GCN_HOST_STATE_ROULETTE,
  DR_GCN_HOST_STATE_AFTER_ROULETTE,
  DR_GCN_HOST_STATE_MINIGAME,

  DR_GCN_HOST_STATE_SIZE
} dr_gcn_host_state;

struct DrGcnHostConfig
{
  std::string core;
  std::string game;

  struct
  {
    /// Code cave: a raw byte blob stamped word-by-word into `cave_addr`
    /// (cave_size bytes) periodically. nullptr = no cave.
    const uint8_t *cave;

    /// The address in which to stamp the code cave
    size_t cave_addr;

    /// The size in bytes of the cave data
    unsigned cave_size;

    /// A list of Gecko codes for hooking functions into our assembly
    const char *cheat_board;
  } cheats;

  struct
  {
    dr_value_t scene;                 // current scene id
    dr_value_t character[4];          // per-slot character id
    dr_value_t controller[4];         // per-slot controller port
    dr_value_t difficulty[4];         // per-slot cpu difficulty
    dr_value_t team[4];               // per-slot team id
    dr_value_t bot[4];                // per-slot cpu flag
    dr_value_t result[4];             // per-slot finishing place
    dr_value_t bonus_result[4];       // per-slot bonus coins
    dr_value_t panel_color[4];        // per-slot panel color
    dr_value_t coins[4];              // per-slot current coins
    dr_value_t stars[4];              // per-slot current stars
    dr_value_t mg_star[4];            // per-slot mini-game star
    dr_value_t minigame_title_color;  // color array the title trampoline copies into
    dr_value_t battle;                // total battle coins
    dr_value_t minigame_type;         // mini-game type byte
    dr_value_t minigame_id;           // chosen mini-game id (width per game)
    dr_value_t title_block;           // base of the injected glyph block
    dr_value_t title_color;           // base of the injected color table
    dr_value_t title_type_duel;       // duel-overlay type byte
    dr_value_t scene_stack;           // base of the native scene stack
    dr_value_t scene_stack_count;     // scene-stack element count
    dr_value_t turn_total;            // total turn count
    dr_value_t turn_current;          // current turn count

    /// The value that tracks whose turn it is on the board
    dr_value_t turn_owner;

    /// The value that tracks which space the current player is standing on
    dr_value_t space_index;
  } values;

  int scene_miniexplain;  // scene id shown while a mini-game is explained
  int scene_miniresults;  // scene id shown on the mini-game results screen

  /// dr_character -> native character id table (DR_CHARACTER_SIZE entries),
  /// read in reverse to resolve a board slot's character
  const uint16_t *character_ids;

  /// Native roulette type byte -> dr_minigame_type (e.g. 0=4P, 1=1v3, 2=2v2)
  const dr_minigame_type *minigame_type_to_dr;
  unsigned minigame_type_to_dr_size;

  size_t host_state_addr;

  /// Save files to ship to netplay clients (see DrHost::saveFilePatterns). A GCN
  /// game's save is a memory-card file in Dolphin's GCI folder, named after the
  /// game code rather than the ROM, so each game names its own. Empty falls back
  /// to the ROM-named default.
  std::vector<std::string> save_files;
};

class MarioPartyGcnHost : public DrHost
{
  Q_OBJECT

public:
  explicit MarioPartyGcnHost(const DrGcnHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;
  QStringList saveFilePatterns(void) const override;

  void run(void);

private:
  void stampCave(void);

  /// Reroll the shared mini-game pool (kept lockstepped across netplay peers).
  void rollMinigames(void);

  /// Redraw MP4's two battle mini-game candidate icons as their names, for
  /// Dolphin's custom texture loader to pick up.
  void stampBattleIcons(void);

  /// Cache `type`'s five candidates and stamp their names into the title block.
  void stampTitles(dr_minigame_type type);

  /// Resolve the roulette's chosen id to a cached candidate and launch it.
  void startMinigame(void);

  /// Fill `players` from the four board slots for the active mini-game type.
  void readPlayers(DrPlayerArray &players);

  int32_t m_PreviousScene = -1;
  dr_minigame_type m_MinigameType = DR_MINIGAME_INVALID;
  std::array<DrMinigameCandidate, 5> m_Candidates = {};

protected:
  DrGcnHostConfig m_config;
  dr_gcn_host_state m_State = DR_GCN_HOST_STATE_INVALID;
};

#endif
