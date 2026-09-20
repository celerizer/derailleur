#include "MarioPartyDSHost.h"

#include <asm/mpds.h>

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>

#include <cstddef>
#include <cstring>

/* ------------------------------------------------------------------------- *
 *  Input injection. Four 0x26-byte InputRecords sit end-to-end at 0x020CCCC0,
 *  one per port, with a separate array of accept flags at 0x020CCD7E:
 *
 *    typedef struct {
 *      u16 buttons;       // 0x00  1 = pressed, NitroSDK PAD_* layout
 *      u8  touchX;        // 0x02  0-255
 *      u8  touchY;        // 0x03  0-191
 *      u16 touch    : 1;  // 0x04  bit 0: pen down
 *      u16 validity : 2;  //       bits 1-2: TP validity, must be 0 to be accepted
 *      u16 flag3    : 1;  //       bit 3: OR'd into a bitmask at 0x020A818D, use 0
 *      u16 unk4     : 4;
 *      u16 aidMask  : 4;  //       bits 8-11: connected-AID bitmap (slot 0 only)
 *      u16 unk12    : 4;
 *      u8  payload[0x20]; // 0x06  sync data, ignored by the input code
 *    } InputRecord;       // 0x26
 * ------------------------------------------------------------------------- */

/* One port's InputRecord fields, plus its accept flag -- which lives in its own
 * array rather than in the record, but is per-port all the same. */
typedef struct
{
  dr_value_t buttons;
  dr_value_t touch_x;
  dr_value_t touch_y;
  dr_value_t flags;
  dr_value_t valid;
} mpds_input_record_t;

/* Real vaddr from physical offsets */
#define MPDS_RAM_BASE 0x02000000

#define MPDS_INPUT_RECORD(record, flag) \
  { { (record) - MPDS_RAM_BASE + 0x00, DR_VALUE_TYPE_U16 }, \
    { (record) - MPDS_RAM_BASE + 0x02, DR_VALUE_TYPE_U8 }, \
    { (record) - MPDS_RAM_BASE + 0x03, DR_VALUE_TYPE_U8 }, \
    { (record) - MPDS_RAM_BASE + 0x04, DR_VALUE_TYPE_U16 }, \
    { (flag) - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 } }

/* Port 0 is listed for completeness only: the game refills it from the real DS
 * every frame, so anything written there is clobbered. */
static const mpds_input_record_t MPDS_INPUT[4] = {
  MPDS_INPUT_RECORD(0x020CCCC0, 0x020CCD7E),
  MPDS_INPUT_RECORD(0x020CCCE6, 0x020CCD7F),
  MPDS_INPUT_RECORD(0x020CCD0C, 0x020CCD80),
  MPDS_INPUT_RECORD(0x020CCD32, 0x020CCD81),
};

/* Report 4 input devices */
static const unsigned MPDS_PORTS = 4;

/* touchX, touchY and the flags word are contiguous at record + 0x02, so the pen
 * state copies as one block. */
static const size_t MPDS_TOUCH_BYTES = 4;

/* The pad state the game actually reads. func_01ffc1e4 builds one of these per
 * frame out of a button word:
 *
 *   typedef struct {
 *     u16 held;          // 0x00  buttons down this frame
 *     u16 pressed;       // 0x02  held & ~held_last_frame
 *     u16 released;      // 0x04  let go this frame
 *     u16 repeat;        // 0x06  pressed, plus auto-repeat retriggers
 *     u16 delay[12];     // 0x08  frames until the next retrigger, per button
 *     u16 count[12];     // 0x20  frames the button has been held, per button
 *   } mpds_pad_t;        // 0x38
 */
static const size_t MPDS_PAD_STRIDE = 0x38;
static const size_t MPDS_PAD_TABLE = 0x020C8298 - MPDS_RAM_BASE;
static const size_t MPDS_SYSTEM_PAD = 0x020C81E0 - MPDS_RAM_BASE;

/* Likewise the pen: one system block and four per-port, 0x20 each. */
static const size_t MPDS_TOUCH_STRIDE = 0x20;
static const size_t MPDS_TOUCH_TABLE = 0x020C8218 - MPDS_RAM_BASE;
static const size_t MPDS_SYSTEM_TOUCH = 0x020C819C - MPDS_RAM_BASE;

static const unsigned MPDS_PAD_BUTTONS = 12;
static const uint16_t MPDS_REPEAT_FIRST = 0x18;
static const uint16_t MPDS_REPEAT_SECOND = 0x10;
static const uint16_t MPDS_REPEAT_FASTEST = 4;

static const struct
{
  unsigned id;
  uint16_t bit;
} MPDS_BUTTONS[] = {
  { RETRO_DEVICE_ID_JOYPAD_A, 0x001 },
  { RETRO_DEVICE_ID_JOYPAD_B, 0x002 },
  { RETRO_DEVICE_ID_JOYPAD_SELECT, 0x004 },
  { RETRO_DEVICE_ID_JOYPAD_START, 0x008 },
  { RETRO_DEVICE_ID_JOYPAD_RIGHT, 0x010 },
  { RETRO_DEVICE_ID_JOYPAD_LEFT, 0x020 },
  { RETRO_DEVICE_ID_JOYPAD_UP, 0x040 },
  { RETRO_DEVICE_ID_JOYPAD_DOWN, 0x080 },
  { RETRO_DEVICE_ID_JOYPAD_R, 0x100 },
  { RETRO_DEVICE_ID_JOYPAD_L, 0x200 },
  { RETRO_DEVICE_ID_JOYPAD_X, 0x400 },
  { RETRO_DEVICE_ID_JOYPAD_Y, 0x800 },
};

/* Scene manager (data_020c1900). */
static const dr_value_t MPDS_SCENE_CURRENT = { 0x020C1C2F - MPDS_RAM_BASE, DR_VALUE_TYPE_U8 };
static const dr_value_t MPDS_SCENE_NEXT = { 0x020C1C2D - MPDS_RAM_BASE, DR_VALUE_TYPE_U8 };
static const dr_value_t MPDS_SCENE_RETURN = { 0x020C1C2E - MPDS_RAM_BASE, DR_VALUE_TYPE_U8 };

#define MPDS_SCENE_RESULTS 0x2a
#define MPDS_SCENE_BATTLE_RESULTS 0x2b

static const size_t MPDS_HOST_STATE_ADDR = MPDS_HOST_STATE - MPDS_RAM_BASE;
static const size_t MPDS_TITLE_BLOCK_ADDR = MPDS_TITLE_BLOCK - MPDS_RAM_BASE;
static const size_t MPDS_COLOUR_BLOCK_ADDR = MPDS_COLOUR_BLOCK - MPDS_RAM_BASE;

/* Titles are UTF-16LE: 31 characters and a NUL per 64-byte slot. The widest DS
 * roulette offers four. */
#define MPDS_TITLE_SLOT_SIZE 64
#define MPDS_TITLE_SLOTS 4

/* host_state.reserved: pub_slot writes the landed slot, 1-based, into the low
 * bits; skip_ret ORs this in when the board hands the mini-game off. */
#define MPDS_HANDOFF 0x80

/* A duel space's pair, set just before its roulette (func_0204b680). */
static const size_t MPDS_DUEL_CHALLENGER = 0x0214D530 - MPDS_RAM_BASE; /* s8 */
static const size_t MPDS_DUEL_OPPONENT = 0x0214D531 - MPDS_RAM_BASE;   /* s8 */

/* The ARM9 binary ships compressed and unpacks itself over its own load
 * address during boot, so a write into the cave or a hook site before then is
 * trampled -- or lands in the compressed stream and crashes the game.
 *
 * Keep this as low as the unpack allows. melonDS's JIT compiles a block the
 * first time it runs and does not recompile it when the frontend writes over
 * the code, so a hook stamped after the game has executed that block is
 * ignored until the block cache is flushed -- which is what loading a savestate
 * does, and why loading any state, even a broken one, repairs the input. */
#define MPDS_WARMUP_FRAMES 10

/* Native roulette type -> dr_minigame_type. Boss and puzzle never roll on the
 * party board. */
static const dr_minigame_type MPDS_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P,      /* 0 */
  DR_MINIGAME_1V3,     /* 1 */
  DR_MINIGAME_2V2,     /* 2 */
  DR_MINIGAME_BATTLE,  /* 3 */
  DR_MINIGAME_DUEL,    /* 4, the duel rule */
  DR_MINIGAME_INVALID, /* 5 boss */
  DR_MINIGAME_INVALID, /* 6 puzzle */
};

/* Which of the three 0x234-byte player contexts is live: mode = (word >> 7) & 7. */
static const dr_value_t MPDS_GAME_MODE_WORD = { 0x0214CD74 - MPDS_RAM_BASE, DR_VALUE_TYPE_U32 };
static const size_t MPDS_CTX_STORY = 0x0214CD88 - MPDS_RAM_BASE; /* mode 0 */
static const size_t MPDS_CTX_PARTY = 0x0214CFBC - MPDS_RAM_BASE; /* mode 3 */
#define MPDS_MODE_STORY 0
#define MPDS_MODE_EXTRAS 1 /* Pen Pals and Desert Duel */
#define MPDS_MODE_PARTY 3
static const size_t MPDS_CTX_OTHER = 0x0214D1F0 - MPDS_RAM_BASE; /* modes 1/2/4 */

/* Offsets within a context. */
#define MPDS_CTX_TURN_OWNER 0x00  /* s8 */
#define MPDS_CTX_TURN_CUR 0x07    /* s8, capped at 30 */
#define MPDS_CTX_TURN_TOTAL 0x08  /* u8, capped at 30 */
#define MPDS_CTX_RULES 0x10       /* u32: rule = (v >> 16) & 0x1f */
#define MPDS_CTX_TEAM_LEADER 0x20 /* s8[2] */
#define MPDS_CTX_PLAYERS 0x24     /* player record[4] */
#define MPDS_PLAYER_STRIDE 0x74

/* Offsets within a player record. */
#define MPDS_PL_FLAGS 0x00     /* u32 bitfield, see the accessors below */
#define MPDS_PL_MG_WINS 0x0B   /* s8 */
#define MPDS_PL_COINS 0x0E     /* s16, 0..999 */
#define MPDS_PL_MG_RESULT 0x10 /* s16, the board's mini-game payout */
#define MPDS_PL_STARS 0x12     /* s16, 0..99 */
#define MPDS_PL_SPACE 0x6E     /* s8 */

/* MPDS_PL_FLAGS accessors. */
#define MPDS_PL_CHARACTER(v) ((v) & 7)
#define MPDS_PL_CPU(v) (((v) >> 3) & 1)
#define MPDS_PL_MG_TEAM(v) (((v) >> 9) & 1) /* 1v3: 0 solo, 1 group; 2v2: 0 or 1 */
#define MPDS_PL_DIFFICULTY(v) (((v) >> 4) & 3) /* [I] */
#define MPDS_PL_DIFFICULTY_SET(v, d) (((v) & ~(3u << 4)) | (((d) & 3u) << 4))
#define MPDS_PL_CHARACTER_SET(v, c) (((v) & ~7u) | ((c) & 7u))
#define MPDS_PL_CPU_SET(v, b) (((v) & ~(1u << 3)) | (((b) & 1u) << 3))

/* Rule, from MPDS_CTX_RULES. In team rule the coins/stars a slot plays for live
 * on the team leader's record, not on its own -- see mpds_record(). */
#define MPDS_RULE(v) (((v) >> 16) & 0x1f)
#define MPDS_RULE_NORMAL 0
#define MPDS_RULE_TEAM 1
#define MPDS_RULE_DUEL 2

/* Mini-game selection. */
static const dr_value_t MPDS_ROULETTE_TYPE = { 0x020CD37C - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const dr_value_t MPDS_ROULETTE_ID = { 0x020CD38A - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const dr_value_t MPDS_ROULETTE_SLOT = { 0x020CD38B - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const size_t MPDS_ROULETTE_LIST = 0x020CD38C - MPDS_RAM_BASE; /* s8[4] */
static const dr_value_t MPDS_MINIGAME_ID = { 0x020AAD4C - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const dr_value_t MPDS_SESSION_TYPE = { 0x020CD31E - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const dr_value_t MPDS_PLAYER_COUNT = { 0x020AA948 - MPDS_RAM_BASE, DR_VALUE_TYPE_S8 };
static const dr_value_t MPDS_BATTLE_POT = { 0x020CD304 - MPDS_RAM_BASE, DR_VALUE_TYPE_S16 };

/* Mini-game type, as the board's roulette picks it (func_020318f8). */
#define MPDS_TYPE_4P 0
#define MPDS_TYPE_1V3 1
#define MPDS_TYPE_2V2 2
#define MPDS_TYPE_BATTLE 3
#define MPDS_TYPE_DUEL 4
#define MPDS_TYPE_BOSS 5
#define MPDS_TYPE_PUZZLE 6

typedef enum
{
  MPDS_FONT_BLACK = 0,
  MPDS_FONT_WHITE = 1,
  MPDS_FONT_RED = 2,
  MPDS_FONT_BLUE = 3,
  MPDS_FONT_GREEN = 4,
  MPDS_FONT_ORANGE = 5,
  MPDS_FONT_PURPLE = 6,
  MPDS_FONT_PINK = 7,
  MPDS_FONT_DARK_GRAY = 8,
  MPDS_FONT_LIGHT_GRAY = 9
} mpds_font_color;

/* RNG: s = s * 0x123967 + 0x1E43F, draw returns (s >> 16) & 0x7fff. */
static const dr_value_t MPDS_RNG_STATE = { 0x020C94B4 - MPDS_RAM_BASE, DR_VALUE_TYPE_U32 };

/* Scene ids. The mini-game scenes double as the mini-game list: a mini-game's
 * index is its scene id - 0x2c (see MPDS_MINIGAME_ID). */
static const dr_scene_name_t MPDS_SCENE_NAMES[] =
{
  { 0x00, "Debug scene select", true },
  { 0x01, "Debug mini-game select", true },
  { 0x25, "Booting up", true },
  { 0x26, "Wireless", true },
  { 0x27, "Wireless", true },
  { 0x28, "Wireless", true },
  { 0x29, "Mini-Game explanation", true },
  { 0x2a, "Mini-Game results", true },
  { 0x2b, "Battle results", true },

  { 0x2d, "Goomba Wrangler", false },
  { 0x2e, "Rail Riders", false },
  { 0x2f, "Dress for Success", false },
  { 0x30, "Camera Shy", false },
  { 0x31, "Hedge Honcho", false },
  { 0x32, "Study Fall", false },
  { 0x33, "Domino Effect", false },
  { 0x34, "Cherry-Go-Round", false },
  { 0x36, "Trace Cadets", false },
  { 0x37, "Soccer Survival", false },
  { 0x38, "Hot Shots", false },
  { 0x39, "Call of the Goomba", false },
  { 0x3a, "Pedal Pushers", false },
  { 0x3b, "Roller Coasters", false },
  { 0x3d, "Get the Lead Out", false },
  { 0x3e, "Shortcut Circuit", false },
  { 0x3f, "Big Blowout", false },
  { 0x40, "Trash Landing", false },
  { 0x41, "Cheep Cheep Chance", false },
  { 0x42, "Whomp-a-thon", false },
  { 0x43, "Twist and Route", false },
  { 0x44, "Crater Crawl", false },
  { 0x45, "Boogie Beam", false },
  { 0x46, "Parachutin' Gallery", false },
  { 0x47, "Boo Tag", false },
  { 0x48, "Dust Buddies", false },
  { 0x49, "Cyber Scamper", false },
  { 0x4a, "Soap Surfers", false },
  { 0x4b, "Sweet Sleuth", false },
  { 0x4c, "Tidal Fools", false },
  { 0x4d, "Raft Riot", false },
  { 0x4e, "All Geared Up", false },
  { 0x4f, "Power Washer", false },
  { 0x50, "Peek-a-Boo", false },
  { 0x51, "Fast Food Frenzy", false },
  { 0x52, "Track Star", false },
  { 0x53, "Shuffleboard Showdown", false },
  { 0x54, "Flash and Dash", false },
  { 0x55, "Rubber Ducky Rodeo", false },
  { 0x56, "Plush Crush", false },
  { 0x57, "Rotisserie Rampage", false },
  { 0x58, "Nothing to Luge", false },
  { 0x59, "Penny Pinchers", false },
  { 0x5a, "Gusty Blizzard", false },
  { 0x5b, "Soil Toil", false },
  { 0x5c, "Double Vision", false },
  { 0x5d, "Memory Mash", false },
  { 0x5e, "Cube Crushers", false },
  { 0x60, "Mole Thrill", false },
  { 0x61, "Sprinkler Scalers", false },
  { 0x62, "Cucumberjacks", false },
  { 0x63, "Hanger Management", false },
  { 0x64, "Book It!", false },
  { 0x65, "Airbrushers", false },
  { 0x66, "Toppling Terror", false },
  { 0x67, "Crazy Crosshairs", false },
  { 0x68, "Shorty Scorers", false },
  { 0x69, "Cheep Chump", false },
  { 0x6a, "Star Catchers", false },
  { 0x6c, "Short Fuse", false },
  { 0x6d, "Globe Gunners", false },
  { 0x6e, "Chips and Dips", false },
  { 0x6f, "Feed and Seed", false },
  { 0x70, "Hammer Chime", false },
  { 0x71, "Hexoskeleton", false },
  { 0x72, "Book Bash", false },
  { 0x73, "Bowser's Block Party", false },
  { 0x74, "Mario's Puzzle Party", false },
  { 0x75, "Bob-omb Breakers", false },
  { 0x76, "Piece Out", false },
  { 0x77, "Block Star", false },
  { 0x78, "Stick & Spin", false },
  { 0x79, "Triangle Twisters", false },

  { 0x7a, "Tutorial board", false },
  { 0x7b, "Wiggler's Garden", false },
  { 0x7c, "Toadette's Music Room", false },
  { 0x7d, "DK's Stone Statue", false },
  { 0x7e, "Kamek's Library", false },
  { 0x7f, "Bowser's Pinball Machine", false },
  { 0x80, "Pen Pals", false },
  { 0x81, "Desert Duel", false },
  { 0x82, "Mini-Game bridge", true },
  { 0x83, "Logo", true },
  { 0x84, "Title screen", true },
  { 0x85, "Title screen", true },
  { 0x86, "Data select", false },
  { 0x87, "Main menu", false },
  { 0x88, "Mode select", false },
  { 0x89, "Download play", true },
  { 0x8a, "Party mode setup", false },
  { 0x8b, "Party results", true },
  { 0x8c, "Mini-Game mode", false },
  { 0x8d, "Free Play", false },
  { 0x8e, "Mini-Game Tournament", false },
  { 0x8f, "Battle Cup", false },
  { 0x90, "Score Attack", false },
  { 0x91, "Time Attack", false },
  { 0x92, "Unknown (ov097)", false },
  { 0x93, "Story mode", false },
  { 0x94, "Extra mode", false },
  { 0x95, "Puzzle mode", false },
  { 0x96, "Gallery", false },
  { 0x97, "Staff roll", true },
  { 0x98, "Collection results", true },
  { 0x99, "Mic test", true },
  { 0x9a, "Story board map", false },
  { 0x9b, "Boss opening", true },
  { 0x9c, "Story event", true },

  { -1, nullptr, false },
};

static const dr_character MPDS_CHARACTERS[8] =
{
  DR_CHARACTER_MARIO, // 0
  DR_CHARACTER_LUIGI, // 1
  DR_CHARACTER_WARIO, // 2
  DR_CHARACTER_YOSHI, // 3
  DR_CHARACTER_PEACH, // 4
  DR_CHARACTER_DAISY, // 5
  DR_CHARACTER_WALUIGI, // 6
  DR_CHARACTER_TOAD, // 7
};

static const dr_difficulty MPDS_DIFFICULTIES[4] =
{
  DR_DIFFICULTY_EASY,
  DR_DIFFICULTY_NORMAL,
  DR_DIFFICULTY_HARD,
  DR_DIFFICULTY_VERY_HARD,
};

static int mpds_character_id(dr_character character)
{
  int i;

  for (i = 0; i < 8; i++)
    if (MPDS_CHARACTERS[i] == character)
      return i;

  return -1;
}

static int mpds_difficulty_id(dr_difficulty difficulty)
{
  int i;

  for (i = 0; i < 4; i++)
    if (MPDS_DIFFICULTIES[i] == difficulty)
      return i;

  return -1;
}

MarioPartyDSHost::MarioPartyDSHost(QObject *parent)
  : DrHost(parent)
{
  const QString corePath = dr_core_path(DR_CORE_MELONDSDS);

  m_gamePath = dr_roms_directory() + "/Mario Party DS (USA) (Rev 2).nds";

  m_core = new QRetro();
  m_ownCore = true;

  /* Hint to use up to 4 controllers */
  m_core->input()->setMaxUsers(MPDS_PORTS);

  /* Video settings */
  m_core->setBilinearFilter(false);

  /* Global remaps */
  for (unsigned port = 0; port < MPDS_PORTS; port++)
  {
    /* Move touch cursor = left stick */
    m_core->input()->joypads()[port].setAnalogStickRemap(
      RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_INDEX_ANALOG_LEFT);

    /* Touch = R2 */
    m_core->input()->remapButton(port,
      RETRO_DEVICE_ID_JOYPAD_R2, RETRO_DEVICE_ID_JOYPAD_R3);

    /* Disable closing the DS lid */
    m_core->input()->remapButton(port,
      RETRO_DEVICE_ID_JOYPAD_NONE, RETRO_DEVICE_ID_JOYPAD_L3);
  }

  if (!m_core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
    return;
  }

  m_core->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  if (!m_core->loadContent(m_gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(m_gamePath)));
    m_valid = false;
    return;
  }

  /* After loadContent, so this lands on top of whatever the core declared during
   * its own init rather than being overwritten by it. */
  m_core->input()->setControllerInfo(MPDS_CONTROLLER_INFO);

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);

  connect(m_core, &QRetro::frameBegin, this, [this]() { cloneStick(); },
    Qt::DirectConnection);
}

int MarioPartyDSHost::gameMode(void)
{
  int64_t word = 0;

  if (readValue(&word, MPDS_GAME_MODE_WORD) != DR_OK)
    return -1;

  return static_cast<int>((static_cast<unsigned>(word) >> 7) & 7);
}

size_t MarioPartyDSHost::context(void)
{
  /* Story is mode 0, party mode 3; 1/2/4 (extra, puzzle, mini-game) share one
   * context. Anything else is a mode we have not seen set a context. */
  switch (gameMode())
  {
  case 0:
    return MPDS_CTX_STORY;
  case 1:
  case 2:
  case 4:
    return MPDS_CTX_OTHER;
  case 3:
    return MPDS_CTX_PARTY;
  default:
    return 0;
  }
}

size_t MarioPartyDSHost::record(size_t ctx, unsigned slot)
{
  uint32_t rules = 0;

  if (!ctx || slot > 3)
    return 0;

  /* Under the team rule a slot's coins and stars live on its team leader's
   * record rather than its own, so redirect there. Everything else (character,
   * mini-game wins) stays per-slot, so callers wanting those index the context
   * directly instead of coming through here. */
  readu32(&rules, ctx + MPDS_CTX_RULES);
  if (MPDS_RULE(rules) == MPDS_RULE_TEAM)
  {
    uint32_t flags = 0;
    int8_t leader = 0;

    /* Which of the two leaders this slot banks through, from its own team bit. */
    readu32(&flags, ctx + MPDS_CTX_PLAYERS + slot * MPDS_PLAYER_STRIDE + MPDS_PL_FLAGS);
    reads8(&leader, ctx + MPDS_CTX_TEAM_LEADER + ((flags >> 8) & 1));
    if (leader >= 0 && leader <= 3)
      slot = static_cast<unsigned>(leader);
  }

  return ctx + MPDS_CTX_PLAYERS + slot * MPDS_PLAYER_STRIDE;
}

uint32_t MarioPartyDSHost::rngValue(void)
{
  int64_t state = 0;

  readValue(&state, MPDS_RNG_STATE);

  return static_cast<uint32_t>(state);
}

unsigned MarioPartyDSHost::battlePot(void)
{
  int64_t pot = 0;

  readValue(&pot, MPDS_BATTLE_POT);

  return pot > 0 ? static_cast<unsigned>(pot) : 0;
}

void MarioPartyDSHost::setCurrentTurn(unsigned turn)
{
  const size_t ctx = context();

  /* The board caps both counters at 30. */
  if (ctx)
    writes8(static_cast<int8_t>(turn > 30 ? 30 : turn), ctx + MPDS_CTX_TURN_CUR);
}

bool MarioPartyDSHost::readPlayerSetup(DrPlayerArray &players)
{
  const size_t ctx = context();
  unsigned i;

  if (!ctx)
    return false;

  for (i = 0; i < 4; i++)
  {
    const size_t rec = ctx + MPDS_CTX_PLAYERS + i * MPDS_PLAYER_STRIDE;
    uint32_t flags = 0;
    int16_t coins = 0, stars = 0;

    readu32(&flags, rec + MPDS_PL_FLAGS);
    reads16(&coins, rec + MPDS_PL_COINS);
    reads16(&stars, rec + MPDS_PL_STARS);

    dr_player_t &p = players[i];
    p.character = MPDS_CHARACTERS[MPDS_PL_CHARACTER(flags)];
    p.control_type = MPDS_PL_CPU(flags) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
    p.difficulty = MPDS_DIFFICULTIES[MPDS_PL_DIFFICULTY(flags)];
    p.coins = coins;
    p.stars = stars;

    /* One slot per DS input record, so the board index is the port. */
    p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + i);
  }

  return true;
}

bool MarioPartyDSHost::writePlayerSetup(const DrPlayerArray &players)
{
  const size_t ctx = context();
  unsigned i;

  if (!ctx)
    return false;

  for (i = 0; i < 4; i++)
  {
    const size_t rec = ctx + MPDS_CTX_PLAYERS + i * MPDS_PLAYER_STRIDE;
    const dr_player_t &p = players[i];
    const int chr = mpds_character_id(p.character);
    const int diff = mpds_difficulty_id(p.difficulty);
    uint32_t flags = 0;

    readu32(&flags, rec + MPDS_PL_FLAGS);

    if (chr >= 0)
      flags = MPDS_PL_CHARACTER_SET(flags, static_cast<unsigned>(chr));
    else
      log(DR_LOG_WARN,
        qPrintable(QString("write players: %1 is not in this game, leaving slot %2 alone")
          .arg(dr_character_name(p.character)).arg(i + 1)));

    if (diff >= 0)
      flags = MPDS_PL_DIFFICULTY_SET(flags, static_cast<unsigned>(diff));

    flags = MPDS_PL_CPU_SET(flags, p.control_type == DR_CONTROL_TYPE_CPU ? 1u : 0u);

    writeu32(flags, rec + MPDS_PL_FLAGS);
  }

  return true;
}

void MarioPartyDSHost::updateHumanPorts(void)
{
  QRetroInput *input = m_core ? m_core->input() : nullptr;
  const size_t ctx = context();
  unsigned port;

  if (!input || !ctx)
    return;

  for (port = 0; port < MPDS_PORTS; port++)
  {
    const size_t rec = ctx + MPDS_CTX_PLAYERS + port * MPDS_PLAYER_STRIDE + MPDS_PL_FLAGS;
    uint32_t flags = 0;
    unsigned id;

    if (readu32(&flags, rec) != DR_OK)
      continue;

    if (m_portActive[port])
    {
      /* Nothing here sets a seat back to CPU, so seeing the bit again means a
       * person did -- the debug menu. Take that as final: drop the latch and
       * leave the seat alone. Pressing anything on the port claims it back. */
      if (MPDS_PL_CPU(flags))
      {
        m_portActive[port] = false;
        log(DR_LOG_INFO, qPrintable(QString("port %1 set to CPU").arg(port + 1)));
      }

      continue;
    }

    for (id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; id++)
      if (input->state(port, RETRO_DEVICE_JOYPAD, 0, id))
        break;

    /* Nothing on this port yet: leave the seat as the game has it. */
    if (id > RETRO_DEVICE_ID_JOYPAD_R3)
      continue;

    m_portActive[port] = true;
    log(DR_LOG_INFO, qPrintable(QString("port %1 has a player").arg(port + 1)));

    if (MPDS_PL_CPU(flags))
      writeu32(MPDS_PL_CPU_SET(flags, 0), rec);
  }
}

/// @todo REMOVE -- one line whenever anything changes: what the frontend
/// reports per port ("in"), what reached that port's pad table ("pad"), who the
/// slot is, whose turn the game thinks it is, and whether the hooks are live.
void MarioPartyDSHost::traceInput(void)
{
  QRetroInput *input = m_core ? m_core->input() : nullptr;
  const size_t ctx = context();
  QString line = QString("mode %1 turn %2").arg(gameMode()).arg(turnOwner());
  unsigned port;

  {
    uint32_t word = 0;
    unsigned bad = 0, first = 0, i;

    for (i = 0; MPDS_HOOK_BOARD[i][0] != 0; i++)
    {
      readu32(&word, MPDS_HOOK_BOARD[i][0] - MPDS_RAM_BASE);
      if (word != MPDS_HOOK_BOARD[i][1])
      {
        if (!bad)
          first = MPDS_HOOK_BOARD[i][0];
        bad++;
      }
    }

    readu32(&word, 0x0201D2B8 - MPDS_RAM_BASE);
    line += QString(" hooks %1 sel %2")
              .arg(bad ? QString("BAD%1@%2").arg(bad).arg(first, 8, 16, QChar('0')) : "ok")
              .arg(word, 8, 16, QChar('0'));
  }

  for (port = 0; port < MPDS_PORTS; port++)
  {
    uint16_t in = 0, pad = 0, rec = 0;
    uint32_t flags = 0;
    int8_t valid = 0;
    unsigned i;

    for (i = 0; input && i < sizeof(MPDS_BUTTONS) / sizeof(MPDS_BUTTONS[0]); i++)
      if (input->state(port, RETRO_DEVICE_JOYPAD, 0, MPDS_BUTTONS[i].id))
        in |= MPDS_BUTTONS[i].bit;

    readu16(&rec, MPDS_INPUT[port].buttons.address);
    reads8(&valid, MPDS_INPUT[port].valid.address);
    readu16(&pad, MPDS_PAD_TABLE + port * MPDS_PAD_STRIDE);
    if (ctx)
      readu32(&flags, ctx + MPDS_CTX_PLAYERS + port * MPDS_PLAYER_STRIDE + MPDS_PL_FLAGS);

    line += QString(" | p%1 in %2 rec %3 v%4 pad %5 %6")
              .arg(port + 1)
              .arg(in, 4, 16, QChar('0'))
              .arg(rec, 4, 16, QChar('0'))
              .arg(valid)
              .arg(pad, 4, 16, QChar('0'))
              .arg(ctx ? (MPDS_PL_CPU(flags) ? "cpu" : "human") : "?");
  }

  if (line == m_lastTrace)
    return;

  m_lastTrace = line;
  log(DR_LOG_INFO, qPrintable(line));
}

int MarioPartyDSHost::turnOwner(void)
{
  const size_t ctx = context();
  int8_t owner = -1;

  if (!ctx || reads8(&owner, ctx + MPDS_CTX_TURN_OWNER) != DR_OK)
    return -1;

  return owner >= 0 && owner < static_cast<int8_t>(MPDS_PORTS) ? owner : -1;
}

void MarioPartyDSHost::writePad(unsigned port, uint16_t buttons)
{
  const size_t base = MPDS_PAD_TABLE + port * MPDS_PAD_STRIDE;
  uint16_t held, pressed = 0, released = 0, repeat = 0, previous = 0;
  unsigned i;

  readu16(&previous, base);

  /* Opposite directions cannot be down together: the game drops the whole d-pad
   * rather than pick one, so do the same before anything reads it. */
  held = buttons;
  if ((held & 0x0F0) == 0x0F0 || (held & 0x030) == 0x030 || (held & 0x0C0) == 0x0C0)
    held = static_cast<uint16_t>(buttons & ~0x0F0);

  pressed = static_cast<uint16_t>(held & (held ^ previous));

  /* Auto-repeat, frame for frame as func_01ffc1e4 does it: a button retriggers
   * once on the way down, again 0x18 frames later, then 0x10, then a frame
   * sooner each time until it is retriggering every 4. Menus scroll on this. */
  for (i = 0; i < MPDS_PAD_BUTTONS; i++)
  {
    const uint16_t bit = static_cast<uint16_t>(1u << i);
    const size_t delayAt = base + 0x08 + i * sizeof(uint16_t);
    const size_t countAt = base + 0x20 + i * sizeof(uint16_t);
    uint16_t delay = 0, count = 0;

    readu16(&delay, delayAt);
    readu16(&count, countAt);

    if (held & bit)
    {
      if (!delay)
      {
        delay = MPDS_REPEAT_FIRST;
        count = 0;
        repeat |= bit;
      }
      else if (++count >= delay)
      {
        if (delay == MPDS_REPEAT_FIRST)
          delay = MPDS_REPEAT_SECOND;
        else if (delay > MPDS_REPEAT_FASTEST)
          delay--;

        count = 0;
        repeat |= bit;
      }
    }
    else
    {
      /* Only a button that was actually down counts as released. */
      if (delay)
        released |= bit;

      delay = 0;
      count = 0;
    }

    writeu16(delay, delayAt);
    writeu16(count, countAt);
  }

  writeu16(held, base);
  writeu16(pressed, base + 0x02);
  writeu16(released, base + 0x04);
  writeu16(repeat, base + 0x06);
}

void MarioPartyDSHost::cloneStick(void)
{
  QRetroInput *input = m_core ? m_core->input() : nullptr;
  const int owner = turnOwner();
  unsigned index, id;

  /* melonDS DS only ever reads a stick on port 0 -- the console has one stylus
   * -- so the turn owner's stick is copied there to drive the touch cursor. An
   * owner of 0 already is port 0, and copying a port onto itself would re-apply
   * its own stick remap, so leave that case alone. */
  if (!input || owner <= 0)
    return;

  /* Both sticks, both axes: the source's own remaps are already folded into
   * analogStick(), so what lands on port 0 is what that player is aiming. */
  for (index = RETRO_DEVICE_INDEX_ANALOG_LEFT; index <= RETRO_DEVICE_INDEX_ANALOG_RIGHT; index++)
    for (id = RETRO_DEVICE_ID_ANALOG_X; id <= RETRO_DEVICE_ID_ANALOG_Y; id++)
      input->joypads()[0].setAnalogStick(index, id,
        input->joypads()[owner].analogStick(index, id));
}

void MarioPartyDSHost::writeInput(void)
{
  QRetroInput *input = m_core ? m_core->input() : nullptr;
  uint8_t touch[MPDS_TOUCH_STRIDE] = {};
  uint16_t buttons[MPDS_PORTS] = {};
  bool have_touch;
  unsigned port;

  if (!input)
    return;

  /* The real DS pen, read once and shared out to every port below. */
  have_touch =
    m_core->memory().readBuffer(touch, MPDS_SYSTEM_TOUCH, MPDS_TOUCH_STRIDE) == MPDS_TOUCH_STRIDE;

  /* Write touch input into every port */
  for (port = 0; port < MPDS_PORTS; port++)
  {
    unsigned i;

    buttons[port] = 0;
    for (i = 0; i < sizeof(MPDS_BUTTONS) / sizeof(MPDS_BUTTONS[0]); i++)
      if (input->state(port, RETRO_DEVICE_JOYPAD, 0, MPDS_BUTTONS[i].id))
        buttons[port] |= MPDS_BUTTONS[i].bit;
  }

  for (port = 0; port < MPDS_PORTS; port++)
  {
    /* The input buffer the game reads: each port's own controller, stamped
     * straight into its record, with the valid bit held so the record is
     * accepted. Nothing is copied between ports. */
    writeValue(buttons[port], MPDS_INPUT[port].buttons);
    writeValue(1, MPDS_INPUT[port].valid);

    writePad(port, buttons[port]);

    /* One pen, shared: the DS has only the one screen, so every port sees the
     * real pen rather than a simulated one. */
    if (have_touch)
      m_core->memory().writeBuffer(touch, MPDS_TOUCH_TABLE + port * MPDS_TOUCH_STRIDE,
        MPDS_TOUCH_STRIDE);
  }
}

void MarioPartyDSHost::stampCave(void)
{
  const size_t text = MPDS_HOST_STATE - MPDS_CAVE_ADDR;
  const size_t table = MPDS_IDTABLE - MPDS_CAVE_ADDR;
  const size_t tableSize = MPDS_TITLE_BLOCK - MPDS_IDTABLE;

  /* The code and the forced idTable, every frame: both are constant, and the
   * hooks branch into the code, so it must never be missing while they are in.
   * host_state belongs to the game and the state machine, and title_block to
   * stampTitles, so neither is touched here. */
  m_core->memory().writeBuffer(MPDS_CAVE, MPDS_CAVE_ADDR - MPDS_RAM_BASE, text);
  m_core->memory().writeBuffer(&MPDS_CAVE[table], MPDS_IDTABLE - MPDS_RAM_BASE, tableSize);
}

void MarioPartyDSHost::applyHooks(void)
{
  unsigned i;

  /* One list now: nothing depends on the game mode any more. The per-mode
   * my_aid hooks are gone -- "this console" is player 1 in every mode, and
   * every other seat reads its own input buffer. */
  for (i = 0; MPDS_HOOK_BOARD[i][0] != 0; i++)
    writeu32(MPDS_HOOK_BOARD[i][1], MPDS_HOOK_BOARD[i][0] - MPDS_RAM_BASE);
}

void MarioPartyDSHost::writeConnectedPorts(void)
{
  uint8_t ports = 0x01; /* port 0 is the real DS and always counts */
  unsigned port;

  /* Which ports have a person on them, which is what the window and menu owner
   * setters are really asking: a window for a port in this mask stays on that
   * port's pad, and one for a port outside it falls back to port 0. So a CPU
   * seat's dialogue -- or any seat nobody has claimed -- can be advanced by
   * whoever is at the DS, instead of waiting on a controller that isn't there.
   *
   * The latch is the right signal for this, not the seats' CPU flags: it means
   * someone is physically holding the port, and the flags are downstream of it. */
  for (port = 1; port < MPDS_PORTS; port++)
    if (m_portActive[port])
      ports |= static_cast<uint8_t>(1u << port);

  writeu8(ports, MPDS_CONNECTED_PORTS - MPDS_RAM_BASE);
}

void MarioPartyDSHost::setState(dr_mpds_host_state state)
{
  static const char *names[] = { "INVALID", "BEFORE_BOARD", "BOARD", "ROULETTE", "AFTER_ROULETTE",
    "MINIGAME" };

  log(
    DR_LOG_INFO, qPrintable(QString("host state: %1 -> %2").arg(names[m_state]).arg(names[state])));
  m_state = state;
}

void MarioPartyDSHost::rollMinigames(void)
{
  /* One shared reroll keeps every netplay peer's pool identical; the rolled
   * type's candidates are cached once the roulette says which it is. */
  if (m_MinigameSource)
    m_MinigameSource->rerollMinigames();
}

void MarioPartyDSHost::stampTitles(dr_minigame_type type)
{
  unsigned slot;

  if (!m_MinigameSource)
    return;

  m_candidates = m_MinigameSource->minigameCandidates(type);

  for (slot = 0; slot < MPDS_TITLE_SLOTS; slot++)
  {
    const dr_mp_minigame_t *minigame = m_candidates[slot].minigame;
    const QString name =
      (minigame && minigame->name) ? QString::fromUtf8(minigame->name) : QString();
    uint8_t buffer[MPDS_TITLE_SLOT_SIZE] = {};
    uint8_t color;
    int i;

    /* Leave the last code unit as the terminator. */
    for (i = 0; i < name.size() && i < MPDS_TITLE_SLOT_SIZE / 2 - 1; i++)
    {
      const uint16_t unit = name.at(i).unicode();

      buffer[i * 2] = static_cast<uint8_t>(unit);
      buffer[i * 2 + 1] = static_cast<uint8_t>(unit >> 8);
    }

    m_core->memory().writeBuffer(
      buffer, MPDS_TITLE_BLOCK_ADDR + slot * MPDS_TITLE_SLOT_SIZE, sizeof(buffer));

    /* Write minigame title color */
    if (minigame->flags.flags.lucky)
      color = MPDS_FONT_ORANGE;
    else if (minigame->flags.flags.unlucky)
      color = MPDS_FONT_RED;
    else if (minigame->flags.flags.mic)
      color = MPDS_FONT_PINK;
    else
      color = MPDS_FONT_WHITE;

    writeu8(color, MPDS_COLOUR_BLOCK_ADDR + slot);
  }
}

void MarioPartyDSHost::readPlayers(DrPlayerArray &players)
{
  const size_t ctx = context();
  const bool extras = gameMode() == MPDS_MODE_EXTRAS;
  int64_t seated = 4;
  uint32_t rules = 0;
  int8_t challenger = -1, opponent = -1;
  unsigned i;

  players = {};
  readPlayerSetup(players);

  /* Check player count, for duel boards */
  readValue(&seated, MPDS_PLAYER_COUNT);

  /* A duel comes from the duel rule or Desert Duel, where both seated players
   * always play, or from a duel space, which named its pair before rolling. */
  if (ctx)
    readu32(&rules, ctx + MPDS_CTX_RULES);
  if (MPDS_RULE(rules) == MPDS_RULE_DUEL || extras)
  {
    challenger = seated > 1 ? 1 : -1;
    opponent = seated > 0 ? 0 : -1;
  }
  else
  {
    reads8(&challenger, MPDS_DUEL_CHALLENGER);
    reads8(&opponent, MPDS_DUEL_OPPONENT);
  }

  for (i = 0; i < 4; i++)
  {
    dr_player_t &p = players[i];
    uint32_t flags = 0;

    if (ctx)
      readu32(&flags, ctx + MPDS_CTX_PLAYERS + i * MPDS_PLAYER_STRIDE + MPDS_PL_FLAGS);

    p.team_id = MPDS_PL_MG_TEAM(flags);

    switch (m_minigameType)
    {
    case DR_MINIGAME_1V3:
      p.team_type = p.team_id == 0 ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
      break;
    case DR_MINIGAME_2V2:
      /* Pen Pals pairs P1 and P2 against the COM-controlled P3 and P4 rather than
       * going through the party board's space colours. */
      if (extras)
        p.team_id = i < 2 ? 0 : 1;
      p.team_type = DR_TEAM_TYPE_2V2;
      break;
    case DR_MINIGAME_DUEL:
      /* 0 target / 1 initiator / 2 nonparticipant, the convention the other boards
       * use. A duel space's challenger is the player whose turn it is; under the
       * duel rule nobody challenged anybody, so the roles just follow the slots. */
      p.team_id = static_cast<int>(i) == challenger ? 1 : static_cast<int>(i) == opponent ? 0 : 2;
      p.team_type = p.team_id == 0   ? DR_TEAM_TYPE_DUEL_TARGET
                    : p.team_id == 1 ? DR_TEAM_TYPE_DUEL_INITIATOR
                                     : DR_TEAM_TYPE_DUEL_NONPARTICIPANT;
      break;
    default:
      p.team_type = DR_TEAM_TYPE_4P;
      break;
    }
  }
}

void MarioPartyDSHost::startMinigame(void)
{
  DrPlayerArray players;

  if (m_slot < 0 || m_slot >= static_cast<int>(m_candidates.size()) || !m_candidates[m_slot].guest)
  {
    log(DR_LOG_ERROR, qPrintable(QString("startMinigame: no candidate for slot %1").arg(m_slot)));
    return;
  }

  readPlayers(players);

  log(DR_LOG_INFO,
    qPrintable(QString("launching mini-game: %1 (slot %2)")
        .arg(
          m_candidates[m_slot].minigame->name ? m_candidates[m_slot].minigame->name : "(unnamed)")
        .arg(m_slot)));

  emit minigameRequested(m_candidates[m_slot], players);
}

void MarioPartyDSHost::run(void)
{
  tickFrameWrites();

  /* At frameEnd, so the records are in place before the next retro_run reads them. */
  writeInput();

  if (m_core->frames() <= MPDS_WARMUP_FRAMES)
    return;

  /* The cave first: the hooks branch into it. */
  stampCave();
  updateHumanPorts();
  writeConnectedPorts();
  applyHooks();

  traceInput(); ///< @todo REMOVE

  const int previousScene = m_lastScene;
  const int currentScene = pollScene();

  switch (m_state)
  {
  case DR_MPDS_HOST_STATE_INVALID:
    if (m_core->frames() <= 120)
      return;
    /* Roll an initial pool; the type-specific set is stamped once a roulette opens. */
    rollMinigames();
    setState(DR_MPDS_HOST_STATE_BEFORE_BOARD);
    break;

  case DR_MPDS_HOST_STATE_BEFORE_BOARD:
    writes8(-1, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, minigame_type));
    writes8(0, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, reserved));
    m_slot = -1;
    setState(DR_MPDS_HOST_STATE_BOARD);
    break;

  case DR_MPDS_HOST_STATE_BOARD:
  {
    int8_t type = -1;

    reads8(&type, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, minigame_type));
    if (type < 0)
      break;

    /* If in an unsupported mode just let it through */
    if (gameMode() != MPDS_MODE_PARTY &&
        gameMode() != MPDS_MODE_STORY &&
        gameMode() != MPDS_MODE_EXTRAS)
    {
      writes8(-1, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, minigame_type));
      break;
    }

    if (static_cast<unsigned>(type) >=
          sizeof(MPDS_MINIGAME_TYPE_TO_DR) / sizeof(*MPDS_MINIGAME_TYPE_TO_DR) ||
        MPDS_MINIGAME_TYPE_TO_DR[type] == DR_MINIGAME_INVALID)
    {
      log(DR_LOG_WARN, qPrintable(QString("roulette type %1 has no dr_minigame_type").arg(type)));
      writes8(-1, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, minigame_type));
      break;
    }

    m_minigameType = MPDS_MINIGAME_TYPE_TO_DR[type];
    log(DR_LOG_INFO,
      qPrintable(
        QString("roulette type %1 (%2)").arg(type).arg(dr_minigame_type_name(m_minigameType))));

    /* Stamp the candidates' names before the roulette draws them. */
    stampTitles(m_minigameType);
    setState(DR_MPDS_HOST_STATE_ROULETTE);
    break;
  }

  case DR_MPDS_HOST_STATE_ROULETTE:
  {
    int8_t landed = 0;

    /* MPDS publishes chosen slot immediately so take it */
    reads8(&landed, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, reserved));
    landed &= ~MPDS_HANDOFF;
    if (landed < 1 || landed > MPDS_TITLE_SLOTS)
      break;

    m_slot = landed - 1;
    setState(DR_MPDS_HOST_STATE_AFTER_ROULETTE);
    break;
  }

  case DR_MPDS_HOST_STATE_AFTER_ROULETTE:
  {
    uint8_t reserved = 0;
    int64_t next = 0;

    /* skip_ret flags the hand-off once the spin has finished. Launch then, while
     * the board is still fading out -- whatever runs next (a results scene, or the
     * board itself settling a duel) reads the payouts as soon as it starts, and
     * the guest has to have written them first. */
    readu8(&reserved, MPDS_HOST_STATE_ADDR + offsetof(dr_host_state_t, reserved));
    if (!(reserved & MPDS_HANDOFF))
      break;

    /* The end-of-turn roulette hands off to a results scene; a duel space hands
     * straight back to the board. */
    readValue(&next, MPDS_SCENE_NEXT);
    m_toResults = next == MPDS_SCENE_RESULTS || next == MPDS_SCENE_BATTLE_RESULTS;

    startMinigame();
    setState(DR_MPDS_HOST_STATE_MINIGAME);
    break;
  }

  case DR_MPDS_HOST_STATE_MINIGAME:
    /* Back on the board once the results scene hands control back -- or at once
     * for a duel space, which has no results scene and settles on the board. */
    if (!m_toResults || (currentScene != previousScene && previousScene == MPDS_SCENE_RESULTS))
    {
      rollMinigames();
      setState(DR_MPDS_HOST_STATE_BEFORE_BOARD);
    }
    break;

  case DR_MPDS_HOST_STATE_SIZE:
    break;
  }
}

int MarioPartyDSHost::pollScene(void)
{
  int64_t scene = 0;

  if (readValue(&scene, MPDS_SCENE_CURRENT) != DR_OK)
    return -1;

  if (static_cast<int16_t>(scene) == m_lastScene)
    return static_cast<int>(scene);

  m_lastScene = static_cast<int16_t>(scene);

  {
    const char *name = dr_scene_name(MPDS_SCENE_NAMES, static_cast<int>(scene));

    if (name)
      log(DR_LOG_INFO,
        qPrintable(
          QString("scene: 0x%1 (%2)").arg(static_cast<int>(scene), 2, 16, QChar('0')).arg(name)));
    else
      log(DR_LOG_WARN, qPrintable(QString("scene: 0x%1 <<< UNKNOWN SCENE ID >>>")
                           .arg(static_cast<int>(scene), 2, 16, QChar('0'))));
  }

  return static_cast<int>(scene);
}

void MarioPartyDSHost::writeResults(DrGuest *guest)
{
  const size_t ctx = context();
  unsigned i;

  if (!ctx)
    return;

  for (i = 0; i < 4; i++)
  {
    const dr_minigame_result_t result = guest->minigameResult(i);
    const size_t rec = ctx + MPDS_CTX_PLAYERS + i * MPDS_PLAYER_STRIDE;
    uint32_t flags = 0;

    /* MPDS has only one results variable; add result + bonus together */
    const int payout =
      m_minigameType == DR_MINIGAME_BATTLE ? result.coins : result.coins + result.bonus_coins;

    writes16(static_cast<int16_t>(payout), rec + MPDS_PL_MG_RESULT);

    readu32(&flags, rec + MPDS_PL_FLAGS);
    log(
      DR_LOG_INFO, qPrintable(resultLogLine(i, MPDS_CHARACTERS[MPDS_PL_CHARACTER(flags)], result)));
  }
}

void MarioPartyDSHost::clearResults(void)
{
  const size_t ctx = context();
  unsigned i;

  if (!ctx)
    return;

  for (i = 0; i < 4; i++)
    writes16(0, ctx + MPDS_CTX_PLAYERS + i * MPDS_PLAYER_STRIDE + MPDS_PL_MG_RESULT);
}
