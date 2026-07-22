#ifndef DR_COMMON_H
#define DR_COMMON_H

#include <QString>

static const char *const DERAILLEUR_DATE_STRING = "July 17, 2026";
static const char *const DERAILLEUR_RELEASE_STRING = "r7";

typedef enum
{
  DR_CHARACTER_INVALID = 0,

  DR_CHARACTER_MARIO,
  DR_CHARACTER_LUIGI,
  DR_CHARACTER_PEACH,
  DR_CHARACTER_YOSHI,
  DR_CHARACTER_WARIO,
  DR_CHARACTER_DONKEY_KONG,

  DR_CHARACTER_WALUIGI,
  DR_CHARACTER_DAISY,

  DR_CHARACTER_TOAD,
  DR_CHARACTER_BOO,
  DR_CHARACTER_KOOPA_KID,

  DR_CHARACTER_TOADETTE,

  DR_CHARACTER_BIRDO,
  DR_CHARACTER_DRY_BONES,

  DR_CHARACTER_BLOOPER,
  DR_CHARACTER_HAMMER_BRO,

  DR_CHARACTER_SIZE
} dr_character;

typedef enum
{
  DR_CONTROL_PORT_INVALID = 0,

  DR_CONTROL_PORT_P1,
  DR_CONTROL_PORT_P2,
  DR_CONTROL_PORT_P3,
  DR_CONTROL_PORT_P4,

  DR_CONTROL_PORT_SIZE
} dr_control_port;

typedef enum
{
  DR_CONTROL_TYPE_INVALID = 0,

  DR_CONTROL_TYPE_HUMAN,
  DR_CONTROL_TYPE_CPU,

  DR_CONTROL_TYPE_SIZE
} dr_control_type;

typedef enum
{
  DR_CORE_INVALID = 0,

  /// Mupen64Plus-Next, Nintendo 64 emulator
  DR_CORE_MUPEN64PLUSNEXT,

  /// Dolphin, GameCube/Wii emulator
  DR_CORE_DOLPHIN,

  /// mGBA, Game Boy Advance emulator
  DR_CORE_MGBA,

  /// Flycast, Sega Dreamcast emulator
  DR_CORE_FLYCAST,

  /// FCEUmm, Nintendo Entertainment System emulator
  DR_CORE_FCEUMM,

  /// Snes9x, Super Nintendo Entertainment System emulator
  DR_CORE_SNES9X,

  DR_CORE_SIZE
} dr_core;

typedef enum
{
  DR_DIFFICULTY_INVALID = 0,

  DR_DIFFICULTY_VERY_EASY,
  DR_DIFFICULTY_EASY,
  DR_DIFFICULTY_NORMAL,
  DR_DIFFICULTY_HARD,
  DR_DIFFICULTY_VERY_HARD,

  DR_DIFFICULTY_SIZE
} dr_difficulty;

/* Wii Remote control layout a mini-game expects (MP8/MP9), packed into the
 * dolphin quirks below. Up to 8 profiles fit in the 3-bit fields. */
typedef enum
{
  DR_WII_CONTROL_INVALID = 0,      /* no remap (default / upright) */
  DR_WII_CONTROL_UPRIGHT,          /* held vertically; stick drives d-pad */
  DR_WII_CONTROL_SIDEWAYS_BUTTONS, /* held horizontally (NES-style) */
  DR_WII_CONTROL_SIDEWAYS_MOTION,  /* held horizontally with motion */
  DR_WII_CONTROL_POINTER,          /* IR pointer aiming */
  DR_WII_CONTROL_WAGGLE,           /* shake: A acts as the shake (R2) input */

  DR_WII_CONTROL_SIZE
} dr_wii_control;

typedef union
{
  /* The bits as a raw integer */
  unsigned raw;

  struct
  {
    unsigned needs_efb_to_texture : 1;
    unsigned needs_safe_texture_cache : 1;
    unsigned needs_native_resolution : 1;
    /* Wii Remote control layout(s); values are dr_wii_control. `control` is the
     * primary (and solo) layout; `control_team` is the 1v3 trio layout, or 0 to
     * inherit `control`. 3 bits each -- enough for DR_WII_CONTROL_SIZE profiles. */
    unsigned control : 3;
    unsigned control_team : 3;
  } dolphin;

  struct
  {
    unsigned needs_native_boundaries : 1;
  } mupen64plus;
} dr_emulation_quirk_t;

/* Bare raw-bit values, for composing several quirks into one initializer, e.g.
 *   { DR_QUIRK_BITS_EFB_TO_TEXTURE | DR_WII_CONTROL_BITS(DR_WII_CONTROL_POINTER) }
 */
#define DR_QUIRK_BITS_EFB_TO_TEXTURE 0x1u     // dolphin.needs_efb_to_texture
#define DR_QUIRK_BITS_SAFE_TEXTURE_CACHE 0x2u // dolphin.needs_safe_texture_cache
#define DR_QUIRK_BITS_NATIVE_RESOLUTION 0x4u  // dolphin.needs_native_resolution
#define DR_QUIRK_BITS_NATIVE_BOUNDARIES 0x1u  // mupen64plus.needs_native_boundaries
/* Wii control layout: `control` occupies bits 3-5, `control_team` bits 6-8. */
#define DR_WII_CONTROL_BITS(primary) ((unsigned)(primary) << 3)
#define DR_WII_CONTROL_SPLIT_BITS(primary, team) \
  (((unsigned)(primary) << 3) | ((unsigned)(team) << 6))

/* Ready-made single-quirk initializers. Combine two by OR-ing the *_BITS forms
 * inside your own braces (see the example above). */
#define DR_NO_QUIRKS { 0u }
#define DR_QUIRK_EFB_TO_TEXTURE { DR_QUIRK_BITS_EFB_TO_TEXTURE }
#define DR_QUIRK_SAFE_TEXTURE_CACHE { DR_QUIRK_BITS_SAFE_TEXTURE_CACHE }
#define DR_QUIRK_NATIVE_RESOLUTION { DR_QUIRK_BITS_NATIVE_RESOLUTION }
#define DR_QUIRK_NATIVE_BOUNDARIES { DR_QUIRK_BITS_NATIVE_BOUNDARIES }
#define DR_WII_CONTROL(primary) { DR_WII_CONTROL_BITS(primary) }
#define DR_WII_CONTROL_SPLIT(primary, team) { DR_WII_CONTROL_SPLIT_BITS(primary, team) }

/* NES PPU palette colors. High nibble = luma (dark/medium/light/pale), low nibble
 * = chroma (hue); the $x0 column is gray and $xD-$xF are black. */
typedef enum
{
  /* Grayscale */
  NES_COLOR_BLACK             = 0x0F,
  NES_COLOR_DARK_GRAY         = 0x00,
  NES_COLOR_LIGHT_GRAY        = 0x10, /* silver */
  NES_COLOR_WHITE             = 0x20,
  NES_COLOR_PALE_GRAY         = 0x30,

  /* Dark ($0x) -- medium mixed with black */
  NES_COLOR_DARK_AZURE        = 0x01,
  NES_COLOR_DARK_BLUE         = 0x02,
  NES_COLOR_DARK_VIOLET       = 0x03,
  NES_COLOR_DARK_MAGENTA      = 0x04,
  NES_COLOR_DARK_ROSE         = 0x05,
  NES_COLOR_DARK_RED          = 0x06, /* maroon */
  NES_COLOR_DARK_ORANGE       = 0x07,
  NES_COLOR_DARK_YELLOW       = 0x08, /* olive */
  NES_COLOR_DARK_CHARTREUSE   = 0x09,
  NES_COLOR_DARK_GREEN        = 0x0A,
  NES_COLOR_DARK_SPRING       = 0x0B,
  NES_COLOR_DARK_CYAN         = 0x0C,

  /* Medium ($1x) */
  NES_COLOR_MEDIUM_AZURE      = 0x11,
  NES_COLOR_MEDIUM_BLUE       = 0x12,
  NES_COLOR_MEDIUM_VIOLET     = 0x13,
  NES_COLOR_MEDIUM_MAGENTA    = 0x14,
  NES_COLOR_MEDIUM_ROSE       = 0x15,
  NES_COLOR_MEDIUM_RED        = 0x16,
  NES_COLOR_MEDIUM_ORANGE     = 0x17,
  NES_COLOR_MEDIUM_YELLOW     = 0x18,
  NES_COLOR_MEDIUM_CHARTREUSE = 0x19,
  NES_COLOR_MEDIUM_GREEN      = 0x1A,
  NES_COLOR_MEDIUM_SPRING     = 0x1B,
  NES_COLOR_MEDIUM_CYAN       = 0x1C,

  /* Light ($2x) */
  NES_COLOR_LIGHT_AZURE       = 0x21,
  NES_COLOR_LIGHT_BLUE        = 0x22,
  NES_COLOR_LIGHT_VIOLET      = 0x23,
  NES_COLOR_LIGHT_MAGENTA     = 0x24,
  NES_COLOR_LIGHT_ROSE        = 0x25,
  NES_COLOR_LIGHT_RED         = 0x26,
  NES_COLOR_LIGHT_ORANGE      = 0x27,
  NES_COLOR_LIGHT_YELLOW      = 0x28,
  NES_COLOR_LIGHT_CHARTREUSE  = 0x29,
  NES_COLOR_LIGHT_GREEN       = 0x2A,
  NES_COLOR_LIGHT_SPRING      = 0x2B,
  NES_COLOR_LIGHT_CYAN        = 0x2C,

  /* Pale ($3x) -- light mixed with white */
  NES_COLOR_PALE_AZURE        = 0x31,
  NES_COLOR_PALE_BLUE         = 0x32,
  NES_COLOR_PALE_VIOLET       = 0x33,
  NES_COLOR_PALE_MAGENTA      = 0x34,
  NES_COLOR_PALE_ROSE         = 0x35,
  NES_COLOR_PALE_RED          = 0x36,
  NES_COLOR_PALE_ORANGE       = 0x37,
  NES_COLOR_PALE_YELLOW       = 0x38,
  NES_COLOR_PALE_CHARTREUSE   = 0x39,
  NES_COLOR_PALE_GREEN        = 0x3A,
  NES_COLOR_PALE_SPRING       = 0x3B,
  NES_COLOR_PALE_CYAN         = 0x3C
} nes_color;

typedef enum
{
  DR_ENDIANNESS_INVALID = 0,

  DR_ENDIANNESS_LITTLE,
  DR_ENDIANNESS_BIG,
  DR_ENDIANNESS_WORDFLIPPED,

  DR_ENDIANNESS_SIZE
} dr_endianness;

typedef enum
{
  DR_GAME_INVALID = 0,

  DR_GAME_MARIOPARTY1,
  DR_GAME_MARIOPARTY2,
  DR_GAME_MARIOPARTY3,

  DR_GAME_SIZE
} dr_game;

typedef enum
{
  DR_GUEST_INVALID = 0,

  DR_GUEST_DOLPHIN,
  DR_GUEST_MARIOPARTY1,
  DR_GUEST_MARIOPARTY2,
  DR_GUEST_MARIOPARTY3,
  DR_GUEST_MARIOPARTY4,
  DR_GUEST_MARIOPARTY5,
  DR_GUEST_MARIOPARTY6,
  DR_GUEST_MARIOPARTY7,
  DR_GUEST_KIRBYAIRRIDE,
  DR_GUEST_SMASHREMIX,
  DR_GUEST_MARIOTENNIS,
  DR_GUEST_MARIOKART64,
  DR_GUEST_MARIOPARTYADVANCE,
  DR_GUEST_MARIOPARTYE,
  DR_GUEST_MARIOKARTDOUBLEDASH,
  DR_GUEST_POKEMONSTADIUM2,
  DR_GUEST_KIRBY64,
  DR_GUEST_BANJOTOOIE,
  DR_GUEST_MARIOPARTY9,
  DR_GUEST_SONICSHUFFLE,
  DR_GUEST_MARIOPARTY8,
  DR_GUEST_SUPERMARIOBROS3,
  DR_GUEST_YOSHISISLAND,

  DR_GUEST_SIZE
} dr_guest;

typedef enum
{
  DR_TEAM_COLOR_INVALID = 0,

  DR_TEAM_COLOR_BLUE,
  DR_TEAM_COLOR_RED,
  DR_TEAM_COLOR_YELLOW,
  DR_TEAM_COLOR_GREEN,

  DR_TEAM_COLOR_SIZE
} dr_team_color;

typedef enum
{
  DR_TEAM_TYPE_INVALID = 0,

  DR_TEAM_TYPE_4P,
  DR_TEAM_TYPE_2V2,
  DR_TEAM_TYPE_1V3_SOLO,
  DR_TEAM_TYPE_1V3_GROUP,

  /* Entirely solo -- used for 1P mini-games, item games, etc. */
  DR_TEAM_TYPE_SOLO,

  DR_TEAM_TYPE_SIZE
} dr_team_type;

typedef struct
{
  dr_character character;
  dr_control_port control_port;
  dr_control_type control_type;
  dr_difficulty difficulty;
  dr_team_color team_color;
  dr_team_type team_type;
  unsigned team_id;
  signed coins; /* current board coins */
  signed stars; /* current board stars */
} dr_player_t;

typedef struct
{
  signed coins;
  signed bonus_coins;
} dr_minigame_result_t;

typedef enum
{
  DR_MINIGAME_INVALID = 0,

  /**
   * Mini-game where all 4 players are against each other
   */
  DR_MINIGAME_4P,

  DR_MINIGAME_1V3,

  DR_MINIGAME_2V2,

  /**
   * Mini-game where a single player receives coins
   * Used in: 1
   */
  DR_MINIGAME_1P,

  /**
   * Slightly longer 4P mini-game which ends in a different results scene
   * Used in: 2 3
   */
  DR_MINIGAME_BATTLE,

  /**
   * Mini-game where a single player receives an item
   * Used in: 2 3
   */
  DR_MINIGAME_ITEM,

  DR_MINIGAME_DUEL,

  DR_MINIGAME_GAME_GUY,

  /**
   * A special mini-game that doesn't fit any of the categories and will
   * probably stay unsupported
   */
  DR_MINIGAME_SPECIAL,

  DR_MINIGAME_SIZE
} dr_minigame_type;

typedef struct
{
  const char *name;
  dr_minigame_type type;
  signed minigame_id;
  signed scene_id;
  dr_emulation_quirk_t quirks;
} dr_mp_minigame_t;

typedef enum
{
  DR_OK = 0,

  DR_ERR_INVALID_PARAMETER,
  DR_ERR_MEMORY_ACCESS_CORE,

  DR_ERROR_SIZE
} dr_error;

typedef enum
{
  DR_LOG_INFO = 0,
  DR_LOG_WARN,
  DR_LOG_ERROR,

  DR_LOG_SIZE
} dr_log_level;

static inline const char *dr_team_color_name(dr_team_color c)
{
  switch (c)
  {
  case DR_TEAM_COLOR_BLUE:
    return "Blue";
  case DR_TEAM_COLOR_RED:
    return "Red";
  case DR_TEAM_COLOR_GREEN:
    return "Green";
  case DR_TEAM_COLOR_YELLOW:
    return "Yellow";
  default:
    return "Unknown";
  }
}

static inline const char *dr_minigame_type_name(dr_minigame_type t)
{
  switch (t)
  {
  case DR_MINIGAME_4P:
    return "4P";
  case DR_MINIGAME_1V3:
    return "1v3";
  case DR_MINIGAME_2V2:
    return "2v2";
  case DR_MINIGAME_1P:
    return "1P";
  case DR_MINIGAME_BATTLE:
    return "Battle";
  case DR_MINIGAME_ITEM:
    return "Item";
  case DR_MINIGAME_DUEL:
    return "Duel";
  case DR_MINIGAME_GAME_GUY:
    return "Game Guy";
  case DR_MINIGAME_SPECIAL:
    return "Special";
  default:
    return "Unknown";
  }
}

static inline const char *dr_character_name(dr_character c)
{
  switch (c)
  {
  case DR_CHARACTER_MARIO:
    return "Mario";
  case DR_CHARACTER_LUIGI:
    return "Luigi";
  case DR_CHARACTER_PEACH:
    return "Peach";
  case DR_CHARACTER_YOSHI:
    return "Yoshi";
  case DR_CHARACTER_WARIO:
    return "Wario";
  case DR_CHARACTER_DONKEY_KONG:
    return "Donkey Kong";
  case DR_CHARACTER_DAISY:
    return "Daisy";
  case DR_CHARACTER_WALUIGI:
    return "Waluigi";
  case DR_CHARACTER_TOAD:
    return "Toad";
  case DR_CHARACTER_BOO:
    return "Boo";
  case DR_CHARACTER_KOOPA_KID:
    return "Koopa Kid";
  case DR_CHARACTER_TOADETTE:
    return "Toadette";
  case DR_CHARACTER_BIRDO:
    return "Birdo";
  case DR_CHARACTER_DRY_BONES:
    return "Dry Bones";
  case DR_CHARACTER_BLOOPER:
    return "Blooper";
  case DR_CHARACTER_HAMMER_BRO:
    return "Hammer Bro";
  default:
    return "Unknown";
  }
}

static inline const char *dr_difficulty_name(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
    return "Very Easy";
  case DR_DIFFICULTY_EASY:
    return "Easy";
  case DR_DIFFICULTY_NORMAL:
    return "Normal";
  case DR_DIFFICULTY_HARD:
    return "Hard";
  case DR_DIFFICULTY_VERY_HARD:
    return "Very Hard";
  default:
    return "Unknown";
  }
}

static inline const char *dr_wii_control_name(dr_wii_control c)
{
  switch (c)
  {
  case DR_WII_CONTROL_INVALID:
    return "Default";
  case DR_WII_CONTROL_UPRIGHT:
    return "Upright";
  case DR_WII_CONTROL_SIDEWAYS_BUTTONS:
    return "Sideways Buttons";
  case DR_WII_CONTROL_SIDEWAYS_MOTION:
    return "Sideways Motion";
  case DR_WII_CONTROL_POINTER:
    return "Pointer";
  case DR_WII_CONTROL_WAGGLE:
    return "Waggle";
  default:
    return "Unknown";
  }
}

QString dr_roms_directory(void);
void dr_set_roms_directory(const QString &path);
QString dr_cores_directory(void);
void dr_set_cores_directory(const QString &path);
QString dr_state_directory(void);
void dr_set_state_directory(const QString &path);
QString dr_save_directory(void);
void dr_set_save_directory(const QString &path);
QString dr_core_path(dr_core core);

/// The dynamic library extension for the current platform (".dll" or ".so").
QString dr_os_extension(void);

void dr_srand(unsigned seed);

int dr_rand(void);

unsigned long dr_rand_count(void);

#endif
