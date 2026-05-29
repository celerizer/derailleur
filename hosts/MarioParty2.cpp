#include "MarioParty2.h"

#include <QRetroDirectories.h>

static const dr_character k_charToDr[] = {
  DR_CHARACTER_MARIO,       // 0x00
  DR_CHARACTER_LUIGI,       // 0x01
  DR_CHARACTER_PEACH,       // 0x02
  DR_CHARACTER_YOSHI,       // 0x03
  DR_CHARACTER_WARIO,       // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty k_diffToDr[] = {
  DR_DIFFICULTY_EASY,   // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD,   // 0x02
  DR_DIFFICULTY_VERY_HARD
};

static const dr_minigame_type k_minigameTypeToDr[] = {
  DR_MINIGAME_4P,     // 0x00
  DR_MINIGAME_1V3,    // 0x01
  DR_MINIGAME_2V2,    // 0x02
  DR_MINIGAME_INVALID,
  DR_MINIGAME_BATTLE, // 0x04
};

static const dr_team_color k_panelColorToDr[] = {
  DR_TEAM_COLOR_INVALID, // 0x00 - TODO: verify
  DR_TEAM_COLOR_BLUE,    // 0x01 - TODO: verify
  DR_TEAM_COLOR_RED,     // 0x02 - TODO: verify
  DR_TEAM_COLOR_GREEN,   // 0x03 - TODO: verify
  DR_TEAM_COLOR_YELLOW,  // 0x04 - TODO: verify
};

static DrHostConfig makeConfig()
{
  DrHostConfig c = {};
  c.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  c.game = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString();
  c.scene_miniexplain        = 0x00; // TODO
  c.scene_miniresults        = 0x00; // TODO
  c.scene_miniresults_battle = 0x6f; // TODO
  c.scene_addr               = 0x00000000; // TODO
  c.character_addr[0]  = 0x00000000; // TODO
  c.character_addr[1]  = 0x00000000; // TODO
  c.character_addr[2]  = 0x00000000; // TODO
  c.character_addr[3]  = 0x00000000; // TODO
  c.controller_addr[0] = 0x00000000; // TODO
  c.controller_addr[1] = 0x00000000; // TODO
  c.controller_addr[2] = 0x00000000; // TODO
  c.controller_addr[3] = 0x00000000; // TODO
  c.difficulty_addr[0] = 0x00000000; // TODO
  c.difficulty_addr[1] = 0x00000000; // TODO
  c.difficulty_addr[2] = 0x00000000; // TODO
  c.difficulty_addr[3] = 0x00000000; // TODO
  c.team_addr[0]       = 0x00000000; // TODO
  c.team_addr[1]       = 0x00000000; // TODO
  c.team_addr[2]       = 0x00000000; // TODO
  c.team_addr[3]       = 0x00000000; // TODO
  c.bot_addr[0]        = 0x00000000; // TODO
  c.bot_addr[1]        = 0x00000000; // TODO
  c.bot_addr[2]        = 0x00000000; // TODO
  c.bot_addr[3]        = 0x00000000; // TODO
  c.result_addr[0]       = 0x00000000; // TODO
  c.result_addr[1]       = 0x00000000; // TODO
  c.result_addr[2]       = 0x00000000; // TODO
  c.result_addr[3]       = 0x00000000; // TODO
  c.bonus_result_addr[0] = 0x00000000; // TODO
  c.bonus_result_addr[1] = 0x00000000; // TODO
  c.bonus_result_addr[2] = 0x00000000; // TODO
  c.bonus_result_addr[3] = 0x00000000; // TODO
  c.panel_color_addr[0] = 0x00000000; // TODO
  c.panel_color_addr[1] = 0x00000000; // TODO
  c.panel_color_addr[2] = 0x00000000; // TODO
  c.panel_color_addr[3] = 0x00000000; // TODO
  c.char_to_dr           = k_charToDr;
  c.char_to_dr_size      = sizeof(k_charToDr) / sizeof(*k_charToDr);
  c.diff_to_dr           = k_diffToDr;
  c.diff_to_dr_size      = sizeof(k_diffToDr) / sizeof(*k_diffToDr);
  c.battle_addr              = 0x00000000; // TODO
  c.panel_color_to_dr        = k_panelColorToDr;
  c.panel_color_to_dr_size   = sizeof(k_panelColorToDr) / sizeof(*k_panelColorToDr);
  c.minigame_type_addr       = 0x800DF6C6;
  c.minigame_type_to_dr      = k_minigameTypeToDr;
  c.minigame_type_to_dr_size = sizeof(k_minigameTypeToDr) / sizeof(*k_minigameTypeToDr);
  c.scene_board_ranges[0]    = {0x00, 0x00}; // TODO
  c.scene_board_range_count  = 0;            // TODO
  c.scene_duel_board_range   = {0x00, 0x00}; // TODO
  return c;
}

MarioParty2::MarioParty2(QObject *parent)
  : DrHost(makeConfig(), parent)
{
}
