#include "MarioParty2Host.h"

#include <cstring>

#include <QRetroDirectories.h>


static const dr_character MP2_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO, // 0x00
  DR_CHARACTER_LUIGI, // 0x01
  DR_CHARACTER_PEACH, // 0x02
  DR_CHARACTER_YOSHI, // 0x03
  DR_CHARACTER_WARIO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty MP2_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD, // 0x02
  DR_DIFFICULTY_VERY_HARD // 0x03
};

static const dr_minigame_type MP2_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_INVALID, // 0x03?
  DR_MINIGAME_BATTLE, // 0x04
};

static const dr_team_color MP2_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE, // 0x01
  DR_TEAM_COLOR_RED, // 0x02
  DR_TEAM_COLOR_YELLOW, // 0x03
  DR_TEAM_COLOR_GREEN, // 0x04
};

static const size_t MP2_SLOT_ADDRS[5] = {
  0x800df6c7,
  0x800df6c3,
  0x800df6c2,
  0x800df6c1,
  0x800df6c0,
};

static const size_t MP2_MINIGAME_TITLE_ADDRS[6] = {
  0xB1157363, 0xB1157389, 0xB11573AF, 0xB11573D1, 0xB11573F7, 0xB115741D,
};

static size_t n64ByteAddr(size_t addr)
{
  return (addr & ~size_t(3)) | (3 - (addr & 3));
}

static DrHostConfig makeConfig()
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString();

  config.scene_miniexplain[0] = 0x5F;
  config.scene_miniexplain[1] = 0x60;
  config.scene_miniexplain_count = 2;
  config.scene_miniresults = 0x70;
  config.scene_miniresults_battle = 0x6f;
  // scene_miniresults_duel: not available in mp2
  config.scene_addr = 0x800FA63C;

  config.scene_board_ranges[0] = { 0x3E, 0x43 };
  config.scene_board_range_count = 1;

  config.character_addr[0] = 0x800fd2c7;
  config.character_addr[1] = 0x800fd2fb;
  config.character_addr[2] = 0x800fd32f;
  config.character_addr[3] = 0x800fd363;
  config.controller_addr[0] = 0x800fd2c0;
  config.controller_addr[1] = 0x800fd2f4;
  config.controller_addr[2] = 0x800fd328;
  config.controller_addr[3] = 0x800fd35c;
  config.difficulty_addr[0] = 0x800fd2c1;
  config.difficulty_addr[1] = 0x800fd2f5;
  config.difficulty_addr[2] = 0x800fd329;
  config.difficulty_addr[3] = 0x800fd35d;
  config.team_addr[0] = 0x800fd2c3;
  config.team_addr[1] = 0x800fd2f7;
  config.team_addr[2] = 0x800fd32b;
  config.team_addr[3] = 0x800fd35f;
  config.bot_addr[0] = 0x800fd2c4;
  config.bot_addr[1] = 0x800fd2f8;
  config.bot_addr[2] = 0x800fd32c;
  config.bot_addr[3] = 0x800fd360;
  config.result_addr[0] = 0x800fd2ce;
  config.result_addr[1] = 0x800fd302;
  config.result_addr[2] = 0x800fd336;
  config.result_addr[3] = 0x800fd36a;
  config.bonus_result_addr[0] = 0x800fd2c8;
  config.bonus_result_addr[1] = 0x800fd2fc;
  config.bonus_result_addr[2] = 0x800fd330;
  config.bonus_result_addr[3] = 0x800fd364;
  config.panel_color_addr[0] = 0x800fd2d8;
  config.panel_color_addr[1] = 0x800fd30c;
  config.panel_color_addr[2] = 0x800fd340;
  config.panel_color_addr[3] = 0x800fd374;

  config.char_to_dr = MP2_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP2_CHAR_TO_DR) / sizeof(*MP2_CHAR_TO_DR);
  config.diff_to_dr = MP2_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR);

  config.battle_addr = 0x800F920A;

  config.panel_color_to_dr = MP2_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP2_PANEL_COLOR_TO_DR) / sizeof(*MP2_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x800DF6C6;
  config.minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR);
  config.next_scene_addr = 0x800fdc08; // hw 0x800fdc0b
  config.minigame_id_addr = 0x800F93CA;
  config.minigame_id_is_8bit = false;

  static const uint8_t blacklist[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  memcpy(config.minigame_blacklist, blacklist, sizeof(blacklist));
  config.minigame_blacklist_count = 6;

  config.title_addrs = MP2_MINIGAME_TITLE_ADDRS;
  config.title_id_base = 0x25;
  config.title_id_step = 2;
  config.title_len_offset = 2;
  config.title_addr_transform = n64ByteAddr;

  config.slot_addrs = MP2_SLOT_ADDRS;
  config.scene_trampoline_addr = 0x800D34E0;

  return config;
}

MarioParty2Host::MarioParty2Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();

        // Scene transition trampoline: hook at 0x8007712C, data cell at 0x800D34E0
        m_core->cheatSet(2, true,
          // Hook: J 0x800D34E4 + NOP at 0x8007712C
          "8107712C 0803"
          "+8107712E 4D39"
          "+81077130 0000"  // NOP displaces original instruction at 0x80077130
          "+81077132 0000"
          // Trampoline at 0x800D34E4
          "+810D34E4 3C08"  // LUI  T0, 0x800D
          "+810D34E6 800D"
          "+810D34E8 8D08"  // LW   T0, 0x34E0(T0)   — T0 = *scene_trampoline_addr
          "+810D34EA 34E0"
          "+810D34EC 1100"  // BEQ  T0, ZERO, +5     — if 0, passthrough
          "+810D34EE 0005"
          "+810D34F0 0000"  // NOP                   — (delay slot)
          "+810D34F2 0000"
          "+810D34F4 0008"  // SRL  A0, T0, 16       — A0 = scene
          "+810D34F6 2402"
          "+810D34F8 3107"  // ANDI A3, T0, 0xFFFF   — A3 = modifier
          "+810D34FA FFFF"
          "+810D34FC 0801"  // J    0x80077134       — return with overridden scene
          "+810D34FE DC4D"
          "+810D3500 0000"  // NOP                   — (delay slot)
          "+810D3502 0000"
          // Passthrough: displaced original instructions from 0x8007712C + 0x80077130
          "+810D3504 AC44"  // TODO: displaced instruction from 0x8007712C
          "+810D3506 0000"
          "+810D3508 A447"  // TODO: displaced instruction from 0x80077130
          "+810D350A 0000"
          "+810D350C 0801"  // J    0x80077134       — return passthrough
          "+810D350E DC4D"
          "+810D3510 0000"  // NOP                   — (delay slot)
          "+810D3512 0000");

        // Force Mini-Game Roulette IDs
        m_core->cheatSet(0, true,
          "8104AF98 0010"  // SLL  V0, S0, 1      — roulette ID offset = slot_index * 2 (step=2)
          "+8104AF9A 1040"
          "+8104AF9C 2442"  // ADDIU V0, V0, 0x25  — roulette ID = offset + 0x25 (base)
          "+8104AF9E 0025"
          "+8104AFA8 A022"  // SB   V0, X(AT)      — store injected ID (upper half: patch source reg to V0)
          "+8104B020 2400"  // NOP                 — suppress downstream overwrite of injected ID
        );

        // Increased Board Speed
        m_core->cheatSet(1, true,
          "810657EE 0005"
          "+D110724A CC90"
          "+811071D6 4080"
          "+D110724A CC90"
          "+811071B6 4080"
          "+D1106A6A 38B0"
          "+81106A5A 0006"
          "+D1110166 4020"
          "+81110166 40A0"
          "+D111019E 4040"
          "+8111019E 40C0");
      }
    },
    Qt::DirectConnection);
}

