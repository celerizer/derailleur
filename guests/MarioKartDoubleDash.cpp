#include "MarioKartDoubleDash.h"

static const size_t MKDD_CHAR1_ADDR[4] = { 0x812C1C04, 0x812C1C20, 0x812C1C3C, 0x812C1C58 };
static const size_t MKDD_CHAR2_ADDR[4] = { 0x812C1C08, 0x812C1C24, 0x812C1C40, 0x812C1C5C };
static const size_t MKDD_KART_ADDR[4]  = { 0x812C1C0C, 0x812C1C28, 0x812C1C44, 0x812C1C60 };

// static const size_t MKDD_CONTROL_TYPE_ADDR[4] = { 0, 0, 0, 0 };
// static const size_t MKDD_RESULT_ADDR[4]       = { 0, 0, 0, 0 };

// #define MKDD_COURSE_ADDR 0
// #define MKDD_RACE_STATE_ADDR 0

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

// Control Types — TODO: verify
#define MKDD_CONTROL_HUMAN 0
#define MKDD_CONTROL_CPU   1
#define MKDD_CONTROL_NONE  2

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

// Courses — TODO: verify course IDs
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
  { "Mario Kart: Luigi Circuit", DR_MINIGAME_4P, MKDD_COURSE_LUIGI_CIRCUIT, 0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

MarioKartDoubleDash::MarioKartDoubleDash(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath(dr_core_path(DR_CORE_DOLPHIN).toStdString())
  , m_discPath((dr_roms_directory() + "/Mario Kart Double Dash (USA)").toStdString())
  , m_statePath((dr_state_directory() + "/mkdd.state.zip").toStdString())
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

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  // TODO: poll the race-state address to detect when the race finishes, then
  //       finishMinigameInFrames(...) after the results settle. See KirbyAirRide.
}

const dr_mp_minigame_t *MarioKartDoubleDash::minigames() const
{
  return MKDD_MINIGAMES;
}

void MarioKartDoubleDash::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_finishPending = false;

  // TODO: write the chosen course (data.minigame->minigame_id) into MKDD_COURSE_ADDR,
  //       e.g. with m_retro->writeForFrames(...).

  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = data.players[i];

    // The player rides up front as their character; the partner is always Toad.
    m_retro->writes32(mkddCharFor(m_players[i].character), MKDD_CHAR1_ADDR[i]);
    m_retro->writes32(MKDD_CHAR_TOAD, MKDD_CHAR2_ADDR[i]);
  }

  applyPlayers();
  startMinigame();
}

dr_minigame_result_t MarioKartDoubleDash::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  // TODO: read the finishing placement for `index` and award coins to the winner.
  (void)index;
  return result;
}

void MarioKartDoubleDash::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];

    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;

    // TODO: map p.character -> kart/driver, p.control_type -> human/CPU slot,
    //       and p.difficulty -> CPU level, writing each to memory.
  }
}
