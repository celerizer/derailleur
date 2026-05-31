#include "MarioParty1Host.h"

#include <QRetroDirectories.h>

static const dr_character MP1_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO,       // 0x00
  DR_CHARACTER_LUIGI,       // 0x01
  DR_CHARACTER_PEACH,       // 0x02
  DR_CHARACTER_YOSHI,       // 0x03
  DR_CHARACTER_WARIO,       // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty MP1_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY,   // 0x00 TODO verify
  DR_DIFFICULTY_NORMAL, // 0x01 TODO verify
  DR_DIFFICULTY_HARD,   // 0x02 TODO verify
};

static const dr_team_color MP1_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE,    // 0x01
  DR_TEAM_COLOR_RED,     // 0x02
  DR_TEAM_COLOR_YELLOW,  // 0x03
  DR_TEAM_COLOR_GREEN,   // 0x04
};

static const dr_minigame_type MP1_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P,  // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_1P   // 0x03 maybe
};

static DrHostConfig makeConfig()
{
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString(),

    .scene_miniexplain = { 0x6F }, // TODO: verify, may have multiple
    .scene_miniexplain_count = 1,
    .scene_miniresults = 0x7C,
    .scene_miniresults_battle = 0, // not available in mp1
    .scene_addr = 0x800C596E,

    .scene_board_ranges      = {},
    .scene_board_range_count = 0,
    .scene_duel_board_range  = {},

    .character_addr    = { 0x800f32b7, 0x800f32e7, 0x800f3317, 0x800f3347 },
    .controller_addr   = { 0x800f32b0, 0x800f32e0, 0x800f3310, 0x800f3340 },
    .difficulty_addr   = { 0x800f32b1, 0x800f32e1, 0x800f3311, 0x800f3341 },
    .team_addr         = { 0x800f32b3, 0x800f32e3, 0x800f3313, 0x800f3343 },
    .bot_addr          = { 0x800f32b4, 0x800f32e4, 0x800f3314, 0x800f3344 },
    .result_addr       = { 0x800f32b8, 0x800f32e8, 0x800f3318, 0x800f3348 },
    .bonus_result_addr = { 0, 0, 0, 0 }, // not available in mp1
    .panel_color_addr  = { 0x800f32c4, 0x800f32f4, 0x800f3324, 0x800f3354 },

    .char_to_dr      = MP1_CHAR_TO_DR,
    .char_to_dr_size = sizeof(MP1_CHAR_TO_DR) / sizeof(*MP1_CHAR_TO_DR),
    .diff_to_dr      = MP1_DIFF_TO_DR,
    .diff_to_dr_size = sizeof(MP1_DIFF_TO_DR) / sizeof(*MP1_DIFF_TO_DR),

    .battle_addr = 0,

    .panel_color_to_dr      = MP1_PANEL_COLOR_TO_DR,
    .panel_color_to_dr_size = sizeof(MP1_PANEL_COLOR_TO_DR) / sizeof(*MP1_PANEL_COLOR_TO_DR),

    .minigame_type_addr = 0x800D645A,
    .minigame_type_to_dr = MP1_MINIGAME_TYPE_TO_DR,
    .minigame_type_to_dr_size = 4,

    .minigame_id_addr         = 0x800ED5DC,
    .minigame_id_is_8bit      = false,
    .minigame_blacklist       = {},
    .minigame_blacklist_count = 0,
  };
}

MarioParty1Host::MarioParty1Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
}
