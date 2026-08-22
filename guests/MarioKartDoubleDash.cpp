#include "MarioKartDoubleDash.h"

static const size_t MKDD_CUP_ADDR = 0x803CB7A8;
static const size_t MKDD_TRACK_ADDR = 0x803CB7AC;

// u16 total laps for the race -- otherwise unused "sForceTotalLapNum"
static const size_t MKDD_TOTAL_LAPS_ADDR = 0x803CB7EC;

typedef enum
{
  MKDD_ITEM_BOX_RECOMMENDED = 0,
  MKDD_ITEM_BOX_BASIC = 1,
  MKDD_ITEM_BOX_FRANTIC = 2,
  MKDD_ITEM_BOX_NONE = 3,
} mkdd_item_box;

// u32 item box option
static const size_t MKDD_VS_ITEM_BOX = 0x812BFB2C;

// u32 laps option (0=recommended, else number of laps)
static const size_t MKDD_VS_LAPS = 0x812BFB30;

typedef enum
{
  MKDD_PLAYERS_1 = 0,
  MKDD_PLAYERS_2 = 1,
  MKDD_PLAYERS_3 = 2,
  MKDD_PLAYERS_4 = 3,
} mkdd_player_count;

// u32 player count (see mkdd_player_count)
static const size_t MKDD_PLAYER_COUNT_ADDR = 0x812C1BC0;

typedef enum
{
  MKDD_GAMETYPE_VERSUS = 1,
  MKDD_GAMETYPE_BATTLE = 3,
} mkdd_gametype;

// u32 game type
static const size_t MKDD_GAMETYPE_ADDR = 0x812C1BCC;

typedef enum
{
  MKDD_CC_50 = 0,
  MKDD_CC_100 = 1,
  MKDD_CC_150 = 2,
  MKDD_CC_MIRROR = 3,
} mkdd_cc;

// u32 engine class
static const size_t MKDD_CC_ADDR = 0x812C1BD0;

// u32 battle type (shine thief can only be a 4p, balloon/bomb can be battle games)
static const size_t MKDD_BATTLE_TYPE = 0x812C1BCC;

static const size_t MKDD_CHAR1_ADDR[4] = { 0x812C1C04, 0x812C1C20, 0x812C1C3C, 0x812C1C58 };
static const size_t MKDD_CHAR2_ADDR[4] = { 0x812C1C08, 0x812C1C24, 0x812C1C40, 0x812C1C5C };
static const size_t MKDD_KART_ADDR[4]  = { 0x812C1C0C, 0x812C1C28, 0x812C1C44, 0x812C1C60 };

/* Pointer chain to a kart's runtime status:
 *   [ [ [0x803561D8] + 0x5B8 ] + 0x430 + kart*4 ] + 0x578  ->  mGameStatus (u32 bitflags)
 * ORing MKDD_STATUS_BOT into it makes that kart CPU-controlled. The objects only
 * exist once the race has started, so the chain reads null until then. */
static const size_t MKDD_KART_ROOT_PTR      = 0x803561D8;
static const size_t MKDD_KART_LIST_OFFSET   = 0x5B8;
static const size_t MKDD_KART_ARRAY_OFFSET  = 0x430;
static const size_t MKDD_KART_STATUS_OFFSET = 0x578;
static const uint32_t MKDD_STATUS_BOT       = 0x8;

typedef enum
{
  MKDD_BATTLE_STAGE_COOKIE_LAND = 0,
  MKDD_BATTLE_STAGE_NINTENDO_GAMECUBE = 1,
  MKDD_BATTLE_STAGE_BLOCK_CITY = 2,
  MKDD_BATTLE_STAGE_PIPE_PLAZA = 3,
  MKDD_BATTLE_STAGE_LUIGIS_MANSION = 4,
  MKDD_BATTLE_STAGE_TILT_A_KART = 5,
} mkdd_battle_stage;

// u32 battle stage
static const size_t MKDD_BATTLE_STAGE = 0x815973D0;

#define MKDD_KART_COUNT 20

// u32 current lap per player; a kart has finished once it reaches the total laps.
static const size_t MKDD_LAPS_ADDR[4] = { 0x8037FF60, 0x8037FF64, 0x8037FF68, 0x8037FF6C };

// u32 finishing placement per player (1 = 1st, 2 = 2nd, ...).
static const size_t MKDD_PLACEMENT_ADDR[4] = { 0x8037FFA0, 0x8037FFA4, 0x8037FFA8, 0x8037FFAC };

typedef enum
{
  MKDD_CHAR_NONE = -1,
  MKDD_CHAR_MARIO = 0x00,
  MKDD_CHAR_LUIGI = 0x01,
  MKDD_CHAR_PEACH = 0x02,
  MKDD_CHAR_DAISY = 0x03,
  MKDD_CHAR_YOSHI = 0x04,
  MKDD_CHAR_BIRDO = 0x05,
  MKDD_CHAR_BABY_MARIO = 0x06,
  MKDD_CHAR_BABY_LUIGI = 0x07,
  MKDD_CHAR_TOAD = 0x08,
  MKDD_CHAR_TOADETTE = 0x09,
  MKDD_CHAR_KOOPA = 0x0A,
  MKDD_CHAR_PARATROOPA = 0x0B,
  MKDD_CHAR_DONKEY_KONG = 0x0C,
  MKDD_CHAR_DIDDY_KONG = 0x0D,
  MKDD_CHAR_BOWSER = 0x0E,
  MKDD_CHAR_BOWSER_JR = 0x0F,
  MKDD_CHAR_WARIO = 0x10,
  MKDD_CHAR_WALUIGI = 0x11,
  MKDD_CHAR_PETEY_PIRANHA = 0x12,
  MKDD_CHAR_KING_BOO = 0x13,
} mkdd_char;

static mkdd_char mkddCharFor(dr_character character)
{
  switch (character)
  {
  case DR_CHARACTER_MARIO:
    return MKDD_CHAR_MARIO;
  case DR_CHARACTER_LUIGI:
    return MKDD_CHAR_LUIGI;
  case DR_CHARACTER_PEACH:
    return MKDD_CHAR_PEACH;
  case DR_CHARACTER_YOSHI:
    return MKDD_CHAR_YOSHI;
  case DR_CHARACTER_WARIO:
    return MKDD_CHAR_WARIO;
  case DR_CHARACTER_DONKEY_KONG:
    return MKDD_CHAR_DONKEY_KONG;
  case DR_CHARACTER_WALUIGI:
    return MKDD_CHAR_WALUIGI;
  case DR_CHARACTER_DAISY:
    return MKDD_CHAR_DAISY;
  case DR_CHARACTER_TOAD:
    return MKDD_CHAR_TOAD;
  case DR_CHARACTER_BOO:
    return MKDD_CHAR_KING_BOO;
  case DR_CHARACTER_KOOPA_KID:
    return MKDD_CHAR_BOWSER_JR;
  case DR_CHARACTER_TOADETTE:
    return MKDD_CHAR_TOADETTE;
  case DR_CHARACTER_BIRDO:
    return MKDD_CHAR_BIRDO;
  case DR_CHARACTER_DRY_BONES:
    return MKDD_CHAR_KOOPA;
  default:
    return MKDD_CHAR_MARIO;
  }
}

#define MKDD_COURSE_LUIGI_CIRCUIT     0
#define MKDD_COURSE_PEACH_BEACH       1
#define MKDD_COURSE_BABY_PARK         2
#define MKDD_COURSE_DRY_DRY_DESERT    3
#define MKDD_COURSE_MUSHROOM_BRIDGE   4
#define MKDD_COURSE_MARIO_CIRCUIT     5
#define MKDD_COURSE_DAISY_CRUISER     6
#define MKDD_COURSE_WALUIGI_STADIUM   7
#define MKDD_COURSE_SHERBET_LAND      8
#define MKDD_COURSE_MUSHROOM_CITY     9
#define MKDD_COURSE_YOSHI_CIRCUIT     10
#define MKDD_COURSE_DK_MOUNTAIN       11
#define MKDD_COURSE_WARIO_COLOSSEUM   12
#define MKDD_COURSE_DINO_DINO_JUNGLE  13
#define MKDD_COURSE_BOWSERS_CASTLE    14
#define MKDD_COURSE_RAINBOW_ROAD      15

/* The scene_id field is unused here, so it carries the race's total laps. */
static const dr_mp_minigame_t MKDD_MINIGAMES[] =
{
  { "Kart: Luigi Circuit", DR_MINIGAME_4P, MKDD_COURSE_LUIGI_CIRCUIT, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Peach Beach", DR_MINIGAME_4P, MKDD_COURSE_PEACH_BEACH, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Baby Park", DR_MINIGAME_4P, MKDD_COURSE_BABY_PARK, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Dry Dry Desert", DR_MINIGAME_4P, MKDD_COURSE_DRY_DRY_DESERT, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Mushroom Bridge", DR_MINIGAME_4P, MKDD_COURSE_MUSHROOM_BRIDGE, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Mario Circuit", DR_MINIGAME_4P, MKDD_COURSE_MARIO_CIRCUIT, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Daisy Cruiser", DR_MINIGAME_4P, MKDD_COURSE_DAISY_CRUISER, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Waluigi Stadium", DR_MINIGAME_4P, MKDD_COURSE_WALUIGI_STADIUM, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Sherbet Land", DR_MINIGAME_4P, MKDD_COURSE_SHERBET_LAND, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Mushroom City", DR_MINIGAME_4P, MKDD_COURSE_MUSHROOM_CITY, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Yoshi Circuit", DR_MINIGAME_4P, MKDD_COURSE_YOSHI_CIRCUIT, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: D.K. Mountain", DR_MINIGAME_4P, MKDD_COURSE_DK_MOUNTAIN, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Wario Colosseum", DR_MINIGAME_4P, MKDD_COURSE_WARIO_COLOSSEUM, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: Dino Dino Jungle", DR_MINIGAME_4P, MKDD_COURSE_DINO_DINO_JUNGLE, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: GCN Bowser's Castle", DR_MINIGAME_4P, MKDD_COURSE_BOWSERS_CASTLE, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "Kart: GCN Rainbow Road", DR_MINIGAME_4P, MKDD_COURSE_RAINBOW_ROAD, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/* Automated menuing state machine */
enum
{
  MKDD_SETUP_DONE = 0,
  MKDD_SETUP_CUP,     /* write the cup, tap A */
  MKDD_SETUP_TRACK,   /* write the track, tap A */
  MKDD_SETUP_CONFIRM, /* tap A once more */
  MKDD_SETUP_START,   /* start the mini-game */
};

MarioKartDoubleDash::MarioKartDoubleDash(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath(dr_core_path(DR_CORE_DOLPHIN).toStdString())
  , m_discPath((dr_roms_directory() + "/Mario Kart - Double Dash!! (USA)").toStdString())
  , m_statePath((dr_state_directory() + "/mariokartdoubledash.state.zip").toStdString())
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioKartDoubleDash::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioKartDoubleDash::run()
{
  m_retro->tickFrameWrites();

  /* Release a forced A press shortly after it starts so it reads as a press */
  if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);

  if (m_setupStep != MKDD_SETUP_DONE && --m_stepDelay <= 0)
    advanceSetup();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* 60 frames after the race starts, flag the CPU players as bots so the game's AI
   * drives them (the kart objects are allocated by then). Done once. */
  if (!m_botsApplied && m_minigameFrames >= 60)
  {
    m_botsApplied = true;
    for (unsigned i = 0; i < 4; i++)
    {
      if (m_players[i].control_type != DR_CONTROL_TYPE_CPU)
        continue;
      const unsigned slot = dr_player_slot(m_players[i], i);
      const size_t addr = kartStatusAddr(slot);
      uint32_t status = 0;
      if (!addr || m_retro->readu32(&status, addr) != DR_OK)
      {
        log(DR_LOG_WARN, qPrintable(QString("MKDD: kart %1 mGameStatus unreadable, bot skipped").arg(slot)));
        continue;
      }
      const uint32_t updated = status | MKDD_STATUS_BOT;
      m_retro->writeu32(updated, addr);
      log(DR_LOG_INFO, qPrintable(QString("MKDD: kart %1 mGameStatus 0x%2 -> 0x%3")
        .arg(slot).arg(status, 8, 16, QChar('0')).arg(updated, 8, 16, QChar('0'))));
    }
  }

  if (!m_finishPending)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      uint32_t laps = 0;
      if (m_retro->readu32(&laps, MKDD_LAPS_ADDR[i]) == DR_OK &&
          laps == static_cast<uint32_t>(m_laps))
      {
        m_finishPending = true;
        finishMinigameInFrames(450);
        break;
      }
    }
  }
}

void MarioKartDoubleDash::pressA()
{
  core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
  m_aReleaseDelay = 8;
}

size_t MarioKartDoubleDash::kartStatusAddr(unsigned kart)
{
  uint32_t rootPtr = 0;
  if (m_retro->readu32(&rootPtr, MKDD_KART_ROOT_PTR) != DR_OK || !rootPtr)
    return 0;

  uint32_t listPtr = 0;
  if (m_retro->readu32(&listPtr, rootPtr + MKDD_KART_LIST_OFFSET) != DR_OK || !listPtr)
    return 0;

  uint32_t kartObj = 0;
  if (m_retro->readu32(&kartObj, listPtr + MKDD_KART_ARRAY_OFFSET + (kart * 4)) != DR_OK || !kartObj)
    return 0;

  return static_cast<size_t>(kartObj) + MKDD_KART_STATUS_OFFSET;
}

/* One step of the post-load menu sequence: write the next value, tap A, and arm
 * the wait before the following step (30 frames each). */
void MarioKartDoubleDash::advanceSetup()
{
  switch (m_setupStep)
  {
  case MKDD_SETUP_CUP:
    m_retro->writes32(m_cup, MKDD_CUP_ADDR);
    pressA();
    m_setupStep = MKDD_SETUP_TRACK;
    m_stepDelay = 30;
    break;
  case MKDD_SETUP_TRACK:
    m_retro->writes32(m_track, MKDD_TRACK_ADDR);
    m_retro->writeu16(static_cast<uint16_t>(m_laps), MKDD_TOTAL_LAPS_ADDR);
    pressA();
    m_setupStep = MKDD_SETUP_CONFIRM;
    m_stepDelay = 30;
    break;
  case MKDD_SETUP_CONFIRM:
    pressA();
    m_setupStep = MKDD_SETUP_START;
    m_stepDelay = 30;
    break;
  case MKDD_SETUP_START:
    m_setupStep = MKDD_SETUP_DONE;
    startMinigame();
    break;
  default:
    break;
  }
}

const dr_mp_minigame_t *MarioKartDoubleDash::minigames() const
{
  return MKDD_MINIGAMES;
}

void MarioKartDoubleDash::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_finishPending = false;
  m_botsApplied = false;
  m_aReleaseDelay = 0;

  const int course = data.minigame ? data.minigame->minigame_id : 0;
  m_cup = course / 4;
  m_track = course % 4;
  m_laps = data.minigame ? data.minigame->scene_id : 1;

  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(m_players[i], i);

    // Pair every player with Toad
    m_retro->writes32(mkddCharFor(m_players[i].character), MKDD_CHAR2_ADDR[slot]);
    m_retro->writes32(MKDD_CHAR_TOAD, MKDD_CHAR1_ADDR[slot]);

    // Random kart for each player.
    m_retro->writes32(dr_rand() % MKDD_KART_COUNT, MKDD_KART_ADDR[slot]);
  }

  applyPlayers();
  pressA();

  m_setupStep = MKDD_SETUP_CUP;
  m_stepDelay = 90;
}

dr_minigame_result_t MarioKartDoubleDash::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index >= 4)
    return result;

  const unsigned slot = dr_player_slot(m_players[index], index);
  uint32_t place = 0;
  if (m_retro->readu32(&place, MKDD_PLACEMENT_ADDR[slot]) == DR_OK && place == 1)
    result.coins = 10;

  return result;
}

void MarioKartDoubleDash::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];

    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;
  }
}
