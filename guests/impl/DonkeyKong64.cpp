#include "DonkeyKong64.h"
#include <QRetro.h>

typedef enum
{
  DK64_CHARACTER_DONKEY_KONG = 0,
  DK64_CHARACTER_DIDDY_KONG = 1,
  DK64_CHARACTER_LANKY_KONG = 2,
  DK64_CHARACTER_TINY_KONG = 3,
  DK64_CHARACTER_CHUNKY_KONG = 4,
  DK64_CHARACTER_KRUSHA = 5
} dk64_character_ids;

// s8 (multiplayer) arena ID
typedef enum
{
  DK64_ARENA_ONE = 0x00,
  DK64_ARENA_TWO = 0x01,
  DK64_ARENA_THREE = 0x02,
} dk64_arena_ids;

// s8 monkey smash game type
typedef enum
{
  DK64_MONKEYSMASH_SURVIVAL = 0x00,
  DK64_MONKEYSMASH_COIN_HOARD = 0x01,
  DK64_MONKEYSMASH_WINS = 0x02,
  DK64_MONKEYSMASH_TIME = 0x03,
  DK64_MONKEYSMASH_CAPTURE = 0x04,
  DK64_MONKEYSMASH_CAPTURE_PAD = 0x05,
} dk64_monkey_smash_game_type;

// u8 wins for each player on the results screen
static const dr_value_t DK64_KONGBATTLE_RESULTS[4] = {
  { 0x80744524, DR_VALUE_TYPE_U8 },
  { 0x80744526, DR_VALUE_TYPE_U8 },
  { 0x8074452A, DR_VALUE_TYPE_U8 },
  { 0x80744528, DR_VALUE_TYPE_U8 },
};

static const dr_value_t DK64_ARCADE_LIVES = { 0x8004c718, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_ARCADE_LEVEL_ID = { 0x8004c721, DR_VALUE_TYPE_S8 };

// u8 Result scores
static const dr_value_t DK64_SCORE[4] = {
  { 0x8012345c, DR_VALUE_TYPE_U8 },
  { 0x8012346c, DR_VALUE_TYPE_U8 },
  { 0x8012347c, DR_VALUE_TYPE_U8 },
  { 0x8012348c, DR_VALUE_TYPE_U8 }
};

// u8 Character IDs
static const dr_value_t DK64_CHARACTER[4] = {
  { 0x8074E77C, DR_VALUE_TYPE_U8 },
  { 0x8074E77D, DR_VALUE_TYPE_U8 },
  { 0x8074E77F, DR_VALUE_TYPE_U8 },
  { 0x8074E780, DR_VALUE_TYPE_U8 }
};

// u8 Color IDs
static const dr_value_t DK64_COLOR[4] = {
  { 0x807552F7, DR_VALUE_TYPE_U8 },
  { 0x807552FB, DR_VALUE_TYPE_U8 },
  { 0x807552FF, DR_VALUE_TYPE_U8 },
  { 0x80755303, DR_VALUE_TYPE_U8 }
};

static const dr_value_t DK64_LOAD_TO_MAP = { 0x807444E7, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_MAP_STATE = { 0x8076A0B1, DR_VALUE_TYPE_U8 };

// u8 Game State
static const dr_value_t DK64_GAME_STATE = { 0x80755318, DR_VALUE_TYPE_U8 };

// u8 Multiplayer results screen
static const dr_value_t DK64_MULTI_MINIGAME_HAS_ENDED = { 0x804B210D, DR_VALUE_TYPE_U8 };

static const dr_mp_minigame_t DK64_MINIGAMES[] = {
  { "Mad Maze Maul", DR_MINIGAME_1P, 0x45, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Speedy Swing Sortie", DR_MINIGAME_1P, 0x87, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Splish Slash Salvage", DR_MINIGAME_1P, 0x84, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Minecart Mayhem", DR_MINIGAME_1P, 0x81, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stealthy Snoop", DR_MINIGAME_1P, 0x7F, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Big Bug Bash", DR_MINIGAME_1P, 0x95, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Teetering Turtle Trouble", DR_MINIGAME_1P, 0x78, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kremling Kosh", DR_MINIGAME_1P, 0x75, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Busy Barrel Barrage", DR_MINIGAME_1P, 0x4F, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beaver Bother", DR_MINIGAME_1P, 0x88, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Peril Path Panic", DR_MINIGAME_1P, 0x92, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Batty Barrel Bandit", DR_MINIGAME_1P, 0x7A, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stash Snatch", DR_MINIGAME_1P, 0x43, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Searchlight Seek", DR_MINIGAME_1P, 0x8B, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Krazy Kong Klamour", DR_MINIGAME_1P, 0x65, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 25m", DR_MINIGAME_1P, 0x02, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 50m", DR_MINIGAME_1P, 0x02, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 75m", DR_MINIGAME_1P, 0x02, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Test Map", DR_MINIGAME_1P, 0x00, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Monkey Smash", DR_MINIGAME_4P, 0xBE, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kong Battle", DR_MINIGAME_4P, 0x6B, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Kong Battle: Duel", DR_MINIGAME_DUEL, 0x6B, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/**
 * ============================================================================
 * Then, the constructor...
 * ============================================================================
 */

DonkeyKong64::DonkeyKong64(QObject *parent)
  : DrGuestN64(parent)
{
  m_retro->init(coreId(), rom());

  /* Reduces slowdowns */
  core()->options()->setOptionValue("mupen64plus-CountPerOp", "1");

  /* Disable the weird blurry textures on the characters that is dependant on how far the camera is away from the player */
  core()->options()->setOptionValue("mupen64plus-EnableLODEmulation", "False");
}

/**
 * ============================================================================
 * Then, the typical functions guest games implement...
 * ============================================================================
 */

/**
 * Function that returns the mini-game list for this guest game.
 * Usually all we need to do is return our array above, but we can also generate one programmatically here
 */
const dr_mp_minigame_t *DonkeyKong64::minigames(void) const
{
  return DK64_MINIGAMES;
}

/**
 * Any code that gets run before the emulator core loads
 * For example, moving save files around, putting custom assets in place, etc.
 */
void DonkeyKong64::onBeforeBoot(const DrGameData &data) {}

void DonkeyKong64::doApplyGameData(const DrGameData &data)
{
  m_retro->loadState(state());

  startMinigame();

  uint32_t player_count = 0;

  m_retro->writeValueForFrames(data.minigame->minigame_id, DK64_LOAD_TO_MAP, 20);

  /* Happens only with multiplayer */
  if (data.minigame->scene_id == 0xAA)
  {
    /** Replace annoying SB with NOP instruction to prevent the game from overwriting the character ID set from P1 to other players */
    const dr_value_t DK64_INST_SB_CHARACTER = { 0x806C8560, DR_VALUE_TYPE_U32 };
    m_retro->writeValueForFrames(0, DK64_INST_SB_CHARACTER, 30);

    /** Controls if the game should load results or not */
    const dr_value_t DK64_LOAD_RESULTS = { 0x80744514, DR_VALUE_TYPE_U8 };

    m_retro->writeValue(2, DK64_LOAD_RESULTS);
  }

  /** Stealing this from Kirby 64  */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = m_players[i].control_port;

    uint32_t character = 0;
    uint32_t color = 0;

    if (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN)
    {
      player_count++;
    }

    switch (m_players[i].character)
    {
    case DR_CHARACTER_MARIO:
      character = DK64_CHARACTER_LANKY_KONG;
      color = 2;
      break;
    case DR_CHARACTER_LUIGI:
      character = DK64_CHARACTER_DIDDY_KONG;
      color = 3;
      break;
    case DR_CHARACTER_PEACH:
      character = DK64_CHARACTER_TINY_KONG;
      color = 1;
      break;
    case DR_CHARACTER_YOSHI:
      character = DK64_CHARACTER_LANKY_KONG;
      color = 1;
      break;
    case DR_CHARACTER_WARIO:
      character = DK64_CHARACTER_CHUNKY_KONG;
      color = 1;
      break;
    case DR_CHARACTER_DONKEY_KONG:
      character = DK64_CHARACTER_DONKEY_KONG;
      color = 0;
      break;
    case DR_CHARACTER_WALUIGI:
      character = DK64_CHARACTER_CHUNKY_KONG;
      color = 3;
      break;
    case DR_CHARACTER_DAISY:
      character = DK64_CHARACTER_TINY_KONG;
      color = 2;
      break;
    default:
      character = DK64_CHARACTER_KRUSHA;
      color = 0;
      break;
    }
    m_retro->writeValue(character, DK64_CHARACTER[slot]);
    m_retro->writeValue(color, DK64_COLOR[slot]);
  }

  /** Should be self-explanatory. */
  const dr_value_t DK64_PLAYER_COUNT = { 0x80750AC0, DR_VALUE_TYPE_U8 };
  m_retro->writeValue(player_count, DK64_PLAYER_COUNT);
}

void DonkeyKong64::run(void)
{

  /** Some of this stuff was written before I (Cray) started using save states */
  //int64_t maploaded;

  //m_retro->readValue(&maploaded, DK64_MAP_STATE);

  //// Prevent it from booting to a minigame when the game is still at the boot logo
  //if (maploaded >= 0x10 && !m_minigameActive)
  //{
  //  startMinigame();
  //}
}

//log(DR_LOG_INFO, "[DK64] Started minigame");

///* When you've determined the mini-game has ended, call `finishMinigame` */
//if (results_screen_is_shown)
//  finishMinigame();

/**
 * Finally, any code for returning the minigame results after the mini-game has finished
 * 
 * `DR_MINIGAME_4P`:
 * `DR_MINIGAME_1V3`:
 * `DR_MINIGAME_2V2`:
 * `DR_MINIGAME_1P`:
 *   result + bonus will be awarded as coins.
 * 
 * `DR_MINIGAME_BATTLE`:
 *   result will be the placement of each player, with:
 *   0=1st, 1=2nd, 2=3rd, 3=4th
 * 
 * `DR_MINIGAME_DUEL`:
 *   result will contain a higher number for the player that won:
 *   10=winner, 0=loser
 * 
 * `DR_MINIGAME_BOWSER`:
 *   result will contain whether the player survived the mini-game:
 *   1=winner, 0=loser
 * 
 * `DR_MINIGAME_DK`:
 *   result will contain 
 * @see dr_minigame_type
 */
dr_minigame_result_t DonkeyKong64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t val;

  /**
   * `index` is decided by order from the host context, which may not be the
   * same as controller order. See other games for examples on how to handle this.
   */
  switch (index)
  {
  case 0:
    m_retro->readValue(&val, DK64_SCORE[0]);
    break;
  case 1:
    m_retro->readValue(&val, DK64_SCORE[1]);
    break;
  case 2:
    m_retro->readValue(&val, DK64_SCORE[2]);
    break;
  case 3:
    m_retro->readValue(&val, DK64_SCORE[3]);
    break;
  }
  result.coins = val;

  return result;
}
