#include "MarioKartWii.h"

#include <QRetro.h>

/* Layouts are from the PAL decomp (System::RaceConfig/RaceManager); the
 * addresses are NTSC-U. */

/* RaceConfig::Player[12] of the menu scenario, 0xF0 apart, copied into the race
 * scenario when the race starts. These live in the heap, so they may move if
 * the savestate is updated. */
#define MKW_PLAYERS_ADDR  0x90893A28
#define MKW_PLAYER_STRIDE 0xF0
#define MKW_PLAYER_COUNT  12

#define MKW_PLAYER_VEHICLE   0x08 /* u32 (see mkw_vehicle) */
#define MKW_PLAYER_CHARACTER 0x0C /* u32 (see mkw_character) */
#define MKW_PLAYER_TYPE      0x10 /* u32 (see mkw_player_type) */

typedef enum
{
  MKW_PLAYER_TYPE_LOCAL = 0,
  MKW_PLAYER_TYPE_CPU = 1
} mkw_player_type;

/* Race settings of the menu scenario, after its 12 players. Also in the heap. */
#define MKW_SETTINGS_ADDR (MKW_PLAYERS_ADDR + MKW_PLAYER_STRIDE * MKW_PLAYER_COUNT)

/* u16 laps to race, in the race scenario's settings. Retro Rewind sets its own
 * count while the race loads, so it's held (see MKW_LAP_COUNT_HOLD_FRAMES). */
static const dr_value_t MKW_LAP_COUNT = { 0x9089399C, DR_VALUE_TYPE_U16 };
#define MKW_LAP_COUNT_HOLD_FRAMES (15 * 60)

/* u16 laps (race number byte, then lap count) in the menu scenario's settings,
 * which is copied over the race scenario when a race starts */
static const dr_value_t MKW_MENU_LAP_COUNT = { MKW_SETTINGS_ADDR + 0x24, DR_VALUE_TYPE_U16 };

/* u32 Retro Rewind track id; overrides the vanilla course id */
static const dr_value_t MKW_RR_TRACK = { 0x804F440C, DR_VALUE_TYPE_U32 };
#define MKW_RR_TRACK_FIRST 0x100 /* SNES Mario Circuit 1 */
#define MKW_RR_TRACK_LAST  0x1A9 /* SW2 Rainbow Road */

/* bool - whether the Retro Rewind track is its alternate version */
static const dr_value_t MKW_RR_TRACK_ALT = { 0x804F4414, DR_VALUE_TYPE_U8 };

/* RaceManager*, null outside of a race */
static const dr_value_t MKW_RACE_MANAGER_PTR = { 0x809B8F70, DR_VALUE_TYPE_POINTER };

/* RaceManager fields */
#define MKW_RACE_MANAGER_PLAYERS 0x0C /* RaceManagerPlayer*[] indexed by player */

/* RaceManagerPlayer fields */
#define MKW_RACE_PLAYER_FLAGS    0x38 /* u32 */
#define MKW_RACE_PLAYER_POSITION 0x20 /* s8, 1 = 1st */
#define MKW_RACE_PLAYER_FINISHED 0x2

/* KPadDirector*, always allocated */
static const dr_value_t MKW_KPAD_DIRECTOR_PTR = { 0x809B8F4C, DR_VALUE_TYPE_POINTER };

/* KPadDirector fields; mirrors KPadDirector::setPlayerController(i, CLASSIC, i) */
#define MKW_KPAD_PLAYERS         0x0004 /* KPadPlayer[4] */
#define MKW_KPAD_PLAYER_STRIDE   0xEC
#define MKW_KPAD_WII_CONTROLLERS 0x1720 /* KPadWiiController[4] */
#define MKW_KPAD_WII_STRIDE      0x920

/* KPadPlayer fields */
#define MKW_KPAD_CONTROLLER     0x04 /* KPadController*, x3 (mController/2/3) */
#define MKW_KPAD_INFO_SOURCE    0xC8 /* eControlSource */
#define MKW_KPAD_INFO_CHAN      0xD4 /* s32 */

/* KPadWiiController fields; calcInner rewrites type/source from the WPAD probe */
#define MKW_KPAD_WII_CHAN       0x8D4 /* s32 */
#define MKW_KPAD_WII_TYPE       0x8D8 /* s32 */
#define MKW_KPAD_WII_SOURCE     0x8DC /* eControlSource */

/* The binary's control sources are one lower than the decomp header's enum
 * (classic 2, GC 3) */
#define MKW_CONTROL_SOURCE_CLASSIC 2

/* SectionManager*, always allocated (found in the savestate; NTSC-U's .bss
 * layout differs from PAL's here) */
static const dr_value_t MKW_SECTION_MANAGER_PTR = { 0x809BD508, DR_VALUE_TYPE_POINTER };

/* u32[] (0x10 apart) - the menu's registered controller per local player.
 * changeSection re-applies these through setPlayerController, overriding any
 * direct KPadDirector binding. Bits 8-11 are the channel + 1; the low byte is
 * the type (0x11 Wiimote, 0x12 Nunchuck, 0x13 Classic, 0x24 GC). */
#define MKW_SECTION_PADS        0x38
#define MKW_SECTION_PAD_STRIDE  0x10
#define MKW_SECTION_PAD_MASK    0xFFF
#define MKW_SECTION_PAD_CLASSIC 0x13

/* SaveManager*, always allocated */
static const dr_value_t MKW_SAVE_MANAGER_PTR = { 0x809B8F88, DR_VALUE_TYPE_POINTER };

/* u8 - SaveManager's "can save" flag. With it clear every license/ghost save
 * is skipped and reports success, so the race starts without a NAND error. */
#define MKW_SAVE_MANAGER_CAN_SAVE 0x25002

/* u8[MKW_CHARACTER_SIZE] costume worn by each character, indexed by mkw_character */
static const size_t MKW_COSTUMES_ADDR = 0x80418750;

/* Retro Rewind code patches so CPUs read the per-character costume table instead
 * of a random roll (one local human) or costume 0 (two or more); RMCETO v6.12 */
static const struct
{
  size_t addr;
  uint32_t original;
  uint32_t patched;
} MKW_CPU_COSTUME_PATCHES[] = {
  { 0x8039CAF0, 0x4182005C, 0x4800005C }, /* beq -> b: skip the random CPU roll */
  { 0x8039CB84, 0x41820014, 0x60000000 }, /* nop: non-local racers use the table */
  { 0x8039CC24, 0x41820014, 0x60000000 }, /* nop: same, single local player */
};

typedef enum
{
  MKW_CHARACTER_MARIO = 0x00,
  MKW_CHARACTER_BABY_PEACH = 0x01,
  MKW_CHARACTER_WALUIGI = 0x02,
  MKW_CHARACTER_BOWSER = 0x03,
  MKW_CHARACTER_BABY_DAISY = 0x04,
  MKW_CHARACTER_DRY_BONES = 0x05,
  MKW_CHARACTER_BABY_MARIO = 0x06,
  MKW_CHARACTER_LUIGI = 0x07,
  MKW_CHARACTER_TOAD = 0x08,
  MKW_CHARACTER_DONKEY_KONG = 0x09,
  MKW_CHARACTER_YOSHI = 0x0A,
  MKW_CHARACTER_WARIO = 0x0B,
  MKW_CHARACTER_BABY_LUIGI = 0x0C,
  MKW_CHARACTER_TOADETTE = 0x0D,
  MKW_CHARACTER_KOOPA_TROOPA = 0x0E,
  MKW_CHARACTER_DAISY = 0x0F,
  MKW_CHARACTER_PEACH = 0x10,
  MKW_CHARACTER_BIRDO = 0x11,
  MKW_CHARACTER_DIDDY_KONG = 0x12,
  MKW_CHARACTER_KING_BOO = 0x13,
  MKW_CHARACTER_BOWSER_JR = 0x14,
  MKW_CHARACTER_DRY_BOWSER = 0x15,
  MKW_CHARACTER_FUNKY_KONG = 0x16,
  MKW_CHARACTER_ROSALINA = 0x17,
  MKW_CHARACTER_MII_S_A_MALE = 0x18,
  MKW_CHARACTER_MII_S_A_FEMALE = 0x19,
  MKW_CHARACTER_MII_S_B_MALE = 0x1A,
  MKW_CHARACTER_MII_S_B_FEMALE = 0x1B,
  MKW_CHARACTER_MII_S_C_MALE = 0x1C,
  MKW_CHARACTER_MII_S_C_FEMALE = 0x1D,
  MKW_CHARACTER_MII_M_A_MALE = 0x1E,
  MKW_CHARACTER_MII_M_A_FEMALE = 0x1F,
  MKW_CHARACTER_MII_M_B_MALE = 0x20,
  MKW_CHARACTER_MII_M_B_FEMALE = 0x21,
  MKW_CHARACTER_MII_M_C_MALE = 0x22,
  MKW_CHARACTER_MII_M_C_FEMALE = 0x23,
  MKW_CHARACTER_MII_L_A_MALE = 0x24,
  MKW_CHARACTER_MII_L_A_FEMALE = 0x25,
  MKW_CHARACTER_MII_L_B_MALE = 0x26,
  MKW_CHARACTER_MII_L_B_FEMALE = 0x27,
  MKW_CHARACTER_MII_L_C_MALE = 0x28,
  MKW_CHARACTER_MII_L_C_FEMALE = 0x29,
  MKW_CHARACTER_MII_M = 0x2A,
  MKW_CHARACTER_MII_S = 0x2B,
  MKW_CHARACTER_MII_L = 0x2C,
  MKW_CHARACTER_PEACH_BIKER = 0x2D,
  MKW_CHARACTER_DAISY_BIKER = 0x2E,
  MKW_CHARACTER_ROSALINA_BIKER = 0x2F,

  MKW_CHARACTER_SIZE
} mkw_character;

typedef enum
{
  MKW_VEHICLE_STANDARD_KART_S = 0x00,
  MKW_VEHICLE_STANDARD_KART_M = 0x01,
  MKW_VEHICLE_STANDARD_KART_L = 0x02,
  MKW_VEHICLE_BABY_BOOSTER = 0x03,
  MKW_VEHICLE_CLASSIC_DRAGSTER = 0x04,
  MKW_VEHICLE_OFFROADER = 0x05,
  MKW_VEHICLE_MINI_BEAST = 0x06,
  MKW_VEHICLE_WILD_WING = 0x07,
  MKW_VEHICLE_FLAME_FLYER = 0x08,
  MKW_VEHICLE_CHEEP_CHARGER = 0x09,
  MKW_VEHICLE_SUPER_BLOOPER = 0x0A,
  MKW_VEHICLE_PIRANHA_PROWLER = 0x0B,
  MKW_VEHICLE_RALLY_ROMPER = 0x0C,
  MKW_VEHICLE_ROYAL_RACER = 0x0D,
  MKW_VEHICLE_JETSETTER = 0x0E,
  MKW_VEHICLE_BLUE_FALCON = 0x0F,
  MKW_VEHICLE_SPRINTER = 0x10,
  MKW_VEHICLE_HONEYCOUPE = 0x11,
  MKW_VEHICLE_STANDARD_BIKE_S = 0x12,
  MKW_VEHICLE_STANDARD_BIKE_M = 0x13,
  MKW_VEHICLE_STANDARD_BIKE_L = 0x14,
  MKW_VEHICLE_BULLET_BIKE = 0x15,
  MKW_VEHICLE_MACH_BIKE = 0x16,
  MKW_VEHICLE_BOWSER_BIKE = 0x17,
  MKW_VEHICLE_BIT_BIKE = 0x18,
  MKW_VEHICLE_BON_BON = 0x19,
  MKW_VEHICLE_WARIO_BIKE = 0x1A,
  MKW_VEHICLE_QUACKER = 0x1B,
  MKW_VEHICLE_RAPIDE = 0x1C,
  MKW_VEHICLE_SHOOTING_STAR = 0x1D,
  MKW_VEHICLE_MAGIKRUISER = 0x1E,
  MKW_VEHICLE_NITROCYCLE = 0x1F,
  MKW_VEHICLE_SPEAR = 0x20,
  MKW_VEHICLE_JET_BUBBLE = 0x21,
  MKW_VEHICLE_DOLPHIN_DASHER = 0x22,
  MKW_VEHICLE_PHANTOM = 0x23,

  MKW_VEHICLE_SIZE
} mkw_vehicle;

/* Vehicles come in small/medium/large triples, so a weight class is an offset into one */
typedef enum
{
  MKW_WEIGHT_SMALL = 0,
  MKW_WEIGHT_MEDIUM = 1,
  MKW_WEIGHT_LARGE = 2
} mkw_weight;

static mkw_weight mkw_character_weight(mkw_character character)
{
  switch (character)
  {
  case MKW_CHARACTER_BABY_PEACH:
  case MKW_CHARACTER_BABY_DAISY:
  case MKW_CHARACTER_DRY_BONES:
  case MKW_CHARACTER_BABY_MARIO:
  case MKW_CHARACTER_TOAD:
  case MKW_CHARACTER_BABY_LUIGI:
  case MKW_CHARACTER_TOADETTE:
  case MKW_CHARACTER_KOOPA_TROOPA:
  case MKW_CHARACTER_MII_S_A_MALE:
  case MKW_CHARACTER_MII_S_A_FEMALE:
  case MKW_CHARACTER_MII_S_B_MALE:
  case MKW_CHARACTER_MII_S_B_FEMALE:
  case MKW_CHARACTER_MII_S_C_MALE:
  case MKW_CHARACTER_MII_S_C_FEMALE:
  case MKW_CHARACTER_MII_S:
    return MKW_WEIGHT_SMALL;
  case MKW_CHARACTER_WALUIGI:
  case MKW_CHARACTER_BOWSER:
  case MKW_CHARACTER_DONKEY_KONG:
  case MKW_CHARACTER_WARIO:
  case MKW_CHARACTER_KING_BOO:
  case MKW_CHARACTER_DRY_BOWSER:
  case MKW_CHARACTER_FUNKY_KONG:
  case MKW_CHARACTER_ROSALINA:
  case MKW_CHARACTER_MII_L_A_MALE:
  case MKW_CHARACTER_MII_L_A_FEMALE:
  case MKW_CHARACTER_MII_L_B_MALE:
  case MKW_CHARACTER_MII_L_B_FEMALE:
  case MKW_CHARACTER_MII_L_C_MALE:
  case MKW_CHARACTER_MII_L_C_FEMALE:
  case MKW_CHARACTER_MII_L:
  case MKW_CHARACTER_ROSALINA_BIKER:
    return MKW_WEIGHT_LARGE;
  default:
    return MKW_WEIGHT_MEDIUM;
  }
}

typedef struct
{
  mkw_character character;
  uint8_t costume;
} mkw_character_costume_t;

static mkw_character_costume_t mkw_character_costume(dr_character character)
{
  mkw_character_costume_t cc = { MKW_CHARACTER_MARIO, 0 };

  switch (character)
  {
  case DR_CHARACTER_MARIO:
    cc.character = MKW_CHARACTER_MARIO;
    break;
  case DR_CHARACTER_LUIGI:
    cc.character = MKW_CHARACTER_LUIGI;
    break;
  case DR_CHARACTER_PEACH:
    cc.character = MKW_CHARACTER_PEACH;
    break;
  case DR_CHARACTER_YOSHI:
    cc.character = MKW_CHARACTER_YOSHI;
    break;
  case DR_CHARACTER_WARIO:
    cc.character = MKW_CHARACTER_WARIO;
    break;
  case DR_CHARACTER_DONKEY_KONG:
    cc.character = MKW_CHARACTER_DONKEY_KONG;
    break;
  case DR_CHARACTER_WALUIGI:
    cc.character = MKW_CHARACTER_WALUIGI;
    break;
  case DR_CHARACTER_DAISY:
    cc.character = MKW_CHARACTER_DAISY;
    break;
  case DR_CHARACTER_TOAD:
    cc.character = MKW_CHARACTER_TOAD;
    break;
  case DR_CHARACTER_BOO:
    cc.character = MKW_CHARACTER_KING_BOO;
    break;
  case DR_CHARACTER_KOOPA_KID:
  case DR_CHARACTER_KOOPA_KID_R:
  case DR_CHARACTER_KOOPA_KID_G:
  case DR_CHARACTER_KOOPA_KID_B:
    cc.character = MKW_CHARACTER_BOWSER_JR;
    break;
  case DR_CHARACTER_TOADETTE:
    cc.character = MKW_CHARACTER_TOADETTE;
    break;
  case DR_CHARACTER_BIRDO:
    cc.character = MKW_CHARACTER_BIRDO;
    break;
  case DR_CHARACTER_DRY_BONES:
    cc.character = MKW_CHARACTER_DRY_BONES;
    break;
  case DR_CHARACTER_BLOOPER:
    cc.character = MKW_CHARACTER_TOADETTE;
    cc.costume = 3;
    break;
  case DR_CHARACTER_HAMMER_BRO:
    cc.character = MKW_CHARACTER_BOWSER_JR;
    cc.costume = 2;
    break;
/*
  case DR_CHARACTER_KOOPA_TROOPA:
    cc.character = MKW_CHARACTER_KOOPA_TROOPA;
    break;
  case DR_CHARACTER_SHY_GUY:
    cc.character = MKW_CHARACTER_BABY_MARIO;
    cc.costume = 4;
    break;
  case DR_CHARACTER_MAGIKOOPA:
    cc.character = MKW_CHARACTER_BOWSER_JR;
    cc.costume = 3;
    break;
  case DR_CHARACTER_BOWSER_JR:
    cc.character = MKW_CHARACTER_BOWSER_JR;
    break;
  case DR_CHARACTER_ROSALINA:
    cc.character = MKW_CHARACTER_ROSALINA;
    break;
  case DR_CHARACTER_SPIKE:
    cc.character = MKW_CHARACTER_BABY_LUIGI;
    cc.costume = 3;
    break;
  case DR_CHARACTER_DIDDY_KONG:
    cc.character = MKW_CHARACTER_DIDDY_KONG;
    break;
  case DR_CHARACTER_BOWSER:
    cc.character = MKW_CHARACTER_BOWSER;
    break;
  case DR_CHARACTER_GOOMBA:
    cc.character = MKW_CHARACTER_DRY_BONES;
    cc.costume = 3;
    break;
  case DR_CHARACTER_MONTY_MOLE:
    cc.character = MKW_CHARACTER_LUIGI;
    cc.costume = 3;
    break;
  case DR_CHARACTER_POM_POM:
    cc.character = MKW_CHARACTER_BOWSER;
    cc.costume = 4;
    break;
  case DR_CHARACTER_PAULINE:
    cc.character = MKW_CHARACTER_ROSALINA;
    cc.costume = 3;
    break;
*/
  default:
    break;
  }

  return cc;
}

#define MKW_RR_TRACK_ALT_FLAG 0x10000

static const dr_mp_minigame_t MKW_MINIGAMES[] =
{
  { "MKW: SNES Mario Circuit 1", DR_MINIGAME_4P, 0x100, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Donut Plains 1", DR_MINIGAME_4P, 0x101, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Ghost Valley 1", DR_MINIGAME_4P, 0x102, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Bowser Castle 1", DR_MINIGAME_4P, 0x103, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Mario Circuit 2", DR_MINIGAME_4P, 0x104, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Choco Island 1", DR_MINIGAME_4P, 0x105, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Ghost Valley 2", DR_MINIGAME_4P, 0x106, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Donut Plains 2", DR_MINIGAME_4P, 0x107, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Bowser Castle 2", DR_MINIGAME_4P, 0x108, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Mario Circuit 3", DR_MINIGAME_4P, 0x109, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Koopa Beach 1", DR_MINIGAME_4P, 0x10A, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Choco Island 2", DR_MINIGAME_4P, 0x10B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Vanilla Lake 1", DR_MINIGAME_4P, 0x10C, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Bowser Castle 3", DR_MINIGAME_4P, 0x10D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Mario Circuit 4", DR_MINIGAME_4P, 0x10E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Donut Plains 3", DR_MINIGAME_4P, 0x10F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Koopa Beach 2", DR_MINIGAME_4P, 0x110, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Ghost Valley 3", DR_MINIGAME_4P, 0x111, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Vanilla Lake 2", DR_MINIGAME_4P, 0x112, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Rainbow Road", DR_MINIGAME_4P, 0x113, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Luigi Raceway", DR_MINIGAME_4P, 0x114, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Moo Moo Farm", DR_MINIGAME_4P, 0x115, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Koopa Troopa Beach", DR_MINIGAME_4P, 0x116, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Kalimari Desert", DR_MINIGAME_4P, 0x117, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Toad's Turnpike", DR_MINIGAME_4P, 0x118, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Frappe Snowland", DR_MINIGAME_4P, 0x119, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Choco Mountain", DR_MINIGAME_4P, 0x11A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Mario Raceway", DR_MINIGAME_4P, 0x11B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Wario Stadium", DR_MINIGAME_4P, 0x11C, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Sherbet Land", DR_MINIGAME_4P, 0x11D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Royal Raceway", DR_MINIGAME_4P, 0x11E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Bowser's Castle", DR_MINIGAME_4P, 0x11F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 DK's Jungle Parkway", DR_MINIGAME_4P, 0x120, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Yoshi Valley", DR_MINIGAME_4P, 0x121, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Banshee Boardwalk", DR_MINIGAME_4P, 0x122, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: N64 Rainbow Road", DR_MINIGAME_4P, 0x123, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Peach Circuit", DR_MINIGAME_4P, 0x124, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Shy Guy Beach", DR_MINIGAME_4P, 0x125, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Riverside Park", DR_MINIGAME_4P, 0x126, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Bowser Castle 1", DR_MINIGAME_4P, 0x127, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Mario Circuit", DR_MINIGAME_4P, 0x128, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Boo Lake", DR_MINIGAME_4P, 0x129, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Cheese Land", DR_MINIGAME_4P, 0x12A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Bowser Castle 2", DR_MINIGAME_4P, 0x12B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Luigi Circuit", DR_MINIGAME_4P, 0x12C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Sky Garden", DR_MINIGAME_4P, 0x12D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Cheep-Cheep Island", DR_MINIGAME_4P, 0x12E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Sunset Wilds", DR_MINIGAME_4P, 0x12F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Snow Land", DR_MINIGAME_4P, 0x130, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Ribbon Road", DR_MINIGAME_4P, 0x131, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Yoshi Desert", DR_MINIGAME_4P, 0x132, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Bowser Castle 3", DR_MINIGAME_4P, 0x133, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Lakeside Park", DR_MINIGAME_4P, 0x134, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Broken Pier", DR_MINIGAME_4P, 0x135, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Bowser Castle 4", DR_MINIGAME_4P, 0x136, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Rainbow Road", DR_MINIGAME_4P, 0x137, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Luigi Circuit", DR_MINIGAME_4P, 0x138, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Peach Beach", DR_MINIGAME_4P, 0x139, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Baby Park", DR_MINIGAME_4P, 0x13A, 5, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Dry Dry Desert", DR_MINIGAME_4P, 0x13B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Mushroom Bridge", DR_MINIGAME_4P, 0x13C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Mario Circuit", DR_MINIGAME_4P, 0x13D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Daisy Cruiser", DR_MINIGAME_4P, 0x13E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Waluigi Stadium", DR_MINIGAME_4P, 0x13F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Sherbet Land", DR_MINIGAME_4P, 0x140, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Mushroom City", DR_MINIGAME_4P, 0x141, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Yoshi Circuit", DR_MINIGAME_4P, 0x142, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN DK Mountain", DR_MINIGAME_4P, 0x143, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Wario Colosseum", DR_MINIGAME_4P, 0x144, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Dino Dino Jungle", DR_MINIGAME_4P, 0x145, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Bowser's Castle", DR_MINIGAME_4P, 0x146, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GCN Rainbow Road", DR_MINIGAME_4P, 0x147, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Figure 8 Circuit", DR_MINIGAME_4P, 0x148, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Yoshi Falls", DR_MINIGAME_4P, 0x149, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Cheep Cheep Beach", DR_MINIGAME_4P, 0x14A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Luigi's Mansion", DR_MINIGAME_4P, 0x14B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Desert Hills", DR_MINIGAME_4P, 0x14C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Delfino Square", DR_MINIGAME_4P, 0x14D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Waluigi Pinball", DR_MINIGAME_4P, 0x14E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Shroom Ridge", DR_MINIGAME_4P, 0x14F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS DK Pass", DR_MINIGAME_4P, 0x150, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Tick-Tock Clock", DR_MINIGAME_4P, 0x151, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Mario Circuit", DR_MINIGAME_4P, 0x152, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Airship Fortress", DR_MINIGAME_4P, 0x153, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Wario Stadium", DR_MINIGAME_4P, 0x154, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Peach Gardens", DR_MINIGAME_4P, 0x155, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Bowser Castle", DR_MINIGAME_4P, 0x156, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: DS Rainbow Road", DR_MINIGAME_4P, 0x157, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Luigi Circuit", DR_MINIGAME_4P, 0x158, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Moo Moo Meadows", DR_MINIGAME_4P, 0x159, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Mushroom Gorge", DR_MINIGAME_4P, 0x15A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Toad's Factory", DR_MINIGAME_4P, 0x15B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Mario Circuit", DR_MINIGAME_4P, 0x15C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Coconut Mall", DR_MINIGAME_4P, 0x15D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii DK Summit", DR_MINIGAME_4P, 0x15E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Wario's Gold Mine", DR_MINIGAME_4P, 0x15F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Daisy Circuit", DR_MINIGAME_4P, 0x160, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Koopa Cape", DR_MINIGAME_4P, 0x161, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Maple Treeway", DR_MINIGAME_4P, 0x162, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Grumble Volcano", DR_MINIGAME_4P, 0x163, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Dry Dry Ruins", DR_MINIGAME_4P, 0x164, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Moonview Highway", DR_MINIGAME_4P, 0x165, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Bowser's Castle", DR_MINIGAME_4P, 0x166, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii Rainbow Road", DR_MINIGAME_4P, 0x167, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Toad Circuit", DR_MINIGAME_4P, 0x168, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Daisy Hills", DR_MINIGAME_4P, 0x169, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Cheep Cheep Lagoon", DR_MINIGAME_4P, 0x16A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Shy Guy Bazaar", DR_MINIGAME_4P, 0x16B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Wuhu Loop", DR_MINIGAME_4P, 0x16C, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Mario Circuit", DR_MINIGAME_4P, 0x16D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Music Park", DR_MINIGAME_4P, 0x16E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Rock Rock Mountain", DR_MINIGAME_4P, 0x16F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Piranha Plant Slide", DR_MINIGAME_4P, 0x170, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Wario's Shipyard", DR_MINIGAME_4P, 0x171, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Neo Bowser City", DR_MINIGAME_4P, 0x172, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Maka Wuhu", DR_MINIGAME_4P, 0x173, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS DK Jungle", DR_MINIGAME_4P, 0x174, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Rosalina's Ice World", DR_MINIGAME_4P, 0x175, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Bowser's Castle", DR_MINIGAME_4P, 0x176, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: 3DS Rainbow Road", DR_MINIGAME_4P, 0x177, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Mario Kart Stadium", DR_MINIGAME_4P, 0x178, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Sweet Sweet Canyon", DR_MINIGAME_4P, 0x179, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Thwomp Ruins", DR_MINIGAME_4P, 0x17A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Mario Circuit", DR_MINIGAME_4P, 0x17B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Toad Harbor", DR_MINIGAME_4P, 0x17C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Sunshine Airport", DR_MINIGAME_4P, 0x17D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Electrodrome", DR_MINIGAME_4P, 0x17E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Cloudtop Cruise", DR_MINIGAME_4P, 0x17F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Bone-Dry Dunes", DR_MINIGAME_4P, 0x180, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Rainbow Road", DR_MINIGAME_4P, 0x181, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Mute City", DR_MINIGAME_4P, 0x182, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Excitebike Arena", DR_MINIGAME_4P, 0x183, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Hyrule Circuit", DR_MINIGAME_4P, 0x184, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Wild Woods", DR_MINIGAME_4P, 0x185, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Animal Crossing", DR_MINIGAME_4P, 0x186, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Super Bell Subway", DR_MINIGAME_4P, 0x187, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Big Blue", DR_MINIGAME_4P, 0x188, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW Sky-High Sundae", DR_MINIGAME_4P, 0x189, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Vancouver Velocity", DR_MINIGAME_4P, 0x18A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Singapore Speedway", DR_MINIGAME_4P, 0x18B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Athens Dash", DR_MINIGAME_4P, 0x18C, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Madrid Drive", DR_MINIGAME_4P, 0x18D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Mario Circuit 1", DR_MINIGAME_4P, 0x18E, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Choco Island 1", DR_MINIGAME_4P, 0x18F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Choco Island 2", DR_MINIGAME_4P, 0x190, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Rainbow Road 1", DR_MINIGAME_4P, 0x191, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Rainbow Road 2", DR_MINIGAME_4P, 0x192, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Vanilla Lake 1", DR_MINIGAME_4P, 0x193, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Vanilla Lake 2", DR_MINIGAME_4P, 0x194, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Ghost Valley 1", DR_MINIGAME_4P, 0x195, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Bowser Castle 1", DR_MINIGAME_4P, 0x196, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: RMX Donut Plains 1", DR_MINIGAME_4P, 0x197, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GP Bananan Ruins", DR_MINIGAME_4P, 0x198, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GP Diamond City", DR_MINIGAME_4P, 0x199, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GP Bowser's Castle", DR_MINIGAME_4P, 0x19A, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Mario Bros. Circuit", DR_MINIGAME_4P, 0x19B, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Crown City", DR_MINIGAME_4P, 0x19C, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Whistlestop Summit", DR_MINIGAME_4P, 0x19D, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 DK Spaceport", DR_MINIGAME_4P, 0x19E, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Faraway Oasis", DR_MINIGAME_4P, 0x19F, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Peach Resort", DR_MINIGAME_4P, 0x1A0, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Great ? Block Ruins", DR_MINIGAME_4P, 0x1A1, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Cheep Cheep Falls", DR_MINIGAME_4P, 0x1A2, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Dandelion Depths", DR_MINIGAME_4P, 0x1A3, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Boo Cinema", DR_MINIGAME_4P, 0x1A4, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Ghost Valley", DR_MINIGAME_4P, 0x1A5, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Bowser's Castle", DR_MINIGAME_4P, 0x1A6, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Mario Circuit", DR_MINIGAME_4P, 0x1A7, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Peach Stadium", DR_MINIGAME_4P, 0x1A8, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Rainbow Road", DR_MINIGAME_4P, 0x1A9, 1, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  // { "MKW: Beta Donut Plains 1", DR_MINIGAME_4P, 0x101 | MKW_RR_TRACK_ALT_FLAG, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Choco Island 1 DX", DR_MINIGAME_4P, 0x105 | MKW_RR_TRACK_ALT_FLAG, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  // { "MKW: Beta Donut Plains 2", DR_MINIGAME_4P, 0x107 | MKW_RR_TRACK_ALT_FLAG, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Choco Island 2 DX", DR_MINIGAME_4P, 0x10B | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Bowser Castle 3", DR_MINIGAME_4P, 0x10D | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SNES Rainbow Road U", DR_MINIGAME_4P, 0x113 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW Kalimari Desert", DR_MINIGAME_4P, 0x117 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Choco Mountain", DR_MINIGAME_4P, 0x11A | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Royal Raceway", DR_MINIGAME_4P, 0x11E | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Peach Circuit", DR_MINIGAME_4P, 0x124 | MKW_RR_TRACK_ALT_FLAG, 3, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Riverside Park", DR_MINIGAME_4P, 0x126 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Mario Circuit U", DR_MINIGAME_4P, 0x128 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Boo Lake", DR_MINIGAME_4P, 0x129 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Cheese Land", DR_MINIGAME_4P, 0x12A | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Luigi Circuit", DR_MINIGAME_4P, 0x12C | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Sky Garden", DR_MINIGAME_4P, 0x12D | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Cheep-Cheep Island", DR_MINIGAME_4P, 0x12E | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Sunset Wilds", DR_MINIGAME_4P, 0x12F | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Snow Land", DR_MINIGAME_4P, 0x130 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Ribbon Road", DR_MINIGAME_4P, 0x131 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour Lakeside Park", DR_MINIGAME_4P, 0x134 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: GBA Rainbow Road DX", DR_MINIGAME_4P, 0x137 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Peach Beach", DR_MINIGAME_4P, 0x139 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Baby Park", DR_MINIGAME_4P, 0x13A | MKW_RR_TRACK_ALT_FLAG, 5, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Dry Dry Desert", DR_MINIGAME_4P, 0x13B | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Sherbet Land", DR_MINIGAME_4P, 0x140 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Cheep Cheep Beach", DR_MINIGAME_4P, 0x14A | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  // { "MKW: Beta Delfino Square", DR_MINIGAME_4P, 0x14D | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  // { "MKW: Beta Waluigi Pinball", DR_MINIGAME_4P, 0x14E | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW Shroom Ridge", DR_MINIGAME_4P, 0x14F | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 DK Pass", DR_MINIGAME_4P, 0x150 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Tick-Tock Clock", DR_MINIGAME_4P, 0x151 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Wii U Wario Stadium", DR_MINIGAME_4P, 0x154 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW Peach Gardens", DR_MINIGAME_4P, 0x155 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Toad's Factory", DR_MINIGAME_4P, 0x15B | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: Tour DK Summit", DR_MINIGAME_4P, 0x15E | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "MKW: SW2 Sky-High Sundae", DR_MINIGAME_4P, 0x189 | MKW_RR_TRACK_ALT_FLAG, 2, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

MarioKartWii::MarioKartWii(QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_corePath(dr_core_path(DR_CORE_DOLPHIN).toStdString())
  , m_discPath((dr_roms_directory() + "/Mario Kart Retro Rewind [RMCETO]").toStdString())
  , m_statePath((dr_state_directory() + "/mariokartretrorewind.state.zip").toStdString())
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioKartWii::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioKartWii::run()
{
  m_retro->tickFrameWrites();

  /* The savestate sits on the menu's "OK"; start once the race scene exists */
  if (!m_minigameActive)
  {
    int64_t manager = 0;

    if (m_retro->readValue(&manager, MKW_RACE_MANAGER_PTR) == DR_OK && manager)
      startMinigame();
    return;
  }

  m_minigameFrames++;

  /* The first of our four to cross the line wins; ties go to the better position */
  if (m_winner < 0)
  {
    unsigned i;
    uint8_t best = 0;

    for (i = 0; i < 4; i++)
    {
      const size_t player = racePlayerAddr(i);
      uint32_t flags = 0;
      uint8_t position = 0;

      if (!player || m_retro->readu32(&flags, player + MKW_RACE_PLAYER_FLAGS) != DR_OK ||
          !(flags & MKW_RACE_PLAYER_FINISHED))
        continue;
      m_retro->readu8(&position, player + MKW_RACE_PLAYER_POSITION);
      if (m_winner < 0 || position < best)
      {
        m_winner = static_cast<int>(i);
        best = position;
      }
    }

    if (m_winner >= 0)
    {
      log(DR_LOG_INFO, qPrintable(QString("MKW: player %1 finished first").arg(m_winner)));
      finishMinigameInFrames(340);
    }
  }
}

void MarioKartWii::disableSaving(void)
{
  int64_t manager = 0;

  if (m_retro->readValue(&manager, MKW_SAVE_MANAGER_PTR) != DR_OK || !manager)
  {
    log(DR_LOG_WARN, "MKW: SaveManager unreadable, saving left enabled");
    return;
  }
  m_retro->writeu8(0, static_cast<size_t>(manager) + MKW_SAVE_MANAGER_CAN_SAVE);
}

void MarioKartWii::patchCpuCostumes(void)
{
  for (const auto &patch : MKW_CPU_COSTUME_PATCHES)
  {
    uint32_t word = 0;

    if (m_retro->readu32(&word, patch.addr) != DR_OK)
      log(DR_LOG_WARN, qPrintable(QString("MKW: costume patch at 0x%1 unreadable")
        .arg(patch.addr, 8, 16, QChar('0'))));
    else if (word == patch.original)
      m_retro->writeu32(patch.patched, patch.addr);
    else if (word != patch.patched)
      log(DR_LOG_WARN, qPrintable(QString("MKW: costume patch at 0x%1 skipped (found 0x%2, "
        "different Retro Rewind build?)").arg(patch.addr, 8, 16, QChar('0'))
        .arg(word, 8, 16, QChar('0'))));
  }
}

void MarioKartWii::bindClassicControllers(void)
{
  int64_t director = 0;
  int64_t sections = 0;
  unsigned i;

  if (m_retro->readValue(&director, MKW_KPAD_DIRECTOR_PTR) != DR_OK || !director)
  {
    log(DR_LOG_WARN, "MKW: KPadDirector unreadable, controllers left as-is");
    return;
  }

  for (i = 0; i < 4; i++)
  {
    const size_t player = static_cast<size_t>(director) + MKW_KPAD_PLAYERS + i * MKW_KPAD_PLAYER_STRIDE;
    const uint32_t wii = static_cast<uint32_t>(director) + MKW_KPAD_WII_CONTROLLERS + i * MKW_KPAD_WII_STRIDE;
    unsigned j;

    m_retro->writeu32(i, wii + MKW_KPAD_WII_CHAN);
    m_retro->writeu32(MKW_CONTROL_SOURCE_CLASSIC, wii + MKW_KPAD_WII_TYPE);
    m_retro->writeu32(MKW_CONTROL_SOURCE_CLASSIC, wii + MKW_KPAD_WII_SOURCE);
    for (j = 0; j < 3; j++)
      m_retro->writeu32(wii, player + MKW_KPAD_CONTROLLER + j * 4);
    m_retro->writeu32(MKW_CONTROL_SOURCE_CLASSIC, player + MKW_KPAD_INFO_SOURCE);
    m_retro->writeu32(i, player + MKW_KPAD_INFO_CHAN);
  }

  /* The race's section change rebinds from the menu's registry, so set it too */
  if (m_retro->readValue(&sections, MKW_SECTION_MANAGER_PTR) != DR_OK || !sections)
  {
    log(DR_LOG_WARN, "MKW: SectionManager unreadable, registered pads left as-is");
    return;
  }
  for (i = 0; i < 4; i++)
  {
    const size_t pad = static_cast<size_t>(sections) + MKW_SECTION_PADS + i * MKW_SECTION_PAD_STRIDE;
    uint32_t id = 0;

    m_retro->readu32(&id, pad);
    id = (id & ~MKW_SECTION_PAD_MASK) | ((i + 1) << 8) | MKW_SECTION_PAD_CLASSIC;
    m_retro->writeu32(id, pad);
  }
}

size_t MarioKartWii::racePlayerAddr(unsigned player)
{
  int64_t manager = 0;
  uint32_t players = 0;
  uint32_t addr = 0;

  if (m_retro->readValue(&manager, MKW_RACE_MANAGER_PTR) != DR_OK || !manager)
    return 0;
  if (m_retro->readu32(&players, static_cast<size_t>(manager) + MKW_RACE_MANAGER_PLAYERS) != DR_OK ||
      !players)
    return 0;
  if (m_retro->readu32(&addr, players + player * 4) != DR_OK)
    return 0;

  return addr;
}

const dr_mp_minigame_t *MarioKartWii::minigames() const
{
  return MKW_MINIGAMES;
}

void MarioKartWii::writePlayer(unsigned slot, dr_character character, bool cpu, unsigned group)
{
  const size_t player = MKW_PLAYERS_ADDR + slot * MKW_PLAYER_STRIDE;
  const mkw_character_costume_t cc = mkw_character_costume(character);

  m_retro->writeu32(cc.character, player + MKW_PLAYER_CHARACTER);
  m_retro->writeu32(cpu ? MKW_PLAYER_TYPE_CPU : MKW_PLAYER_TYPE_LOCAL, player + MKW_PLAYER_TYPE);

  /* Costumes are per character, so two players sharing one get the last's */
  m_retro->writeu8(cc.costume, MKW_COSTUMES_ADDR + cc.character);

  /* The race's vehicle, in the character's weight class */
  m_retro->writeu32(group * 3 + mkw_character_weight(cc.character), player + MKW_PLAYER_VEHICLE);
}

void MarioKartWii::doApplyGameData(const DrGameData &data)
{
  bool taken[MKW_CHARACTER_SIZE] = { false };
  dr_character pool[DR_CHARACTER_SIZE];
  unsigned count = 0;
  unsigned i;
  const int id = data.minigame ? data.minigame->minigame_id : 0;
  int track = id & ~MKW_RR_TRACK_ALT_FLAG;
  const unsigned group = dr_rand() % (MKW_VEHICLE_SIZE / 3); /* one vehicle for everyone */

  m_minigameFrames = 0;
  m_winner = -1;

  if (track < MKW_RR_TRACK_FIRST || track > MKW_RR_TRACK_LAST)
    track = MKW_RR_TRACK_FIRST + dr_rand() % (MKW_RR_TRACK_LAST - MKW_RR_TRACK_FIRST + 1);
  m_retro->writeValue(track, MKW_RR_TRACK);
  m_retro->writeValue((id & MKW_RR_TRACK_ALT_FLAG) ? 1 : 0, MKW_RR_TRACK_ALT);
  m_retro->writeValueForFrames(data.minigame ? data.minigame->scene_id : 1, MKW_LAP_COUNT,
    MKW_LAP_COUNT_HOLD_FRAMES);
  m_retro->writeValueForFrames(data.minigame ? data.minigame->scene_id : 1, MKW_MENU_LAP_COUNT,
    MKW_LAP_COUNT_HOLD_FRAMES);
  log(DR_LOG_INFO, qPrintable(QString("MKW: track 0x%1%2").arg(track, 0, 16)
    .arg((id & MKW_RR_TRACK_ALT_FLAG) ? " (alt)" : "")));

  bindClassicControllers();
  disableSaving();
  patchCpuCostumes(); /* before any frame runs, so the JIT compiles the patched code */

  for (i = 0; i < 4; i++)
  {
    writePlayer(dr_player_slot(m_players[i], i), m_players[i].character,
      m_players[i].control_type == DR_CONTROL_TYPE_CPU, group);
    taken[mkw_character_costume(m_players[i].character).character] = true;
  }

  /* The other racers are CPUs as random unplayed characters. Characters are
   * picked by game character so no two racers fight over one's costume. */
  for (i = DR_CHARACTER_INVALID + 1; i < DR_CHARACTER_SIZE; i++)
    pool[count++] = static_cast<dr_character>(i);
  for (i = 4; i < MKW_PLAYER_COUNT; i++)
  {
    dr_character character = DR_CHARACTER_INVALID;

    while (count > 0)
    {
      const unsigned pick = dr_rand() % count;
      const dr_character candidate = pool[pick];

      pool[pick] = pool[--count];
      if (!taken[mkw_character_costume(candidate).character])
      {
        character = candidate;
        break;
      }
    }
    if (character == DR_CHARACTER_INVALID)
      break;
    taken[mkw_character_costume(character).character] = true;
    writePlayer(i, character, true, group);
  }
}

dr_minigame_result_t MarioKartWii::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index >= 4)
    return result;

  if (m_winner >= 0 && dr_player_slot(m_players[index], index) == static_cast<unsigned>(m_winner))
    result.coins = 10;

  return result;
}
