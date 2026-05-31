#include "MarioParty3Host.h"

#include <QRetroDirectories.h>

static const dr_character MP3_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO, // 0x00
  DR_CHARACTER_LUIGI, // 0x01
  DR_CHARACTER_PEACH, // 0x02
  DR_CHARACTER_YOSHI, // 0x03
  DR_CHARACTER_WARIO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
  DR_CHARACTER_WALUIGI, // 0x06
  DR_CHARACTER_DAISY, // 0x07
};

static const dr_difficulty MP3_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD, // 0x02
};

static const dr_minigame_type MP3_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_ITEM, // 0x03
  DR_MINIGAME_BATTLE, // 0x04
  DR_MINIGAME_DUEL, // 0x05
};

static const dr_team_color MP3_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE, // 0x01
  DR_TEAM_COLOR_RED, // 0x02
  DR_TEAM_COLOR_YELLOW, // 0x03
  DR_TEAM_COLOR_GREEN, // 0x04
};

static DrHostConfig makeConfig()
{
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString(),

    .scene_miniexplain = { 0x70 },
    .scene_miniexplain_count = 1,
    .scene_miniresults = 0x71,
    .scene_miniresults_battle = 0x74,
    .scene_addr = 0x800ce200,

    .scene_board_ranges = { { 0x48, 0x4D }, { 0x5B, 0x60 } },
    .scene_board_range_count = 2,
    .scene_duel_board_range = { 0x5B, 0x60 },

    .character_addr = { 0x800d1108, 0x800d1140, 0x800d1178, 0x800d11b0 },
    .controller_addr = { 0x800d1109, 0x800d1141, 0x800d1179, 0x800d11b1 },
    .difficulty_addr = { 0x800d110a, 0x800d1142, 0x800d117a, 0x800d11b2 },
    .team_addr = { 0x800d110b, 0x800d1143, 0x800d117b, 0x800d11b3 },
    .bot_addr = { 0x800d110f, 0x800d1147, 0x800d117f, 0x800d11b7 },
    .result_addr = { 0x800d1112, 0x800d114a, 0x800d1182, 0x800d11ba },
    .bonus_result_addr = { 0x800d110c, 0x800d1144, 0x800d117c, 0x800d11b4 },
    .panel_color_addr = { 0x800d1127, 0x800d115f, 0x800d1197, 0x800d11cf },

    .char_to_dr = MP3_CHAR_TO_DR,
    .char_to_dr_size = sizeof(MP3_CHAR_TO_DR) / sizeof(*MP3_CHAR_TO_DR),
    .diff_to_dr = MP3_DIFF_TO_DR,
    .diff_to_dr_size = sizeof(MP3_DIFF_TO_DR) / sizeof(*MP3_DIFF_TO_DR),

    .battle_addr = 0x800cc69a,

    .panel_color_to_dr = MP3_PANEL_COLOR_TO_DR,
    .panel_color_to_dr_size = sizeof(MP3_PANEL_COLOR_TO_DR) / sizeof(*MP3_PANEL_COLOR_TO_DR),

    .minigame_type_addr = 0x80102C0E,
    .minigame_type_to_dr = MP3_MINIGAME_TYPE_TO_DR,
    .minigame_type_to_dr_size = sizeof(MP3_MINIGAME_TYPE_TO_DR) / sizeof(*MP3_MINIGAME_TYPE_TO_DR),
    .minigame_id_addr = 0x800cd06b,
    .minigame_id_is_8bit = true,
    .minigame_blacklist = { 0x43, 0x44, 0x45 }, // ignore game guy
    .minigame_blacklist_count = 3,
  };
}

MarioParty3Host::MarioParty3Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
}
