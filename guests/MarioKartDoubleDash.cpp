#include "MarioKartDoubleDash.h"

static const size_t MKDD_CUP_ADDR = 0x803CB7A8;

static const size_t MKDD_TRACK_ADDR = 0x803CB7AC;

// u32 item box option (0=recommended, 1=basic, 2=frantic, 3=none)
static const size_t MKDD_VS_ITEM_BOX = 0x812BFB2C;

// u32 laps option (0=recommended, else number of laps)
static const size_t MKDD_VS_LAPS = 0x812BFB30;

// u32 players (0=1, 1=2, etc)
static const size_t MKDD_PLAYER_COUNT_ADDR = 0x812C1BC0;

// u32 game type (1=versus, 3=battle)
static const size_t MKDD_GAMETYPE_ADDR = 0x812C1BCC;

// u32 engine class (0=50cc, 1=100cc, 2=150cc, 3=mirror)
static const size_t MKDD_CC_ADDR = 0x812C1BD0;

static const size_t MKDD_CHAR1_ADDR[4] = { 0x812C1C04, 0x812C1C20, 0x812C1C3C, 0x812C1C58 };
static const size_t MKDD_CHAR2_ADDR[4] = { 0x812C1C08, 0x812C1C24, 0x812C1C40, 0x812C1C5C };
static const size_t MKDD_KART_ADDR[4]  = { 0x812C1C0C, 0x812C1C28, 0x812C1C44, 0x812C1C60 };

#define MKDD_KART_COUNT 20

// u32 lap count per player (a race is 3 laps).
static const size_t MKDD_LAPS_ADDR[4] = { 0x8037FF60, 0x8037FF64, 0x8037FF68, 0x8037FF6C };

// u32 finishing placement per player (1 = 1st, 2 = 2nd, ...).
static const size_t MKDD_PLACEMENT_ADDR[4] = { 0x8037FFA0, 0x8037FFA4, 0x8037FFA8, 0x8037FFAC };

// static const size_t MKDD_CONTROL_TYPE_ADDR[4] = { 0, 0, 0, 0 };
// static const size_t MKDD_RESULT_ADDR[4]       = { 0, 0, 0, 0 };

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

static const dr_mp_minigame_t MKDD_MINIGAMES[] =
{
  { "Mario Kart: Luigi Circuit",    DR_MINIGAME_4P, MKDD_COURSE_LUIGI_CIRCUIT,    0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Peach Beach",      DR_MINIGAME_4P, MKDD_COURSE_PEACH_BEACH,      0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Baby Park",        DR_MINIGAME_4P, MKDD_COURSE_BABY_PARK,        0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Dry Dry Desert",   DR_MINIGAME_4P, MKDD_COURSE_DRY_DRY_DESERT,   0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Mushroom Bridge",  DR_MINIGAME_4P, MKDD_COURSE_MUSHROOM_BRIDGE,  0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Mario Circuit",    DR_MINIGAME_4P, MKDD_COURSE_MARIO_CIRCUIT,    0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Daisy Cruiser",    DR_MINIGAME_4P, MKDD_COURSE_DAISY_CRUISER,    0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Waluigi Stadium",  DR_MINIGAME_4P, MKDD_COURSE_WALUIGI_STADIUM,  0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Sherbet Land",     DR_MINIGAME_4P, MKDD_COURSE_SHERBET_LAND,     0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Mushroom City",    DR_MINIGAME_4P, MKDD_COURSE_MUSHROOM_CITY,    0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Yoshi Circuit",    DR_MINIGAME_4P, MKDD_COURSE_YOSHI_CIRCUIT,    0xFF, DR_NO_QUIRKS },
  { "Mario Kart: DK Mountain",      DR_MINIGAME_4P, MKDD_COURSE_DK_MOUNTAIN,      0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Wario Colosseum",  DR_MINIGAME_4P, MKDD_COURSE_WARIO_COLOSSEUM,  0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Dino Dino Jungle", DR_MINIGAME_4P, MKDD_COURSE_DINO_DINO_JUNGLE, 0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Bowser's Castle",  DR_MINIGAME_4P, MKDD_COURSE_BOWSERS_CASTLE,   0xFF, DR_NO_QUIRKS },
  { "Mario Kart: Rainbow Road",     DR_MINIGAME_4P, MKDD_COURSE_RAINBOW_ROAD,     0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
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

  if (!m_finishPending)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      uint32_t laps = 0;
      if (m_retro->readu32(&laps, MKDD_LAPS_ADDR[i]) == DR_OK && laps >= 3)
      {
        m_finishPending = true;
        finishMinigameInFrames(360);
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
  m_aReleaseDelay = 0;

  const int course = data.minigame ? data.minigame->minigame_id : 0;
  m_cup = course / 4;
  m_track = course % 4;

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
