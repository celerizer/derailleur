#include "MarioParty1Host.h"

#include <QRetroDirectories.h>

static size_t n64ByteAddr(size_t addr)
{
  return (addr & ~size_t(3)) | (3 - (addr & 3));
}

static const dr_character MP1_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO, // 0x00
  DR_CHARACTER_LUIGI, // 0x01
  DR_CHARACTER_PEACH, // 0x02
  DR_CHARACTER_YOSHI, // 0x03
  DR_CHARACTER_WARIO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty MP1_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00 TODO verify
  DR_DIFFICULTY_NORMAL, // 0x01 TODO verify
  DR_DIFFICULTY_HARD, // 0x02 TODO verify
};

static const dr_team_color MP1_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE, // 0x01
  DR_TEAM_COLOR_RED, // 0x02
  DR_TEAM_COLOR_YELLOW, // 0x03
  DR_TEAM_COLOR_GREEN, // 0x04
};

static const size_t MP1_MINIGAME_TITLE_ADDRS[6] = {
  0xB0FDBE54, 0xB0FDBE92, 0xB0FDBED6, 0xB0FDBF1C, 0xB0FDBF5A,
  0xB0FDBF98, // sentinel — TODO: verify
};

static const dr_minigame_type MP1_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_1P // 0x03
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
    .scene_miniresults_duel = 0, // not available in mp1
    .scene_addr = 0x800C596E,

    .scene_board_ranges = { { 0x36, 0x3D } },
    .scene_board_range_count = 1,
    .scene_duel_board_range = {},

    .character_addr = { 0x800f32b7, 0x800f32e7, 0x800f3317, 0x800f3347 },
    .controller_addr = { 0x800f32b0, 0x800f32e0, 0x800f3310, 0x800f3340 },
    .difficulty_addr = { 0x800f32b1, 0x800f32e1, 0x800f3311, 0x800f3341 },
    .team_addr = { 0x800f32b3, 0x800f32e3, 0x800f3313, 0x800f3343 },
    .bot_addr = { 0x800f32b4, 0x800f32e4, 0x800f3314, 0x800f3344 },
    .result_addr = { 0x800f32b8, 0x800f32e8, 0x800f3318, 0x800f3348 },
    .bonus_result_addr = { 0, 0, 0, 0 }, // not available in mp1
    .panel_color_addr = { 0x800f32c4, 0x800f32f4, 0x800f3324, 0x800f3354 },

    .char_to_dr = MP1_CHAR_TO_DR,
    .char_to_dr_size = sizeof(MP1_CHAR_TO_DR) / sizeof(*MP1_CHAR_TO_DR),
    .diff_to_dr = MP1_DIFF_TO_DR,
    .diff_to_dr_size = sizeof(MP1_DIFF_TO_DR) / sizeof(*MP1_DIFF_TO_DR),

    .battle_addr = 0,

    .panel_color_to_dr = MP1_PANEL_COLOR_TO_DR,
    .panel_color_to_dr_size = sizeof(MP1_PANEL_COLOR_TO_DR) / sizeof(*MP1_PANEL_COLOR_TO_DR),

    .minigame_type_addr = 0x800D645A,
    .minigame_type_to_dr = MP1_MINIGAME_TYPE_TO_DR,
    .minigame_type_to_dr_size = 4,

    .next_scene_addr = 0x800F09F4,
    .next_scene_modifier_addr = 0,
    .minigame_id_addr = 0x800ED5DC,
    .minigame_id_is_8bit = false,
    .minigame_blacklist = {},
    .minigame_blacklist_count = 0,

    .title_addrs = MP1_MINIGAME_TITLE_ADDRS,
    .title_id_base = 0x01,
    .title_id_step = 4,
    .title_len_offset = 2,
    .title_addr_transform = n64ByteAddr,
    .slot_addrs = nullptr,

    .scene_trampoline_addr = 0x800CA9A0,
    .turn_total_addr = 0,
    .turn_current_addr = 0,
    .scene_duel_slot0_addr = 0,
    .cheat_regular_board = nullptr,
    .cheat_duel_board = nullptr,
  };
}

MarioParty1Host::MarioParty1Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();

        // Scene transition trampoline: data cell at 0x800CA9A0, code at 0x800CA9A4
        m_core->cheatSet(0, true,
          // Trampoline at 0x800CA9A0
          "810CA9A4 3C08"  // LUI  T0, 0x800D
          "+810CA9A6 800D"
          "+810CA9A8 8D08"  // LW   T0, 0xA9A0(T0)   — T0 = *0x800CA9A0
          "+810CA9AA A9A0"
          "+810CA9AC 1100"  // BEQ  T0, ZERO, +7     — if 0, passthrough
          "+810CA9AE 0007"
          "+810CA9B0 0000"  // NOP                   — (delay slot)
          "+810CA9B2 0000"
          // Override path
          "+810CA9B4 0008"  // SRL  A0, T0, 16       — A0 = scene
          "+810CA9B6 2402"
          "+810CA9B8 3105"  // ANDI A1, T0, 0xFFFF   — A1 = modifier
          "+810CA9BA FFFF"
          "+810CA9BC 0080"  // ADDU S1, A0, ZERO     — save scene to S1
          "+810CA9BE 8821"
          "+810CA9C0 3C04"  // LUI  A0, 0x800F       — load game state pointer
          "+810CA9C2 800F"
          "+810CA9C4 0801"  // J    0x8005E05C       — return
          "+810CA9C6 7817"
          "+810CA9C8 0000"  // NOP                   — (delay slot)
          "+810CA9CA 0000"
          // Passthrough path
          "+810CA9CC 0080"  // ADDU S1, A0, ZERO     — save scene arg to S1
          "+810CA9CE 8821"
          "+810CA9D0 3C04"  // LUI  A0, 0x800F       — load game state pointer
          "+810CA9D2 800F"
          "+810CA9D4 0801"  // J    0x8005E05C       — return
          "+810CA9D6 7817"
          "+810CA9D8 0000"  // NOP                   — (delay slot)
          "+810CA9DA 0000"
          // Hook at 0x8005E054: J 0x800CA9A4 + NOP
          "+8105E054 0803"  // J    0x800CA9A4
          "+8105E056 2A69"
          "+8105E058 2400"  // NOP                   — (delay slot, displaces LUI A0, 0x800F)
        );

        // Force Mini-Game Roulette IDs
        m_core->cheatSet(1, true,
          "81043AB4 0010"  // SLL  V0, S0, 2      — roulette ID offset = slot_index * 4 (step=4)
          "+81043AB6 1080"
          "+81043AB8 2442"  // ADDIU V0, V0, BASE  — roulette ID = offset + base
          "+81043ABA 0001"
          "+81043B08 2400" // NOP
          "+81043B74 2400"  // NOP
        );
      }
    },
    Qt::DirectConnection);
}
