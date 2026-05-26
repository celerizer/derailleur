#include "MarioParty3.h"

#include <QRetroDirectories.h>

static const dr_character k_charToDr[] = {
  DR_CHARACTER_MARIO,       // 0x00
  DR_CHARACTER_LUIGI,       // 0x01
  DR_CHARACTER_PEACH,       // 0x02
  DR_CHARACTER_YOSHI,       // 0x03
  DR_CHARACTER_WARIO,       // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
  DR_CHARACTER_WALUIGI,     // 0x06
  DR_CHARACTER_DAISY,       // 0x07
};

static const dr_difficulty k_diffToDr[] = {
  DR_DIFFICULTY_EASY,   // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD,   // 0x02
};

static const dr_minigame_type k_minigameTypeToDr[] = {
  DR_MINIGAME_4P,     // 0x00
  DR_MINIGAME_1V3,    // 0x01
  DR_MINIGAME_2V2,    // 0x02
  DR_MINIGAME_ITEM,   // 0x03
  DR_MINIGAME_BATTLE, // 0x04
  DR_MINIGAME_DUEL,   // 0x05
};

static const dr_team_color k_panelColorToDr[] = {
  DR_TEAM_COLOR_INVALID, // 0x00 = uninitialized
  DR_TEAM_COLOR_BLUE,    // 0x01
  DR_TEAM_COLOR_RED,     // 0x02
  DR_TEAM_COLOR_GREEN,   // 0x03
  DR_TEAM_COLOR_YELLOW,  // 0x04
};

static DrHostConfig makeConfig()
{
  DrHostConfig c;
  c.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  c.game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString();
  c.scene_miniexplain        = 0x70;
  c.scene_miniresults        = 0x71;
  c.scene_miniresults_battle = 0x74;
  c.scene_board_ranges[0]    = {0x48, 0x4D}; // normal boards
  c.scene_board_ranges[1]    = {0x5B, 0x60}; // duel boards
  c.scene_board_range_count  = 2;
  c.scene_duel_board_range   = {0x5B, 0x60};
  c.scene_addr        = 0x800ce200;
  c.character_addr[0]  = 0x800d1108;
  c.character_addr[1]  = 0x800d1140;
  c.character_addr[2]  = 0x800d1178;
  c.character_addr[3]  = 0x800d11b0;
  c.controller_addr[0] = 0x800d1109;
  c.controller_addr[1] = 0x800d1141;
  c.controller_addr[2] = 0x800d1179;
  c.controller_addr[3] = 0x800d11b1;
  c.difficulty_addr[0] = 0x800d110a;
  c.difficulty_addr[1] = 0x800d1142;
  c.difficulty_addr[2] = 0x800d117a;
  c.difficulty_addr[3] = 0x800d11b2;
  c.team_addr[0]       = 0x800d110b;
  c.team_addr[1]       = 0x800d1143;
  c.team_addr[2]       = 0x800d117b;
  c.team_addr[3]       = 0x800d11b3;
  c.bot_addr[0]        = 0x800d110f;
  c.bot_addr[1]        = 0x800d1147;
  c.bot_addr[2]        = 0x800d117f;
  c.bot_addr[3]        = 0x800d11b7;
  c.result_addr[0]       = 0x800d1112;
  c.result_addr[1]       = 0x800d114a;
  c.result_addr[2]       = 0x800d1182;
  c.result_addr[3]       = 0x800d11ba;
  c.bonus_result_addr[0] = 0x800d110c;
  c.bonus_result_addr[1] = 0x800d1144;
  c.bonus_result_addr[2] = 0x800d117c;
  c.bonus_result_addr[3] = 0x800d11b4;
  c.panel_color_addr[0] = 0x800d1127;
  c.panel_color_addr[1] = 0x800d115f;
  c.panel_color_addr[2] = 0x800d1197;
  c.panel_color_addr[3] = 0x800d11cf;
  c.char_to_dr           = k_charToDr;
  c.char_to_dr_size      = sizeof(k_charToDr) / sizeof(*k_charToDr);
  c.diff_to_dr           = k_diffToDr;
  c.diff_to_dr_size      = sizeof(k_diffToDr) / sizeof(*k_diffToDr);
  c.battle_addr            = 0x800cc69a;
  c.panel_color_to_dr        = k_panelColorToDr;
  c.panel_color_to_dr_size   = sizeof(k_panelColorToDr) / sizeof(*k_panelColorToDr);
  c.minigame_type_addr       = 0x80102C0E;
  c.minigame_type_to_dr      = k_minigameTypeToDr;
  c.minigame_type_to_dr_size = sizeof(k_minigameTypeToDr) / sizeof(*k_minigameTypeToDr);
  return c;
}

MarioParty3::MarioParty3(QObject *parent)
  : DrHost(makeConfig(), parent)
{
}
