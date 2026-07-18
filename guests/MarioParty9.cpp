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
  { "Ruins Rumble", DR_MINIGAME_1V3, 0x00, MP9_CONTROL_SPLIT(MP9_CONTROL_SIDEWAYS_MOTION, MP9_CONTROL_SIDEWAYS), DR_NO_QUIRKS },
  { "Hazard Hold", DR_MINIGAME_1V3, 0x01, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Line in the Sand", DR_MINIGAME_1V3, 0x02, MP9_CONTROL_SPLIT(MP9_CONTROL_POINTER, MP9_CONTROL_SIDEWAYS), DR_NO_QUIRKS }, // non-functional. needs something
  { "Block and Roll", DR_MINIGAME_1V3, 0x03, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Tackle Takedown", DR_MINIGAME_1V3, 0x04, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Weird Wheels", DR_MINIGAME_1V3, 0x05, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Spike-n-Span", DR_MINIGAME_1V3, 0x06, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Hole Hogs", DR_MINIGAME_1V3, 0x07, MP9_CONTROL_SPLIT(MP9_CONTROL_POINTER, MP9_CONTROL_SIDEWAYS), DR_NO_QUIRKS },
  { "Pix Fix", DR_MINIGAME_1V3, 0x08, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Mob Sleds", DR_MINIGAME_1V3, 0x09, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },

  { "Mecha March", DR_MINIGAME_INVALID, 0x0A, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Bowser Pop", DR_MINIGAME_INVALID, 0x0B, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Double Pounder", DR_MINIGAME_INVALID, 0x0C, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Zoom Room", DR_MINIGAME_INVALID, 0x0D, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Cage Match", DR_MINIGAME_INVALID, 0x0E, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Crossfire Caverns", DR_MINIGAME_INVALID, 0x0F, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Bumper Sparks", DR_MINIGAME_INVALID, 0x10, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Sand Trap", DR_MINIGAME_INVALID, 0x11, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Pair of Aces", DR_MINIGAME_INVALID, 0x12, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
  { "Pedal to the Paddle", DR_MINIGAME_INVALID, 0x13, MP9_CONTROL_INVALID, DR_NO_QUIRKS },

  { "Urn It", DR_MINIGAME_4P, 0x14, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Billistics", DR_MINIGAME_4P, 0x15, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Snow Go", DR_MINIGAME_4P, 0x16, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Skyjinks", DR_MINIGAME_4P, 0x17, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Player Conveyor", DR_MINIGAME_4P, 0x18, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Fungi Frenzy", DR_MINIGAME_4P, 0x19, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Jigsaw Jumble", DR_MINIGAME_4P, 0x1A, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  // { "Twist Ending", DR_MINIGAME_4P, 0x1B, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS }, /// @todo requires rotation, nearly unplayable
  { "Peak Precision", DR_MINIGAME_4P, 0x1C, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Speeding Bullets", DR_MINIGAME_4P, 0x1D, MP9_CONTROL_SIDEWAYS_MOTION, DR_NO_QUIRKS }, /// @todo sideways motion
  { "Launch Break", DR_MINIGAME_4P, 0x1E, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Polar Extreme", DR_MINIGAME_4P, 0x1F, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Logger Heads", DR_MINIGAME_4P, 0x20, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  { "Smash Compactor", DR_MINIGAME_4P, 0x21, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Goomba Bowling", DR_MINIGAME_4P, 0x22, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS }, /// @todo swing
  // { "Pianta Pool", DR_MINIGAME_4P, 0x23, MP9_CONTROL_SIDEWAYS_MOTION, DR_NO_QUIRKS }, /// @todo sideways motion
  { "Bumper Bubbles", DR_MINIGAME_4P, 0x24, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Buddy Bounce", DR_MINIGAME_4P, 0x25, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Pizza Me, Mario", DR_MINIGAME_4P, 0x26, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  { "Chain Event", DR_MINIGAME_4P, 0x27, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  // { "Pit or Platter", DR_MINIGAME_4P, 0x28, MP9_CONTROL_SIDEWAYS_MOTION, DR_NO_QUIRKS }, /// @todo sideways motion
  { "Skipping Class", DR_MINIGAME_4P, 0x29, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Flinger Painting", DR_MINIGAME_4P, 0x2A, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Goomba Spotting", DR_MINIGAME_4P, 0x2B, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  { "Thwomper Room", DR_MINIGAME_4P, 0x2C, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Ballistic Beach", DR_MINIGAME_4P, 0x2D, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Plunder Ground", DR_MINIGAME_4P, 0x2E, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Tumble Temple", DR_MINIGAME_4P, 0x2F, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Tuber Tug", DR_MINIGAME_4P, 0x30, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS }, /// sucks
  { "Piranha Patch", DR_MINIGAME_4P, 0x31, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Upward Mobility", DR_MINIGAME_4P, 0x32, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Manor of Escape", DR_MINIGAME_4P, 0x33, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Toad and Go Seek", DR_MINIGAME_4P, 0x34, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Goomba Village", DR_MINIGAME_4P, 0x35, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Growing Up", DR_MINIGAME_4P, 0x36, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Card Smarts", DR_MINIGAME_4P, 0x37, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Bomb Barge", DR_MINIGAME_4P, 0x38, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Ring Leader", DR_MINIGAME_4P, 0x39, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS }, /// swing?
  { "Magma Mayhem", DR_MINIGAME_4P, 0x3A, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  // { "Don't Look", DR_MINIGAME_4P, 0x3B, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS }, /// swing
  { "Pinball Fall", DR_MINIGAME_4P, 0x3C, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Pier Pressure", DR_MINIGAME_4P, 0x3D, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "10 to Win", DR_MINIGAME_4P, 0x3E, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Mecha Choice", DR_MINIGAME_4P, 0x3F, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },

  { "Castle Clearout", DR_MINIGAME_DUEL, 0x40, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },

  // 0x41 Bowser Jr. Breakdown -- duplicate id; use 0x47 instead
  { "Sock It to Lakitu", DR_MINIGAME_BATTLE, 0x42, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Whomp Stomp", DR_MINIGAME_BATTLE, 0x43, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  { "Deck Dry Bones", DR_MINIGAME_BATTLE, 0x44, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  // { "Cheep Cheep Shot", DR_MINIGAME_BATTLE, 0x45, MP9_CONTROL_SIDEWAYS_MOTION, DR_NO_QUIRKS }, /// @todo 
  { "Spike Strike", DR_MINIGAME_BATTLE, 0x46, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Bowser Jr. Breakdown", DR_MINIGAME_BATTLE, 0x47, MP9_CONTROL_UPRIGHT, DR_NO_QUIRKS },
  { "Diddy's Banana Blast", DR_MINIGAME_4P, 0x48, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Wiggler Bounce", DR_MINIGAME_BATTLE, 0x49, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Bombard King Bob-omb", DR_MINIGAME_BATTLE, 0x4A, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "King Boo's Puzzle Attack", DR_MINIGAME_BATTLE, 0x4B, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Blooper Barrage", DR_MINIGAME_BATTLE, 0x4C, MP9_CONTROL_POINTER, DR_NO_QUIRKS },
  { "Chain Chomp Romp", DR_MINIGAME_BATTLE, 0x4D, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "Bowser's Block Battle", DR_MINIGAME_BATTLE, 0x4E, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },
  { "DK's Banana Bonus", DR_MINIGAME_4P, 0x4F, MP9_CONTROL_SIDEWAYS, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, MP9_CONTROL_INVALID, DR_NO_QUIRKS },
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

  applyRemap(data.minigame->scene_id);

  startMinigame();
}

/* Applies one control layout to a single player's joypad. */
static void mp9ApplyControl(QRetroInputJoypad &jp, Mp9Control control)
{
  /* Remove existing remaps */
  jp.clearButtonRemaps();
  jp.clearAnalogStickRemaps();

  switch (control)
  {
  case MP9_CONTROL_INVALID:
  case MP9_CONTROL_UPRIGHT:
    /* Held Wii Remote: the stick drives the d-pad. (Invalid falls back here.) */
    jp.setAnalogStickToDigitalPad(true);
    break;
  case MP9_CONTROL_SIDEWAYS:
    /* 2 = A, 1 = B, flip dpad 90 degrees left */
    jp.setAnalogStickToDigitalPad(true);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_Y);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_A);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_B);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_UP);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_DOWN);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    break;
  case MP9_CONTROL_SIDEWAYS_MOTION:
    jp.setAnalogStickToDigitalPad(false);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_Y);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_A);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_B);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_UP);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_DOWN);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    break;
  case MP9_CONTROL_POINTER:
    /* Pointer aiming: keep the stick analog, and feed the core's left stick
     * (the pointer) from the physical right stick. */
    jp.setAnalogStickToDigitalPad(false);
    jp.setAnalogStickRemap(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_INDEX_ANALOG_LEFT);
    break;
  }
}

void MarioParty9::applyRemap(int control)
{
  QRetro *retro = m_retro ? m_retro->core() : nullptr;
  if (!retro || !retro->input())
    return;

  /* The control slot packs two layouts: the solo player uses the low byte, the
   * trio uses the high byte. A zero high byte means everyone shares the low
   * byte's control (all non-split mini-games). */
  const Mp9Control solo = static_cast<Mp9Control>(control & 0xFF);
  Mp9Control team = static_cast<Mp9Control>((control >> 8) & 0xFF);
  if (team == MP9_CONTROL_INVALID)
    team = solo;

  QRetroInputJoypad *joypads = retro->input()->joypads();
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = mp9Slot(m_players[i], i);
    const bool isSolo = m_players[i].team_type == DR_TEAM_TYPE_1V3_SOLO;
    mp9ApplyControl(joypads[slot], isSolo ? solo : team);
  }
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
