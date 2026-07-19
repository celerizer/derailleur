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
  0x800df6c4,
  0x800df6c0,
  0x800df6c1,
  0x800df6c2,
  0x800df6c3,
};

static const size_t MP2_MINIGAME_TITLE_ADDRS[6] = {
  0xB1157363, 0xB1157389, 0xB11573AF, 0xB11573D1, 0xB11573F7, 0xB115741D,
};

static const dr_scene_name_t MP2_SCENE_NAMES[] =
{
  { 0x00, "Booting up" },

  { 0x01, "Bowser Slots" },
  { 0x02, "Roll Out the Barrels" },
  { 0x03, "Coffin Congestion" },
  { 0x04, "Hammer Slammer" },
  { 0x05, "Give Me a Brake!" },
  { 0x06, "Mallet-Go-Round" },
  { 0x07, "Grab Bag" },
  { 0x08, "Lava Tile Isle" },
  { 0x09, "Bumper Balloon Cars" },
  { 0x0a, "Rakin' 'em In" },
  { 0x0b, "Day at the Races" },
  { 0x0c, "Hot Rope Jump" },
  { 0x0d, "Hot Bob-omb" },
  { 0x0e, "Bowl Over" },
  { 0x0f, "Rainbow Run" },
  { 0x10, "Crane Game" },
  { 0x11, "Move to the Music" },
  { 0x12, "Bob-omb Barrage" },
  { 0x13, "Look Away" },
  { 0x14, "Shock, Drop or Roll" },
  { 0x15, "Lights Out" },
  { 0x16, "Filet Relay" },
  { 0x17, "Archer-ival" },
  { 0x18, "Toad Bandstand" },
  { 0x19, "Bobsled Run" },
  { 0x1a, "Handcar Havoc" },
  { 0x1b, "Balloon Burst" },
  { 0x1c, "Sky Pilots" },
  { 0x1d, "Speed Hockey" },
  { 0x1e, "Cake Factory" },
  { 0x1f, "Dungeon Dash" },
  { 0x20, "Magnet Carta" },
  { 0x21, "Face Lift" },
  { 0x22, "Shell Shocked" },
  { 0x23, "Crazy Cutters" },
  { 0x24, "Toad in the Box" },
  { 0x25, "Mecha-Marathon" },
  { 0x26, "Roll Call" },
  { 0x27, "Abandon Ship" },
  { 0x28, "Platform Peril" },
  { 0x29, "Totem Pole Pound" },
  { 0x2a, "Bumper Balls" },
  { 0x2b, "Bombs Away" },
  { 0x2c, "Tipsy Tourney" },
  { 0x2d, "Honeycomb Havoc" },
  { 0x2e, "Hexagon Heat" },
  { 0x2f, "Skateboard Scamper" },
  { 0x30, "Slot Car Derby" },
  { 0x31, "Shy Guy Says" },
  { 0x32, "Sneak 'n' Snore" },
  { 0x33, "Driver's Ed" },
  { 0x34, "Chance Time" },
  { 0x35, "Looney Lumberjacks" },
  { 0x36, "Dizzy Dancing" },
  { 0x37, "Tile Driver" },
  { 0x38, "Quicksand Cache" },
  { 0x39, "Bowser's Big Blast" },
  { 0x3a, "Torpedo Targets" },
  { 0x3b, "Destruction Duet" },
  { 0x3c, "Deep Sea Salvage" },
  { 0x3d, "Loading" },
  { 0x3e, "Western Land" },
  { 0x3f, "Quick Draw Corks" },
  { 0x40, "Last 5 Turns" },
  { 0x41, "Pirate Land" },
  { 0x42, "Saber Swipes" },
  { 0x43, "Horror Land" },
  { 0x44, "Mushroom Brew" },
  { 0x45, "Space Land" },
  { 0x46, "Time Bomb" },
  { 0x47, "Mystery Land" },
  { 0x48, "Psychic Safari" },
  { 0x49, "Bowser Land" },
  { 0x4a, "Rock, Paper, Mario" },
  { 0x4b, "Mini-Game Trial" },
  { 0x4c, "Rules Land" },
  { 0x4d, "Rules Land intro" },
  { 0x4e, "Battle Mode / Duel Mode" },
  { 0x4f, "Board ending" },
  { 0x50, "Board ending" },
  { 0x51, "Board results" },
  { 0x52, "The Adventure Ends" },
  { 0x53, "Bowser Event" },
  // { 0x54, "" },
  { 0x55, "Board intro" },
  // { 0x56, "" },
  { 0x57, "Booting up" }, // N64/Hudson logos
  { 0x58, "In the Pipe" },
  // { 0x59, "" },
  // { 0x5a, "" },
  { 0x5b, "Information Center" },
  { 0x5c, "Mini-Game Land" },
  { 0x5d, "Mini-Game Park" },
  { 0x5e, "Mini-Game Park loading" },
  { 0x5f, "Mini-Game explanation" },
  { 0x60, "Mini-Game explanation" },
  { 0x61, "Game ending" },
  { 0x62, "Title screen" }, // and game intro
  { 0x63, "Mini-Game Coaster loading" },
  { 0x64, "Mini-Game Coaster" },
  // { 0x65, "" },
  // { 0x66, "" },
  // { 0x67, "" },
  // { 0x68, "" },
  // { 0x69, "" },
  // { 0x6a, "" },
  // { 0x6b, "" },
  // { 0x6c, "" },
  { 0x6d, "Mini-Game Coaster ending" },
  { 0x6e, "Mini-Game Coaster intro" },
  { 0x6f, "Battle Game results" },
  { 0x70, "Mini-Game results" },
  { 0x71, "Message test" },
  { 0x72, "Credits" },

  { -1, nullptr },
};

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
  config.scene_board_results = 0x52;
  config.scene_last_five_turns = 0x40;
  config.scene_addr = 0x800FA63E; // u16

  static const uint8_t mp2_boards[] = { 0x3E, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B };
  memcpy(config.scene_board_ids, mp2_boards, sizeof(mp2_boards));
  config.scene_board_id_count = sizeof(mp2_boards) / sizeof(*mp2_boards);

  config.character_addr[0] = 0x800fd2c4;
  config.character_addr[1] = 0x800fd2f8;
  config.character_addr[2] = 0x800fd32c;
  config.character_addr[3] = 0x800fd360;
  config.controller_addr[0] = 0x800fd2c3;
  config.controller_addr[1] = 0x800fd2f7;
  config.controller_addr[2] = 0x800fd32b;
  config.controller_addr[3] = 0x800fd35f;
  config.difficulty_addr[0] = 0x800fd2c2;
  config.difficulty_addr[1] = 0x800fd2f6;
  config.difficulty_addr[2] = 0x800fd32a;
  config.difficulty_addr[3] = 0x800fd35e;
  config.team_addr[0] = 0x800fd2c0;
  config.team_addr[1] = 0x800fd2f4;
  config.team_addr[2] = 0x800fd328;
  config.team_addr[3] = 0x800fd35c;
  config.bot_addr[0] = 0x800fd2c7;
  config.bot_addr[1] = 0x800fd2fb;
  config.bot_addr[2] = 0x800fd32f;
  config.bot_addr[3] = 0x800fd363;
  config.result_addr[0] = 0x800fd2cc;
  config.result_addr[1] = 0x800fd300;
  config.result_addr[2] = 0x800fd334;
  config.result_addr[3] = 0x800fd368;
  config.bonus_result_addr[0] = 0x800fd2ca;
  config.bonus_result_addr[1] = 0x800fd2fe;
  config.bonus_result_addr[2] = 0x800fd332;
  config.bonus_result_addr[3] = 0x800fd366;
  config.panel_color_addr[0] = 0x800fd2db;
  config.panel_color_addr[1] = 0x800fd30f;
  config.panel_color_addr[2] = 0x800fd343;
  config.panel_color_addr[3] = 0x800fd377;

  config.char_to_dr = MP2_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP2_CHAR_TO_DR) / sizeof(*MP2_CHAR_TO_DR);
  config.diff_to_dr = MP2_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR);

  config.battle_addr = 0x800F9208; // u16

  config.turn_total_addr = 0x800F93AF;   // u8
  config.turn_current_addr = 0x800F93B1; // u8

  config.panel_color_to_dr = MP2_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP2_PANEL_COLOR_TO_DR) / sizeof(*MP2_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x800DF6C5; // u8
  config.minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR);
  config.next_scene_addr = 0x800fdc0b; // unused
  config.minigame_id_addr = 0x800F93C8; // u16 (minigame_id_is_8bit == false)
  config.minigame_id_is_8bit = false;

  static const uint8_t blacklist[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  memcpy(config.minigame_blacklist, blacklist, sizeof(blacklist));
  config.minigame_blacklist_count = 6;

  config.title_addrs = MP2_MINIGAME_TITLE_ADDRS;
  config.title_id_base = 0x25;
  config.title_id_step = 2;
  config.title_len_offset = 2;

  config.slot_addrs = MP2_SLOT_ADDRS;
  config.scene_trampoline_addr = 0x800D34E0;

  config.scene_names = MP2_SCENE_NAMES;

  return config;
}

MarioParty2Host::MarioParty2Host(QObject *parent)
  : MarioPartyN64Host(makeConfig(), parent)
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

