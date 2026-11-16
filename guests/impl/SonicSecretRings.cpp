#include "SonicSecretRings.h"
#include "DrPaths.h"

#include <QRetro.h>

/// u8 - Party mode mini-game id to load (live +0x09)
static const dr_value_t SSR_MINIGAME_ID = { 0x8065B7DD, DR_VALUE_TYPE_U8 };

typedef enum
{
  SSR_MINIGAME_HEAVY_BALL            = 0x04,
  SSR_MINIGAME_HOME_RUN              = 0x06,
  SSR_MINIGAME_COGWHEEL_RELAY        = 0x07,
  SSR_MINIGAME_MINE_CART_RACE        = 0x09,
  SSR_MINIGAME_PARASOL_BALL          = 0x0B,
  SSR_MINIGAME_PARASOL_DIVING        = 0x0D,
  SSR_MINIGAME_STEEL_SURVIVAL        = 0x0E,
  SSR_MINIGAME_BALANCE_BATTLE        = 0x0F,
  SSR_MINIGAME_BUBBLE_HUNT           = 0x11,
  SSR_MINIGAME_FRUIT_CATCH           = 0x12,
  SSR_MINIGAME_GIANT_STAKES          = 0x16,
  SSR_MINIGAME_CANOE_RACE            = 0x18,
  SSR_MINIGAME_CANOE_SALVAGE         = 0x1A,
  SSR_MINIGAME_CANOE_SURVIVAL        = 0x1B,
  SSR_MINIGAME_SHINE_ON_KRI_MA_DJINN = 0x1C,
  SSR_MINIGAME_TREASURE_BOX          = 0x1D,
  SSR_MINIGAME_PITCH_BLACK           = 0x20,
  SSR_MINIGAME_EDGE_RACE             = 0x21,
  SSR_MINIGAME_WEIGHT_PUZZLE         = 0x22,
  SSR_MINIGAME_PIRATE_FLAG           = 0x28,
  SSR_MINIGAME_SAFE_DIAL             = 0x29,
  SSR_MINIGAME_BIG_CROSSBOW          = 0x2A,
  SSR_MINIGAME_COLOR_BALLOON         = 0x2B,
  SSR_MINIGAME_HATCHET               = 0x2C,
  SSR_MINIGAME_BIG_BALLOON           = 0x2D,
  SSR_MINIGAME_CARPET                = 0x2E,
  SSR_MINIGAME_SKYDIVING             = 0x30,
  SSR_MINIGAME_PIRATE_SHIP           = 0x31,
  SSR_MINIGAME_BATTLE_FISHING        = 0x32,
  SSR_MINIGAME_BONE_KNIGHT           = 0x33,
  SSR_MINIGAME_BALANCE_RACE          = 0x34,
  SSR_MINIGAME_BIG_FIRE              = 0x35,
  SSR_MINIGAME_PTERO_RIDER           = 0x36,
  SSR_MINIGAME_PROPELLER_RACE        = 0x37,
  SSR_MINIGAME_PUTT_PUTT             = 0x38,
  SSR_MINIGAME_VIOLIN                = 0x39,
  SSR_MINIGAME_TRAP                  = 0x3A,
  SSR_MINIGAME_MARKSMAN              = 0x3B,
  SSR_MINIGAME_TREASURE_HAUL         = 0x3D,
  SSR_MINIGAME_GRAB_IT_KRI_MA_DJINN  = 0x3F,
} ssr_minigame;

typedef enum
{
  SSR_MINIGAME_STATE_NONE = 0,
  SSR_MINIGAME_STATE_PLAYING,
  SSR_MINIGAME_STATE_INTRO,
  SSR_MINIGAME_STATE_ENDING,
  SSR_MINIGAME_STATE_FINISHED
} ssr_minigame_state;

static const dr_value_t SSR_MINIGAME_STATE = { 0x8065B7DE, DR_VALUE_TYPE_U8 };

typedef enum
{
  SSR_CHARACTER_SONIC = 0,
  SSR_CHARACTER_TAILS,
  SSR_CHARACTER_KNUCKLES,
  SSR_CHARACTER_AMY,
  SSR_CHARACTER_SHADOW,
  SSR_CHARACTER_CREAM,
  SSR_CHARACTER_SILVER,
  SSR_CHARACTER_BLAZE
} ssr_character;

/// u8[4] - Character per player @see ssr_character
static const size_t SSR_BACKUP_CHARACTER_ADDR = 0x8065BF4C;
static const size_t SSR_MENU_CHARACTER_ADDR = 0x8065BF3C;
static const size_t SSR_LIVE_CHARACTER_ADDR = 0x8065B7EC;

/// u8[4] - CPU difficulty per player: 0 = Easy, 1 = Normal, 2 = Hard, 3 = Extreme
static const size_t SSR_BACKUP_DIFFICULTY_ADDR = 0x8065BF50;
static const size_t SSR_MENU_DIFFICULTY_ADDR = 0x8065BF40;
static const size_t SSR_LIVE_DIFFICULTY_ADDR = 0x8065B7F4;

/// u8 - Number of human players minus one. Slots below the count are human, the
/// rest CPUs; player 1 is always human.
static const dr_value_t SSR_MENU_HUMAN_COUNT = { 0x8065BF44, DR_VALUE_TYPE_U8 };

/// u8[4] - Player type per slot, derived from the human count: 0 = human, 1 = CPU, 2 = none
static const size_t SSR_LIVE_PLAYER_TYPE_ADDR = 0x8065B7E4;
static const uint8_t SSR_PLAYER_TYPE_HUMAN = 0;
static const uint8_t SSR_PLAYER_TYPE_CPU = 1;

/// f32[4] - Each player's mini-game score (live +0x24); higher is better once the
/// mini-game type's callback has normalised it
static const size_t SSR_SCORE_ADDR = 0x8065B7F8;

/// u32[4] - Each player's running Party points
static const size_t SSR_POINTS_ADDR = 0x8065BFBC;
static const uint32_t SSR_FIRST_PLACE_POINTS = 10;

static const dr_value_t SSR_ALLOW_BACK = { 0x80698D58, DR_VALUE_TYPE_U8 };

static uint8_t ssr_cpu_difficulty(dr_difficulty difficulty)
{
  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0;
  case DR_DIFFICULTY_HARD:
    return 2;
  case DR_DIFFICULTY_VERY_HARD:
    return 3;
  default:
    return 1;
  }
}

static const dr_mp_minigame_t SSR_MINIGAMES[] =
{
  // perfect
  { "Roll it! Heavy Ball", DR_MINIGAME_4P, SSR_MINIGAME_HEAVY_BALL, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Hit it! Home Run", DR_MINIGAME_4P, SSR_MINIGAME_HOME_RUN, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // bad controls
  { "Spin! Cogwheel Relay", DR_MINIGAME_INVALID, SSR_MINIGAME_COGWHEEL_RELAY, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // perfect
  { "Go! Mine Cart Race", DR_MINIGAME_4P, SSR_MINIGAME_MINE_CART_RACE, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Hit it! Parasol Ball", DR_MINIGAME_4P, SSR_MINIGAME_PARASOL_BALL, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Get 'em! Parasol Diving", DR_MINIGAME_4P, SSR_MINIGAME_PARASOL_DIVING, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // bad
  { "Spin! Steel Survival!", DR_MINIGAME_INVALID, SSR_MINIGAME_STEEL_SURVIVAL, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // perfect
  { "Knockdown! Balance Battle", DR_MINIGAME_4P, SSR_MINIGAME_BALANCE_BATTLE, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Grab it! Bubble Hunt", DR_MINIGAME_4P, SSR_MINIGAME_BUBBLE_HUNT, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Spike! Fruit Catch", DR_MINIGAME_4P, SSR_MINIGAME_FRUIT_CATCH, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Whack! Giant Stakes", DR_MINIGAME_4P, SSR_MINIGAME_GIANT_STAKES, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },

  // uncontrollable -- held sideways with left/right circular motion to row left or right
  { "Row! Canoe Race", DR_MINIGAME_INVALID, SSR_MINIGAME_CANOE_RACE, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Row! Canoe Salvage", DR_MINIGAME_INVALID, SSR_MINIGAME_CANOE_SALVAGE, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Row! Canoe Survival", DR_MINIGAME_INVALID, SSR_MINIGAME_CANOE_SURVIVAL, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // uncontrollable -- hold wii remote upright, use slight motion to tilt forward and back
  { "Shine on! Kri Ma Djinn", DR_MINIGAME_INVALID, SSR_MINIGAME_SHINE_ON_KRI_MA_DJINN, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },

  // needs wii remote speaker -- DONT BOTHER
  { "Recognize! Treasure Box", DR_MINIGAME_INVALID, SSR_MINIGAME_TREASURE_BOX, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // perfect
  { "Spot! Pitch Black", DR_MINIGAME_4P, SSR_MINIGAME_PITCH_BLACK, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Seek! Edge Race", DR_MINIGAME_4P, SSR_MINIGAME_EDGE_RACE, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Spill! Weight Puzzle", DR_MINIGAME_4P, SSR_MINIGAME_WEIGHT_PUZZLE, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  
  // uncontrollable -- left/right are inverse, up/down are impossible
  { "Aye, aye Captain! Pirate Flag", DR_MINIGAME_INVALID, SSR_MINIGAME_PIRATE_FLAG, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // uncontrollable
  { "Spin! Safe Dial", DR_MINIGAME_INVALID, SSR_MINIGAME_SAFE_DIAL, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // unplayable, needs pointer + moving remote further/closer to screen
  { "Aim! Big Crossbow", DR_MINIGAME_INVALID, SSR_MINIGAME_BIG_CROSSBOW, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_NO_FLAGS },
  { "Aim! Color Balloon", DR_MINIGAME_INVALID, SSR_MINIGAME_COLOR_BALLOON, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_NO_FLAGS },
  
  // perfect
  { "Whack! Hatchet", DR_MINIGAME_4P, SSR_MINIGAME_HATCHET, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Stay Tight! Big Balloon!", DR_MINIGAME_4P, SSR_MINIGAME_BIG_BALLOON, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Pull on it! Carpet", DR_MINIGAME_4P, SSR_MINIGAME_CARPET, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Look up! Skydiving", DR_MINIGAME_4P, SSR_MINIGAME_SKYDIVING, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Sink 'em! Pirate Ship", DR_MINIGAME_4P, SSR_MINIGAME_PIRATE_SHIP, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Reel! Battle Fishing", DR_MINIGAME_4P, SSR_MINIGAME_BATTLE_FISHING, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },

  // uncontrollable
  { "Thrust! Bone Knight", DR_MINIGAME_INVALID, SSR_MINIGAME_BONE_KNIGHT, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // perfect
  { "Cross! Balance Race", DR_MINIGAME_4P, SSR_MINIGAME_BALANCE_RACE, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Put it out! Big Fire", DR_MINIGAME_4P, SSR_MINIGAME_BIG_FIRE, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Fly! Ptero-Rider", DR_MINIGAME_4P, SSR_MINIGAME_PTERO_RIDER, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Spin! Propeller Race", DR_MINIGAME_4P, SSR_MINIGAME_PROPELLER_RACE, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },

  // with current control you always win, needs finer up/down acceleration?
  { "Fore! Putt-Putt", DR_MINIGAME_INVALID, SSR_MINIGAME_PUTT_PUTT, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // perfect
  { "Play! Violin", DR_MINIGAME_4P, SSR_MINIGAME_VIOLIN, 0, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  
  // perfect
  { "Watch out! Trap", DR_MINIGAME_4P, SSR_MINIGAME_TRAP, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // unplayable, cant point gun down
  { "Fire! Marksman", DR_MINIGAME_INVALID, SSR_MINIGAME_MARKSMAN, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // unplayable, needs moving remote further/closer to screen
  { "Hold it! Treasure Haul", DR_MINIGAME_4P, SSR_MINIGAME_TREASURE_HAUL, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  
  // unplayable, needs pointer + moving remote further/closer to screen
  { "Grab it! Kri Ma Djinn", DR_MINIGAME_4P, SSR_MINIGAME_GRAB_IT_KRI_MA_DJINN, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

SonicSecretRings::SonicSecretRings(QObject *parent)
  : DrGuestWii("wii-ssr", (DrPaths::instance().romsDirectory() + "/Sonic and the Secret Rings (USA) (En,Ja,Fr,De,Es,It)").toStdString(), parent)
{
}

const dr_mp_minigame_t *SonicSecretRings::minigames() const
{
  return SSR_MINIGAMES;
}

void SonicSecretRings::run()
{
  int64_t state = SSR_MINIGAME_STATE_NONE;
  unsigned winners = 0;
  unsigned i;

  if (m_retro->booting())
    return;

  if (!m_minigameActive)
  {
    if (++m_minigameFrames >= 48)
    {
      m_minigameFrames = 0;
      startMinigame();
    }
    return;
  }

  m_minigameFrames++;

  if (!m_minigame || m_finishScheduled)
    return;

  if (m_retro->readValue(&state, SSR_MINIGAME_STATE) != DR_OK)
    return;

  /* Ending -> finished marks the end of the mini-game */
  if (m_lastState == SSR_MINIGAME_STATE_ENDING && state == SSR_MINIGAME_STATE_FINISHED)
  {
    log(DR_LOG_INFO, qPrintable(QString("mini-game finished at frame %1").arg(m_minigameFrames)));
    for (i = 0; i < 4; i++)
      if (minigameResult(i).coins > 0)
      {
        log(DR_LOG_INFO, qPrintable(QString("player %1 won 10 coins").arg(i + 1)));
        winners++;
      }
    if (!winners)
      log(DR_LOG_INFO, "nobody won 10 coins");
    m_finishScheduled = true;
    finishMinigame();
  }
  m_lastState = state;
}

void SonicSecretRings::doApplyGameData(const DrGameData &data)
{
  unsigned humans = 0;
  unsigned i;

  m_retro->loadState(state());
  m_retro->writeValue(0, SSR_ALLOW_BACK);

  m_minigameFrames = 0;
  m_finishScheduled = false;
  m_lastState = SSR_MINIGAME_STATE_NONE;

  /* Points before the mini-game, so the result is what it added */
  for (i = 0; i < 4; i++)
    if (m_retro->readu32(&m_startPoints[i], SSR_POINTS_ADDR + i * 4) != DR_OK)
      m_startPoints[i] = 0;

  m_retro->writeValue(data.minigame->minigame_id, SSR_MINIGAME_ID);

  for (i = 0; i < 4; i++)
  {
    const dr_player_t &player = data.players[i];
    const unsigned slot = player.control_port;
    const bool cpu = player.control_type == DR_CONTROL_TYPE_CPU && slot != 0;

    if (slot >= 4)
      continue;
    if (cpu)
    {
      const uint8_t difficulty = ssr_cpu_difficulty(player.difficulty);

      m_retro->writeu8(difficulty, SSR_BACKUP_DIFFICULTY_ADDR + slot);
      m_retro->writeu8(difficulty, SSR_MENU_DIFFICULTY_ADDR + slot);
      m_retro->writeu8(difficulty, SSR_LIVE_DIFFICULTY_ADDR + slot);
    }
    else
      humans++;
    m_retro->writeu8(cpu ? SSR_PLAYER_TYPE_CPU : SSR_PLAYER_TYPE_HUMAN, SSR_LIVE_PLAYER_TYPE_ADDR + slot);
  }

  if (humans == 0)
    humans = 1;
  m_retro->writeValue(humans - 1, SSR_MENU_HUMAN_COUNT);

  applyControlRemap(data.minigame->quirks, data.players);
}

dr_minigame_result_t SonicSecretRings::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  unsigned slot;
  uint32_t points = 0;

  if (index >= 4)
    return result;
  slot = m_players[index].control_port;
  if (slot >= 4 || m_retro->readu32(&points, SSR_POINTS_ADDR + slot * 4) != DR_OK)
    return result;

  /* First place, ties included, wins */
  if (points >= m_startPoints[slot] && points - m_startPoints[slot] >= SSR_FIRST_PLACE_POINTS)
    result.coins = 10;

  return result;
}
