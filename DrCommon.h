#ifndef DR_COMMON_H
#define DR_COMMON_H

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

  DR_CORE_MUPEN64PLUSNEXT,
  DR_CORE_DOLPHIN,

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
    unsigned unused;
  } mupen64plus;
} dr_emulation_quirk_t;

#define DR_NO_QUIRKS { .raw=0 }
#define DR_QUIRK_EFB_TO_TEXTURE { .dolphin={ 1, 0, 0 } }
#define DR_QUIRK_SAFE_TEXTURE_CACHE { .dolphin={ 0, 1, 0 } }
#define DR_QUIRK_NATIVE_RESOLUTION { .dolphin={ 0, 0, 1 } }

typedef enum
{
  DR_TEAM_COLOR_INVALID = 0,

  DR_TEAM_COLOR_BLUE,
  DR_TEAM_COLOR_RED,

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
  const char           *name;
  dr_minigame_type      type;
  unsigned              minigame_id;
  unsigned              scene_id;
  dr_emulation_quirk_t  quirks;
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

static inline const char* dr_character_name(dr_character c)
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

static inline const char* dr_core_path(dr_core core)
{
  switch (core)
  {
  case DR_CORE_MUPEN64PLUSNEXT:
    return "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so";
  case DR_CORE_DOLPHIN:
    return "/media/keith/devtools/libretro/cores/dolphin_libretro.so";
  default:
    return nullptr;
  }
}

#endif
