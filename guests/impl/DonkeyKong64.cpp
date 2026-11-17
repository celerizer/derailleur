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

static const dr_value_t DK64_ARCADE_LEVEL_ID = { 0x8004c723, DR_VALUE_TYPE_S8 };

static const dr_value_t DK64_LOAD_TO_MAP = { 0x807444E7, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_CURRENT_MAP = { 0x8076A0AB, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_MAP_STATE = { 0x8076A0B1, DR_VALUE_TYPE_U8 };

// u8 DONKEY KONG Scene, Lives, etc
static const dr_value_t DK64_ARCADE_SCENE = {0x8004C733, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_LIVES = {0x8004C71B, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_TOTAL_LIVES = {0x8004C71C, DR_VALUE_TYPE_U8};

// u8 Gameover toggle
static const dr_value_t DK64_ARCADE_GAMEOVER = { 0x8004A764, DR_VALUE_TYPE_U8 };

// Heap pointer for minigames
static const dr_value_t DK64_HEAP_PTR = {0x807FB970, DR_VALUE_TYPE_POINTER};

// u8 Result scores
static const dr_value_t DK64_SCORE[4] = {
  { 0x80744525, DR_VALUE_TYPE_U8 },
  { 0x80744527, DR_VALUE_TYPE_U8 },
  { 0x80744529, DR_VALUE_TYPE_U8 },
  { 0x8074452B, DR_VALUE_TYPE_U8 }
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

static const dr_mp_minigame_t DK64_MINIGAMES[] = {
  { "Mad Maze Maul", DR_MINIGAME_1P, 0x01, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Speedy Swing Sortie", DR_MINIGAME_1P, 0x02, 0x87, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Splish Slash Salvage", DR_MINIGAME_1P, 0x03, 0x84, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Minecart Mayhem", DR_MINIGAME_1P, 0x04, 0x81, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stealthy Snoop", DR_MINIGAME_1P, 0x05, 0x7F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Big Bug Bash", DR_MINIGAME_1P, 0x06, 0x95, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Teetering Turtle Trouble", DR_MINIGAME_1P, 0x07, 0x78, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kremling Kosh", DR_MINIGAME_1P, 0x08, 0x75, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Busy Barrel Barrage", DR_MINIGAME_1P, 0x09, 0x4F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beaver Bother", DR_MINIGAME_1P, 0x0A, 0x88, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Peril Path Panic", DR_MINIGAME_1P, 0x0B, 0x92, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Batty Barrel Bandit", DR_MINIGAME_1P, 0x0C, 0x7A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stash Snatch", DR_MINIGAME_1P, 0x0D, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Searchlight Seek", DR_MINIGAME_1P, 0x0E, 0x8B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Krazy Kong Klamour", DR_MINIGAME_1P, 0x0F, 0x65, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 25m", DR_MINIGAME_1P, 0x02, 0x04, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 50m", DR_MINIGAME_1P, 0x02, 0x05, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 75m", DR_MINIGAME_1P, 0x02, 0x06, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 100m", DR_MINIGAME_1P, 0x02, 0x07, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Jetpac", DR_MINIGAME_1P, 0x09, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Monkey Smash", DR_MINIGAME_4P, 0xBE, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kong Battle", DR_MINIGAME_4P, 0x6B, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Kong Battle: Duel", DR_MINIGAME_DUEL, 0x6B, 0xAA, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

// u16 Minigame States
static const dr_value_t DK64_STATES[] = {
  { 0x80449FB4, DR_VALUE_TYPE_U16 }, // Mad Maze Maul
  { 0x8046EE64, DR_VALUE_TYPE_U16 }, // Speedy Swing Sortie
  { 0x804AE794, DR_VALUE_TYPE_U16 }, // Splish Splash Salvage
  { 0x80494BA4, DR_VALUE_TYPE_U16 }, // Minecart Mayhem
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Stealthy Snoop
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Big Bug Bash
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Teetering Turtle Trouble
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Kremling Kosh
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Busy Barrel Barrage
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Beaver Bother
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Peril Path Panic
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Batty Barrel Bandit
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Stash Snatch
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Searchlight Seek 804B55B4
  { 0x804B55B4, DR_VALUE_TYPE_U16 }, // Krazy Kong Klamour
};



DonkeyKong64::DonkeyKong64(QObject *parent)
  : DrGuestN64(parent)
{
  m_retro->init(coreId(), rom());

  /* Reduces slowdowns */
  core()->options()->setOptionValue("mupen64plus-CountPerOp", "1");

  /* Disable the weird blurry textures on the characters that is dependant on how far the camera is away from the player */
  core()->options()->setOptionValue("mupen64plus-EnableLODEmulation", "False");
}

const dr_mp_minigame_t *DonkeyKong64::minigames(void) const
{
  return DK64_MINIGAMES;
}

void DonkeyKong64::onBeforeBoot(const DrGameData &data) {}

void DonkeyKong64::doApplyGameData(const DrGameData &data)
{
  m_State = -1;
  m_retro->loadState(state());

  m_retro->writeValue(data.minigame->scene_id, DK64_LOAD_TO_MAP);

  /* Happens only with multiplayer */
  if (data.minigame->scene_id == 0xAA)
  {
    /** Replace annoying SB with NOP instruction to prevent the game from overwriting the character ID set from P1 to other players */
    const dr_value_t DK64_INST_SB_CHARACTER = { 0x806C8560, DR_VALUE_TYPE_U32 };
    m_retro->writeValueForFrames(0, DK64_INST_SB_CHARACTER, 30);

    /** Needs to be set to two otherwise it won't add to the Player's score counter */
    const dr_value_t DK64_LOAD_RESULTS = { 0x80744514, DR_VALUE_TYPE_U8 };

    m_retro->writeValue(2, DK64_LOAD_RESULTS);
  }

  if (data.minigame->minigame_id == 0x02) {
    m_arcadelvl = data.minigame->scene_id;
  }

  /** Stealing this from Kirby 64  */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = m_players[i].control_port;

    uint32_t character = 0;
    uint32_t color = 0;

    if (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN)
    {
      m_playercount++;
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
  m_retro->writeValue(m_playercount, DK64_PLAYER_COUNT);


}

void DonkeyKong64::run(void)
{
  // m_State
  // -1 Minigame is not running
  // 0 Minigame is in explanation screen
  // 1 Minigame is running.

  int64_t mapstate, currentmap;

  m_retro->readValue(&currentmap, DK64_CURRENT_MAP);
  m_retro->readValue(&mapstate, DK64_MAP_STATE);

  /** If the map is loading start it right away */
  if (mapstate == 0x1C && !m_minigameActive) {
    log(DR_LOG_INFO, qPrintable(QString("[DK64] Loaded map: %1").arg(currentmap)));
    startMinigame();
    m_State = 0;
  }

  if (!m_minigameActive) {
    return;
  }

  switch (currentmap) {
  case 0x02:
    /// DONKEY KONG
    int64_t arcadescene, gameover, currentlevel;

    m_retro->readValue(&arcadescene, DK64_ARCADE_SCENE);
    m_retro->readValue(&gameover, DK64_ARCADE_GAMEOVER);
    m_retro->readValue(&currentlevel, DK64_ARCADE_LEVEL_ID);

    //** Donkey Kong Arcade and Jetpac require an "A" press before it can start applying data to it */
    if (arcadescene == 0x1B) { // Start of the game when Donkey Kong climbs up the ladder
      m_retro->writeValue(0, DK64_ARCADE_LIVES);
      m_retro->writeValue(1, DK64_ARCADE_TOTAL_LIVES);
      m_retro->writeValue(m_arcadelvl, DK64_ARCADE_LEVEL_ID);
    }

    if (finishPending()) {
      return;
    }

    if ((gameover == 0x01 || m_arcadelvl != currentlevel) && currentlevel > 3) {
      // Give the game over text a bit to breath on screen before ending it
      finishMinigameInFrames(60);
    }

    break;
  case 0x09:
    /// Jetpac
    break;
  default:
    /// Bonus Stages
    /** This one is a bit of a doozy, because of the pointer chain */

    /** There are moments where the value gets relocated to where it is by moving backwards 80 bytes.. annoyingly when the minigame is restarted */
    /** TODO: Disable the restart and exit level button from the pause menu, so it doesn't do this ^^ */

    int64_t ptr;
    m_retro->readValue(&ptr, DK64_HEAP_PTR);

    if (mapstate == 0x38 && m_State == 0) {
      // Go in a loop to try to find the minigame state
      for (unsigned i = 0; i <= 16; i++) {
        int64_t chain1, chain2, final;
        dr_value_t POINTER_CHAIN1 = {0x807FB930 + (8 * i), DR_VALUE_TYPE_POINTER};
        m_retro->readValue(&chain1, POINTER_CHAIN1);

        chain1 += 0x11C; // offset it to locate the pointer
        dr_value_t POINTER_CHAIN2 = {static_cast<size_t>(chain1), DR_VALUE_TYPE_POINTER};
        m_retro->readValue(&chain2, POINTER_CHAIN2);

        chain2 += 0x154; // offset it to find the state value of the minigame
        dr_value_t POINTER_CHAIN3 = {static_cast<size_t>(chain2), DR_VALUE_TYPE_U32};
        m_retro->readValue(&final, POINTER_CHAIN3);

        if (final == 16777216) {
          adrchain = static_cast<size_t>(chain2);
          m_State = 1;
          break;
        }
      }
    }

    if (m_State == 1) {
      const dr_value_t MINIGAME_STATE = {adrchain, DR_VALUE_TYPE_U16};
      m_retro->readValue(&m_minigameState, MINIGAME_STATE);
    }

    /** if the pointer doesn't exist anymore end the minigame.*/
    if (!ptr && m_State == 1 && mapstate == 0x0C) {
      finishMinigame();
    }

    break;
  }
}

  //if (m_winnerIndex < 0)
  //{

  //  int64_t wins[4] = {};
  //  for (unsigned i = 0; i < 4; i++)
  //    m_retro->readValue(&wins[i], DK64_SCORE[i]);

  //  /* Wins are indexed by in-game slot; map back to the board player index. */
  //  for (unsigned slot = 0; slot < 4; slot++)
  //  {
  //    if (wins[slot] == 1)
  //    {
  //      m_winnerIndex = m_slotToIndex[slot];
  //      break;
  //    }
  //  }

  //  if (m_winnerIndex >= 0)
  //    finishMinigameInFrames(150);
  //}


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
  /**
   * `index` is decided by order from the host context, which may not be the
   * same as controller order. See other games for examples on how to handle this.
   */
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
