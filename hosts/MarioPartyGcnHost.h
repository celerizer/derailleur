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

    /// Board hooks stamped every frame: { address, value, width } rows ending in
    /// an all-zero row (e.g. MP4_HOOK_BOARD). nullptr = no hooks.
    const unsigned int (*hooks)[3];
  } cheats;

  /// Core options forced for this game, applied after the core loads and before
  /// content does, so they beat whatever the user's Dolphin config carries.
  /// Terminated by a row with a null key. nullptr = leave the core alone.
  const dr_core_option_t *options;

  /// This board builds separate mic and non-mic roulette lists and says which one
  /// it is filling through dr_host_state_t::mic, so candidates are rolled to match
  /// (MP6 and MP7). Boards without that split take mic mini-games like any other
  /// and flag them on the roulette instead.
  bool mic_lists;

  /// The battle roulette picks from pictures, so they are redrawn as the names of
  /// the candidates on offer. Dolphin loads the replacements out of `dir` under
  /// the save directory. A null `dir` means this game doesn't do that.
  struct
  {
    /// Texture directory, relative to the save directory (e.g. Dolphin's GMPE01).
    const char *dir;

    /// One file per icon the roulette shows, in the order the candidates fill
    /// them. Terminated by a null entry.
    const char *const *files;

    /// The texture's real size; it is drawn at 2x so the text stays sharp.
    int width;
    int height;

    /// Draw onto white rather than transparency, inverting the text and its
    /// outline to suit.
    bool white_background;
  } battle_icons;

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
    dr_value_t battle_ante[4];        // per-slot contribution, summed into the pot
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

    /// The game's RNG state
    dr_value_t rng;
  } values;

  int scene_miniexplain;  // scene id shown while a mini-game is explained
  int scene_miniresults;  // scene id shown on the mini-game results screen

  /// Scene id -> descriptive name, terminated by a scene_id of -1. Used to log
  /// scene changes and to spot ids the table doesn't cover yet. nullptr = no table.
  const dr_scene_name_t *scene_names;

  /// Native character id -> dr_character, DR_CHARACTER_INVALID for ids the game
  /// doesn't use. nullptr = no mapping.
  dr_character (*char_to_dr)(unsigned chr);

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

  /// MP4 keeps no pot global -- TakeCoins() only ever sums it into a local -- so
  /// rebuild it from what each player actually handed over.
  unsigned battlePot(void) override
  {
    unsigned pot = 0;

    for (unsigned i = 0; i < 4; i++)
    {
      int64_t ante = 0;

      if (m_config.values.battle_ante[i].address &&
          readValue(&ante, m_config.values.battle_ante[i]) == DR_OK)
        pot += static_cast<unsigned>(ante);
    }

    return pot;
  }

  uint32_t rngValue(void) override
  {
    int64_t value = 0;

    if (!m_config.values.rng.address || readValue(&value, m_config.values.rng) != DR_OK)
      return 0;

    return static_cast<uint32_t>(value);
  }

  bool readPlayerSetup(DrPlayerArray &players) override;
  bool writePlayerSetup(const DrPlayerArray &players) override;

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;
  QStringList saveFilePatterns(void) const override;

  dr_host_platform platform(void) const override { return DR_HOST_PLATFORM_GCWII; }

  void run(void);

private:
  void stampCave(void);

  /// Redraw MP4's two battle mini-game candidate icons as their names, for
  /// Dolphin's custom texture loader to pick up.
  void stampBattleIcons(void);

  /// Cache `type`'s five candidates and stamp their names into the title block.
  void stampTitles(dr_minigame_type type);

  /// Which candidate set this board wants, from the list the cave says it is
  /// building. Always DR_MIC_ANY unless the board splits its lists.
  dr_mic_mode micMode(void);

  /// Resolve the roulette's chosen id to a cached candidate and launch it.
  void startMinigame(void);

  /// Fill `players` from the four board slots for the active mini-game type.
  void readPlayers(DrPlayerArray &players);

  int32_t m_PreviousScene = -1;
  std::array<DrMinigameCandidate, 5> m_Candidates = {};

  /// For MP8, frames counted between "after roulette" and "mini-game" state
  unsigned m_AfterRouletteTimer = 0;

protected:
  /// Last chance to change what the guest reported for slot `index` before it is
  /// written to the board. Returns it untouched unless a game overrides this.
  virtual dr_minigame_result_t adjustResult(unsigned index, const dr_minigame_result_t &result)
  {
    (void)index;

    return result;
  }

  /// The type the roulette named for the mini-game being played, kept until the
  /// next roulette so writeResults can still tell what was launched.
  dr_mic_mode m_MicMode = DR_MIC_ANY; // list the roulette asked for
  dr_minigame_type m_MinigameType = DR_MINIGAME_INVALID;

  /// Reroll the shared mini-game pool (kept lockstepped across netplay peers).
  void rollMinigames(void);

  /// Stamps one { address, value, width } hook table, up to its all-zero row.
  /// Does nothing when `hooks` is null.
  void applyHooks(const unsigned int (*hooks)[3]);

  /// Per-frame patching a single game needs on top of its board hooks, called
  /// once the frame's scene id is known. MP8 uses it to install the roulette
  /// hook belonging to whichever board overlay is loaded.
  virtual void applyGameHooks(int32_t scene) { (void)scene; }

  DrGcnHostConfig m_config;
  dr_gcn_host_state m_State = DR_GCN_HOST_STATE_INVALID;
};

#endif
