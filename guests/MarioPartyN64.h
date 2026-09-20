#ifndef DR_GUEST_MARIO_PARTY_N64_H
#define DR_GUEST_MARIO_PARTY_N64_H

#include "../DrGuest.h"
#include <string>

/// One conditional patch, applied every frame while a game walks its own boot into
/// a mini-game instead of loading a savestate: while `when` reads `equals`, `value`
/// is written to `dest`. This is the shape of a GameShark D1/8x pair, but run by us
/// -- the core's cheat engine applies nothing but boot codes for its first 60 VIs,
/// which is the entire window these patches have to act in.
struct MpN64BootPatch
{
  dr_value_t when;
  int64_t equals;
  dr_value_t dest;
  int64_t value;
};

struct MpN64Config
{
  std::string core;
  std::string game;
  std::string state;

  /// Where a character-injected copy of the ROM is built, when the Waluigi/Daisy
  /// option is on (see dr_settings::character_injection). Empty = never rebuilt.
  std::string temp_rom;

  /// Where a boot_patches game snapshots itself once its first boot has settled,
  /// so later launches can load that instead of booting again. Rewritten each
  /// session and removed on shutdown. Empty = snapshot nothing.
  std::string temp_state;

  int scene_miniexplain[2]; // two because MP2 has two overlays for this
  int scene_miniresults;

  dr_value_t scene;
  dr_value_t minigame;

  /// Conditional writes that walk the boot into the mini-game explanation, in
  /// place of a savestate. A zero `when` address ends the list; an empty list
  /// means this game loads a savestate instead. Sized for the boot-logo timing
  /// immediates, which are a patch apiece.
  MpN64BootPatch boot_patches[32];

  /// The game's RNG state, reseeded as each mini-game starts. 0 = don't touch it.
  dr_value_t rng;

  dr_value_t controller[4];
  dr_value_t difficulty[4];
  dr_value_t team[4];
  dr_value_t bot[4];
  dr_value_t character[4];
  dr_value_t bonus_result[4];
  dr_value_t result[4];

  /// Board coins and stars, copied in from the host so the mini-game shows what
  /// the board has. Read at the width each game declares (stars are a byte in
  /// some, a halfword in others; coins are signed). 0 address = don't write.
  dr_value_t coins[4];
  dr_value_t stars[4];

  /// The battle pot global, copied in from the host so the battle results pay out
  /// what the board actually collected. 0 address = the game has no battles (MP1).
  dr_value_t battle_pot;

  /// Debug-menu launch mode: 0 GAME (hands the id to the explanation overlay),
  /// 1 BOARD (calls the row's overlay directly), 2 QUEST. Duels exist only in
  /// board context and are unreachable from the GAME branch, so they alone are
  /// launched in BOARD mode. 0 address = leave the mode alone.
  dr_value_t debug_mode;

  /// An injected character's model names its deformation clusters after itself
  /// ("c301l_0_DEF" for Daisy), but the engine looks those nodes up by name from
  /// a hardcoded table keyed on the slot the model is loaded into ("c002l_0_DEF"
  /// for Peach's slot), and a miss leaves the mini-game hung on entry.
  ///
  /// Rather than rewrite every injected mesh, point the table at the names the
  /// mesh actually carries. Two entries per character, "cNNNn_0_DEF" and
  /// "cNNNl_0_DEF", each 12 bytes and descending as the character id rises, so
  /// entry(slot) = base - 12 * slot. Writing the first four bytes replaces
  /// "cNNN" and leaves the family letter and "_0_DEF" tail alone.
  /// 0 base = this game does not need the fixup.
  size_t cluster_name_n;   ///< address of character 0's "cNNNn_0_DEF"
  size_t cluster_name_l;   ///< address of character 0's "cNNNl_0_DEF"
  int cluster_name_stride; ///< bytes between entries (negative: they descend)

  /// "c" plus the three digits an injected character's clusters are named with,
  /// packed big-endian ('c','3','0','1'). 0 = not injectable in this game.
  uint32_t cluster_prefix_waluigi;
  uint32_t cluster_prefix_daisy;

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
  std::string gamePath() const override { return m_romPath.empty() ? m_config.game : m_romPath; }
  std::string corePath() const override { return m_config.core; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

protected:
  /// Rearranges m_slotOf before the players are written, for a game whose
  /// mini-games expect their participants in particular slots. The default
  /// leaves every player in their own.
  virtual void remapSlots(void) {}

  void run() override;
  /// Reseeds the game's RNG from dr_rand, so a mini-game replayed from the same
  /// savestate doesn't play out identically. No-op without a configured address.
  void seedRng();
  /// True if the hidden character is playable in `minigame` (see MpN64Config::hidden).
  bool hiddenCharacterPlayable(const dr_mp_minigame_t *minigame) const;
  /// Stamps the hidden character back into every slot in m_hiddenSlots.
  void writeHiddenCharacters(void);
  /// True when this game reaches its mini-game by being walked through its own
  /// boot rather than by loading a savestate (see MpN64Config::boot_patches).
  /// Follows the Waluigi/Daisy option: the rebuilt ROM and the boot path go
  /// together, and with the option off the shipped savestate is used instead.
  bool bootsWithoutState(void) const
  {
    return dr_settings_get().character_injection && m_config.boot_patches[0].when.address != 0;
  }
  /// Builds m_config.temp_rom with Waluigi and/or Daisy in the slots the players
  /// who picked them resolve to, and points gamePath() at it. Runs once.
  void buildCharacterRom(const DrGameData &data);
  void onBeforeBoot(const DrGameData &data) override;
  /// Runs boot_patches once, every frame until the mini-game starts.
  void applyBootPatches(void);
  /// Writes the players into the game. Split out of doApplyGameData so a reset can
  /// defer it until after the reboot it schedules. `settled` means memory is done
  /// moving (a state load), so the writes go in once instead of being held.
  void applyPlayers(const DrGameData &data, bool settled);
  /// Snapshots the booted game to m_config.temp_state.
  void captureBootState(void);
  void doApplyGameData(const DrGameData &data) override;
  DrRetro *m_retro = nullptr;
  MpN64Config m_config;
  int16_t m_lastScene = -1;
  /// Last non-zero value seen in each result field while the mini-game ran.
  /// ovl_7C cashes coins_mg in and clears it, and it does that in the same
  /// emulated frame the scene id changes -- so reading the field once the
  /// transition is visible is always too late. Sampling every frame and keeping
  /// the last non-zero reading catches the value while it is still live, and a
  /// player who genuinely scored nothing stays at zero.
  int64_t m_resultSeen[4] = { 0, 0, 0, 0 };

  int m_minigameFrames = 0;
  /// Bitmask of the slots playing as the hidden character this mini-game.
  uint8_t m_hiddenSlots = 0;

  /// Set once the first launch has applied. Only relevant to the no-savestate
  /// path, where the initial cold boot reaches the mini-game on its own and only
  /// a relaunch has to be sent back around with a reset.
  bool m_booted = false;

  /// Players waiting on a reset's reboot, and the frames left before they are
  /// stamped. Zero when nothing is pending.
  DrGameData m_pendingPlayers;
  int m_applyCountdown = 0;

  /// Frames left before the first boot is snapshotted. Zero when not waiting.
  int m_bootStateCountdown = 0;

  /// Whether this session has written m_config.temp_state. Never set from a file
  /// found on disk, so a snapshot from an earlier run is never loaded.
  bool m_bootStateSaved = false;

  /// The rebuilt ROM to boot instead of m_config.game, and whether the build has
  /// been attempted. Empty path = boot the stock ROM.
  std::string m_romPath;
  bool m_romBuilt = false;

  /// Board player index -> game slot, identity unless remapSlots() says otherwise.
  int m_slotOf[4] = { 0, 1, 2, 3 };
};

#endif
