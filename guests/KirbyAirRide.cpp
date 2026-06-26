#include "KirbyAirRide.h"

#include <QRandomGenerator>

// Addresses
static const size_t KAR_COLOR_ADDR[4]        = { 0x80535BF9, 0x80535BFA, 0x80535BFB, 0x80535BFC };
static const size_t KAR_MACHINE_ADDR[4]      = { 0x80535C09, 0x80535C0A, 0x80535C0B, 0x80535C0C };
static const size_t KAR_CPU_LEVEL_ADDR[4]    = { 0x80535C05, 0x80535C06, 0x80535C07, 0x80535C08 };
static const size_t KAR_CONTROL_TYPE_ADDR[4] = { 0x80535BED, 0x80535BEE, 0x80535BEF, 0x80535BF0 };
static const size_t KAR_RESULT_ADDR[4]       = { 0x80535CCC, 0x80535CCD, 0x80535CCE, 0x80535CCF };

#define KAR_STADIUM_GAME_ADDR 0x80535F85
#define KAR_STADIUM_STATE_ADDR 0x80535F87

// Colors
#define KAR_COLOR_PINK    0
#define KAR_COLOR_YELLOW  1
#define KAR_COLOR_BLUE    2
#define KAR_COLOR_RED     3
#define KAR_COLOR_GREEN   4
#define KAR_COLOR_PURPLE  5
#define KAR_COLOR_BROWN   6
#define KAR_COLOR_WHITE   7

// Machines
#define KAR_MACHINE_COMPACT_STAR     0
#define KAR_MACHINE_WARP_STAR        1
#define KAR_MACHINE_TURBO_STAR       2
#define KAR_MACHINE_FORMULA_STAR     3
#define KAR_MACHINE_SLICK_STAR       4
#define KAR_MACHINE_SWERVE_STAR      5
#define KAR_MACHINE_WAGON_STAR       6
#define KAR_MACHINE_BULK_STAR        7
#define KAR_MACHINE_SHADOW_STAR      8
#define KAR_MACHINE_WINGED_STAR      9
#define KAR_MACHINE_JET_STAR         10
#define KAR_MACHINE_ROCKET_STAR      11
#define KAR_MACHINE_WHEELIE_SCOOTER  12
#define KAR_MACHINE_WHEELIE_BIKE     13
#define KAR_MACHINE_REX_WHEELIE      14

// Control Types
#define KAR_CONTROL_HUMAN  0
#define KAR_CONTROL_CPU    2
#define KAR_CONTROL_NONE   3

// CPU Levels
#define KAR_CPU_LEVEL_MIN  0
#define KAR_CPU_LEVEL_MAX  8

// Stadium Games
#define KAR_STADIUM_DRAG_RACE_1                    0
#define KAR_STADIUM_DRAG_RACE_2                    1
#define KAR_STADIUM_DRAG_RACE_3                    2
#define KAR_STADIUM_DRAG_RACE_4                    3
#define KAR_STADIUM_AIR_GLIDER                     4
#define KAR_STADIUM_TARGET_FLIGHT                  5
#define KAR_STADIUM_HIGH_JUMP                      6
#define KAR_STADIUM_KIRBY_MELEE_1                  7
#define KAR_STADIUM_KIRBY_MELEE_2                  8
#define KAR_STADIUM_DESTRUCTION_DERBY_1            9
#define KAR_STADIUM_DESTRUCTION_DERBY_2            10
#define KAR_STADIUM_DESTRUCTION_DERBY_3            11
#define KAR_STADIUM_DESTRUCTION_DERBY_4            12
#define KAR_STADIUM_DESTRUCTION_DERBY_5            13
#define KAR_STADIUM_SINGLE_RACE_FANTASY_MEADOWS    14
#define KAR_STADIUM_SINGLE_RACE_MAGMA_FLOWS        15
#define KAR_STADIUM_SINGLE_RACE_SKY_SANDS          16
#define KAR_STADIUM_SINGLE_RACE_FROZEN_HILLSIDE    17
#define KAR_STADIUM_SINGLE_RACE_BEANSTALK_PARK     18
#define KAR_STADIUM_SINGLE_RACE_CELESTIAL_VALLEY   19
#define KAR_STADIUM_SINGLE_RACE_MACHINE_PASSAGE    20
#define KAR_STADIUM_SINGLE_RACE_CHECKER_KNIGHTS    21
#define KAR_STADIUM_SINGLE_RACE_NEBULA_BELT        22
#define KAR_STADIUM_VS_KING_DEDEDE                 23

static const dr_mp_minigame_t KAR_MINIGAMES[] =
{
  { "Drag Race", DR_MINIGAME_4P, KAR_STADIUM_DRAG_RACE_1, 0xFF, DR_NO_QUIRKS },
  //{ "Air Glider", DR_MINIGAME_4P, KAR_STADIUM_AIR_GLIDER, 0xFF, DR_NO_QUIRKS },
  //{ "Target Flight", DR_MINIGAME_4P, KAR_STADIUM_TARGET_FLIGHT, 0xFF, DR_NO_QUIRKS },
  //{ "High Jump", DR_MINIGAME_4P, KAR_STADIUM_HIGH_JUMP, 0xFF, DR_NO_QUIRKS },
  //{ "Kirby Melee", DR_MINIGAME_4P, KAR_STADIUM_KIRBY_MELEE_1, 0xFF, DR_NO_QUIRKS },
  //{ "Destruction Derby", DR_MINIGAME_4P, KAR_STADIUM_DESTRUCTION_DERBY_1, 0xFF, DR_NO_QUIRKS },
  //{ "Single Race", DR_MINIGAME_4P, KAR_STADIUM_SINGLE_RACE_FANTASY_MEADOWS, 0xFF, DR_NO_QUIRKS },
  //{ "VS King Dedede", DR_MINIGAME_4P, KAR_STADIUM_VS_KING_DEDEDE, 0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

KirbyAirRide::KirbyAirRide(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath((dr_cores_directory() + "/dolphin_libretro.so").toStdString())
  , m_discPath((dr_roms_directory() + "/Kirby Air Ride (USA).rvz").toStdString())
  , m_statePath((dr_state_directory() + "/kar.state.zip").toStdString())
{
  m_retro = new DrRetro(sharedCore, this);
}

void KirbyAirRide::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void KirbyAirRide::run()
{
  m_retro->tickFrameWrites();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  uint8_t state;
  if (m_finishPending || m_minigameFrames < 60 ||
      m_retro->readu8(&state, KAR_STADIUM_STATE_ADDR) != DR_OK)
    return;

  // Air Glider signals "finished" with state 2; every other stadium uses 1.
  uint8_t finishState =
    (m_minigame && m_minigame->minigame_id == KAR_STADIUM_AIR_GLIDER) ? 2 : 1;
  if (state == finishState)
  {
    log(DR_LOG_INFO, qPrintable(QString("%1 stadium finished").arg(name())));
    // Results land a bit after the state changes; wait before reading them.
    m_finishPending = true;
    finishMinigameInFrames(240);
  }
}

const dr_mp_minigame_t *KirbyAirRide::minigames() const
{
  return KAR_MINIGAMES;
}

void KirbyAirRide::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_minigameFrames = 0;
  m_finishPending = false;

  // Each entry stores the first variant of its stadium type; for types with
  // multiple variants, pick one at random from the contiguous id range.
  auto *rng = QRandomGenerator::global();
  uint8_t stadium;
  switch (minigame->minigame_id)
  {
  case KAR_STADIUM_DRAG_RACE_1:
    stadium = KAR_STADIUM_DRAG_RACE_1 + rng->bounded(4);
    break;
  case KAR_STADIUM_KIRBY_MELEE_1:
    stadium = KAR_STADIUM_KIRBY_MELEE_1 + rng->bounded(2);
    break;
  case KAR_STADIUM_DESTRUCTION_DERBY_1:
    stadium = KAR_STADIUM_DESTRUCTION_DERBY_1 + rng->bounded(3); // rng->bounded(5);
    break;
  case KAR_STADIUM_SINGLE_RACE_FANTASY_MEADOWS:
    stadium = KAR_STADIUM_SINGLE_RACE_FANTASY_MEADOWS; // + rng->bounded(9);
    break;
  default:
    stadium = static_cast<uint8_t>(minigame->minigame_id);
    break;
  }
  m_retro->writeForFrames(KAR_STADIUM_GAME_ADDR, &stadium, sizeof(stadium), 120);
  startMinigame();
}

dr_minigame_result_t KirbyAirRide::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4)
  {
    // Results are stored by framework index (KAR orders players by index).
    // Memory stores a placing (0 = 1st, 1 = 2nd, ...); award the winner coins.
    uint8_t place;
    if (m_retro->readu8(&place, KAR_RESULT_ADDR[index]) == DR_OK && place == 0)
      result.coins = 10;
  }
  return result;
}

// Kirby Air Ride orders players by index (unlike Mario Party, which orders by
// control port), so each framework index maps directly to its game slot, and
// the result placings come back in that same index order.
void KirbyAirRide::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;

    // Every racer is Kirby, so a character is represented by a Kirby color.
    uint8_t color;
    switch (p.character)
    {
    case DR_CHARACTER_MARIO:       color = KAR_COLOR_RED;    break;
    case DR_CHARACTER_LUIGI:       color = KAR_COLOR_BLUE;   break;
    case DR_CHARACTER_PEACH:       color = KAR_COLOR_PINK;   break;
    case DR_CHARACTER_YOSHI:       color = KAR_COLOR_GREEN;  break;
    case DR_CHARACTER_WARIO:       color = KAR_COLOR_PURPLE; break;
    case DR_CHARACTER_DONKEY_KONG: color = KAR_COLOR_BROWN;  break;
    case DR_CHARACTER_WALUIGI:     color = KAR_COLOR_WHITE;  break;
    case DR_CHARACTER_DAISY:       color = KAR_COLOR_YELLOW; break;
    default:                       color = KAR_COLOR_PINK;   break;
    }
    m_retro->writeForFrames(KAR_COLOR_ADDR[i], &color, sizeof(color), 120);

    uint8_t ctrl = (p.control_type == DR_CONTROL_TYPE_CPU) ? KAR_CONTROL_CPU : KAR_CONTROL_HUMAN;
    m_retro->writeForFrames(KAR_CONTROL_TYPE_ADDR[i], &ctrl, sizeof(ctrl), 120);

    uint8_t level;
    switch (p.difficulty)
    {
    case DR_DIFFICULTY_VERY_EASY: level = KAR_CPU_LEVEL_MIN; break;
    case DR_DIFFICULTY_EASY:      level = 2; break;
    case DR_DIFFICULTY_NORMAL:    level = 4; break;
    case DR_DIFFICULTY_HARD:      level = 6; break;
    case DR_DIFFICULTY_VERY_HARD: level = KAR_CPU_LEVEL_MAX; break;
    default:                      level = 4; break;
    }
    m_retro->writeForFrames(KAR_CPU_LEVEL_ADDR[i], &level, sizeof(level), 120);
  }
}

dr_error KirbyAirRide::doSetPlayerCharacter(unsigned index, dr_character character)
{
  m_players[index].character = character;
  return DR_OK;
}

dr_error KirbyAirRide::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  m_players[index].control_port = control_port;
  return DR_OK;
}

dr_error KirbyAirRide::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  m_players[index].control_type = control_type;
  return DR_OK;
}

dr_error KirbyAirRide::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  m_players[index].difficulty = difficulty;
  return DR_OK;
}

dr_error KirbyAirRide::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  // Stadium events are free-for-all; teams are unused, but this is the last
  // setter called per player, so flush everything to memory here.
  m_players[index].team_color = color;
  m_players[index].team_type = type;
  m_players[index].team_id = team_id;
  applyPlayers();
  return DR_OK;
}
