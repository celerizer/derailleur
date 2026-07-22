#include "MarioParty8.h"

#include <QRetro.h>

/// s16 - mini-game id to load
static const size_t MP8_MINIGAME_TO_LOAD_ADDR = 0x802287CC;

/// s32 - current scene id
static const size_t MP8_SCENE_ID_ADDR = 0x802CD220;

/// s16 - minigame variant perhaps? course id for moped
static const size_t MP8_MINIGAME_VARIANT_ADDR = 0x802CE35E;

/// scene id of the mini-game explanation screen
static const int32_t MP8_SCENE_MINIGAME_EXPLAIN = 0x16;

/* MP8 has two per-player layouts: a compact pre-game "setup" array (0xA / 10-byte
 * stride) and the "real" in-game player structs (0x118 stride, holds coins etc.
 * below). A third region mirrors the difficulty/bot flags at their menu locations
 * (MP8_MENU_*). The setup array holds, per slot:
 *   +0x0  character      u16  MP8_CHARACTER_ADDR
 *   +0x2  controller port u16  MP8_CONTROL_PORT_ADDR
 *   +0x4  difficulty     u16  MP8_CPU_DIFFICULTY_ADDR
 *   +0x6  team           u16  MP8_TEAM_ADDR
 *   +0x8  is bot         u16  MP8_IS_BOT_ADDR
 */

/// character roster id per in-game slot -- setup array, 0xA (10-byte) stride.
static const size_t MP8_CHARACTER_ADDR[4] =
  { 0x802282D0, 0x802282DA, 0x802282E4, 0x802282EE };

/* Extras Zone per-player selection -- 0x6-byte stride, four slots. Used only in
 * the Extras Zone, where players choose a character or a Mii.
 *   +0x0  chosen character  u16  MP8_EXTRAS_CHARACTER_ADDR
 *   +0x2  unknown (0 or 4)  u16
 *   +0x4  mii index         u16  MP8_MII_INDEX_ADDR
 */
static const size_t MP8_EXTRAS_CHARACTER_ADDR[4] =
  { 0x8023AAA0, 0x8023AAA6, 0x8023AAAC, 0x8023AAB2 };

/// u16 - which Mii to use (index into the console's Mii list) per player, when
/// the chosen character is a Mii (MP8_CHARACTER_MII_1..4).
static const size_t MP8_MII_INDEX_ADDR[4] =
  { 0x8023AAA4, 0x8023AAAA, 0x8023AAB0, 0x8023AAB6 };

/// u16 - controller port per slot -- setup array, 0xA stride (+2 from the
/// character).
static const size_t MP8_CONTROL_PORT_ADDR[4] =
  { 0x802282D2, 0x802282DC, 0x802282E6, 0x802282F0 };

/// u16 - CPU difficulty per slot -- setup array, 0xA stride (+4 from the
/// character). This is the difficulty that actually takes effect in-game.
static const size_t MP8_CPU_DIFFICULTY_ADDR[4] =
  { 0x802282D4, 0x802282DE, 0x802282E8, 0x802282F2 };

/// u16 - mini-game team per slot (1v3: group = 1, solo = 0) -- setup array, 0xA
/// stride (+6 from the character).
static const size_t MP8_TEAM_ADDR[4] =
  { 0x802282D6, 0x802282E0, 0x802282EA, 0x802282F4 };

/// u16 - whether this slot is a bot (0 or 1) -- setup array, 0xA stride (+8 from
/// the character). Setting this also flips a bit in MP8_CPU_FLAGS_ADDR.
static const size_t MP8_IS_BOT_ADDR[4] =
  { 0x802282D8, 0x802282E2, 0x802282EC, 0x802282F6 };

/* In-game player struct -- 0x118-byte stride, four consecutive slots. P1 begins
 * at 0x802282F8, directly after the 4x0xA setup array above (no gap). Offsets
 * below are from that base; "HOLE" marks bytes we haven't identified. Addresses
 * are P1; add slot*0x118 for slots 2-4.
 *
 *   +0x00  cpu/behavior flags u16    MP8_CPU_FLAGS_ADDR (0x4001 human/0xE001 bot)
 *   +0x02  ...........       HOLE (4 bytes)
 *   +0x06  item slots        u8[3]   MP8_ITEM_ADDR
 *   +0x09  item slots (2)    u8[3]   MP8_ITEM2_ADDR
 *   +0x0C  ...........       HOLE (2 bytes)
 *   +0x0E  spaces to move    u16     MP8_SPACES_LEFT_ADDR
 *   +0x10  ...........       HOLE (10 bytes)
 *   +0x1A  candy this turn   s16     MP8_CANDY_THIS_TURN_ADDR (-1 = none)
 *   +0x1C  ...........       HOLE (10 bytes)
 *   +0x26  coins             u16     MP8_COINS_ADDR
 *   +0x28  minigame star     u16     MP8_MINIGAME_STAR_ADDR
 *   +0x2A  ...........       HOLE (2 bytes)
 *   +0x2C  coin star         u16     MP8_COIN_STAR_ADDR
 *   +0x2E  ...........       HOLE (4 bytes)
 *   +0x32  minigame result   u16     MP8_RESULT_ADDR
 *   +0x34  ...........       HOLE (4 bytes)
 *   +0x38  stars             u16     MP8_STARS_ADDR
 *   +0x3A  max stars held    u16     MP8_MAX_STARS_ADDR
 *   +0x3C  candies used      u16     MP8_CANDIES_USED_ADDR
 */

/// u16 - CPU/behavior flags (0x4001 human -> 0xE001 bot). A bit here tracks the
/// bot state also set via MP8_IS_BOT_ADDR.
static const size_t MP8_CPU_FLAGS_ADDR[4] =
  { 0x802282F8, 0x80228410, 0x80228528, 0x80228640 };

/// u8[3] - the player's three item slots (three contiguous bytes starting here).
static const size_t MP8_ITEM_ADDR[4] =
  { 0x802282FE, 0x80228416, 0x8022852E, 0x80228646 };

/// u8[3] - a second group of three item slots directly after MP8_ITEM_ADDR.
/// Kept separate as it's unclear whether these are used.
static const size_t MP8_ITEM2_ADDR[4] =
  { 0x80228301, 0x80228419, 0x80228531, 0x80228649 };

typedef enum
{
  MP8_ITEM_NONE = -1,
  MP8_ITEM_TWICE_CANDY   = 0x00,
  MP8_ITEM_THRICE_CANDY  = 0x01,
  MP8_ITEM_SLOWGO_CANDY  = 0x02,
  MP8_ITEM_UNUSED1 = 3,
  MP8_ITEM_SPRINGO_CANDY = 0x04,
  MP8_ITEM_CASHZAP_CANDY = 0x05,
  MP8_ITEM_VAMPIRE_CANDY = 0x06,
  MP8_ITEM_BITSIZE_CANDY = 0x07,
  MP8_ITEM_BLOWAY_CANDY  = 0x08,
  MP8_ITEM_BOWLO_CANDY   = 0x09,
  MP8_ITEM_WEEGLEE_CANDY = 0x0A,
  MP8_ITEM_UNUSED2 = 11,
  MP8_ITEM_THWOMP_CANDY  = 0x0C,
  MP8_ITEM_BULLET_CANDY  = 0x0D,
  MP8_ITEM_BOWSER_CANDY  = 0x0E,
  MP8_ITEM_DUELO_CANDY   = 0x0F,
  MP8_ITEM_UNUSED3 = 16
} mp8_item;

/// u16 - spaces left to move this turn per player. @todo confirm width (u8/u16).
static const size_t MP8_SPACES_LEFT_ADDR[4] =
  { 0x80228306, 0x8022841E, 0x80228536, 0x8022864E };

/// s16 - candy used this turn per player (mp8_item id, or -1 for none).
static const size_t MP8_CANDY_THIS_TURN_ADDR[4] =
  { 0x80228312, 0x8022842A, 0x80228542, 0x8022865A };

/// u16 - current coins per player.
static const size_t MP8_COINS_ADDR[4] =
  { 0x8022831E, 0x80228436, 0x8022854E, 0x80228666 };

/// u16 - running count for the "Minigame Star" bonus per player.
static const size_t MP8_MINIGAME_STAR_ADDR[4] =
  { 0x80228320, 0x80228438, 0x80228550, 0x80228668 };

/// u16 - running count for the "Coin Star" bonus per player.
static const size_t MP8_COIN_STAR_ADDR[4] =
  { 0x80228324, 0x8022843C, 0x80228554, 0x8022866C };

/// u16 - mini-game result per player.
static const size_t MP8_RESULT_ADDR[4] =
  { 0x8022832A, 0x80228442, 0x8022855A, 0x80228672 };

/// u16 - current stars per player.
static const size_t MP8_STARS_ADDR[4] =
  { 0x80228330, 0x80228448, 0x80228560, 0x80228678 };

/// u16 - most stars held at any one time per player (MP8_STARS_ADDR + 2).
static const size_t MP8_MAX_STARS_ADDR[4] =
  { 0x80228332, 0x8022844A, 0x80228562, 0x8022867A };

/// u16 - number of candies used per player.
static const size_t MP8_CANDIES_USED_ADDR[4] =
  { 0x80228334, 0x8022844C, 0x80228564, 0x8022867C };

/// u16 - CPU difficulty at the pre-game menu location -- 4-byte stride, same
/// region as MP8_MENU_IS_BOT_ADDR. Noted only; we drive difficulty via the setup
/// array (MP8_CPU_DIFFICULTY_ADDR).
static const size_t MP8_MENU_DIFFICULTY_ADDR[4] =
  { 0x80740EEA, 0x80740EEE, 0x80740EF2, 0x80740EF6 };

/// u8 - bot flag at the pre-game menu location -- contiguous per-slot, 1-byte
/// stride. Noted only; the in-game bot state is driven via MP8_IS_BOT_ADDR.
static const size_t MP8_MENU_IS_BOT_ADDR[4] =
  { 0x80740EE0, 0x80740EE1, 0x80740EE2, 0x80740EE3 };

typedef enum
{
  MP8_CHARACTER_NONE = -1,
  MP8_CHARACTER_MARIO = 0x0,
  MP8_CHARACTER_LUIGI = 0x1,
  MP8_CHARACTER_PEACH = 0x2,
  MP8_CHARACTER_YOSHI = 0x3,
  MP8_CHARACTER_WARIO = 0x4,
  MP8_CHARACTER_DAISY = 0x5,
  MP8_CHARACTER_WALUIGI = 0x6,
  MP8_CHARACTER_TOAD = 0x7,
  MP8_CHARACTER_BOO = 0x8,
  MP8_CHARACTER_TOADETTE = 0x9,
  MP8_CHARACTER_BIRDO = 0xA,
  MP8_CHARACTER_DRY_BONES = 0xB,
  MP8_CHARACTER_BLOOPER = 0xC,
  MP8_CHARACTER_HAMMER_BRO = 0xD,

  /* Extras Zone only */
  MP8_CHARACTER_MII_1 = 0xE,
  MP8_CHARACTER_MII_2 = 0xF,
  MP8_CHARACTER_MII_3 = 0x10,
  MP8_CHARACTER_MII_4 = 0x11,

  /* Moped Mayhem only -- hold the character ID through the loading screen to force */
  MP8_CHARACTER_DONKEY_KONG = 0x0E,
  MP8_CHARACTER_BOWSER = 0x0F,
  MP8_CHARACTER_KAMEK = 0x10,
  MP8_CHARACTER_GOOMBA = 0x11,
  MP8_CHARACTER_SHY_GUY = 0x12,
  MP8_CHARACTER_KOOPA_TROOPA = 0x13,
  MP8_CHARACTER_KOOPA_PARATROOPA = 0x14,
  MP8_CHARACTER_WHOMP = 0x15,
  MP8_CHARACTER_THWOMP = 0x16,
  MP8_CHARACTER_UKIKI = 0x17,
  MP8_CHARACTER_PENGUIN = 0x18,
  MP8_CHARACTER_MONTY_MOLE = 0x19,
  MP8_CHARACTER_BANDIT = 0x1A,
  MP8_CHARACTER_BOB_OMB = 0x1B,
  MP8_CHARACTER_PIANTA = 0x1C,
  MP8_CHARACTER_FLUTTER = 0x1D,
} mp8_character;

static int32_t mp8Character(dr_character c, bool moped = false)
{
  switch (c)
  {
  case DR_CHARACTER_MARIO:
    return MP8_CHARACTER_MARIO;
  case DR_CHARACTER_LUIGI:
    return MP8_CHARACTER_LUIGI;
  case DR_CHARACTER_PEACH:
    return MP8_CHARACTER_PEACH;
  case DR_CHARACTER_YOSHI:
    return MP8_CHARACTER_YOSHI;
  case DR_CHARACTER_WARIO:
    return MP8_CHARACTER_WARIO;
  case DR_CHARACTER_DAISY:
    return MP8_CHARACTER_DAISY;
  case DR_CHARACTER_WALUIGI:
    return MP8_CHARACTER_WALUIGI;
  case DR_CHARACTER_TOAD:
    return MP8_CHARACTER_TOAD;
  case DR_CHARACTER_BOO:
    return MP8_CHARACTER_BOO;
  case DR_CHARACTER_TOADETTE:
    return MP8_CHARACTER_TOADETTE;
  case DR_CHARACTER_BIRDO:
    return MP8_CHARACTER_BIRDO;
  case DR_CHARACTER_DRY_BONES:
    return MP8_CHARACTER_DRY_BONES;
  case DR_CHARACTER_BLOOPER:
    return MP8_CHARACTER_BLOOPER;
  case DR_CHARACTER_HAMMER_BRO:
    return MP8_CHARACTER_HAMMER_BRO;

  /* Not in MP8 */
  case DR_CHARACTER_DONKEY_KONG:
    return moped ? MP8_CHARACTER_DONKEY_KONG : MP8_CHARACTER_BOO;
  case DR_CHARACTER_KOOPA_KID:
    return moped ? MP8_CHARACTER_BOWSER : MP8_CHARACTER_HAMMER_BRO;

  default:
    return MP8_CHARACTER_MARIO;
  }
}

static int32_t mp8Difficulty(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0;
  case DR_DIFFICULTY_NORMAL:
    return 1;
  case DR_DIFFICULTY_HARD:
    return 2;
  case DR_DIFFICULTY_VERY_HARD:
    return 3;
  default:
    return 1;
  }
}

/* In-game player slot from a board player's controller port (Mario Party assigns
 * ports non-linearly), falling back to the board index if out of range. */
static unsigned mp8Slot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

static const dr_mp_minigame_t MP8_MINIGAMES[] =
{
  { "Speedy Graffiti", DR_MINIGAME_4P, 0x00, 0x17, { DR_QUIRK_BITS_EFB_TO_TEXTURE | DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) } },
  { "Swing Kings", DR_MINIGAME_4P, 0x01, 0x18, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) },
  { "Water Ski Spree", DR_MINIGAME_4P, 0x02, 0x19, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Punch-a-Bunch", DR_MINIGAME_4P, 0x03, 0x1A, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) },
  //{ "Crank to Rank", DR_MINIGAME_4P, 0x04, 0x1B, DR_NO_QUIRKS }, ??
  //{ "At the Chomp Wash", DR_MINIGAME_4P, 0x05, 0x1C, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) }, ??
  { "Mosh-Pit Playroom", DR_MINIGAME_4P, 0x06, 0x1D, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Mario Matrix", DR_MINIGAME_4P, 0x07, 0x1E, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "??? - Hammer de Pokari", DR_MINIGAME_INVALID, 0x08, 0x1F, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Grabby Giridion", DR_MINIGAME_2V2, 0x09, 0x20, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Lava or Leave 'Em", DR_MINIGAME_4P, 0x0A, 0x21, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Kartastrophe", DR_MINIGAME_4P, 0x0B, 0x22, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "??? - Ribbon Game", DR_MINIGAME_INVALID, 0x0C, 0x23, DR_NO_QUIRKS },
  { "Aim of the Game", DR_MINIGAME_BATTLE, 0x0D, 0x24, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Rudder Madness", DR_MINIGAME_4P, 0x0E, 0x25, DR_NO_QUIRKS },
  { "Gun the Runner", DR_MINIGAME_1V3, 0x0F, 0x26, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_SIDEWAYS_BUTTONS, DR_WII_CONTROL_POINTER) },
  { "Grabbin' Gold", DR_MINIGAME_1V3, 0x10, 0x27, DR_NO_QUIRKS },
  { "Power Trip", DR_MINIGAME_1V3, 0x11, 0x28, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_SIDEWAYS_MOTION, DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Bob-ombs Away", DR_MINIGAME_1V3, 0x12, 0x29, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Swervin' Skies", DR_MINIGAME_1V3, 0x13, 0x2A, DR_NO_QUIRKS },
  { "Picture Perfect", DR_MINIGAME_1V3, 0x14, 0x2B, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Snow Way Out", DR_MINIGAME_1V3, 0x15, 0x2C, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Thrash 'n' Crash", DR_MINIGAME_1V3, 0x16, 0x2D, DR_WII_CONTROL_SPLIT(DR_WII_CONTROL_POINTER, DR_WII_CONTROL_UPRIGHT) },
  { "Chump Rope", DR_MINIGAME_1V3, 0x17, 0x2E, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Sick and Twisted", DR_MINIGAME_4P, 0x18, 0x2F, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Bumper Balloons", DR_MINIGAME_2V2, 0x19, 0x30, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Rowed to Victory", DR_MINIGAME_2V2, 0x1A, 0x31, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) },
  { "Winner or Dinner", DR_MINIGAME_2V2, 0x1B, 0x32, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Paint Misbehavin'", DR_MINIGAME_2V2, 0x1C, 0x33, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Sugar Rush", DR_MINIGAME_2V2, 0x1D, 0x34, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "King of the Thrill", DR_MINIGAME_2V2, 0x1E, 0x35, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  // { "Shake It Up", DR_MINIGAME_4P, 0x1F, 0x36, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) }, sucks
  { "Lean, Mean Ravine", DR_MINIGAME_2V2, 0x20, 0x37, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Boo-ting Gallery", DR_MINIGAME_2V2, 0x21, 0x38, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Crops 'n' Robbers", DR_MINIGAME_2V2, 0x22, 0x39, DR_NO_QUIRKS },
  { "In the Nick of Time", DR_MINIGAME_4P, 0x23, 0x3A, DR_NO_QUIRKS },
  { "Cut from the Team", DR_MINIGAME_BATTLE, 0x24, 0x3B, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Snipe for the Picking", DR_MINIGAME_BATTLE, 0x25, 0x3C, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Saucer Swarm", DR_MINIGAME_BATTLE, 0x26, 0x3D, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Glacial Meltdown", DR_MINIGAME_BATTLE, 0x27, 0x3E, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  // { "Attention Grabber", DR_MINIGAME_DUEL, 0x28, 0x3F, DR_NO_QUIRKS }, ??
  // { "Blazing Lassos", DR_MINIGAME_DUEL, 0x29, 0x40, DR_NO_QUIRKS },

  { "Wing and a Scare", DR_MINIGAME_DUEL, 0x2A, 0x41, DR_NO_QUIRKS },
  { "Lob to Rob", DR_MINIGAME_DUEL, 0x2B, 0x42, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  // { "Pumper Cars", DR_MINIGAME_DUEL, 0x2C, 0x43, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) }, sucks
  { "Cosmic Slalom", DR_MINIGAME_DUEL, 0x2D, 0x44, DR_NO_QUIRKS },
  { "Lava Lobbers", DR_MINIGAME_DUEL, 0x2E, 0x45, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },

  { "Loco Motives", DR_MINIGAME_DUEL, 0x2F, 0x46, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Specter Inspector", DR_MINIGAME_DUEL, 0x30, 0x47, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Frozen Assets", DR_MINIGAME_DUEL, 0x31, 0x48, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  // { "Breakneck Building", DR_MINIGAME_DUEL, 0x32, 0x49, DR_QUIRK_NATIVE_RESOLUTION }, sucks
  { "Surf's Way Up", DR_MINIGAME_DUEL, 0x33, 0x4A, DR_NO_QUIRKS },
  { "??? - Bull Riding", DR_MINIGAME_INVALID, 0x34, 0x4B, DR_NO_QUIRKS },
  { "Balancing Act", DR_MINIGAME_INVALID, 0x35, 0x4C, DR_NO_QUIRKS }, /// @todo wut
  { "Ion the Prize", DR_MINIGAME_DUEL, 0x36, 0x4D, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "You're the Bob-omb", DR_MINIGAME_DUEL, 0x37, 0x4E, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Scooter Pursuit", DR_MINIGAME_4P, 0x38, 0x4F, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  { "Cardiators", DR_MINIGAME_DUEL, 0x39, 0x50, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) },
  // { "Rotation Station", DR_MINIGAME_DUEL, 0x3A, 0x51, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS) }, todo
  // { "Eyebrawl", DR_MINIGAME_DUEL, 0x3B, 0x52, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) }, todo

  /* Extras Zone -- free-play only, not board mini-games. */
  { "Table Menace", DR_MINIGAME_SPECIAL, 0x3C, 0x53, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) },
  { "Flagging Rights - TODO: nunchuk", DR_MINIGAME_INVALID, 0x3D, 0x54, DR_NO_QUIRKS }, /// @todo
  { "Trial by Tile", DR_MINIGAME_SPECIAL, 0x3E, 0x55, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Star Carnival Bowling - TODO: controls", DR_MINIGAME_INVALID, 0x3F, 0x56, DR_NO_QUIRKS }, /// @todo
  { "Puzzle Pillars", DR_MINIGAME_SPECIAL, 0x40, 0x57, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Canyon Cruisers", DR_MINIGAME_SPECIAL, 0x41, 0x58, DR_NO_QUIRKS },
  { "Chomping Frenzy", DR_MINIGAME_SPECIAL, 0x42, 0x59, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Settle It in Court", DR_MINIGAME_DUEL, 0x43, 0x5A, DR_NO_QUIRKS },
  { "Moped Mayhem", DR_MINIGAME_SPECIAL, 0x44, 0x5B, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) }, /// @todo duel
  { "Flip the Chimp", DR_MINIGAME_4P, 0x45, 0x5C, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE) },

  /* Challenge -- Star Battle Arena / Duel Battles, single player. */
  { "Pour to Score", DR_MINIGAME_1P, 0x46, 0x5D, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },
  { "Fruit Picker", DR_MINIGAME_1P, 0x47, 0x5E, DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) },
  { "Stampede", DR_MINIGAME_1P, 0x48, 0x5F, DR_NO_QUIRKS },
  { "Superstar Showdown", DR_MINIGAME_SPECIAL, 0x49, 0x60, DR_NO_QUIRKS }, // vs. Bowser (The Last)
  { "Alpine Assault", DR_MINIGAME_4P, 0x4A, 0x61, DR_NO_QUIRKS },
  { "Treacherous Tightrope", DR_MINIGAME_4P, 0x4B, 0x62, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION) },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0x00, DR_NO_QUIRKS },
};

MarioParty8::MarioParty8(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath(dr_core_path(DR_CORE_DOLPHIN).toStdString())
  , m_discPath((dr_roms_directory() + "/Mario Party 8 (USA, Asia) (Rev 2)").toStdString())
  , m_statePath((dr_state_directory() + "/mp8.state.zip").toStdString())
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioParty8::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioParty8::run()
{
  m_retro->tickFrameWrites();

  if (m_minigameActive)
    m_minigameFrames++;

  int32_t val;
  if (m_retro->reads32(&val, MP8_SCENE_ID_ADDR) == DR_OK && val != m_lastScene)
  {
    log(DR_LOG_INFO, qPrintable(QString("%1 scene: 0x%2").arg(name()).arg(val, 4, 16, QChar('0'))));
    m_lastScene = val;

    /* The explanation screen is navigated with the pointer; switch to the
     * mini-game's actual layout once the mini-game scene itself is reached. */
    if (m_minigame && val == MP8_SCENE_MINIGAME_EXPLAIN)
      applyControlProfile(DR_WII_CONTROL_POINTER);
    else if (m_minigame && val == m_minigame->scene_id)
      applyControlRemap(m_minigame->quirks, m_players);

    /* Once the scene leaves the mini-game (its own scene_id), the explanation
     * screen and the -1 loading state, the mini-game is over. */
    if (m_minigameActive && m_minigameFrames >= 60 &&
        val != MP8_SCENE_MINIGAME_EXPLAIN && val != m_minigame->scene_id &&
        val != -1)
    {
      log(DR_LOG_INFO, qPrintable(QString("finishing minigame on scene 0x%1").arg(val, 4, 16, QChar('0'))));
      finishMinigame();
    }
  }

  /* In Moped Mayhem, write the character every frame */
  if (m_minigame && m_minigame->minigame_id == 0x44)
  {
    const int16_t p1 = static_cast<int16_t>(mp8Character(m_players[0].character, true));
    const int16_t p2 = static_cast<int16_t>(mp8Character(m_players[1].character, true));
    m_retro->writes16(p1, MP8_CHARACTER_ADDR[0]);
    m_retro->writes16(p2, MP8_CHARACTER_ADDR[1]);
  }
}

const dr_mp_minigame_t *MarioParty8::minigames() const
{
  return MP8_MINIGAMES;
}

void MarioParty8::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_lastScene = -1;

  for (unsigned i = 0; i < 4; i++)
    m_players[i] = data.players[i];

  int16_t id = static_cast<int16_t>(data.minigame->minigame_id);
  m_retro->writes16(id, MP8_MINIGAME_TO_LOAD_ADDR);

  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = mp8Slot(m_players[i], i);
    m_retro->writeu16(static_cast<uint16_t>(mp8Character(m_players[i].character)),
      MP8_CHARACTER_ADDR[slot]);
    m_retro->writeu16(static_cast<uint16_t>(mp8Difficulty(m_players[i].difficulty)),
      MP8_CPU_DIFFICULTY_ADDR[slot]);
    m_retro->writeu16(m_players[i].control_type == DR_CONTROL_TYPE_CPU ? 1 : 0,
      MP8_IS_BOT_ADDR[slot]);

    /* Flip the same bot bits (0x4001 human -> 0xE001 bot) in the flags word,
     * preserving the rest. */
    uint16_t flags = 0;
    m_retro->readu16(&flags, MP8_CPU_FLAGS_ADDR[slot]);
    if (m_players[i].control_type == DR_CONTROL_TYPE_CPU)
      flags |= 0xA000;
    else
      flags &= ~0xA000;
    m_retro->writeu16(flags, MP8_CPU_FLAGS_ADDR[slot]);

    /* Continuous write -- the game overwrites the team as the match spins up. */
    uint16_t team = static_cast<uint16_t>(m_players[i].team_id);
    m_retro->writeForFrames(MP8_TEAM_ADDR[slot], &team, sizeof(team), 60);
  }

  applyControlRemap(data.minigame->quirks, m_players);

  startMinigame();
}

dr_minigame_result_t MarioParty8::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index >= 4)
    return result;

  const unsigned slot = mp8Slot(m_players[index], index);

  /* MP8 stores each player's coin outcome in the result field; report it back to
   * the host as coins earned. Read signed -- Battle/Duel can be negative. @todo
   * confirm this holds the coin delta (not a win/place code). */
  int16_t coins = 0;
  if (m_retro->reads16(&coins, MP8_RESULT_ADDR[slot]) != DR_OK)
    return result;

  result.coins = coins;
  return result;
}
