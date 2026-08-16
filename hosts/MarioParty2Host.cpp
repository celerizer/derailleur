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

/* Roulette-title trampolines + hooks (see DrHostConfig::cheat_title_hook). Assembly in
 * scratch RAM at 0x800BF878; glyph block at +0x6000 (0x800C5878), 8 title colors at +0x6500
 * (0x800C5D78). A (list-create, slot in S0) computes block + (type*5 + slot)*32 into A1 and
 * copies the 8 colors into the game's color array (0x800C8D78) before tail-calling the loader
 * (0x800890CC); B (re-read, slot in V1) just redirects. Type byte 0x800DF6C5. (Generated.) */
static const char MP2_CHEAT_TITLE_HOOK[] =
  "810BF878 3C01"  // LUI  AT, type_hi
  "+810BF87A 800E"
  "+810BF87C 9021"  // LBU  AT, type_byte
  "+810BF87E F6C5"
  "+810BF880 0001"  // SLL  V0, AT, 2
  "+810BF882 1080"
  "+810BF884 0041"  // ADDU V0, V0, AT (type*5)
  "+810BF886 1021"
  "+810BF888 0050"  // ADDU V0, V0, S0 (+slot)
  "+810BF88A 1021"
  "+810BF88C 0002"  // SLL  V0, V0, 5 (*32)
  "+810BF88E 1140"
  "+810BF890 3C01"  // LUI  AT, block_hi
  "+810BF892 800C"
  "+810BF894 0022"  // ADDU AT, AT, V0
  "+810BF896 0821"
  "+810BF898 2425"  // ADDIU A1, AT, block_lo
  "+810BF89A 5878"
  "+810BF89C 3C01"  // LUI  AT, type_hi
  "+810BF89E 800E"
  "+810BF8A0 9028"  // LBU  T0, type_byte
  "+810BF8A2 F6C5"
  "+810BF8A4 0008"  // SLL  T0, T0, 3 (type*8)
  "+810BF8A6 40C0"
  "+810BF8A8 3C01"  // LUI  AT, colors_hi
  "+810BF8AA 800C"
  "+810BF8AC 0028"  // ADDU T0, AT, T0 (colors + type*8)
  "+810BF8AE 4021"
  "+810BF8B0 8D02"  // LW   V0, colrow+0
  "+810BF8B2 5D78"
  "+810BF8B4 3C01"  // LUI  AT, coldst_hi
  "+810BF8B6 800D"
  "+810BF8B8 AC22"  // SW   V0, coldst+0
  "+810BF8BA BD78"
  "+810BF8BC 8D02"  // LW   V0, colrow+4
  "+810BF8BE 5D7C"
  "+810BF8C0 AC22"  // SW   V0, coldst+4
  "+810BF8C2 BD7C"
  "+810BF8C4 0802"  // J    loader
  "+810BF8C6 2433"
  "+810BF8C8 0000"  // NOP
  "+810BF8CA 0000"
  "+810BF8CC 3C01"  // LUI  AT, type_hi
  "+810BF8CE 800E"
  "+810BF8D0 9021"  // LBU  AT, type_byte
  "+810BF8D2 F6C5"
  "+810BF8D4 0001"  // SLL  V0, AT, 2
  "+810BF8D6 1080"
  "+810BF8D8 0041"  // ADDU V0, V0, AT
  "+810BF8DA 1021"
  "+810BF8DC 0043"  // ADDU V0, V0, V1 (+cursor)
  "+810BF8DE 1021"
  "+810BF8E0 0002"  // SLL  V0, V0, 5
  "+810BF8E2 1140"
  "+810BF8E4 3C01"  // LUI  AT, block_hi
  "+810BF8E6 800C"
  "+810BF8E8 0022"  // ADDU AT, AT, V0
  "+810BF8EA 0821"
  "+810BF8EC 0802"  // J    loader
  "+810BF8EE 2433"
  "+810BF8F0 2425"  // ADDIU A1, AT, block_lo (delay)
  "+810BF8F2 5878"
  "+8104B168 0C02"  // JAL 0x800BF878 -- list-create -> A
  "+8104B16A FE1E"
  "+8104A564 0C02"  // JAL 0x800BF8CC -- re-read -> B
  "+8104A566 FE33";

/* Force the mini-game roulette onto the slot index (0-4), so the chosen minigame id
 * reads back as exactly the slot. */
static const char MP2_CHEAT_FORCE_ID[] =
  "8104AF9C 2602"   // ADDIU V0, S0, 1           — ID = slot index + 1 (1-5)
  "+8104AF9E 0001"
  "+8104AFAC 1000"  // BEQ  ZERO, ZERO, 0x8004B148 (accept)
  "+8104AFAE 0066";

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
  config.scene_item_first = 0x01; // item mini-games
  config.scene_item_last = 0x06;
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

  config.minigame_title_color_addr = 0x800C8D78;

  config.char_to_dr = MP2_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP2_CHAR_TO_DR) / sizeof(*MP2_CHAR_TO_DR);
  config.diff_to_dr = MP2_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR);

  config.battle_addr = 0x800F9208; // u16

  config.turn_total_addr = 0x800F93AF;   // u8
  config.turn_current_addr = 0x800F93B1; // u8
  config.turn_owner_addr = 0x800F93C6;   // s16 whose turn
  config.space_index_addr = 0x800F93CA;  // s16 current space index
  config.board_guard_is_8bit = false;

  config.panel_color_to_dr = MP2_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP2_PANEL_COLOR_TO_DR) / sizeof(*MP2_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x800DF6C5; // u8
  config.minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR);
  config.minigame_id_addr = 0x800F93C8; // u16 (minigame_id_is_8bit == false)
  config.minigame_id_is_8bit = false;

  static const uint8_t blacklist[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  memcpy(config.minigame_blacklist, blacklist, sizeof(blacklist));
  config.minigame_blacklist_count = 6;

  config.title_block_addr = 0x800C5878; // 0x800BF878 + 0x6000
  config.title_color_addr = 0x800C5D78; // + 0x6500 (after the block)
  config.cheat_title_hook = MP2_CHEAT_TITLE_HOOK;
  config.cheat_force_id = MP2_CHEAT_FORCE_ID;

  config.scene_stack_addr = 0x800E1F58;       // 5 x { s32 scene, s16 event, s16 stat }
  config.scene_stack_count_addr = 0x800E1F52; // s16 element count
  config.scene_stat_board = 0x0192;
  config.scene_stat_minigame = 0x0094;
  // scene_stat_duel: no duels in MP2

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

        // Recommended Codes
        m_core->cheatSet(1, true,
          /* Force always save on... */
          "800F93CC 0000"
          "+800F93CE 0000"
          /* ...except for Mini-Game Trial. */
          "+D10FA63E 004B"
          "+800F93CC 0002"

          /* Advance "START" prompt */
          "+8104F0FC 2400"

          /* Save check -- no longer needed
          "+D10D8BE8 2E03"
          "+800C3C92 00CE"
          "+D30D8BE8 2E03"
          "+800C3C92 0000"
          "+D10FA63E 005B"
          "+800C3C93 001E"
          "+D10C3C92 CE1E"
          "+81113068 0008"
          "+D10C3C92 CE1E"
          "+8111306A 0031"
          */

          /* Honestly don't remember. These may both be speed */
          "+810657EE 0005"
          "+81062D8C 1000"

          /* Disable proceed on board results */
          "+D10FA63E 0051"
          "+811072A0 2400");
      }
    },
    Qt::DirectConnection);
}

