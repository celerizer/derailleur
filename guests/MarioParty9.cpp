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

static const dr_mp_minigame_t MP9_MINIGAMES[] =
{
  { "Ruins Rumble", DR_MINIGAME_1V3, 0x00, 0, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_SIDEWAYS_MOTION, DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Hazard Hold", DR_MINIGAME_1V3, 0x01, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Line in the Sand", DR_MINIGAME_1V3, 0x02, 0, { DR_QUIRK_BITS_EFB_TO_TEXTURE | DR_WII_CONTROL_SPLIT_BITS(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_SIDEWAYS_BUTTONS) }, DR_FLAG_NO_NETPLAY },
  { "Block and Roll", DR_MINIGAME_1V3, 0x03, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Tackle Takedown", DR_MINIGAME_1V3, 0x04, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Weird Wheels", DR_MINIGAME_1V3, 0x05, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Spike-n-Span", DR_MINIGAME_1V3, 0x06, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Hole Hogs", DR_MINIGAME_1V3, 0x07, 0, { DR_QUIRK_BITS_SAFE_TEXTURE_CACHE | DR_WII_CONTROL_SPLIT_BITS(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_SIDEWAYS_BUTTONS) }, DR_FLAG_NO_NETPLAY },
  { "Pix Fix", DR_MINIGAME_1V3, 0x08, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Mob Sleds", DR_MINIGAME_1V3, 0x09, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },

  { "Mecha March", DR_MINIGAME_SPECIAL, 0x0A, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bowser Pop", DR_MINIGAME_SPECIAL, 0x0B, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Double Pounder", DR_MINIGAME_SPECIAL, 0x0C, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Zoom Room", DR_MINIGAME_SPECIAL, 0x0D, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cage Match", DR_MINIGAME_SPECIAL, 0x0E, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crossfire Caverns", DR_MINIGAME_SPECIAL, 0x0F, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bumper Sparks", DR_MINIGAME_SPECIAL, 0x10, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sand Trap", DR_MINIGAME_SPECIAL, 0x11, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pair of Aces", DR_MINIGAME_SPECIAL, 0x12, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pedal to the Paddle", DR_MINIGAME_SPECIAL, 0x13, 0, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Urn It", DR_MINIGAME_4P, 0x14, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Billistics", DR_MINIGAME_4P, 0x15, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Snow Go", DR_MINIGAME_4P, 0x16, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Skyjinks", DR_MINIGAME_4P, 0x17, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Player Conveyor", DR_MINIGAME_4P, 0x18, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Fungi Frenzy", DR_MINIGAME_4P, 0x19, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Jigsaw Jumble", DR_MINIGAME_4P, 0x1A, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Twist Ending", DR_MINIGAME_4P, 0x1B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Peak Precision", DR_MINIGAME_4P, 0x1C, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Speeding Bullets", DR_MINIGAME_4P, 0x1D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Launch Break", DR_MINIGAME_4P, 0x1E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Polar Extreme", DR_MINIGAME_4P, 0x1F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Logger Heads", DR_MINIGAME_4P, 0x20, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Smash Compactor", DR_MINIGAME_4P, 0x21, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Goomba Bowling", DR_MINIGAME_4P, 0x22, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Pianta Pool", DR_MINIGAME_4P, 0x23, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Bumper Bubbles", DR_MINIGAME_4P, 0x24, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Buddy Bounce", DR_MINIGAME_4P, 0x25, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Pizza Me, Mario", DR_MINIGAME_4P, 0x26, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Chain Event", DR_MINIGAME_4P, 0x27, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Pit or Platter", DR_MINIGAME_4P, 0x28, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Skipping Class", DR_MINIGAME_4P, 0x29, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Flinger Painting", DR_MINIGAME_4P, 0x2A, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Goomba Spotting", DR_MINIGAME_4P, 0x2B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Thwomper Room", DR_MINIGAME_4P, 0x2C, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Ballistic Beach", DR_MINIGAME_4P, 0x2D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Plunder Ground", DR_MINIGAME_4P, 0x2E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Tumble Temple", DR_MINIGAME_4P, 0x2F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Tuber Tug", DR_MINIGAME_4P, 0x30, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS }, /// sucks
  { "Piranha Patch", DR_MINIGAME_4P, 0x31, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Upward Mobility", DR_MINIGAME_4P, 0x32, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Manor of Escape", DR_MINIGAME_4P, 0x33, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Toad and Go Seek", DR_MINIGAME_4P, 0x34, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Goomba Village", DR_MINIGAME_4P, 0x35, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Growing Up", DR_MINIGAME_4P, 0x36, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Card Smarts", DR_MINIGAME_4P, 0x37, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Bomb Barge", DR_MINIGAME_4P, 0x38, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Ring Leader", DR_MINIGAME_4P, 0x39, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Magma Mayhem", DR_MINIGAME_4P, 0x3A, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Don't Look - todo controls", DR_MINIGAME_INVALID, 0x3B, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Pinball Fall", DR_MINIGAME_4P, 0x3C, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Pier Pressure", DR_MINIGAME_4P, 0x3D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "10 to Win", DR_MINIGAME_4P, 0x3E, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Mecha Choice", DR_MINIGAME_4P, 0x3F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },

  { "Castle Clearout", DR_MINIGAME_SPECIAL, 0x40, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },

  // 0x41 Bowser Jr. Breakdown -- duplicate id; use 0x47 instead
  { "Sock It to Lakitu", DR_MINIGAME_BATTLE, 0x42, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Whomp Stomp", DR_MINIGAME_BATTLE, 0x43, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Deck Dry Bones", DR_MINIGAME_BATTLE, 0x44, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Cheep Cheep Shot", DR_MINIGAME_BATTLE, 0x45, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Spike Strike", DR_MINIGAME_BATTLE, 0x46, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Bowser Jr. Breakdown", DR_MINIGAME_BATTLE, 0x47, 0, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Diddy's Banana Blast", DR_MINIGAME_4P, 0x48, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Wiggler Bounce", DR_MINIGAME_BATTLE, 0x49, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Bombard King Bob-omb", DR_MINIGAME_BATTLE, 0x4A, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "King Boo's Puzzle Attack", DR_MINIGAME_BATTLE, 0x4B, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Blooper Barrage", DR_MINIGAME_BATTLE, 0x4C, 0, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Chain Chomp Romp", DR_MINIGAME_BATTLE, 0x4D, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Bowser's Block Battle", DR_MINIGAME_BATTLE, 0x4E, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "DK's Banana Bonus", DR_MINIGAME_4P, 0x4F, 0, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0, DR_NO_QUIRKS, DR_NO_FLAGS },
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
  {
    if (++m_minigameFrames >= 48)
    {
      m_minigameFrames = 0;
      startMinigame();
    }
    return;
  }

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
      if (dr_netplay_active())
        m_resyncCountdown = 180;
    }
  }

  /* Hold the resync ~3s past the start so it lands on the running mini-game
   * rather than the frame the controls swap on. */
  if (m_resyncCountdown > 0 && --m_resyncCountdown == 0)
    emit hardResyncRequested();

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
  m_resyncCountdown = 0;

  m_partyPointsStart = 0;
  m_retro->readu32(&m_partyPointsStart, MP9_PARTY_POINTS_ADDR);

  int32_t id = static_cast<int32_t>(data.minigame->minigame_id);
  m_retro->writes32(id, MP9_MINIGAME_TO_LOAD_ADDR);

  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(m_players[i], i);

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
}

dr_minigame_result_t MarioParty9::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index >= 4)
    return result;

  const unsigned slot = dr_player_slot(m_players[index], index);

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
