#ifndef DR_COMMON_H
#define DR_COMMON_H

#include <QString>

static const char *const DERAILLEUR_DATE_STRING = "July 12, 2026";
static const char *const DERAILLEUR_RELEASE_STRING = "r6";

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

typedef union
{
  /* The bits as a raw integer */
  unsigned raw;

  struct
  {
    unsigned needs_efb_to_texture : 1;
    unsigned needs_safe_texture_cache : 1;
    unsigned needs_native_resolution : 1;
  } dolphin;

  struct
  {
    unsigned needs_native_boundaries : 1;
  } mupen64plus;
} dr_emulation_quirk_t;

#define DR_NO_QUIRKS { 0u }
#define DR_QUIRK_EFB_TO_TEXTURE { 0x1u }     // dolphin.needs_efb_to_texture
#define DR_QUIRK_SAFE_TEXTURE_CACHE { 0x2u } // dolphin.needs_safe_texture_cache
#define DR_QUIRK_NATIVE_RESOLUTION { 0x4u }  // dolphin.needs_native_resolution
#define DR_QUIRK_NATIVE_BOUNDARIES { 0x1u }  // mupen64plus.needs_native_boundaries

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
