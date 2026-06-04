#include "MarioParty3Host.h"

#include <QRetro.h>
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

static const size_t MP3_SLOT_ADDRS[5] = {
  0x80102C0B, // slot 0 (raw 0x80102C08)
  0x80102C0A, // slot 1 (raw 0x80102C09)
  0x80102C09, // slot 2 (raw 0x80102C0A)
  0x80102C08, // slot 3 (raw 0x80102C0B)
  0x80102C0F, // slot 4 (raw 0x80102C0C)
};

// TABLE_BASE=0xB122D74A, HEADER=0x44, ENTRY=0x30
// Slots use odd entries (1,3,5,7,9), each spanning into the next entry for capacity
static const size_t MP3_MINIGAME_TITLE_ADDRS[6] = {
  0xB122D7BF, // entry  1 len byte
  0xB122D81F, // entry  3 len byte
  0xB122D87F, // entry  5 len byte
  0xB122D8DF, // entry  7 len byte
  0xB122D93F, // entry  9 len byte
  0xB122D99F, // entry 11 len byte (sentinel)
};

static size_t n64ByteAddr(size_t addr)
{
  return (addr & ~size_t(3)) | (3 - (addr & 3));
}

// 72 game guy results
/* duel results on duel map scene 0x73 */

static DrHostConfig makeConfig()
{
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString(),

    .scene_miniexplain = { 0x70 },
    .scene_miniexplain_count = 1,
    .scene_miniresults = 0x71,
    .scene_miniresults_battle = 0x74,
    .scene_miniresults_duel = 0x73,
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
    .next_scene_addr = 0x800D2030, // 16-bit value at logical 0x800D2032, physical = addr ^ 2
    .next_scene_modifier_addr = 0x800D2036, // 16-bit value at logical 0x800D2034
    .minigame_id_addr = 0x800cd06b,
    .minigame_id_is_8bit = true,
    .minigame_blacklist = { 0x43, 0x44, 0x45 }, // ignore game guy
    .minigame_blacklist_count = 3,

    .title_addrs = MP3_MINIGAME_TITLE_ADDRS,
    .title_id_base = 2,
    .title_id_step = 2,
    .title_len_offset = 3,
    .title_addr_transform = n64ByteAddr,
    .slot_addrs = MP3_SLOT_ADDRS,
    .scene_trampoline_addr = 0x800A7A54,
  };
}

MarioParty3Host::MarioParty3Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();

        // Hook decompressor at 0x8000B004: if A0 == 0x0122D74A (title table),
        // set A3 = 0 to skip decompression and use our patched copy in place
        m_core->cheatSet(0, true,
          // Hook: J 0x800A7A30 + NOP at 0x8000B004
          "8100B004 0802"
          "+8100B006 9E8C"
          "+8100B008 0000"
          "+8100B00A 0000"
          // Trampoline at 0x800A7A30
          "+810A7A30 AFBF"
          "+810A7A32 0020"
          "+810A7A34 AFA4"
          "+810A7A36 0014"
          "+810A7A38 3C01"
          "+810A7A3A 0123"
          "+810A7A3C 2421"
          "+810A7A3E D74A"
          "+810A7A40 1481"
          "+810A7A42 0002"
          "+810A7A44 0000"
          "+810A7A46 0000"
          "+810A7A48 0000"
          "+810A7A4A 3825"
          "+810A7A4C 0800"
          "+810A7A4E 2C03"
          "+810A7A50 0000"
          "+810A7A52 0000"
          
          // This code intercepts the "next scene" value change to load them
          // instead from our owned memory if nonzero, otherwise use the
          // unmodified function.

          // Hook at 80048168 to 800A7A58
          "+81048168 0802"
          "+8104816A 9E96"
          "+8104816C 2400"
          // Trampoline at 0x800A7A58
          "+810A7A58 3C08"  // LUI  T0, 0x800A
          "+810A7A5A 800A"
          "+810A7A5C 8D08"  // LW   T0, 0x7A54(T0)
          "+810A7A5E 7A54"
          "+810A7A60 1100"  // BEQ  T0, ZERO, 5
          "+810A7A62 0005"
          "+810A7A64 0000"  // NOP
          "+810A7A66 0000"
          "+810A7A68 0008"  // SRL  A0, T0, 16
          "+810A7A6A 2402"
          "+810A7A6C 3107"  // ANDI A3, T0, 0xFFFF
          "+810A7A6E FFFF"
          "+810A7A70 0801"  // J    0x80048170
          "+810A7A72 205C"
          "+810A7A74 0000"  // NOP
          "+810A7A76 0000"
          "+810A7A78 AC44"  // SW   A0, 0(V0)
          "+810A7A7A 0000"
          "+810A7A7C A447"  // SH   A3, 4(V0)
          "+810A7A7E 0004"
          "+810A7A80 0801"  // J    0x80048170
          "+810A7A82 205C"
          "+810A7A84 0000"  // NOP
          "+810A7A86 0000");

        // Unlock all minigames
        m_core->cheatSet(2, true,
          "81035C00 2404"
          "+81035C02 00FF"
          "+D110AE18 1040"
          "+8111B75E 0041"
          "+D110AE18 1040"
          "+8011B761 0047");

        // load next scene as immediate
        //m_core->cheatSet(3, true,
        //  "81048168 3404"
        //  "+8104816C 3407");

        // Force Mini-Game Roulette IDs
        m_core->cheatSet(1, true,
          "810DFE80 0010"
          "+810DFE82 1040"
          "+810DFE84 2442"
          "+810DFE86 0002"
          "+810DFE90 A022"
          "+810DFEBC 2400" // NOP minigame history check
          "+810DFF28 2400" // NOP minigame history check 2? 800CC4A8 keeps a list of the minis played this game
        );

        // Write 16-entry title table to ROM at 0xB122D74A
        static constexpr size_t   TABLE_BASE   = 0xB122D74A;
        static constexpr uint32_t ENTRY_COUNT  = 16;
        static constexpr uint32_t ENTRY_SIZE   = 48;
        static constexpr uint32_t HEADER_SIZE  = 4 + ENTRY_COUNT * 4; // 0x44

        auto xw = [this](uint8_t val, size_t addr) {
          writeu8(val, n64ByteAddr(addr));
        };
        auto xw32 = [&xw](uint32_t val, size_t addr) {
          xw(uint8_t(val >> 24), addr);
          xw(uint8_t(val >> 16), addr + 1);
          xw(uint8_t(val >>  8), addr + 2);
          xw(uint8_t(val      ), addr + 3);
        };

        xw32(ENTRY_COUNT, TABLE_BASE);
        for (uint32_t i = 0; i < ENTRY_COUNT; i++)
          xw32(HEADER_SIZE + i * ENTRY_SIZE, TABLE_BASE + 4 + i * 4);

        for (uint32_t i = 0; i < ENTRY_COUNT; i++)
        {
          size_t base = TABLE_BASE + HEADER_SIZE + i * ENTRY_SIZE;
          xw(0x00, base);     // alignment
          xw(0x03, base + 1); // len (empty: strlen 0 + offset 3)
          xw(0x0B, base + 2); // marker
          for (uint32_t j = 3; j < ENTRY_SIZE; j++)
            xw(0x00, base + j);
        }
      }
    },
    Qt::DirectConnection);
}

