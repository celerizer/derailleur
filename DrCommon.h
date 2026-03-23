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
  DR_DIFFICULTY_INVALID = 0,

  DR_DIFFICULTY_VERY_EASY,
  DR_DIFFICULTY_EASY,
  DR_DIFFICULTY_NORMAL,
  DR_DIFFICULTY_HARD,
  DR_DIFFICULTY_VERY_HARD,

  DR_DIFFICULTY_SIZE
} dr_difficulty;

typedef enum
{
  DR_TEAM_INVALID = 0,

  DR_TEAM_BLUE,
  DR_TEAM_RED,

  DR_TEAM_SIZE
} dr_team;

typedef struct
{
  dr_character    character;
  dr_control_port control_port;
  dr_control_type control_type;
  dr_difficulty   difficulty;
  dr_team         team;
} dr_player_t;

typedef struct
{
  unsigned coins;
  unsigned bonus_coins;
} dr_minigame_result_t;

typedef enum
{
  DR_MINIGAME_INVALID,
  DR_MINIGAME_4P,
  DR_MINIGAME_1V3,
  DR_MINIGAME_2V2,
  DR_MINIGAME_BATTLE,
  DR_MINIGAME_DUEL,
  DR_MINIGAME_ITEM,
} dr_minigame_type;

typedef struct
{
  const char      *name;
  dr_minigame_type type;
  unsigned         minigame_id;
  unsigned         scene_id;
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
  default:
    return "Unknown";
  }
}

#endif
