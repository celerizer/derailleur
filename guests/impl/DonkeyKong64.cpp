#include "DonkeyKong64.h"
#include "DrRandom.h"
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

#define DK64_ARENA_1                 0x6D
#define DK64_ARENA_2                 0xBE
#define DK64_ARENA_3                 0xC0

#define DK64_MONKEYSMASH_SURVIVAL          0x00
#define DK64_MONKEYSMASH_COIN_HOARD        0x01
#define DK64_MONKEYSMASH_WINS               0x02
#define DK64_MONKEYSMASH_TIME               0x03
#define DK64_MONKEYSMASH_CAPTURE            0x04
#define DK64_MONKEYSMASH_CAPTURE_PAD        0x05


static const dr_value_t DK64_ARCADE_LEVEL_ID = { 0x8004c723, DR_VALUE_TYPE_S8 };

static const dr_value_t DK64_LOAD_TO_MAP = { 0x807444E7, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_CURRENT_MAP = { 0x8076A0AB, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_MAP_STATE = { 0x8076A0B1, DR_VALUE_TYPE_U8 };

// u8 DONKEY KONG Scene, Lives, etc
static const dr_value_t DK64_ARCADE_SCENE = {0x8004C733, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_LIVES = {0x8004C71B, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_TOTAL_LIVES = {0x8004C71C, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_GAMEOVER = { 0x8004A764, DR_VALUE_TYPE_U8 };

// Heap pointer for minigames
static const dr_value_t DK64_HEAP_PTR = {0x807FB970, DR_VALUE_TYPE_POINTER};

// u8 Kong Battle
static const dr_value_t DK64_KONGBATTLE_GAMEMODE = {0x80750AB8, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_KONGBATTLE_GAMETYPE = {0x8076A0D7, DR_VALUE_TYPE_U8};

// u8 Player count
static const dr_value_t DK64_PLAYER_COUNT = { 0x80750AC0, DR_VALUE_TYPE_U8 };

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
  { "DONKEY KONG: 25m", DR_MINIGAME_1P, 0x10, 0x04, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 50m", DR_MINIGAME_1P, 0x11, 0x05, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 75m", DR_MINIGAME_1P, 0x12, 0x06, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 100m", DR_MINIGAME_1P, 0x13, 0x07, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Jetpac", DR_MINIGAME_1P, 0x14, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Monkey Smash", DR_MINIGAME_4P, 0x15, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Battle Arena", DR_MINIGAME_4P, 0x16, 0x6B, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Battle Arena: Duel", DR_MINIGAME_DUEL, 0x17, 0x6B, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
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

bool DonkeyKong64::getMinigameState() {

  if (!m_minigameState && !m_stateAddress)
    return true;

  for (unsigned i = 0; i <= 24; i++) {
    int64_t chain1, chain1val, chain2, value, value2;
    /* Try to find the value that is stored somewhere within the second pointer offset by 154 bytes.. */

    // Go through the address table
    dr_value_t POINTER_CHAIN1 = {0x807FB930 + (8 * i), DR_VALUE_TYPE_POINTER};
    m_retro->readValue(&chain1, POINTER_CHAIN1);
    m_retro->readValue(&chain1val, POINTER_CHAIN1);

    chain1 += 0x11C;
    dr_value_t POINTER_CHAIN2 = {static_cast<size_t>(chain1), DR_VALUE_TYPE_POINTER};
    m_retro->readValue(&chain2, POINTER_CHAIN2);

    chain2 += 0x154;
    dr_value_t POINTER_VALUE1 = {static_cast<size_t>(chain2), DR_VALUE_TYPE_U32};
    m_retro->readValue(&value, POINTER_VALUE1);

    if (value == 0x2010000) {
      m_stateAddress = static_cast<size_t>(chain2);
      return true;
    }

    /*  Double check scan
     * Try to find the value if it's stored somewhere else offset from the first pointer. */

    chain1val += 0x154;
    dr_value_t POINTER_VALUE2 = {static_cast<size_t>(chain1val), DR_VALUE_TYPE_U32};
    m_retro->readValue(&value2, POINTER_VALUE2);

    if (value2 == 0x2010000) {
      m_stateAddress = static_cast<size_t>(chain1val);
      return true;
    }
  }

   return false;
}

unsigned DonkeyKong64::computeWinners() {

  unsigned winners = 0;
  int64_t score[4] = {};
  if (m_minigame->minigame_id > 0x14) {
    int16_t best = score[0];
    for (unsigned i = 0; i < 4; i++) {
      m_retro->readValue(&score[i], DK64_SCORE[i]);

      if (score[i] < best)
        best = score[i];
    }

    for (unsigned slot = 0; slot < 4; slot++)
      if (score[slot] == best)
        winners |= (1u << m_slotToIndex[slot]);

  } else {
    // Singleplayer
  }

  return winners;
}

void DonkeyKong64::doApplyGameData(const DrGameData &data)
{
  uint32_t playercount = 0;

  m_State = -1;
  m_retro->loadState(state());

  if (data.minigame->minigame_id == 0x15) {
    const uint8_t randArena = (DrRandom::instance().next() % 3);

    switch (randArena) {
    case 0:
      m_kongBattleArena = DK64_ARENA_1;
      break;
    case 1:
      m_kongBattleArena = DK64_ARENA_2;
      break;
    case 2:
      m_kongBattleArena = DK64_ARENA_3;
      break;
    }

    m_retro->writeValue(m_kongBattleArena, DK64_LOAD_TO_MAP);
  } else {
    m_retro->writeValue(data.minigame->scene_id, DK64_LOAD_TO_MAP);
  }


  /** Stealing this from Kirby 64  */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = m_players[i].control_port;

    uint32_t character = 0;
    uint32_t color = 0;

    if (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN)
    {
      playercount++;
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

  if (data.minigame->minigame_id > 0x14)
  {
    /** Replace annoying SB with NOP instruction to prevent the game from overwriting the character ID set from P1 to other players */
    const dr_value_t DK64_INST_SB_CHARACTER = { 0x806C8560, DR_VALUE_TYPE_U32 };
    m_retro->writeValueForFrames(0, DK64_INST_SB_CHARACTER, 30);

    /** Needs to be set to two otherwise it won't add to the Player's score counter */
    const dr_value_t DK64_LOAD_RESULTS = { 0x80744514, DR_VALUE_TYPE_U8 };
    m_retro->writeValue(2, DK64_LOAD_RESULTS);

    m_retro->writeValue(playercount, DK64_PLAYER_COUNT);

    if (data.minigame->scene_id == 0x6B) {
      m_retro->writeValue(1, DK64_KONGBATTLE_GAMEMODE);
    }
  }
}

void DonkeyKong64::run(void)
{

  int64_t mapstate, currentmap;

  m_retro->readValue(&currentmap, DK64_CURRENT_MAP);
  m_retro->readValue(&mapstate, DK64_MAP_STATE);

  /** If the map is loading start it right away */
  if (mapstate == 0x1C && !m_minigameActive) {
    log(DR_LOG_INFO, qPrintable(QString("[DK64] Loaded map: %1").arg(currentmap)));
    startMinigame();
    m_State = 0;
  } else if (mapstate == 0x08 && !m_minigameActive && currentmap == m_kongBattleArena) {
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
      m_retro->writeValue(m_minigame->scene_id, DK64_ARCADE_LEVEL_ID);
    }

    if (finishPending()) {
      return;
    }

    if ((gameover == 0x01 || m_minigame->scene_id != currentlevel) && currentlevel > 3) {
      // Give the game over text a bit to breath on screen before ending it
      finishMinigameInFrames(60);
    }

    break;
  case 0x09:
    /// Jetpac
    break;
  case 0xBE:
    if (mapstate == 0x08 && m_State == 0) {
      m_State = 1;
    }

    if (mapstate == 0x0D && m_State == 1) {
      computeWinners();
      finishMinigame();
    }

    break;
  case 0x6B:
    if (mapstate == 0x08 && m_State == 0) {
      m_State = 1;
    }

    if (mapstate == 0x80 && m_State == 1) {
      computeWinners();
      finishMinigameInFrames(60);
    }

    break;
  default:
    /// Bonus Stages
    // This one is a bit of a doozy, because of the pointer chain..

    /* TODO: Disable the restart and exit level button from the pause menu */

    int64_t ptr;
    m_retro->readValue(&ptr, DK64_HEAP_PTR);

    if (mapstate == 0x28 && m_State == 0 && getMinigameState()) {
      m_State++;
    }

    if (m_State == 1) {
      const dr_value_t MINIGAME_STATE = {m_stateAddress, DR_VALUE_TYPE_U16};
      m_retro->readValue(&m_minigameState, MINIGAME_STATE);
    }

    if (m_State == 1 && mapstate == 0x38) {
      computeWinners();
      m_State++;
    }

    /** If the pointer doesn't exist anymore, end the minigame. */
    if (!ptr && mapstate == 0x0C) {
      finishMinigame();
    }

    break;
  }
}


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
