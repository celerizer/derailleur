#ifndef DR_MARIO_PARTY_N64_HOST_H
#define DR_MARIO_PARTY_N64_HOST_H

#include "../DrHost.h"
#include <array>
#include <string>

/// One element of the game's native scene stack (see DrHostConfig::scene_stack_addr).
typedef struct
{
  int32_t id;
  int16_t event;
  int16_t stat;
} dr_mp64_overlay_t;

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

  /// Native character id -> dr_character, DR_CHARACTER_INVALID for ids the game
  /// doesn't use. nullptr = no mapping.
  dr_character (*char_to_dr)(unsigned chr);

  const dr_difficulty *diff_to_dr;
  unsigned diff_to_dr_size;
  
  const dr_minigame_type *minigame_type_to_dr;
  unsigned minigame_type_to_dr_size;

  struct
  {
    /// The trampoline/code "cave": a raw byte blob stamped word-by-word into
    /// `cave_addr` (cave_size bytes) when the board hooks are enabled, rather
    /// than applied as a GameShark code. nullptr = no cave.
    const uint8_t *cave;
    size_t cave_addr;
    unsigned cave_size;

    /// A libretro-formatted GameShark code for forcing the mini-game IDs in
    /// the roulette, for any board in MP1/2, or Battle Royale boards in MP3
    const char *cheat_board;

    /// A libretro-formatted GameShark code for forcing the mini-game IDs in
    /// the roulette, for duel boards in MP3 only
    const char *cheat_duel;
  } cheats;

  struct
  {
    /// Scenes for only one player where we might want to enable golf mode on
    /// netplay. Terminated by -1.
    int single_player_ids[16] = { -1 };

    int minigame_explain[4] = { -1 };

    int boards[16] = { -1 };

    int boards_duel[16] = { -1 };

    /// The overlay ID for the main menu; abandons all game state when entered
    int main_menu;

    /// The overlay ID for the final board results screen
    int board_results;

    /// The overlay ID for the "Last 5 Turns" event
    int last_five_turns;

    int minigame_results;
    int minigame_results_battle;
    int minigame_results_duel;
  } scenes;

  struct
  {
    int16_t board;
    int16_t minigame;
    int16_t duel;
  } stat;

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
    dr_value_t battle_pot;            // coins collected into the battle pot
    dr_value_t minigame_type;         // mini-game type byte
    dr_value_t minigame_id;           // chosen mini-game id (width per game)
    dr_value_t title_block;           // base of the injected glyph block
    dr_value_t title_color;           // base of the injected color table
    dr_value_t title_type_duel;       // duel-overlay type byte
    dr_value_t scene_stack;           // base of the native scene stack (see setSceneQueue)
    dr_value_t scene_stack_count;     // scene-stack element count
    dr_value_t turn_total;            // total turn count
    dr_value_t turn_current;          // current turn count

    /// The value that tracks whose turn it is on the board
    dr_value_t turn_owner;

    /// The value that tracks which space the current player is standing on
    dr_value_t space_index;

    /// The game's RNG state
    dr_value_t rng;
  } values;

  size_t host_state_addr;

  /// An array of all overlay IDs and their associated friendly names
  const dr_scene_name_t *scene_names;
};

class MarioPartyN64Host : public DrHost
{
  Q_OBJECT

public:
  explicit MarioPartyN64Host(const DrHostConfig &config, QObject *parent = nullptr);

  /// The pot is a single global here, holding what the battle actually collected
  /// (a player short of the ante contributes only what they have).
  unsigned battlePot(void) override
  {
    int64_t value = 0;

    if (!m_config.values.battle_pot.address ||
        readValue(&value, m_config.values.battle_pot) != DR_OK)
      return 0;

    return static_cast<unsigned>(value);
  }

  uint32_t rngValue(void) override
  {
    int64_t value = 0;

    if (!m_config.values.rng.address || readValue(&value, m_config.values.rng) != DR_OK)
      return 0;

    return static_cast<uint32_t>(value);
  }

  void writeResults(DrGuest *guest) override;
  void clearResults() override;
  void setCurrentTurn(unsigned turn) override;
  bool readPlayerSetup(DrPlayerArray &players) override;
  bool writePlayerSetup(const DrPlayerArray &players) override;
  void startMinigame(unsigned index);

  dr_host_platform platform(void) const override { return DR_HOST_PLATFORM_N64; }

  void run(void);

private:
  /// Wipes all per-game runtime state and returns to INVALID, so the host re-arms
  /// from scratch. Triggered on returning to the main menu (see scenes.main_menu).
  void resetRuntimeState();

  /// Address of the active mini-game type byte: the duel-overlay byte on a duel
  /// board, otherwise the game's normal type byte.
  const dr_value_t &mgTypeValue() const
  {
    return (m_isDuelBoard && m_config.values.title_type_duel.address)
      ? m_config.values.title_type_duel : m_config.values.minigame_type;
  }
  /// Reads a roulette-flow guard value (turn owner / space index) per its dr_value_t type.
  int64_t readBoardGuard(const dr_value_t &value);
  /// Snapshots the guard values as the roulette begins (see turn_owner).
  void captureBoardGuard();
  /// True if a watched guard value moved since captureBoardGuard().
  bool boardGuardTripped();
  void readPlayers(dr_minigame_type type);
  /// Copies the five cached candidates for `type` into m_candidates (for launching the
  /// chosen guest). Does not reroll -- the pool is rolled by rollAndStampTitles.
  void rollCandidates(dr_minigame_type type);
  /// Rerolls the shared pool and stamps every mini-game type's five names into its row
  /// of the title block, so the roulette can read any row without reacting to the roll.
  void rollAndStampTitles(void);
  /// Encodes and writes one 5-slot title row (block index `row`) into the block.
  void stampTitleRow(unsigned row, const std::array<DrMinigameCandidate, 5> &candidates);
  /// Writes one type row's 8 title colors (5 candidate colors + padding) into the color
  /// table at title_color_addr + row*8, for the trampoline to copy by type.
  void stampTitleColors(unsigned row, const std::array<DrMinigameCandidate, 5> &candidates);
  void writeBattleCoins();
  /// Writes all 5 scene-stack elements from `overlays` and sets the element count.
  /// No-op when the stack isn't configured.
  void setSceneQueue(const dr_mp64_overlay_t overlays[5], int overlay_count);
  /// Finds the first live scene-stack element whose id matches one of the -1-terminated
  /// `match_ids`, overwrites it with `overlay`, and writes the stack back. Returns false
  /// (leaving the stack untouched) when the stack is empty or no element matches.
  bool replaceSceneOverlay(const int *match_ids, const dr_mp64_overlay_t &overlay);

  DrHostConfig m_config;
  int m_writing = 0;
  uint8_t m_lastScene = 0xFF;
  int16_t m_lastMinigameId = -1;
  uint8_t m_lastBoardScene = 0;
  uint8_t m_resultsScene = 0;
  int16_t m_resultsModifier = 0;
  signed m_MinigameType = -1;
  bool m_isDuelBoard = false;
  uint8_t m_lastDuelType = 0xFF; // previous duel type byte; roulette gates on (non-0)->0
  int m_sceneChangeGrace = 0;    // frames to ignore the mg-type byte after a scene change
  int64_t m_guardTurn = 0;       // snapshot of turn_owner while the roulette runs
  int64_t m_guardSpace = 0;      // snapshot of space_index while the roulette runs
  uint8_t m_pendingStartIndex = 0;
  int m_startDelay = 0; // frames to spin after queuing the results scene before launching
  bool m_itemPending = false;
  bool m_itemSceneLeft = false;
  uint8_t m_itemChosenId = 0;
  bool m_hostGolfMode = false; // host granted golf mode for the running mini-game (1P)

  dr_host_state m_State = DR_HOST_STATE_INVALID;

  std::array<DrMinigameCandidate, 5> m_candidates = {};
  std::array<dr_player_t, 4> m_pendingPlayers = {};
};

#endif
