#include "SmashRemix.h"

#include <QApplication>
#include <QFile>

static const dr_mp_minigame_t SR_MINIGAMES[] = {
  { "Remix Free-for-all", DR_MINIGAME_4P, 0x00, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Remix Battle", DR_MINIGAME_BATTLE, 0x00, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Remix Team Battle", DR_MINIGAME_2V2, 0x01, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Remix Giant Battle", DR_MINIGAME_1V3, 0x02, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Remix Tiny Battle", DR_MINIGAME_1V3, 0x03, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Remix Golden Gun", DR_MINIGAME_1V3, 0x04, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Remix Hammer", DR_MINIGAME_1V3, 0x06, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Remix PKMN", DR_MINIGAME_4P, 0x05, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Remix Duel", DR_MINIGAME_DUEL, 0x00, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/* Whether the game is in team battle mode */
static const dr_value_t SR_GAME_TYPE = { 0x800a4d0a, DR_VALUE_TYPE_U8 };
static const dr_value_t SR_STAGE = { 0x800a4d09, DR_VALUE_TYPE_U8 };

static const dr_value_t SR_CHARACTER[4] = {
  { 0x800a4d2b, DR_VALUE_TYPE_U8 },
  { 0x800a4d9f, DR_VALUE_TYPE_U8 },
  { 0x800a4e13, DR_VALUE_TYPE_U8 },
  { 0x800a4e87, DR_VALUE_TYPE_U8 }
};

/* Player type for this slot: 0 = human, 1 = CPU, 2 = inactive (no player) */
static const dr_value_t SR_PLAYER_TYPE[4] = {
  { 0x800a4d2a, DR_VALUE_TYPE_U8 },
  { 0x800a4d9e, DR_VALUE_TYPE_U8 },
  { 0x800a4e12, DR_VALUE_TYPE_U8 },
  { 0x800a4e86, DR_VALUE_TYPE_U8 }
};

static const dr_value_t SR_DIFFICULTY[4] = {
  { 0x800a4d28, DR_VALUE_TYPE_U8 },
  { 0x800a4dac, DR_VALUE_TYPE_U8 },
  { 0x800a4e10, DR_VALUE_TYPE_U8 },
  { 0x800a4e84, DR_VALUE_TYPE_U8 }
};

static const dr_value_t SR_COLOR[4] = {
  { 0x800a4d2e, DR_VALUE_TYPE_U8 },
  { 0x800a4da2, DR_VALUE_TYPE_U8 },
  { 0x800a4e16, DR_VALUE_TYPE_U8 },
  { 0x800a4e8a, DR_VALUE_TYPE_U8 }
};

static const dr_value_t SR_TEAM_1[4] = {
  { 0x800a4d2d, DR_VALUE_TYPE_U8 },
  { 0x800a4da1, DR_VALUE_TYPE_U8 },
  { 0x800a4e15, DR_VALUE_TYPE_U8 },
  { 0x800a4e89, DR_VALUE_TYPE_U8 }
};
static const dr_value_t SR_TEAM_2[4] = {
  { 0x800a4d2c, DR_VALUE_TYPE_U8 },
  { 0x800a4da0, DR_VALUE_TYPE_U8 },
  { 0x800a4e14, DR_VALUE_TYPE_U8 },
  { 0x800a4e88, DR_VALUE_TYPE_U8 }
};

static const dr_value_t SR_STOCKS[4] = {
  { 0x800a4d33, DR_VALUE_TYPE_S8 },
  { 0x800a4da7, DR_VALUE_TYPE_S8 },
  { 0x800a4e1b, DR_VALUE_TYPE_S8 },
  { 0x800a4e8f, DR_VALUE_TYPE_S8 }
};

/* The controller port for this player, or 4 if CPU-controlled */
static const dr_value_t SR_PORT[4] = {
  { 0x800a4d32, DR_VALUE_TYPE_U8 },
  { 0x800a4da6, DR_VALUE_TYPE_U8 },
  { 0x800a4e1a, DR_VALUE_TYPE_U8 },
  { 0x800a4e8e, DR_VALUE_TYPE_U8 }
};

/* The color drawn behind the percentage, should match SR_PORT */
static const dr_value_t SR_PORT_COLOR[4] = {
  { 0x800a4d30, DR_VALUE_TYPE_U8 },
  { 0x800a4da4, DR_VALUE_TYPE_U8 },
  { 0x800a4e18, DR_VALUE_TYPE_U8 },
  { 0x800a4e8c, DR_VALUE_TYPE_U8 }
};

/* The size of the player, a u32. 0=normal, 1=giant, 2=tiny */
static const dr_value_t SR_SIZE_1[4] = {
  { 0x80502fac, DR_VALUE_TYPE_U32 },
  { 0x80502fb0, DR_VALUE_TYPE_U32 },
  { 0x80502fb4, DR_VALUE_TYPE_U32 },
  { 0x80502fb8, DR_VALUE_TYPE_U32 }
};
static const dr_value_t SR_SIZE_2[4] = {
  { 0x80502fbc, DR_VALUE_TYPE_U32 },
  { 0x80502fc0, DR_VALUE_TYPE_U32 },
  { 0x80502fc4, DR_VALUE_TYPE_U32 },
  { 0x80502fc8, DR_VALUE_TYPE_U32 }
};

/* The item the player starts with */
static const dr_value_t SR_START_ITEM[4] = {
  { 0x80453748, DR_VALUE_TYPE_U32 },
  { 0x8045374c, DR_VALUE_TYPE_U32 },
  { 0x80453750, DR_VALUE_TYPE_U32 },
  { 0x80453754, DR_VALUE_TYPE_U32 }
};

/* The item the player can spawn by taunting */
static const dr_value_t SR_TAUNT_ITEM[4] = {
  { 0x804539a8, DR_VALUE_TYPE_U32 },
  { 0x804539ac, DR_VALUE_TYPE_U32 },
  { 0x804539b0, DR_VALUE_TYPE_U32 },
  { 0x804539b4, DR_VALUE_TYPE_U32 }
};

typedef enum
{
  SR_ITEM_NONE        = 0,
  SR_ITEM_BEAM_SWORD  = 1,
  SR_ITEM_HOMERUN_BAT = 2,
  SR_ITEM_FAN         = 3,
  SR_ITEM_STAR_ROD    = 4,
  SR_ITEM_RAY_GUN     = 5,
  SR_ITEM_FIRE_FLOWER = 6,
  SR_ITEM_HAMMER      = 7,
  SR_ITEM_MS_BOMB     = 8,
  SR_ITEM_BOBOMB      = 9,
  SR_ITEM_BUMPER      = 10,
  SR_ITEM_GREEN_SHELL = 11,
  SR_ITEM_RED_SHELL   = 12,
  SR_ITEM_POKEBALL    = 13,
  SR_ITEM_SPINY_SHELL = 14,
  SR_ITEM_DEKU_NUT    = 15,
  SR_ITEM_PITFALL     = 16,
  SR_ITEM_GOLDEN_GUN  = 17,
  SR_ITEM_MR_SATURN   = 18,
  SR_ITEM_RANDOM      = 19
} sr_item;

/* Which items are allowed to spawn, a u32 bitfield.
 *
 * Bits 0-3 are the containers (capsule, crate, barrel, egg) and bits 4-6 the
 * recovery items. From bit 7 up the bits follow sr_item, so item N is bit
 * (N + 6):
 *
 *   beam sword 0x00000080, home run bat 0x00000100, fan 0x00000200,
 *   star rod 0x00000400, hammer 0x00002000, motion sensor bomb 0x00004000,
 *   bob-omb 0x00008000
 */
static const dr_value_t SR_ITEM_SWITCH = { 0x800a4d14, DR_VALUE_TYPE_U32 };

/* The same, for the items Smash Remix adds on top of the vanilla list */
static const dr_value_t SR_REMIX_ITEM_SWITCH[2] = {
  { 0x80445424, DR_VALUE_TYPE_U32 },
  { 0x80445428, DR_VALUE_TYPE_U32 }
};

/* How often items spawn, a u8 */
static const dr_value_t SR_ITEM_FREQUENCY = { 0x800a4d24, DR_VALUE_TYPE_U8 };

typedef enum
{
  SR_ITEM_FREQUENCY_NONE = 0,
  SR_ITEM_FREQUENCY_VERY_LOW = 1,
  SR_ITEM_FREQUENCY_LOW = 2,
  SR_ITEM_FREQUENCY_MIDDLE = 3,
  SR_ITEM_FREQUENCY_HIGH = 4,
  SR_ITEM_FREQUENCY_VERY_HIGH = 5
} sr_item_frequency;

/* Builds an item switch bitfield enabling only the given items */
static uint32_t sr_item_mask(const sr_item *items, unsigned count)
{
  uint32_t mask = 0;
  unsigned i;

  for (i = 0; i < count; i++)
    if (items[i] > SR_ITEM_NONE && items[i] < SR_ITEM_RANDOM)
      mask |= 1u << (items[i] + 6);

  return mask;
}

/* Native character ids. Not every one has a dr_character to map onto: the table
 * below only covers those the Mario Party roster can stand in for. */
typedef enum
{
  SR_CHARACTER_MARIO = 0x00,
  SR_CHARACTER_FOX = 0x01,
  SR_CHARACTER_DONKEY_KONG = 0x02,
  SR_CHARACTER_SAMUS = 0x03,
  SR_CHARACTER_LUIGI = 0x04,
  SR_CHARACTER_LINK = 0x05,
  SR_CHARACTER_YOSHI = 0x06,
  SR_CHARACTER_CAPTAIN_FALCON = 0x07,
  SR_CHARACTER_KIRBY = 0x08,
  SR_CHARACTER_PIKACHU = 0x09,
  SR_CHARACTER_JIGGLYPUFF = 0x0a,
  SR_CHARACTER_NESS = 0x0b,
  SR_CHARACTER_MASTER_HAND = 0x0c,
  SR_CHARACTER_METAL_MARIO = 0x0d,
  SR_CHARACTER_POLYGON_MARIO = 0x0e,
  SR_CHARACTER_POLYGON_FOX = 0x0f,
  SR_CHARACTER_POLYGON_DONKEY_KONG = 0x10,
  SR_CHARACTER_POLYGON_SAMUS = 0x11,
  SR_CHARACTER_POLYGON_LUIGI = 0x12,
  SR_CHARACTER_POLYGON_LINK = 0x13,
  SR_CHARACTER_POLYGON_YOSHI = 0x14,
  SR_CHARACTER_POLYGON_CAPTAIN_FALCON = 0x15,
  SR_CHARACTER_POLYGON_KIRBY = 0x16,
  SR_CHARACTER_POLYGON_PIKACHU = 0x17,
  SR_CHARACTER_POLYGON_JIGGLYPUFF = 0x18,
  SR_CHARACTER_POLYGON_NESS = 0x19,
  SR_CHARACTER_GIANT_DONKEY_KONG = 0x1a,
  /* 0x1b-0x1c unused */
  SR_CHARACTER_FALCO = 0x1d,
  SR_CHARACTER_GANONDORF = 0x1e,
  SR_CHARACTER_YOUNG_LINK = 0x1f,
  SR_CHARACTER_DR_MARIO = 0x20,
  SR_CHARACTER_WARIO = 0x21,
  SR_CHARACTER_DARK_SAMUS = 0x22,
  SR_CHARACTER_ALT_LINK = 0x23,
  SR_CHARACTER_ALT_SAMUS = 0x24,
  SR_CHARACTER_ALT_NESS = 0x25,
  SR_CHARACTER_LUCAS = 0x26,
  SR_CHARACTER_ALT_LINK_2 = 0x27,
  SR_CHARACTER_ALT_CAPTAIN_FALCON = 0x28,
  SR_CHARACTER_ALT_FOX = 0x29,
  SR_CHARACTER_ALT_MARIO = 0x2a,
  SR_CHARACTER_ALT_LUIGI = 0x2b,
  SR_CHARACTER_ALT_DONKEY_KONG = 0x2c,
  SR_CHARACTER_ALT_PIKACHU = 0x2d,
  SR_CHARACTER_ALT_JIGGLYPUFF = 0x2e,
  SR_CHARACTER_ALT_JIGGLYPUFF_2 = 0x2f,
  SR_CHARACTER_ALT_KIRBY = 0x30,
  SR_CHARACTER_ALT_YOSHI = 0x31,
  SR_CHARACTER_ALT_PIKACHU_2 = 0x32,
  SR_CHARACTER_ALT_SAMUS_2 = 0x33,
  SR_CHARACTER_BOWSER = 0x34,
  SR_CHARACTER_GIGA_BOWSER = 0x35,
  SR_CHARACTER_MAD_PIANO = 0x36,
  SR_CHARACTER_WOLF = 0x37,
  SR_CHARACTER_CONKER = 0x38,
  SR_CHARACTER_MEWTWO = 0x39,
  SR_CHARACTER_MARTH = 0x3a,
  SR_CHARACTER_SONIC = 0x3b,
  SR_CHARACTER_SANDBAG = 0x3c,
  SR_CHARACTER_SUPER_SONIC = 0x3d,
  SR_CHARACTER_SHEIK = 0x3e,
  SR_CHARACTER_MARINA = 0x3f,
  SR_CHARACTER_DEDEDE = 0x40,
  SR_CHARACTER_GOEMON = 0x41,
  SR_CHARACTER_PEPPY = 0x42,
  SR_CHARACTER_SLIPPY = 0x43,
  SR_CHARACTER_BANJO = 0x44,
  SR_CHARACTER_METAL_LUIGI = 0x45,
  SR_CHARACTER_EBISUMARU = 0x46,
  SR_CHARACTER_DRAGON_KING = 0x47,
  SR_CHARACTER_CRASH = 0x48,
  SR_CHARACTER_PEACH = 0x49,
  SR_CHARACTER_ROY = 0x4a,
  SR_CHARACTER_DR_LUIGI = 0x4b,
  SR_CHARACTER_LANKY = 0x4c
} sr_character;

typedef struct
{
  dr_character character;
  sr_character character_value;
  unsigned color_value;
} sr_character_t;

static const sr_character_t SR_CHARACTER_ID[] = {
  { DR_CHARACTER_MARIO, SR_CHARACTER_MARIO, 0x00 },
  { DR_CHARACTER_LUIGI, SR_CHARACTER_LUIGI, 0x00 },
  { DR_CHARACTER_PEACH, SR_CHARACTER_PEACH, 0x00 },
  { DR_CHARACTER_YOSHI, SR_CHARACTER_YOSHI, 0x00 },
  { DR_CHARACTER_WARIO, SR_CHARACTER_WARIO, 0x00 },
  { DR_CHARACTER_DONKEY_KONG, SR_CHARACTER_DONKEY_KONG, 0x00 },

  { DR_CHARACTER_WALUIGI, SR_CHARACTER_LUIGI, 0x04 }, // Luigi (purple)
  { DR_CHARACTER_DAISY, SR_CHARACTER_PEACH, 0x01 }, // Peach (yellow)

  { DR_CHARACTER_TOAD, SR_CHARACTER_NESS, 0x04 }, // Ness (white)
  { DR_CHARACTER_BOO, SR_CHARACTER_KIRBY, 0x05 }, // Kirby (white)
  { DR_CHARACTER_KOOPA_KID, SR_CHARACTER_BOWSER, 0x00 }, // Bowser
  { DR_CHARACTER_KOOPA_KID_R, SR_CHARACTER_BOWSER, 0x01 }, // Bowser (red)
  { DR_CHARACTER_KOOPA_KID_G, SR_CHARACTER_BOWSER, 0x04 }, // Bowser (green)
  { DR_CHARACTER_KOOPA_KID_B, SR_CHARACTER_BOWSER, 0x02 }, // Bowser (blue)

  { DR_CHARACTER_TOADETTE, SR_CHARACTER_LUCAS, 0x02 }, // Lucas (pink)
  { DR_CHARACTER_BIRDO, SR_CHARACTER_YOSHI, 0x04 }, // Yoshi (pink)
  { DR_CHARACTER_DRY_BONES, SR_CHARACTER_BOWSER, 0x03 }, // Bowser (black)
  { DR_CHARACTER_BLOOPER, SR_CHARACTER_MEWTWO, 0x05 }, // Mewtwo (cyan)
  { DR_CHARACTER_HAMMER_BRO, SR_CHARACTER_DEDEDE, 0x04 }, // Dedede (green)
};

typedef enum
{
  SR_STAGE_PEACHS_CASTLE = 0x00,
  SR_STAGE_SECTOR_Z = 0x01,
  SR_STAGE_CONGO_JUNGLE = 0x02,
  SR_STAGE_PLANET_ZEBES = 0x03,
  SR_STAGE_HYRULE_CASTLE = 0x04,
  SR_STAGE_YOSHIS_ISLAND = 0x05,
  SR_STAGE_DREAM_LAND = 0x06,
  SR_STAGE_SAFFRON_CITY = 0x07,
  SR_STAGE_MUSHROOM_KINGDOM = 0x08,
  SR_STAGE_DREAM_LAND_BETA_1 = 0x09,
  SR_STAGE_DREAM_LAND_BETA_2 = 0x0a,
  SR_STAGE_HOW_TO_PLAY = 0x0b,
  SR_STAGE_MINI_YOSHIS_ISLAND = 0x0c,
  SR_STAGE_META_CRYSTAL = 0x0d,
  SR_STAGE_DUEL_ZONE = 0x0e,
  SR_STAGE_RACE_TO_THE_FINISH = 0x0f,
  SR_STAGE_FINAL_DESTINATION = 0x10,
  SR_STAGE_BTT_MARIO = 0x11,
  SR_STAGE_BTT_FOX = 0x12,
  SR_STAGE_BTT_DONKEY_KONG = 0x13,
  SR_STAGE_BTT_SAMUS = 0x14,
  SR_STAGE_BTT_LUIGI = 0x15,
  SR_STAGE_BTT_LINK = 0x16,
  SR_STAGE_BTT_YOSHI = 0x17,
  SR_STAGE_BTT_FALCON = 0x18,
  SR_STAGE_BTT_KIRBY = 0x19,
  SR_STAGE_BTT_PIKACHU = 0x1a,
  SR_STAGE_BTT_JIGGLYPUFF = 0x1b,
  SR_STAGE_BTT_NESS = 0x1c,
  SR_STAGE_BTP_MARIO = 0x1d,
  SR_STAGE_BTP_FOX = 0x1e,
  SR_STAGE_BTP_DONKEY_KONG = 0x1f,
  SR_STAGE_BTP_SAMUS = 0x20,
  SR_STAGE_BTP_LUIGI = 0x21,
  SR_STAGE_BTP_LINK = 0x22,
  SR_STAGE_BTP_YOSHI = 0x23,
  SR_STAGE_BTP_FALCON = 0x24,
  SR_STAGE_BTP_KIRBY = 0x25,
  SR_STAGE_BTP_PIKACHU = 0x26,
  SR_STAGE_BTP_JIGGLYPUFF = 0x27,
  SR_STAGE_BTP_NESS = 0x28,
  SR_STAGE_DEKU_TREE = 0x29,
  SR_STAGE_FIRST_DESTINATION = 0x2a,
  SR_STAGE_GANONS_TOWER = 0x2b,
  SR_STAGE_GYM_LEADER_CASTLE = 0x2c,
  SR_STAGE_POKEMON_STADIUM = 0x2d,
  SR_STAGE_TALTAL = 0x2e,
  SR_STAGE_GLACIAL = 0x2f,
  SR_STAGE_WARIOWARE = 0x30,
  SR_STAGE_BATTLEFIELD = 0x31,
  SR_STAGE_FLAT_ZONE = 0x32,
  SR_STAGE_DR_MARIO = 0x33,
  SR_STAGE_COOLCOOL = 0x34,
  SR_STAGE_DRAGONKING = 0x35,
  SR_STAGE_GREAT_BAY = 0x36,
  SR_STAGE_FRAYS_STAGE = 0x37,
  SR_STAGE_TOH = 0x38,
  SR_STAGE_FOD = 0x39,
  SR_STAGE_MUDA = 0x3a,
  SR_STAGE_MEMENTOS = 0x3b,
  SR_STAGE_SHOWDOWN = 0x3c,
  SR_STAGE_SPIRALM = 0x3d,
  SR_STAGE_N64 = 0x3e,
  SR_STAGE_MUTE_DL = 0x3f,
  SR_STAGE_MADMM = 0x40,
  SR_STAGE_SMBBF = 0x41,
  SR_STAGE_SMBO = 0x42,
  SR_STAGE_BOWSERB = 0x43,
  SR_STAGE_PEACH2 = 0x44,
  SR_STAGE_DELFINO = 0x45,
  SR_STAGE_CORNERIA2 = 0x46,
  SR_STAGE_KITCHEN = 0x47,
  SR_STAGE_BLUE = 0x48,
  SR_STAGE_ONETT = 0x49,
  SR_STAGE_ZLANDING = 0x4a,
  SR_STAGE_FROSTY = 0x4b,
  SR_STAGE_SMASHVILLE2 = 0x4c,
  SR_STAGE_BTT_DRM = 0x4d,
  SR_STAGE_BTT_GND = 0x4e,
  SR_STAGE_BTT_YL = 0x4f,
  SR_STAGE_BATTLEFIELD_DL = 0x50,
  SR_STAGE_BTT_DS = 0x51,
  SR_STAGE_BTT_STG1 = 0x52,
  SR_STAGE_BTT_FALCO = 0x53,
  SR_STAGE_BTT_WARIO = 0x54,
  SR_STAGE_HTEMPLE = 0x55,
  SR_STAGE_BTT_LUCAS = 0x56,
  SR_STAGE_BTP_GND = 0x57,
  SR_STAGE_NPC = 0x58,
  SR_STAGE_BTP_DS = 0x59,
  SR_STAGE_SMASHKETBALL = 0x5a,
  SR_STAGE_BTP_DRM = 0x5b,
  SR_STAGE_NORFAIR = 0x5c,
  SR_STAGE_RAIDBLUE = 0x5d,
  SR_STAGE_FALLS = 0x5e,
  SR_STAGE_OSOHE = 0x5f,
  SR_STAGE_YOSHI_STORY_2 = 0x60,
  SR_STAGE_WORLD1 = 0x61,
  SR_STAGE_FLAT_ZONE_2 = 0x62,
  SR_STAGE_GERUDO = 0x63,
  SR_STAGE_BTP_YL = 0x64,
  SR_STAGE_BTP_FALCO = 0x65,
  SR_STAGE_BTP_POLY = 0x66,
  SR_STAGE_HCASTLE_DL = 0x67,
  SR_STAGE_HCASTLE_O = 0x68,
  SR_STAGE_CONGOJ_DL = 0x69,
  SR_STAGE_CONGOJ_O = 0x6a,
  SR_STAGE_PCASTLE_DL = 0x6b,
  SR_STAGE_PCASTLE_O = 0x6c,
  SR_STAGE_BTP_WARIO = 0x6d,
  SR_STAGE_FRAYS_STAGE_NIGHT = 0x6e,
  SR_STAGE_GOOMBA_ROAD = 0x6f,
  SR_STAGE_BTP_LUCAS2 = 0x70,
  SR_STAGE_SECTOR_Z_DL = 0x71,
  SR_STAGE_SAFFRON_DL = 0x72,
  SR_STAGE_YOSHI_ISLAND_DL = 0x73,
  SR_STAGE_ZEBES_DL = 0x74,
  SR_STAGE_SECTOR_Z_O = 0x75,
  SR_STAGE_SAFFRON_O = 0x76,
  SR_STAGE_YOSHI_ISLAND_O = 0x77,
  SR_STAGE_DREAM_LAND_O = 0x78,
  SR_STAGE_ZEBES_O = 0x79,
  SR_STAGE_BTT_BOWSER = 0x7a,
  SR_STAGE_BTP_BOWSER = 0x7b,
  SR_STAGE_BOWSERS_KEEP = 0x7c,
  SR_STAGE_RITH_ESSA = 0x7d,
  SR_STAGE_VENOM = 0x7e,
  SR_STAGE_BTT_WOLF = 0x7f,
  SR_STAGE_BTP_WOLF = 0x80,
  SR_STAGE_BTT_CONKER = 0x81,
  SR_STAGE_BTP_CONKER = 0x82,
  SR_STAGE_WINDY = 0x83,
  SR_STAGE_DATA = 0x84,
  SR_STAGE_CLANCER = 0x85,
  SR_STAGE_JAPES = 0x86,
  SR_STAGE_BTT_MARTH = 0x87,
  SR_STAGE_GB_LAND = 0x88,
  SR_STAGE_BTT_MTWO = 0x89,
  SR_STAGE_BTP_MARTH = 0x8a,
  SR_STAGE_REST = 0x8b,
  SR_STAGE_BTP_MTWO = 0x8c,
  SR_STAGE_CSIEGE = 0x8d,
  SR_STAGE_YOSHIS_ISLAND_II = 0x8e,
  SR_STAGE_FINAL_DESTINATION_DL = 0x8f,
  SR_STAGE_FINAL_DESTINATION_TENT = 0x90,
  SR_STAGE_COOLCOOL_REMIX = 0x91,
  SR_STAGE_DUEL_ZONE_DL = 0x92,
  SR_STAGE_COOLCOOL_DL = 0x93,
  SR_STAGE_META_CRYSTAL_DL = 0x94,
  SR_STAGE_DREAM_LAND_SR = 0x95,
  SR_STAGE_PCASTLE_BETA = 0x96,
  SR_STAGE_HCASTLE_REMIX = 0x97,
  SR_STAGE_SECTOR_Z_REMIX = 0x98,
  SR_STAGE_MUTE = 0x99,
  SR_STAGE_HRC = 0x9a,
  SR_STAGE_MK_REMIX = 0x9b,
  SR_STAGE_GHZ = 0x9c,
  SR_STAGE_SUBCON = 0x9d,
  SR_STAGE_PIRATE = 0x9e,
  SR_STAGE_CASINO = 0x9f,
  SR_STAGE_BTT_SONIC = 0xa0,
  SR_STAGE_BTP_SONIC = 0xa1,
  SR_STAGE_MMADNESS = 0xa2,
  SR_STAGE_RAINBOWROAD = 0xa3,
  SR_STAGE_POKEMON_STADIUM_2 = 0xa4,
  SR_STAGE_NORFAIR_REMIX = 0xa5,
  SR_STAGE_TOADSTURNPIKE = 0xa6,
  SR_STAGE_TALTAL_REMIX = 0xa7,
  SR_STAGE_BTP_SHEIK = 0xa8,
  SR_STAGE_WINTER_DL = 0xa9,
  SR_STAGE_BTT_SHEIK = 0xaa,
  SR_STAGE_GLACIAL_REMIX = 0xab,
  SR_STAGE_BTT_MARINA = 0xac,
  SR_STAGE_DRAGONKING_REMIX = 0xad,
  SR_STAGE_BTP_MARINA = 0xae,
  SR_STAGE_BTT_DEDEDE = 0xaf,
  SR_STAGE_DRACULAS_CASTLE = 0xb0,
  SR_STAGE_INVERTED_CASTLE = 0xb1,
  SR_STAGE_BTP_DEDEDE = 0xb2,
  SR_STAGE_MT_DEDEDE = 0xb3,
  SR_STAGE_EDO = 0xb4,
  SR_STAGE_DEKU_TREE_DL = 0xb5,
  SR_STAGE_ZLANDING_DL = 0xb6,
  SR_STAGE_BTT_GOEMON = 0xb7,
  SR_STAGE_FIRST_REMIX = 0xb8,
  SR_STAGE_BTP_GOEMON = 0xb9,
  SR_STAGE_TWILIGHT_CITY = 0xba,
  SR_STAGE_MELRODE = 0xbb,
  SR_STAGE_META_REMIX = 0xbc,
  SR_STAGE_REMIX_RTTF = 0xbd,
  SR_STAGE_REAPERS = 0xbe,
  SR_STAGE_SCUTTLE_TOWN = 0xbf,
  SR_STAGE_BIG_BOOS_HAUNT = 0xc0,
  SR_STAGE_YOSHIS_ISLAND_MELEE = 0xc1,
  SR_STAGE_BTT_BANJO = 0xc2,
  SR_STAGE_SPAWNED_FEAR = 0xc3,
  SR_STAGE_SMASHVILLE_REMIX = 0xc4,
  SR_STAGE_BTP_BANJO = 0xc5,
  SR_STAGE_POKEFLOATS = 0xc6,
  SR_STAGE_BIG_SNOWMAN = 0xc7,
  SR_STAGE_DL_BETA_DL = 0xc8,
  SR_STAGE_LMAO_CASTLE = 0xc9,
  SR_STAGE_DISCOVERY_FALLS = 0xca,
  SR_STAGE_BTT_CRASH = 0xcb,
  SR_STAGE_DISCOVERY_FALLS_REMIX = 0xcc,
  SR_STAGE_N64_REMIX = 0xcd,
  SR_STAGE_BTP_CRASH = 0xce,
  SR_STAGE_BTT_PEACH = 0xcf,
  SR_STAGE_BTP_PEACH = 0xd0,
  SR_STAGE_SOCCER = 0xd1,
  SR_STAGE_TIME_TWISTER = 0xd2,
  SR_STAGE_TIME_TWISTER_SSS = 0xd3,
  SR_STAGE_NSANITY_BEACH = 0xd4,
  SR_STAGE_SNOW_GO = 0xd5,
  SR_STAGE_FUTURE_FRENZY = 0xd6,
  SR_STAGE_HTP_FALL = 0xd7,

  SR_STAGE_BTX_FIRST = 0x11,
  SR_STAGE_BTX_LAST = 0x28,
  SR_STAGE_MAX = 0xd7,
  SR_STAGE_RANDOM = 0xde
} sr_stage;

typedef struct
{
  sr_stage stage;
  const char *name;
  bool selectable;
} sr_stage_t;

static const sr_stage_t SR_STAGES[] = {
  { SR_STAGE_PEACHS_CASTLE, "Peach's Castle", true },
  { SR_STAGE_SECTOR_Z, "Sector Z", true },
  { SR_STAGE_CONGO_JUNGLE, "Congo Jungle", true },
  { SR_STAGE_PLANET_ZEBES, "Planet Zebes", true },
  { SR_STAGE_HYRULE_CASTLE, "Hyrule Castle", true },
  { SR_STAGE_YOSHIS_ISLAND, "Yoshi's Island", true },
  { SR_STAGE_DREAM_LAND, "Dream Land", true },
  { SR_STAGE_SAFFRON_CITY, "Saffron City", true },
  { SR_STAGE_MUSHROOM_KINGDOM, "Mushroom Kingdom", true },
  { SR_STAGE_DREAM_LAND_BETA_1, "Dream Land Beta 1", false },
  { SR_STAGE_DREAM_LAND_BETA_2, "Dream Land Beta 2", false },
  { SR_STAGE_HOW_TO_PLAY, "How to Play", false },
  { SR_STAGE_MINI_YOSHIS_ISLAND, "Mini Yoshi's Island", true },
  { SR_STAGE_META_CRYSTAL, "Meta Crystal", true },
  { SR_STAGE_DUEL_ZONE, "Duel Zone", true },
  { SR_STAGE_RACE_TO_THE_FINISH, "Race to the Finish", false },
  { SR_STAGE_FINAL_DESTINATION, "Final Destination", true },
  { SR_STAGE_BTT_MARIO, "Break the Targets (Mario)", false },
  { SR_STAGE_BTT_FOX, "Break the Targets (Fox)", false },
  { SR_STAGE_BTT_DONKEY_KONG, "Break the Targets (Donkey Kong)", false },
  { SR_STAGE_BTT_SAMUS, "Break the Targets (Samus)", false },
  { SR_STAGE_BTT_LUIGI, "Break the Targets (Luigi)", false },
  { SR_STAGE_BTT_LINK, "Break the Targets (Link)", false },
  { SR_STAGE_BTT_YOSHI, "Break the Targets (Yoshi)", false },
  { SR_STAGE_BTT_FALCON, "Break the Targets (Falcon)", false },
  { SR_STAGE_BTT_KIRBY, "Break the Targets (Kirby)", false },
  { SR_STAGE_BTT_PIKACHU, "Break the Targets (Pikachu)", false },
  { SR_STAGE_BTT_JIGGLYPUFF, "Break the Targets (Jigglypuff)", false },
  { SR_STAGE_BTT_NESS, "Break the Targets (Ness)", false },
  { SR_STAGE_BTP_MARIO, "Board the Platforms (Mario)", false },
  { SR_STAGE_BTP_FOX, "Board the Platforms (Fox)", false },
  { SR_STAGE_BTP_DONKEY_KONG, "Board the Platforms (Donkey Kong)", false },
  { SR_STAGE_BTP_SAMUS, "Board the Platforms (Samus)", false },
  { SR_STAGE_BTP_LUIGI, "Board the Platforms (Luigi)", false },
  { SR_STAGE_BTP_LINK, "Board the Platforms (Link)", false },
  { SR_STAGE_BTP_YOSHI, "Board the Platforms (Yoshi)", false },
  { SR_STAGE_BTP_FALCON, "Board the Platforms (Falcon)", false },
  { SR_STAGE_BTP_KIRBY, "Board the Platforms (Kirby)", false },
  { SR_STAGE_BTP_PIKACHU, "Board the Platforms (Pikachu)", false },
  { SR_STAGE_BTP_JIGGLYPUFF, "Board the Platforms (Jigglypuff)", false },
  { SR_STAGE_BTP_NESS, "Board the Platforms (Ness)", false },
  { SR_STAGE_DEKU_TREE, "Deku Tree", true },
  { SR_STAGE_FIRST_DESTINATION, "First Destination", true },
  { SR_STAGE_GANONS_TOWER, "Ganon's Tower", true },
  { SR_STAGE_GYM_LEADER_CASTLE, "Gym Leader Castle", true },
  { SR_STAGE_POKEMON_STADIUM, "Pokemon Stadium", true },
  { SR_STAGE_TALTAL, "Tal Tal Heights", true },
  { SR_STAGE_GLACIAL, "Glacial River", true },
  { SR_STAGE_WARIOWARE, "WarioWare, Inc.", true },
  { SR_STAGE_BATTLEFIELD, "Battlefield", true },
  { SR_STAGE_FLAT_ZONE, "Flat Zone", true },
  { SR_STAGE_DR_MARIO, "Dr. Mario", true },
  { SR_STAGE_COOLCOOL, "Cool Cool Mountain", true },
  { SR_STAGE_DRAGONKING, "Dragon King", true },
  { SR_STAGE_GREAT_BAY, "Great Bay", true },
  { SR_STAGE_FRAYS_STAGE, "Fray's Stage", true },
  { SR_STAGE_TOH, "Tower of Heaven", true },
  { SR_STAGE_FOD, "Fountain of Dreams", true },
  { SR_STAGE_MUDA, "Muda Kingdom", true },
  { SR_STAGE_MEMENTOS, "Mementos", true },
  { SR_STAGE_SHOWDOWN, "Showdown", true },
  { SR_STAGE_SPIRALM, "Spiral Mountain", true },
  { SR_STAGE_N64, "N64", true },
  { SR_STAGE_MUTE_DL, "Mute City DL", true },
  { SR_STAGE_MADMM, "Mad Monster Mansion", true },
  { SR_STAGE_SMBBF, "Mushroom Kingdom Battlefield", true },
  { SR_STAGE_SMBO, "Mushroom Kingdom Omega", true },
  { SR_STAGE_BOWSERB, "Bowser's Battleship", true },
  { SR_STAGE_PEACH2, "Peach's Castle II", true },
  { SR_STAGE_DELFINO, "Delfino Plaza", true },
  { SR_STAGE_CORNERIA2, "Corneria City", true },
  { SR_STAGE_KITCHEN, "Kitchen Island", true },
  { SR_STAGE_BLUE, "Big Blue", true },
  { SR_STAGE_ONETT, "Onett", true },
  { SR_STAGE_ZLANDING, "Zebes Landing", true },
  { SR_STAGE_FROSTY, "Frosty Village", true },
  { SR_STAGE_SMASHVILLE2, "Smashville II", true },
  { SR_STAGE_BTT_DRM, "Break the Targets (Dr. Mario)", false },
  { SR_STAGE_BTT_GND, "Break the Targets (Ganondorf)", false },
  { SR_STAGE_BTT_YL, "Break the Targets (Young Link)", false },
  { SR_STAGE_BATTLEFIELD_DL, "Battlefield DL", true },
  { SR_STAGE_BTT_DS, "Break the Targets (Dark Samus)", false },
  { SR_STAGE_BTT_STG1, "Break the Targets (Stage 1)", false },
  { SR_STAGE_BTT_FALCO, "Break the Targets (Falco)", false },
  { SR_STAGE_BTT_WARIO, "Break the Targets (Wario)", false },
  { SR_STAGE_HTEMPLE, "Hyrule Temple", true },
  { SR_STAGE_BTT_LUCAS, "Break the Targets (Lucas)", false },
  { SR_STAGE_BTP_GND, "Board the Platforms (Ganondorf)", false },
  { SR_STAGE_NPC, "New Pork City", true },
  { SR_STAGE_BTP_DS, "Board the Platforms (Dark Samus)", false },
  { SR_STAGE_SMASHKETBALL, "Smashketball", false },
  { SR_STAGE_BTP_DRM, "Board the Platforms (Dr. Mario)", false },
  { SR_STAGE_NORFAIR, "Norfair", true },
  { SR_STAGE_RAIDBLUE, "Raid on Blue", true },
  { SR_STAGE_FALLS, "Falls", true },
  { SR_STAGE_OSOHE, "Osohe Castle", true },
  { SR_STAGE_YOSHI_STORY_2, "Yoshi's Story II", true },
  { SR_STAGE_WORLD1, "World 1-1", true },
  { SR_STAGE_FLAT_ZONE_2, "Flat Zone 2", true },
  { SR_STAGE_GERUDO, "Gerudo Valley", true },
  { SR_STAGE_BTP_YL, "Board the Platforms (Young Link)", false },
  { SR_STAGE_BTP_FALCO, "Board the Platforms (Falco)", false },
  { SR_STAGE_BTP_POLY, "Board the Platforms (Polygon)", false },
  { SR_STAGE_HCASTLE_DL, "Hyrule Castle DL", true },
  { SR_STAGE_HCASTLE_O, "Hyrule Castle Omega", true },
  { SR_STAGE_CONGOJ_DL, "Congo Jungle DL", true },
  { SR_STAGE_CONGOJ_O, "Congo Jungle Omega", true },
  { SR_STAGE_PCASTLE_DL, "Peach's Castle DL", true },
  { SR_STAGE_PCASTLE_O, "Peach's Castle Omega", true },
  { SR_STAGE_BTP_WARIO, "Board the Platforms (Wario)", false },
  { SR_STAGE_FRAYS_STAGE_NIGHT, "Fray's Stage (Night)", true },
  { SR_STAGE_GOOMBA_ROAD, "Goomba Road", true },
  { SR_STAGE_BTP_LUCAS2, "Board the Platforms (Lucas)", false },
  { SR_STAGE_SECTOR_Z_DL, "Sector Z DL", true },
  { SR_STAGE_SAFFRON_DL, "Saffron City DL", true },
  { SR_STAGE_YOSHI_ISLAND_DL, "Yoshi's Island DL", true },
  { SR_STAGE_ZEBES_DL, "Planet Zebes DL", true },
  { SR_STAGE_SECTOR_Z_O, "Sector Z Omega", true },
  { SR_STAGE_SAFFRON_O, "Saffron City Omega", true },
  { SR_STAGE_YOSHI_ISLAND_O, "Yoshi's Island Omega", true },
  { SR_STAGE_DREAM_LAND_O, "Dream Land Omega", true },
  { SR_STAGE_ZEBES_O, "Planet Zebes Omega", true },
  { SR_STAGE_BTT_BOWSER, "Break the Targets (Bowser)", false },
  { SR_STAGE_BTP_BOWSER, "Board the Platforms (Bowser)", false },
  { SR_STAGE_BOWSERS_KEEP, "Bowser's Keep", true },
  { SR_STAGE_RITH_ESSA, "Rith Essa", true },
  { SR_STAGE_VENOM, "Venom", true },
  { SR_STAGE_BTT_WOLF, "Break the Targets (Wolf)", false },
  { SR_STAGE_BTP_WOLF, "Board the Platforms (Wolf)", false },
  { SR_STAGE_BTT_CONKER, "Break the Targets (Conker)", false },
  { SR_STAGE_BTP_CONKER, "Board the Platforms (Conker)", false },
  { SR_STAGE_WINDY, "Windy", true },
  { SR_STAGE_DATA, "Data Select", true },
  { SR_STAGE_CLANCER, "Clancer", true },
  { SR_STAGE_JAPES, "Jungle Japes", true },
  { SR_STAGE_BTT_MARTH, "Break the Targets (Marth)", false },
  { SR_STAGE_GB_LAND, "Game Boy Land", true },
  { SR_STAGE_BTT_MTWO, "Break the Targets (Mewtwo)", false },
  { SR_STAGE_BTP_MARTH, "Board the Platforms (Marth)", false },
  { SR_STAGE_REST, "Rest Area", true },
  { SR_STAGE_BTP_MTWO, "Board the Platforms (Mewtwo)", false },
  { SR_STAGE_CSIEGE, "Castle Siege", true },
  { SR_STAGE_YOSHIS_ISLAND_II, "Yoshi's Island II", true },
  { SR_STAGE_FINAL_DESTINATION_DL, "Final Destination DL", true },
  { SR_STAGE_FINAL_DESTINATION_TENT, "Final Destination (Tent)", true },
  { SR_STAGE_COOLCOOL_REMIX, "Cool Cool Mountain Remix", true },
  { SR_STAGE_DUEL_ZONE_DL, "Duel Zone DL", true },
  { SR_STAGE_COOLCOOL_DL, "Cool Cool Mountain DL", true },
  { SR_STAGE_META_CRYSTAL_DL, "Meta Crystal DL", true },
  { SR_STAGE_DREAM_LAND_SR, "Dream Land SR", true },
  { SR_STAGE_PCASTLE_BETA, "Peach's Castle Beta", false },
  { SR_STAGE_HCASTLE_REMIX, "Hyrule Castle Remix", true },
  { SR_STAGE_SECTOR_Z_REMIX, "Sector Z Remix", true },
  { SR_STAGE_MUTE, "Mute City", true },
  { SR_STAGE_HRC, "Home Run Contest", false },
  { SR_STAGE_MK_REMIX, "Mushroom Kingdom Remix", true },
  { SR_STAGE_GHZ, "Green Hill Zone", true },
  { SR_STAGE_SUBCON, "Subcon", true },
  { SR_STAGE_PIRATE, "Pirate Land", true },
  { SR_STAGE_CASINO, "Casino Night Zone", true },
  { SR_STAGE_BTT_SONIC, "Break the Targets (Sonic)", false },
  { SR_STAGE_BTP_SONIC, "Board the Platforms (Sonic)", false },
  { SR_STAGE_MMADNESS, "Metallic Madness", true },
  { SR_STAGE_RAINBOWROAD, "Rainbow Road", true },
  { SR_STAGE_POKEMON_STADIUM_2, "Pokemon Stadium 2", true },
  { SR_STAGE_NORFAIR_REMIX, "Norfair Remix", true },
  { SR_STAGE_TOADSTURNPIKE, "Toad's Turnpike", true },
  { SR_STAGE_TALTAL_REMIX, "Tal Tal Heights Remix", true },
  { SR_STAGE_BTP_SHEIK, "Board the Platforms (Sheik)", false },
  { SR_STAGE_WINTER_DL, "Winter DL", true },
  { SR_STAGE_BTT_SHEIK, "Break the Targets (Sheik)", false },
  { SR_STAGE_GLACIAL_REMIX, "Glacial River Remix", true },
  { SR_STAGE_BTT_MARINA, "Break the Targets (Marina)", false },
  { SR_STAGE_DRAGONKING_REMIX, "Dragon King Remix", true },
  { SR_STAGE_BTP_MARINA, "Board the Platforms (Marina)", false },
  { SR_STAGE_BTT_DEDEDE, "Break the Targets (Dedede)", false },
  { SR_STAGE_DRACULAS_CASTLE, "Dracula's Castle", true },
  { SR_STAGE_INVERTED_CASTLE, "Inverted Castle", true },
  { SR_STAGE_BTP_DEDEDE, "Board the Platforms (Dedede)", false },
  { SR_STAGE_MT_DEDEDE, "Mt. Dedede", true },
  { SR_STAGE_EDO, "Edo Town", true },
  { SR_STAGE_DEKU_TREE_DL, "Deku Tree DL", true },
  { SR_STAGE_ZLANDING_DL, "Zebes Landing DL", true },
  { SR_STAGE_BTT_GOEMON, "Break the Targets (Goemon)", false },
  { SR_STAGE_FIRST_REMIX, "First Destination Remix", true },
  { SR_STAGE_BTP_GOEMON, "Board the Platforms (Goemon)", false },
  { SR_STAGE_TWILIGHT_CITY, "Twilight City", true },
  { SR_STAGE_MELRODE, "Melrode", true },
  { SR_STAGE_META_REMIX, "Meta Crystal Remix", true },
  { SR_STAGE_REMIX_RTTF, "Remix Race to the Finish", false },
  { SR_STAGE_REAPERS, "Reapers", true },
  { SR_STAGE_SCUTTLE_TOWN, "Scuttle Town", true },
  { SR_STAGE_BIG_BOOS_HAUNT, "Big Boo's Haunt", true },
  { SR_STAGE_YOSHIS_ISLAND_MELEE, "Yoshi's Island (Melee)", true },
  { SR_STAGE_BTT_BANJO, "Break the Targets (Banjo)", false },
  { SR_STAGE_SPAWNED_FEAR, "Spawned Fear", true },
  { SR_STAGE_SMASHVILLE_REMIX, "Smashville Remix", true },
  { SR_STAGE_BTP_BANJO, "Board the Platforms (Banjo)", false },
  { SR_STAGE_POKEFLOATS, "Poke Floats", true },
  { SR_STAGE_BIG_SNOWMAN, "Big Snowman", true },
  { SR_STAGE_DL_BETA_DL, "Dream Land Beta DL", false },
  { SR_STAGE_LMAO_CASTLE, "LMAO Castle", true },
  { SR_STAGE_DISCOVERY_FALLS, "Discovery Falls", true },
  { SR_STAGE_BTT_CRASH, "Break the Targets (Crash)", false },
  { SR_STAGE_DISCOVERY_FALLS_REMIX, "Discovery Falls Remix", true },
  { SR_STAGE_N64_REMIX, "N64 Remix", true },
  { SR_STAGE_BTP_CRASH, "Board the Platforms (Crash)", false },
  { SR_STAGE_BTT_PEACH, "Break the Targets (Peach)", false },
  { SR_STAGE_BTP_PEACH, "Board the Platforms (Peach)", false },
  { SR_STAGE_SOCCER, "Soccer", false },
  { SR_STAGE_TIME_TWISTER, "Time Twister", true },
  { SR_STAGE_TIME_TWISTER_SSS, "Time Twister SSS", false },
  { SR_STAGE_NSANITY_BEACH, "N. Sanity Beach", true },
  { SR_STAGE_SNOW_GO, "Snow Go", true },
  { SR_STAGE_FUTURE_FRENZY, "Future Frenzy", true },
  { SR_STAGE_HTP_FALL, "How to Play (Fall)", false },
};

static const sr_stage_t *srRandomStage(void)
{
  unsigned count = 0;
  unsigned pick;
  unsigned i;

  for (i = 0; i < sizeof(SR_STAGES) / sizeof(*SR_STAGES); i++)
    if (SR_STAGES[i].selectable)
      count++;

  if (!count)
    return &SR_STAGES[0];

  pick = dr_rand() % count;
  for (i = 0; i < sizeof(SR_STAGES) / sizeof(*SR_STAGES); i++)
    if (SR_STAGES[i].selectable && !pick--)
      return &SR_STAGES[i];

  return &SR_STAGES[0];
}

void SmashRemix::run(void)
{
  /* After the ~3s hold, start the minigame — which fades the overlay and unmutes. */
  if (m_startDelay > 0 && --m_startDelay == 0)
    startMinigame();

  if (!m_minigame || !m_minigameActive)
    return;

  int64_t stocks[4];
  for (unsigned i = 0; i < 4; i++)
  {
    if (m_retro->readValue(&stocks[i], SR_STOCKS[i]) != DR_OK)
      return;
  }

  if (!m_winners && !m_finishCountdown)
  {
    if (m_minigame->type == DR_MINIGAME_DUEL)
    {
      for (unsigned slot = 0; slot < 4; slot++)
      {
        int idx = m_slotToIndex[slot];
        if (idx < 0 || stocks[slot] >= 0)
          continue;
        for (unsigned wslot = 0; wslot < 4; wslot++)
        {
          int widx = m_slotToIndex[wslot];
          if (widx >= 0 && wslot != slot && stocks[wslot] >= 0)
          {
            m_winners |= (1u << widx);
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[wslot]))));
          }
        }
        m_finishCountdown = 120;
        break;
      }
    }
    else if (m_minigame->type == DR_MINIGAME_BATTLE)
    {
      for (unsigned slot = 0; slot < 4; slot++)
      {
        int idx = m_slotToIndex[slot];
        if (idx < 0)
          continue;
        if (m_prevStocks[slot] == -2)
        {
          m_prevStocks[slot] = stocks[slot];
          continue;
        }
        if (m_prevStocks[slot] >= 0 && stocks[slot] < 0 && m_placement[idx] == -1)
        {
          m_placement[idx] = 3 - (int)m_eliminationCount++;
          log(DR_LOG_INFO,
            qPrintable(QString("%1 eliminated (%2th)").arg(
              dr_character_name(m_slotCharacters[slot])).arg(m_placement[idx] + 1)));
        }
        m_prevStocks[slot] = stocks[slot];
      }
      if (m_eliminationCount >= 3)
      {
        for (unsigned slot = 0; slot < 4; slot++)
        {
          int idx = m_slotToIndex[slot];
          if (idx >= 0 && m_placement[idx] == -1)
          {
            m_placement[idx] = 0;
            log(DR_LOG_INFO,
              qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
          }
        }
        m_finishCountdown = 120;
      }
    }
    else
    {
      unsigned total_stocks = 0;
      for (unsigned i = 0; i < 4; i++)
        total_stocks += (unsigned)(stocks[i] + 1);

      if (total_stocks == 0)
      {
        log(DR_LOG_INFO, "It's a draw!");
        m_finishCountdown = 120;
      }
      else if (m_minigame->type == DR_MINIGAME_4P)
      {
        if (total_stocks == 1)
        {
          for (unsigned slot = 0; slot < 4; slot++)
          {
            if (stocks[slot] >= 0)
            {
              m_winners |= (1u << m_slotToIndex[slot]);
              log(DR_LOG_INFO,
                qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
              break;
            }
          }
          m_finishCountdown = 120;
        }
      }
      else
      {
        /* Team game (2v2 or 1v3): a side wins when the opposing side has no stocks */
        unsigned teamStocks[2] = {};
        for (unsigned slot = 0; slot < 4; slot++)
        {
          int idx = m_slotToIndex[slot];
          if (idx >= 0 && m_players[idx].team_id < 2)
            teamStocks[m_players[idx].team_id] += (unsigned)(stocks[slot] + 1);
        }

        int winningTeam = -1;
        if (teamStocks[0] == 0 && teamStocks[1] > 0)
          winningTeam = 1;
        else if (teamStocks[1] == 0 && teamStocks[0] > 0)
          winningTeam = 0;

        if (winningTeam >= 0)
        {
          for (unsigned slot = 0; slot < 4; slot++)
          {
            int idx = m_slotToIndex[slot];
            if (idx >= 0 && (int)m_players[idx].team_id == winningTeam)
            {
              m_winners |= (1u << idx);
              log(DR_LOG_INFO,
                qPrintable(QString("%1 wins!").arg(dr_character_name(m_slotCharacters[slot]))));
            }
          }
          m_finishCountdown = 120;
        }
      }
    }
  }

  if (m_finishCountdown > 0 && --m_finishCountdown == 0)
    finishMinigame();
}

SmashRemix::SmashRemix(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  m_retro->init(coreId(), rom()); /* also applies N64 remaps */

  /* Additionally, per-port smash remaps on top of the base N64 layout: */
  for (unsigned i = 0; i < 4; i++)
  {
    /* ...map all D-pad directions to L (taunt) */
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_SELECT);
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_SELECT);

    /* ...map R2 to Z (shield) */
    m_retro->core()->input()->remapButton(i, RETRO_DEVICE_ID_JOYPAD_R2, RETRO_DEVICE_ID_JOYPAD_L2);
  }
}


const dr_mp_minigame_t *SmashRemix::minigames() const
{
  return SR_MINIGAMES;
}

void SmashRemix::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;

  m_winners = 0;
  m_finishCountdown = 0;
  m_eliminationCount = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    m_slotToIndex[i] = -1;
    m_prevStocks[i] = -2;
    m_placement[i] = -1;
  }
  loadState(state());

  /* Use a random selectable stage */
  unsigned long rc = dr_rand_count();
  const sr_stage_t *stage = srRandomStage();
  log(DR_LOG_INFO, qPrintable(QString("SR stage=%1 (0x%2) randcount=%3")
                                .arg(stage->name)
                                .arg(stage->stage, 2, 16, QChar('0'))
                                .arg(rc)));
  m_retro->writeValue(stage->stage, SR_STAGE);

  /* Enable team battle for the 2v2 and 1v3 minigames */
  bool teamBattle = (minigame->type == DR_MINIGAME_2V2 || minigame->type == DR_MINIGAME_1V3);
  m_retro->writeValue(teamBattle ? 1 : 0, SR_GAME_TYPE);

  /* Remix PKMN spawns Poke Balls and nothing else; every other minigame is
   * itemless, handing out its start/taunt items directly (see applyPlayers). */
  if (minigame->minigame_id == 0x05)
  {
    static const sr_item pkmn_items[] = { SR_ITEM_POKEBALL };
    m_retro->writeValue(sr_item_mask(pkmn_items, 1), SR_ITEM_SWITCH);
    m_retro->writeValue(SR_ITEM_FREQUENCY_VERY_HIGH, SR_ITEM_FREQUENCY);
  }
  else
  {
    m_retro->writeValue(0, SR_ITEM_SWITCH);
    m_retro->writeValue(SR_ITEM_FREQUENCY_NONE, SR_ITEM_FREQUENCY);
  }

  /* No Remix item ever spawns */
  m_retro->writeValue(0, SR_REMIX_ITEM_SWITCH[0]);
  m_retro->writeValue(0, SR_REMIX_ITEM_SWITCH[1]);

  log(DR_LOG_INFO, qPrintable(QString("Smash Remix starting!")));

  applyPlayers();

  /* Hold the loading overlay up ~48 frames before starting (see run()), giving the
   * match time to spin up behind it. The core stays muted until then. */
  m_startDelay = 48;
}

dr_minigame_result_t SmashRemix::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index >= 4)
    return result;

  if (m_minigame && m_minigame->type == DR_MINIGAME_BATTLE)
  {
    int place = m_placement[index];
    if (place >= 0 && place <= 3)
      result.coins = (unsigned)place;
  }
  else if (m_winners & (1u << index))
  {
    result.coins = 10;
  }

  return result;
}

void SmashRemix::applyPlayers()
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];

    if (p.control_port == DR_CONTROL_PORT_INVALID || p.control_port >= DR_CONTROL_PORT_SIZE)
      continue;

    /* Bystanders (unfilled slots, or duel nonparticipants) don't spawn. */
    if (!dr_team_type_participates(p.team_type))
      continue;

    unsigned slot = p.control_port - DR_CONTROL_PORT_P1;
    m_slotToIndex[slot] = i;
    m_slotCharacters[slot] = p.character;

    bool isBot = (p.control_type == DR_CONTROL_TYPE_CPU);
    m_retro->writeValue(isBot ? 1 : 0, SR_PLAYER_TYPE[slot]);

    for (const auto &entry : SR_CHARACTER_ID)
    {
      if (entry.character == p.character)
      {
        m_retro->writeValue(entry.character_value, SR_CHARACTER[slot]);
        m_retro->writeValue(entry.color_value, SR_COLOR[slot]);
        m_retro->writeValue(isBot ? 4 : slot, SR_PORT[slot]);
        break;
      }
    }

    uint8_t difficulty;
    switch (p.difficulty)
    {
    case DR_DIFFICULTY_VERY_EASY:
      difficulty = 1;
      break;
    case DR_DIFFICULTY_EASY:
      difficulty = 3;
      break;
    case DR_DIFFICULTY_NORMAL:
      difficulty = 5;
      break;
    case DR_DIFFICULTY_HARD:
      difficulty = 7;
      break;
    case DR_DIFFICULTY_VERY_HARD:
      difficulty = 9;
      break;
    default:
      difficulty = 5;
      break;
    }
    m_retro->writeValue(static_cast<uint8_t>(difficulty), SR_DIFFICULTY[slot]);

    uint8_t color, team;
    if (m_minigame->type == DR_MINIGAME_1V3 || m_minigame->type == DR_MINIGAME_2V2)
    {
      switch (p.team_id)
      {
      case 0:
        color = 0x00;
        team = 0x00;
        break;
      case 1:
        color = 0x01;
        team = 0x01;
        break;
      default:
        color = 0x04;
        team = 0x00;
        break;
      }
    }
    else
    {
      /* Free-for-all mini-game; set color based on slot, or gray for CPU */
      if (p.control_type == DR_CONTROL_TYPE_CPU)
        color = 0x04;
      else
        color = slot;
      team = slot;
    }
    m_retro->writeValue(color, SR_PORT_COLOR[slot]);
    m_retro->writeValue(team, SR_TEAM_1[slot]);
    m_retro->writeValue(team, SR_TEAM_2[slot]);

    uint8_t size = 0; // normal
    if (m_minigame->minigame_id == 0x02) // Giant Battle: solo is giant
      size = (p.team_type == DR_TEAM_TYPE_1V3_SOLO) ? 1 : 0;
    else if (m_minigame->minigame_id == 0x03) // Tiny Battle: group is tiny
      size = (p.team_type == DR_TEAM_TYPE_1V3_SOLO) ? 0 : 2;
    m_retro->writeValue(size, SR_SIZE_1[slot]);
    m_retro->writeValue(size, SR_SIZE_2[slot]);

    /* Items: none by default. */
    sr_item startItem = SR_ITEM_NONE;
    sr_item tauntItem = SR_ITEM_NONE;

    if (m_minigame->minigame_id == 0x04 && p.team_type == DR_TEAM_TYPE_1V3_SOLO)
    {
      startItem = SR_ITEM_GOLDEN_GUN;
      tauntItem = SR_ITEM_GOLDEN_GUN;
    }
    else if (m_minigame->minigame_id == 0x05)
      startItem = SR_ITEM_POKEBALL;
    else if (m_minigame->minigame_id == 0x06 && p.team_type == DR_TEAM_TYPE_1V3_SOLO)
      startItem = SR_ITEM_HAMMER;
      
    m_retro->writeValue(startItem, SR_START_ITEM[slot]);
    m_retro->writeValue(tauntItem, SR_TAUNT_ITEM[slot]);
  }

  unsigned activeSlots = 0;
  for (unsigned slot = 0; slot < 4; slot++)
    if (m_slotToIndex[slot] >= 0)
      activeSlots++;

  unsigned expectedPlayers = (m_minigame->type == DR_MINIGAME_DUEL) ? 2 : 4;
  if (activeSlots >= expectedPlayers)
  {
    for (unsigned slot = 0; slot < 4; slot++)
      if (m_slotToIndex[slot] == -1)
      {
        /* Mark the slot totally inactive (2), not just out of stocks — otherwise
         * a slot the savestate had populated (e.g. slot 1 when the duelists land
         * on slots 0 and 2) still spawns a stale player. */
        m_retro->writeValue(2, SR_PLAYER_TYPE[slot]);
        m_retro->writeValue(-1, SR_STOCKS[slot]);
      }
  }
}
