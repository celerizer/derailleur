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

#define DK64_KONGBATTLE_TIME               0x00
#define DK64_KONGBATTLE_WINS               0x01
#define DK64_KONGBATTLE_COIN_HOARD        0x02
#define DK64_KONGBATTLE_CAPTURE            0x03
#define DK64_KONGBATTLE_SURVIVAL           0x04
#define DK64_KONGBATTLE_CAPTURE_PAD        0x05

#define DK64_ARENA_1                 0x6D
#define DK64_ARENA_2                 0xBE
#define DK64_ARENA_3                 0xC0

static const dr_value_t DK64_LOAD_TO_MAP = { 0x807444E7, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_CURRENT_MAP = { 0x8076A0AB, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_MAP_STATE = { 0x8076A0B1, DR_VALUE_TYPE_U8 };

// u8 DONKEY KONG Scene, Lives, etc
static const dr_value_t DK64_ARCADE_LEVEL_ID = { 0x8004c723, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_ARCADE_SCENE = {0x8004C733, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_LIVES = {0x8004C71B, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_ARCADE_TOTAL_LIVES = {0x8004C71C, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_ARCADE_GAMEOVER = { 0x8004A764, DR_VALUE_TYPE_U8 };

// u8 Jetpac
static const dr_value_t DK64_JETPAC_LIVES = { 0x8002EC57, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_JETPAC_LEVEL = { 0x8002EC4F, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_JETPAC_STATE = { 0x8002F3BF, DR_VALUE_TYPE_U8 };

// u8 Kong Battle
static const dr_value_t DK64_KONGBATTLE_GAMEMODE = {0x80750AB8, DR_VALUE_TYPE_U8};
static const dr_value_t DK64_KONGBATTLE_GAMETYPE = {0x807552EB, DR_VALUE_TYPE_U8};

// u8 Wins, Timer, Lives
static const dr_value_t DK64_KONGBATTLE_ATTRIBUTE = {0x807552F3, DR_VALUE_TYPE_U8};

// u8 Player counts
static const dr_value_t DK64_PLAYER_COUNT = { 0x80750AC0, DR_VALUE_TYPE_U8 };
static const dr_value_t DK64_PLAYER_COUNT_UI = { 0x807552E7, DR_VALUE_TYPE_U8 };


// Heap pointer for minigames
static const dr_value_t DK64_HEAP_PTR[2] = {
  { 0x807FB970, DR_VALUE_TYPE_POINTER },
  { 0x807550C0, DR_VALUE_TYPE_POINTER },
  };

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
  { 0x8074E77E, DR_VALUE_TYPE_U8 },
  { 0x8074E77F, DR_VALUE_TYPE_U8 }
};

// u8 Color IDs
static const dr_value_t DK64_COLOR[4] = {
  { 0x807552F7, DR_VALUE_TYPE_U8 },
  { 0x807552FB, DR_VALUE_TYPE_U8 },
  { 0x807552FF, DR_VALUE_TYPE_U8 },
  { 0x80755303, DR_VALUE_TYPE_U8 }
};

// u8 Swimming moves
const dr_value_t DK64_SWIMMING_MOVE[3] = {
  {0x807ED084, DR_VALUE_TYPE_U8},
  {0x807ED230, DR_VALUE_TYPE_U8},
  {0x807ED260, DR_VALUE_TYPE_U8},
};

static const dr_mp_minigame_t DK64_MINIGAMES[] = {
  { "Mad Maze Maul", DR_MINIGAME_1P, 0x01, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Speedy Swing Sortie", DR_MINIGAME_1P, 0x02, 0x87, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Splish-Splash Salvage", DR_MINIGAME_1P, 0x03, 0x60, DR_NO_QUIRKS, DR_NO_FLAGS },
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
  { "DONKEY KONG: 25m", DR_MINIGAME_1P, 0x10, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 50m", DR_MINIGAME_1P, 0x11, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 75m", DR_MINIGAME_1P, 0x12, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG: 100m", DR_MINIGAME_1P, 0x13, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Jetpac", DR_MINIGAME_1P, 0x14, 0x09, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Monkey Smash: Time", DR_MINIGAME_4P, 0x15, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Battle Arena: Time", DR_MINIGAME_4P, 0x16, 0x6B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Monkey Smash: Survival", DR_MINIGAME_DUEL, 0x15, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Battle Arena: Survival", DR_MINIGAME_DUEL, 0x16, 0x6B, DR_NO_QUIRKS, DR_NO_FLAGS },

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

void DonkeyKong64::onBeforeBoot(const DrGameData &data) {
  m_guestState = -1;
  m_winnerIndex = -1;
  m_minigameState = 0;

  core()->input()->clearPortRoutes();
}

unsigned DonkeyKong64::computeWinners() {

  unsigned winners = 0;
  int64_t score[4] = {};

  if (m_minigame->minigame_id > 0x14) {
    for (unsigned i = 0; i < 4; i++) {
      m_retro->readValue(&score[i], DK64_SCORE[i]);

      if (score[i] == 1) {
        m_winnerIndex = m_slotToIndex[i];
        winners = score[i] << 3 | 2; // 10
        return winners;
      }
    }
  }

  if (m_minigameState != 1) {
    return 0;
  }

  for (unsigned i = 0; i < 4; i++) {
    if (m_players[i].team_type == DR_TEAM_TYPE_SOLO) {
      m_winnerIndex = m_players[i].control_port;
      winners = 10;
    }
  }

  return winners;
}

void DonkeyKong64::setupMultiplayer(const int8_t playerCount) {

  m_retro->writeValue(playerCount, DK64_PLAYER_COUNT);

  m_retro->writeValue(DK64_KONGBATTLE_TIME, DK64_KONGBATTLE_GAMETYPE);
  m_retro->writeValue(2, DK64_KONGBATTLE_ATTRIBUTE); // Default set to time for FFA

  /* If it's a duel, then set it to survival */
  if(m_minigame->type == DR_MINIGAME_DUEL) {
    m_retro->writeValue(DK64_KONGBATTLE_SURVIVAL, DK64_KONGBATTLE_GAMETYPE);
    m_retro->writeValue(1, DK64_KONGBATTLE_ATTRIBUTE); // Lives
  }
  uint8_t arena = DK64_ARENA_1;

  if (m_minigame->scene_id == 0xFF) {

    /* Replace BEQZ (Branch Equal to ZERO) with NOP (No Operation). so it makes it write the values for the guns to be usable. */
    const dr_value_t DK64_INST_BEQZ_CHARACTER = { 0x806C9B60, DR_VALUE_TYPE_U32 };
    m_retro->writeValueForFrames(0, DK64_INST_BEQZ_CHARACTER, 70);

    m_retro->writeValue(playerCount, DK64_PLAYER_COUNT_UI);

    /* Pull out a random number between 0 to 2 and then pick the arena from the switch case */
    const uint8_t randnum = (DrRandom::instance().next() % 3);

    switch (randnum) {
    case 0:
      arena = DK64_ARENA_1;
      break;
    case 1:
      arena = DK64_ARENA_2;
      break;
    case 2:
      arena = DK64_ARENA_3;
      break;
    }

  } else {
    arena = m_minigame->scene_id;
    // Set to zero. Battle Arena only works in one screen
    m_retro->writeValue(0, DK64_PLAYER_COUNT_UI);
    m_retro->writeValue(1, DK64_KONGBATTLE_GAMEMODE);
  }

  m_retro->writeValue(arena, DK64_LOAD_TO_MAP);

  return;
}

void DonkeyKong64::doApplyGameData(const DrGameData &data) {
  uint32_t playercount = 0;

  m_retro->loadState(state());

  /* Replace SB (Store Byte) with NOP to prevent the game from overwriting the character ID set from P1 to other players */
  const dr_value_t DK64_INST_SB_CHARACTER = { 0x806C8560, DR_VALUE_TYPE_U32 };
  m_retro->writeValueForFrames(0, DK64_INST_SB_CHARACTER, 70);

  unsigned slot = 0;

  for (unsigned i = 0; i < 4; i++)
  {
    if (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN && m_players[i].team_type != DR_TEAM_TYPE_NONPARTICIPANT)
      playercount++;

    uint32_t character = 0;
    uint32_t color = 0;

    switch (m_players[i].character)
    {
    case DR_CHARACTER_MARIO:
      character = DK64_CHARACTER_DIDDY_KONG;
      color = 0;
      break;
    case DR_CHARACTER_LUIGI:
      character = DK64_CHARACTER_DIDDY_KONG;
      color = 3;
      break;
    case DR_CHARACTER_PEACH:
      character = DK64_CHARACTER_TINY_KONG;
      color = 0;
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
      character = DK64_CHARACTER_LANKY_KONG;
      color = 2;
      break;
    case DR_CHARACTER_DAISY:
      character = DK64_CHARACTER_DONKEY_KONG;
      color = 3;
      break;
    default:
      character = DK64_CHARACTER_KRUSHA;
      color = 0;
      break;
    }

    if (!dr_team_type_participates(m_players[i].team_type))
      continue;

    m_slotToIndex[slot] = static_cast<int>(i);

    /* Route the ports to their respective character from the board context's turn order, this also works for singleplayer */
    core()->input()->setPortRoute(slot, static_cast<int>(m_players[i].control_port));
    log(DR_LOG_WARN, qPrintable(QString("[DK64] Port %1 routed to Port %2").arg(m_players[i].control_port).arg(slot)));

    m_retro->writeValue(character, DK64_CHARACTER[slot]);
    m_retro->writeValue(color, DK64_COLOR[slot]);

    slot++;
  }

  if (m_minigame->type == DR_MINIGAME_1P)
  {
    /* Enable swimming moves, otherwise the player can't swim in Splish-Splash Salvage */
    for (unsigned i = 0; i < 3; i++) {
      m_retro->writeValueForFrames(0x2C, DK64_SWIMMING_MOVE[i], 70);
    }

    m_retro->writeValue(data.minigame->scene_id, DK64_LOAD_TO_MAP);
  } else {
    setupMultiplayer(playercount);
  }

}

void DonkeyKong64::run(void)
{
  if (finishPending()) {
    return;
  }

  int64_t mapstate, currentmap;

  m_retro->readValue(&currentmap, DK64_CURRENT_MAP);
  m_retro->readValue(&mapstate, DK64_MAP_STATE);

  /* If the map is loading start it right away */
  if (mapstate == 0x0D && !m_minigameActive && (currentmap == m_minigame->scene_id || m_minigame->minigame_id >= 0x15)) {
    log(DR_LOG_INFO, qPrintable(QString("[DK64] Loaded map: %1").arg(currentmap)));
    muteForFrames(35); // Silence you transition!
    startMinigame();
    m_guestState = 0;
  }

  if (!m_minigameActive) {
    return;
  }

  switch (m_minigame->minigame_id) {
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
    /// DONKEY KONG
    int64_t arcadescene, arcade_gameover, arcade_levelid;

    arcade_levelid = 0x04;

    for (int8_t i = 0x10; i < m_minigame->minigame_id; i++) {
      arcade_levelid++;
    }

    m_retro->readValue(&arcadescene, DK64_ARCADE_SCENE);
    m_retro->readValue(&arcade_gameover, DK64_ARCADE_GAMEOVER);

    /* Donkey Kong Arcade and Jetpac require a "START" press before it can start applying data to it */
    if (arcadescene == 0x1B && m_guestState == DR_OK) { // Start of the game when Donkey Kong climbs up the ladder
      m_retro->writeValue(0, DK64_ARCADE_LIVES);
      m_retro->writeValue(1, DK64_ARCADE_TOTAL_LIVES);
      m_retro->writeValue(arcade_levelid, DK64_ARCADE_LEVEL_ID);
      m_guestState++;
    }

    if (finishPending()) {
      return;
    }

    int64_t arcade_currentlevel;

    m_retro->readValue(&arcade_currentlevel, DK64_ARCADE_LEVEL_ID);

    if (arcade_gameover == 0x01) {
      /* Give the game over text a bit to breath on screen before ending it */
      finishMinigameInFrames(60);
    } else if ((arcade_currentlevel > 3 && arcade_levelid != arcade_currentlevel)) {
      m_minigameState = 1;
      finishMinigame();
    }

    break;
  case 0x14:
    /// Jetpac
    int64_t jetpac_lives, jetpac_levelid, jetpac_state;
    m_retro->readValue(&jetpac_lives, DK64_JETPAC_LIVES);
    m_retro->readValue(&jetpac_levelid, DK64_JETPAC_LEVEL);
    m_retro->readValue(&jetpac_state, DK64_JETPAC_STATE);

    /* Make a forced "START" press so that the logic doesn't get confused */
    if (jetpac_lives == 0x00  && m_guestState == DR_OK) {
      core()->setBilinearFilter(false);
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, true);
    }

    if (jetpac_lives != 0x00 && m_guestState == DR_OK) {
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, false);

      m_retro->writeValue(2, DK64_JETPAC_LEVEL);
      m_retro->writeValue(1, DK64_JETPAC_LIVES);

      m_guestState++;
    }

    if ((jetpac_state == 1 && m_guestState == 1) ) {
      m_minigameState = 1;
      m_guestState++;
    }

    if ((m_guestState == 2 || jetpac_lives == 0xFF) && !finishPending()) {
      finishMinigameInFrames(80);
      core()->setBilinearFilter(true);
    }

    break;
  case 0x15:
  case 0x16:
    /// Kong Battle
    if ((mapstate == 0x08 && m_guestState == 0) || (mapstate == 0x0D && m_guestState == 1)) {
      m_guestState++;
    }

    if (mapstate == 0x08 && m_guestState == 2) {
      finishMinigame();
    }
    break;
  default:
    /// Bonus Stages

    int64_t ptr[2];
    m_retro->readValue(&ptr[0], DK64_HEAP_PTR[0]);
    m_retro->readValue(&ptr[1], DK64_HEAP_PTR[1]);

    if ((mapstate == 0x28 && m_guestState == 0) || (mapstate == 0x38 && m_guestState == 1)) {
      m_guestState++;
    }

    // Return until the minigame has ended
    if (m_guestState != 2)
      return;

    if (ptr[1]) {
      const dr_value_t TEXT = {static_cast<size_t>(ptr[1] + 0x24), DR_VALUE_TYPE_U32};
      int64_t val;
      m_retro->readValue(&val, TEXT);

      // TRY [AGAI]N?
      if (val == 0x41474149) {
        m_minigameState = 2;
      }
    }

    /** If the first pointer doesn't exist anymore or the Try Again shows up, end the minigame */
    if ((!ptr[0] && mapstate == 0x0C) || m_minigameState == 2) {
      if (!m_minigameState) {
        m_minigameState = 1;
      }
      finishMinigameInFrames(40);
    }

    break;
  }
}


dr_minigame_result_t DonkeyKong64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  const unsigned winners = computeWinners();

  if ((int)index == m_winnerIndex)
    result.coins = winners;

  // Reset options
  core()->options()->setOptionValue("mupen64plus-CountPerOp", "0");
  core()->options()->setOptionValue("mupen64plus-EnableLODEmulation", "True");

  return result;
}
