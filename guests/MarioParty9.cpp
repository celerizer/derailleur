#include "MarioParty9.h"

#include <QRetro.h>

static const size_t MP9_MINIGAME_TO_LOAD_ADDR = 0x816FF828;

static const size_t MP9_NUM_PLAYERS_ADDR = 0x81752534;

/// bool - Whether or not this player is a bot
static const size_t MP9_IS_BOT_ADDR[4] = { 0x81752548, 0x81752570, 0x81752598, 0x817525C0 };

/// s32 - Mini-game team. In a 1v3 the group is 1 and the solo is 0 (matches other MPs).
static const size_t MP9_TEAM_ADDR[4] = { 0x8175254C, 0x81752574, 0x8175259C, 0x817525C4 };

/// s32
static const size_t MP9_CPU_DIFFICULTY_ADDR[4] = { 0x81752550, 0x81752578, 0x817525A0, 0x817525C8 };

/// s32
static const size_t MP9_CHARACTER_ADDR[4] = { 0x81752554, 0x8175257C, 0x817525A4, 0x817525CC };

/// s32 - The results placement after the mini-game. 0=1st, 1=2nd, 2=3rd, 3=4th
static const size_t MP9_RESULT_PLACEMENT_ADDR[4] = { 0x81752558, 0x81752580, 0x817525A8, 0x817525D0 };

/// s32 - The number of mini-stars obtained from the mini-game
static const size_t MP9_RESULT_AWARD_ADDR[4] = { 0x81752560, 0x81752588, 0x817525B0, 0x817525D8 };

/// s32 - The number of mini-stars the player has
static const size_t MP9_MINI_STARS_ADDR[4] = { 0x81752568, 0x81752590, 0x817525B8, 0x817525E0 };

/// u32 - total party points; when this increases the mini-game has finished
static const size_t MP9_PARTY_POINTS_ADDR = 0x81752240;

/// s32 - -1 until the mini-game actually starts. Controls stay on the pointer
/// while this reads -1, then switch to the mini-game's own layout.
static const size_t MP9_MINIGAME_STARTED_ADDR = 0x81752520;

/// u32 - partner index in Bowser Jr. mini-games. Unused for now; noted here in case we ever support those.
static const size_t MP9_BOWSER_JR_PARTNER_ADDR = 0x8175261C;

/* dr_character -> MP9 roster id. MP9 lacks DK/Boo/Toadette/Dry Bones, so those
 * fall back to the nearest available character. */
static int32_t mp9Character(dr_character c)
{
  switch (c)
  {
  case DR_CHARACTER_MARIO:
    return 0x0;
  case DR_CHARACTER_LUIGI:
    return 0x1;
  case DR_CHARACTER_PEACH:
    return 0x2;
  case DR_CHARACTER_DAISY:
    return 0x3;
  case DR_CHARACTER_WARIO:
    return 0x4;
  case DR_CHARACTER_WALUIGI:
    return 0x5;
  case DR_CHARACTER_YOSHI:
    return 0x6;
  case DR_CHARACTER_BIRDO:
    return 0x7;
  case DR_CHARACTER_TOAD:
    return 0x8;
  case DR_CHARACTER_TOADETTE:
    return 0x8; // Toad
  case DR_CHARACTER_KOOPA_KID:
    return 0x9; // Koopa
  case DR_CHARACTER_DRY_BONES:
    return 0x9; // Koopa
  case DR_CHARACTER_BOO:
    return 0xA; // Shy Guy
  case DR_CHARACTER_DONKEY_KONG:
    return 0xB; // Kamek
  default:
    return 0x0;
  }
}

static int32_t mp9Difficulty(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
    return 0;
  case DR_DIFFICULTY_EASY:
    return 1;
  case DR_DIFFICULTY_NORMAL:
    return 2;
  case DR_DIFFICULTY_HARD:
    return 3;
  case DR_DIFFICULTY_VERY_HARD:
    return 4;
  default:
    return 2;
  }
}

/* In-game player slot from a board player's controller port (Mario Party assigns
 * ports non-linearly), falling back to the board index if out of range. */
static unsigned mp9Slot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

static const dr_mp_minigame_t MP9_MINIGAMES[] =
{
  { "Ruins Rumble", DR_MINIGAME_1V3, 0x00, 0, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_SIDEWAYS_MOTION, DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Hazard Hold", DR_MINIGAME_1V3, 0x01, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Line in the Sand", DR_MINIGAME_1V3, 0x02, 0, { DR_QUIRK_BITS_EFB_TO_TEXTURE | DR_WII_CONTROL_SPLIT_BITS(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_SIDEWAYS_BUTTONS) } },
  { "Block and Roll", DR_MINIGAME_1V3, 0x03, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Tackle Takedown", DR_MINIGAME_1V3, 0x04, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Weird Wheels", DR_MINIGAME_1V3, 0x05, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Spike-n-Span", DR_MINIGAME_1V3, 0x06, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Hole Hogs", DR_MINIGAME_1V3, 0x07, 0, { DR_QUIRK_BITS_SAFE_TEXTURE_CACHE | DR_WII_CONTROL_SPLIT_BITS(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_SIDEWAYS_BUTTONS) } },
  { "Pix Fix", DR_MINIGAME_1V3, 0x08, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Mob Sleds", DR_MINIGAME_1V3, 0x09, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  { "Mecha March", DR_MINIGAME_SPECIAL, 0x0A, 0, DR_NO_QUIRKS },
  { "Bowser Pop", DR_MINIGAME_SPECIAL, 0x0B, 0, DR_NO_QUIRKS },
  { "Double Pounder", DR_MINIGAME_SPECIAL, 0x0C, 0, DR_NO_QUIRKS },
  { "Zoom Room", DR_MINIGAME_SPECIAL, 0x0D, 0, DR_NO_QUIRKS },
  { "Cage Match", DR_MINIGAME_SPECIAL, 0x0E, 0, DR_NO_QUIRKS },
  { "Crossfire Caverns", DR_MINIGAME_SPECIAL, 0x0F, 0, DR_NO_QUIRKS },
  { "Bumper Sparks", DR_MINIGAME_SPECIAL, 0x10, 0, DR_NO_QUIRKS },
  { "Sand Trap", DR_MINIGAME_SPECIAL, 0x11, 0, DR_NO_QUIRKS },
  { "Pair of Aces", DR_MINIGAME_SPECIAL, 0x12, 0, DR_NO_QUIRKS },
  { "Pedal to the Paddle", DR_MINIGAME_SPECIAL, 0x13, 0, DR_NO_QUIRKS },

  { "Urn It", DR_MINIGAME_4P, 0x14, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Billistics", DR_MINIGAME_4P, 0x15, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Snow Go", DR_MINIGAME_4P, 0x16, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Skyjinks", DR_MINIGAME_4P, 0x17, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Player Conveyor", DR_MINIGAME_4P, 0x18, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Fungi Frenzy", DR_MINIGAME_4P, 0x19, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Jigsaw Jumble", DR_MINIGAME_4P, 0x1A, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Twist Ending", DR_MINIGAME_4P, 0x1B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Peak Precision", DR_MINIGAME_4P, 0x1C, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Speeding Bullets", DR_MINIGAME_4P, 0x1D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Launch Break", DR_MINIGAME_4P, 0x1E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Polar Extreme", DR_MINIGAME_4P, 0x1F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Logger Heads", DR_MINIGAME_4P, 0x20, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Smash Compactor", DR_MINIGAME_4P, 0x21, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Goomba Bowling", DR_MINIGAME_4P, 0x22, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Pianta Pool", DR_MINIGAME_4P, 0x23, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Bumper Bubbles", DR_MINIGAME_4P, 0x24, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Buddy Bounce", DR_MINIGAME_4P, 0x25, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Pizza Me, Mario", DR_MINIGAME_4P, 0x26, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Chain Event", DR_MINIGAME_4P, 0x27, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Pit or Platter", DR_MINIGAME_4P, 0x28, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Skipping Class", DR_MINIGAME_4P, 0x29, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Flinger Painting", DR_MINIGAME_4P, 0x2A, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Goomba Spotting", DR_MINIGAME_4P, 0x2B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Thwomper Room", DR_MINIGAME_4P, 0x2C, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Ballistic Beach", DR_MINIGAME_4P, 0x2D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Plunder Ground", DR_MINIGAME_4P, 0x2E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Tumble Temple", DR_MINIGAME_4P, 0x2F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Tuber Tug", DR_MINIGAME_4P, 0x30, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) }, /// sucks
  { "Piranha Patch", DR_MINIGAME_4P, 0x31, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Upward Mobility", DR_MINIGAME_4P, 0x32, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Manor of Escape", DR_MINIGAME_4P, 0x33, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Toad and Go Seek", DR_MINIGAME_4P, 0x34, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Goomba Village", DR_MINIGAME_4P, 0x35, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Growing Up", DR_MINIGAME_4P, 0x36, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Card Smarts", DR_MINIGAME_4P, 0x37, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Bomb Barge", DR_MINIGAME_4P, 0x38, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Ring Leader", DR_MINIGAME_4P, 0x39, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Magma Mayhem", DR_MINIGAME_4P, 0x3A, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Don't Look - todo controls", DR_MINIGAME_INVALID, 0x3B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Pinball Fall", DR_MINIGAME_4P, 0x3C, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Pier Pressure", DR_MINIGAME_4P, 0x3D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "10 to Win", DR_MINIGAME_4P, 0x3E, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Mecha Choice", DR_MINIGAME_4P, 0x3F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  { "Castle Clearout", DR_MINIGAME_SPECIAL, 0x40, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  // 0x41 Bowser Jr. Breakdown -- duplicate id; use 0x47 instead
  { "Sock It to Lakitu", DR_MINIGAME_BATTLE, 0x42, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Whomp Stomp", DR_MINIGAME_BATTLE, 0x43, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Deck Dry Bones", DR_MINIGAME_BATTLE, 0x44, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Cheep Cheep Shot", DR_MINIGAME_BATTLE, 0x45, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Spike Strike", DR_MINIGAME_BATTLE, 0x46, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Bowser Jr. Breakdown", DR_MINIGAME_BATTLE, 0x47, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT) },
  { "Diddy's Banana Blast", DR_MINIGAME_4P, 0x48, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Wiggler Bounce", DR_MINIGAME_BATTLE, 0x49, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Bombard King Bob-omb", DR_MINIGAME_BATTLE, 0x4A, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "King Boo's Puzzle Attack", DR_MINIGAME_BATTLE, 0x4B, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Blooper Barrage", DR_MINIGAME_BATTLE, 0x4C, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER) },
  { "Chain Chomp Romp", DR_MINIGAME_BATTLE, 0x4D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Bowser's Block Battle", DR_MINIGAME_BATTLE, 0x4E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "DK's Banana Bonus", DR_MINIGAME_4P, 0x4F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0, DR_NO_QUIRKS },
};

MarioParty9::MarioParty9(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath(dr_core_path(DR_CORE_DOLPHIN).toStdString())
  , m_discPath((dr_roms_directory() + "/Mario Party 9 (USA, Asia) (En,Fr,Es)").toStdString())
  , m_statePath((dr_state_directory() + "/mp9.state.zip").toStdString())
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioParty9::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioParty9::run()
{
  m_retro->tickFrameWrites();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* Hold the pointer layout until the mini-game starts, then swap to its
   * preferred controls. Ignore the value for the first frames so one carried in
   * by the savestate can't swap us immediately. */
  if (!m_controlsApplied && m_minigame && m_minigameFrames >= 60)
  {
    int32_t started = -1;
    if (m_retro->reads32(&started, MP9_MINIGAME_STARTED_ADDR) == DR_OK && started != -1)
    {
      applyControlRemap(m_minigame->quirks, m_players);
      m_controlsApplied = true;
    }
  }

  if (!m_finishScheduled)
  {
    uint32_t points = 0;
    if (m_retro->readu32(&points, MP9_PARTY_POINTS_ADDR) == DR_OK && points > m_partyPointsStart)
    {
      m_finishScheduled = true;
      finishMinigameInFrames(150);
    }
  }
}

const dr_mp_minigame_t *MarioParty9::minigames() const
{
  return MP9_MINIGAMES;
}

void MarioParty9::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_finishScheduled = false;

  m_partyPointsStart = 0;
  m_retro->readu32(&m_partyPointsStart, MP9_PARTY_POINTS_ADDR);

  for (unsigned i = 0; i < 4; i++)
    m_players[i] = data.players[i];

  int32_t id = static_cast<int32_t>(data.minigame->minigame_id);
  m_retro->writes32(id, MP9_MINIGAME_TO_LOAD_ADDR);

  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = mp9Slot(m_players[i], i);

    int32_t chr = mp9Character(m_players[i].character);
    m_retro->writes32(chr, MP9_CHARACTER_ADDR[slot]);

    int32_t diff = mp9Difficulty(m_players[i].difficulty);
    m_retro->writes32(diff, MP9_CPU_DIFFICULTY_ADDR[slot]);

    uint8_t bot = m_players[i].control_type == DR_CONTROL_TYPE_CPU ? 1 : 0;
    m_retro->writeu8(bot, MP9_IS_BOT_ADDR[slot]);

    /* Set "1-vs.-Rivals" teams -- use a continuous write because free play overwrites it */
    int32_t team = m_players[i].team_type == DR_TEAM_TYPE_1V3_GROUP ? 1 : 0;
    m_retro->writeForFrames(MP9_TEAM_ADDR[slot], &team, sizeof(team), 60);
  }

  /* The start button is clicked with the pointer; run() swaps to the mini-game's
   * own layout once MP9_MINIGAME_STARTED_ADDR leaves -1. */
  m_controlsApplied = false;
  applyControlProfile(DR_WII_CONTROL_POINTER);

  startMinigame();
}

dr_minigame_result_t MarioParty9::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index >= 4)
    return result;

  const unsigned slot = mp9Slot(m_players[index], index);

  int32_t place = -1;
  if (m_retro->reads32(&place, MP9_RESULT_PLACEMENT_ADDR[slot]) != DR_OK)
    return result;

  const dr_minigame_type type = m_minigame ? m_minigame->type : DR_MINIGAME_4P;

  /**
   * 4P: 0 is the winner(s)
   * 1-vs.-Rivals: 1 is the winner(s)
   * Battle: Ordered by placement
   */

  if (type == DR_MINIGAME_BATTLE)
    result.coins = place;
  else if (place == (type == DR_MINIGAME_1V3 ? 1 : 0))
    result.coins = 10;

  return result;
}
