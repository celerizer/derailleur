#include "MarioParty3Host.h"

#include <cstring>

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

static const char MP3_CHEAT_REGULAR_BOARD[] =
  "810DFE80 0010"
  "+810DFE82 1040"
  "+810DFE84 2442"
  "+810DFE86 0002"
  "+810DFE90 A022"
  "+810DFEBC 2400"
  "+810DFF28 2400";

static const char MP3_CHEAT_DUEL_BOARD[] =
  "810DFE70 0010"
  "+810DFE72 1040"
  "+810DFE74 2442"
  "+810DFE76 0002"
  "+810DFE80 A022"
  "+810DFEC4 2400"
  "+810DE2AC 2400";

/* Roulette-title trampolines + hooks (see DrHostConfig::cheat_title_hook). Assembly in
 * scratch RAM at 0x80097724; glyph block at +0x6000 (0x8009D724), 8 title colors at +0x6500
 * (0x8009DC24). MP3 has two toggled overlays sharing this base: the shared_board board hook
 * and the name_81 duel/story hook. A (list-create, slot in S0) computes block + (type*5 +
 * slot)*32 into A1 and copies the 8 colors into the game's color array (0x80100E9C) before
 * tail-calling the loader (0x8005B43C); the board A also backs out to the loader on an ITEM
 * type so items keep their real names. B (re-read, slot in V1) just redirects. (Generated.) */
static const char MP3_CHEAT_TITLE_HOOK_BOARD[] =
  "81097724 3C01"  // LUI  AT, type_hi
  "+81097726 8010"
  "+81097728 9021"  // LBU  AT, type_byte
  "+8109772A 2C0D"
  "+8109772C 2402"  // ADDIU V0, ZERO, 3 (ITEM)
  "+8109772E 0003"
  "+81097730 1022"  // BEQ  AT, V0, back_out
  "+81097732 0013"
  "+81097734 0001"  // SLL  V0, AT, 2
  "+81097736 1080"
  "+81097738 0041"  // ADDU V0, V0, AT (type*5)
  "+8109773A 1021"
  "+8109773C 0050"  // ADDU V0, V0, S0 (+slot)
  "+8109773E 1021"
  "+81097740 0002"  // SLL  V0, V0, 5 (*32)
  "+81097742 1140"
  "+81097744 3C01"  // LUI  AT, block_hi
  "+81097746 800A"
  "+81097748 0022"  // ADDU AT, AT, V0
  "+8109774A 0821"
  "+8109774C 2425"  // ADDIU A1, AT, block_lo
  "+8109774E D724"
  "+81097750 3C01"  // LUI  AT, type_hi
  "+81097752 8010"
  "+81097754 9028"  // LBU  T0, type_byte
  "+81097756 2C0D"
  "+81097758 0008"  // SLL  T0, T0, 3 (type*8)
  "+8109775A 40C0"
  "+8109775C 3C01"  // LUI  AT, colors_hi
  "+8109775E 800A"
  "+81097760 0028"  // ADDU T0, AT, T0 (colors + type*8)
  "+81097762 4021"
  "+81097764 8D02"  // LW   V0, colrow+0
  "+81097766 DC24"
  "+81097768 3C01"  // LUI  AT, coldst_hi
  "+8109776A 8010"
  "+8109776C AC22"  // SW   V0, coldst+0
  "+8109776E 0E9C"
  "+81097770 8D02"  // LW   V0, colrow+4
  "+81097772 DC28"
  "+81097774 AC22"  // SW   V0, coldst+4
  "+81097776 0EA0"
  "+81097778 0801"  // J    loader
  "+8109777A 6D0F"
  "+8109777C 0000"  // NOP
  "+8109777E 0000"
  "+81097780 0801"  // J    loader (back_out)
  "+81097782 6D0F"
  "+81097784 0000"  // NOP
  "+81097786 0000"
  "+81097788 3C01"  // LUI  AT, type_hi
  "+8109778A 8010"
  "+8109778C 9021"  // LBU  AT, type_byte
  "+8109778E 2C0D"
  "+81097790 0001"  // SLL  V0, AT, 2
  "+81097792 1080"
  "+81097794 0041"  // ADDU V0, V0, AT
  "+81097796 1021"
  "+81097798 0043"  // ADDU V0, V0, V1 (+cursor)
  "+8109779A 1021"
  "+8109779C 0002"  // SLL  V0, V0, 5
  "+8109779E 1140"
  "+810977A0 3C01"  // LUI  AT, block_hi
  "+810977A2 800A"
  "+810977A4 0022"  // ADDU AT, AT, V0
  "+810977A6 0821"
  "+810977A8 0801"  // J    loader
  "+810977AA 6D0F"
  "+810977AC 2425"  // ADDIU A1, AT, block_lo (delay)
  "+810977AE D724"
  "+810DFFD8 0C02"  // JAL 0x80097724 -- list-create -> A
  "+810DFFDA 5DC9"
  "+810DF480 0C02"  // JAL 0x80097788 -- re-read -> B
  "+810DF482 5DE2";

static const char MP3_CHEAT_TITLE_HOOK_DUEL[] =
  /* --- A: list-create, slot in S0 --- */
  "81097724 0010"  // SLL   V0, S0, 5           - slot*32 (row 0)
  "+81097726 1140"
  "+81097728 3C01"  // LUI   AT, block_hi
  "+8109772A 800A"
  "+8109772C 0022"  // ADDU  AT, AT, V0
  "+8109772E 0821"
  "+81097730 2425"  // ADDIU A1, AT, block_lo    - block base 0x8009D724
  "+81097732 D724"
  "+81097734 3C01"  // LUI   AT, colors_hi
  "+81097736 800A"
  "+81097738 8C22"  // LW    V0, colors+0
  "+8109773A DC24"
  "+8109773C 3C01"  // LUI   AT, coldst_hi
  "+8109773E 8010"
  "+81097740 AC22"  // SW    V0, coldst+0        - name_81 color table 0x80100EEC
  "+81097742 0EEC"
  "+81097744 3C01"  // LUI   AT, colors_hi
  "+81097746 800A"
  "+81097748 8C22"  // LW    V0, colors+4
  "+8109774A DC28"
  "+8109774C 3C01"  // LUI   AT, coldst_hi
  "+8109774E 8010"
  "+81097750 AC22"  // SW    V0, coldst+4
  "+81097752 0EF0"
  "+81097754 0801"  // J     0x8005B43C
  "+81097756 6D0F"
  "+81097758 0000"  // NOP
  "+8109775A 0000"
  /* --- B: re-read, slot in V0 --- */
  "+8109775C 0002"  // SLL   V0, V0, 5           - slot*32 (row 0)
  "+8109775E 1140"
  "+81097760 3C01"  // LUI   AT, block_hi
  "+81097762 800A"
  "+81097764 0022"  // ADDU  AT, AT, V0
  "+81097766 0821"
  "+81097768 0801"  // J     0x8005B43C
  "+8109776A 6D0F"
  "+8109776C 2425"  // ADDIU A1, AT, block_lo    (delay slot)
  "+8109776E D724"
  /* --- stash the slot before A0 is clobbered (dead insn) --- */
  "+810DE148 0080"  // ADDU  V0, A0, ZERO
  "+810DE14A 1021"
  /* --- hooks --- */
  "+810E0164 0C02"   // JAL 0x80097724 -> A
  "+810E0166 5DC9"
  "+810DE160 0C02"  // JAL 0x8009775C -- re-read -> B
  "+810DE162 5DD7";

/* Force the mini-game roulette onto the slot index (0-4). Like the title hook, MP3
 * has a board (shared_board) and a duel/story (name_81) variant, toggled by the host. */
static const char MP3_CHEAT_FORCE_ID_BOARD[] =
  "810DFE84 2602"   // ADDIU V0, S0, 1 — ID = slot index + 1 (1-5)
  "+810DFE86 0001"
  "+810DFE94 1000"  // BEQ  ZERO, ZERO, 0x800DFF80 (accept)
  "+810DFE96 003A";

static const char MP3_CHEAT_FORCE_ID_DUEL[] =
  "810DFE74 2602"   // ADDIU V0, S0, 1 — ID = slot index + 1 (1-5)
  "+810DFE76 0001"
  "+810DFE84 1000"  // BEQ ZERO, ZERO, +0x98 -> 0x800E00E8 (accept)
  "+810DFE86 0098";

static const dr_scene_name_t MP3_SCENE_NAMES[] =
{
  { 0x00, "Booting up" },

  { 0x01, "Hand, Line and Sinker" },
  { 0x02, "Coconut Conk" },
  { 0x03, "Spotlight Swim" },
  { 0x04, "Boulder Ball" },
  { 0x05, "Crazy Cogs" },
  { 0x06, "Hide and Sneak" },
  { 0x07, "Ridiculous Relay" },
  { 0x08, "Thwomp Pull" },
  { 0x09, "River Raiders" },
  { 0x0a, "Tidal Toss" },
  { 0x0b, "Eatsa Pizza" },
  { 0x0c, "Baby Bowser Broadside" },
  { 0x0d, "Pump, Pump and Away" },
  { 0x0e, "Hyper Hydrants" },
  { 0x0f, "Picking Panic" },
  { 0x10, "Cosmic Coaster" },
  { 0x11, "Puddle Paddle" },
  { 0x12, "Etch 'n' Catch" },
  { 0x13, "Log Jam" },
  { 0x14, "Slot Synch" },
  { 0x15, "Treadmill Grill" },
  { 0x16, "Toadstool Titan" },
  { 0x17, "Aces High" },
  { 0x18, "Bounce 'n' Trounce" },
  { 0x19, "Ice Rink Risk" },
  { 0x1a, "Locked Out" },
  { 0x1b, "Chip Shot Challenge" },
  { 0x1c, "Parasol Plummet" },
  { 0x1d, "Messy Memory" },
  { 0x1e, "Picture Imperfect" },
  { 0x1f, "Mario's Puzzle Party" },
  { 0x20, "The Beat Goes On" },
  { 0x21, "M.P.I.Q." },
  { 0x22, "Curtain Call" },
  { 0x23, "Water Whirled" },
  { 0x24, "Frigid Bridges" },
  { 0x25, "Awful Tower" },
  { 0x26, "Cheep Cheep Chase" },
  { 0x27, "Pipe Cleaners" },
  { 0x28, "Snowball Summit" },
  { 0x29, "All Fired Up" },
  { 0x2a, "Stacked Deck" },
  { 0x2b, "Three Door Monty" },
  { 0x2c, "Rockin' Raceway" },
  { 0x2d, "Merry-Go-Chomp" },
  { 0x2e, "Slap Down" },
  { 0x2f, "Storm Chasers" },
  { 0x30, "Eye Sore" },
  { 0x31, "Vine With Me" },
  { 0x32, "Popgun Pick-Off" },
  { 0x33, "End of the Line" },
  { 0x34, "Bowser Toss" },
  { 0x35, "Baby Bowser Bonkers" },
  { 0x36, "Motor Rooter" },
  { 0x37, "Silly Screws" },
  { 0x38, "Crowd Cover" },
  { 0x39, "Tick Tock Hop" },
  { 0x3a, "Fowl Play" },
  { 0x3b, "Winner's Wheel" },
  { 0x3c, "Hey, Batter, Batter!" },
  { 0x3d, "Bobbing Bow-loons" },
  { 0x3e, "Dorrie Dip" },
  { 0x3f, "Swinging with Sharks" },
  { 0x40, "Swing 'n' Swipe" },
  { 0x41, "Stardust Battle" },
  { 0x42, "Game Guy's Roulette" },
  { 0x43, "Game Guy's Lucky 7" },
  { 0x44, "Game Guy's Magic Boxes" },
  { 0x45, "Game Guy's Sweet Surprise" },
  { 0x46, "Dizzy Dinghies" },

  { 0x47, "Loading" },
  { 0x48, "Chilly Waters" },
  { 0x49, "Deep Bloober Sea" },
  { 0x4a, "Spiny Desert" },
  { 0x4b, "Woody Woods" },
  { 0x4c, "Creepy Cavern" },
  { 0x4d, "Waluigi's Island" },
  { 0x4e, "Battle Royal Rule Map" },
  { 0x4f, "Board result" }, // result cutscene
  { 0x50, "Bowser Event" },
  { 0x51, "Last 5 Turns" },
  { 0x52, "Mushroom Genie" },
  { 0x53, "Board intro" },
  { 0x54, "Battle Royal Rule Map intro" },
  { 0x55, "Board result" }, // result screen
  { 0x56, "mchar" },
  { 0x57, "mchar2" }, // unused
  { 0x58, "Booting up" }, // Nintendo/Hudson logos
  { 0x59, "sldebug" }, // unused

  { 0x5a, "Loading" },
  { 0x5b, "Gate Guy" },
  { 0x5c, "Arrowhead" },
  { 0x5d, "Pipesqueak" },
  { 0x5e, "Blowhard" },
  { 0x5f, "Mr. Mover" },
  { 0x60, "Backtrack" },
  // { 0x61, "" },
  // { 0x62, "" },
  // { 0x63, "" },
  // { 0x64, "" },
  // { 0x65, "" },
  // { 0x66, "" },
  { 0x67, "Initializing save file" },
  // { 0x68, "" },
  { 0x69, "Mini-Game Room" },
  { 0x6a, "Chance Time" },
  // { 0x6b, "" },
  // { 0x6c, "" },
  // { 0x6d, "" },
  // { 0x6e, "" },
  // { 0x6f, "" },
  { 0x70, "Mini-Game explanation" },
  { 0x71, "Mini-Game results" },
  { 0x72, "Game Guy results" },
  { 0x73, "Duel Game results" },
  { 0x74, "Battle Game results" },
  // { 0x75, "" },
  // { 0x76, "" },
  { 0x77, "Castle Grounds" },
  { 0x78, "Star Lift" },
  { 0x79, "File select" },
  { 0x7a, "Cutscene" },
  { 0x7b, "Princess Peach's Castle" },
  { 0x7c, "Credits" },
  { 0x7d, "Story Mode result" },
  // { 0x7e, "" },
  { 0x7f, "selmenu" }, // unused

  { -1, nullptr },
};

static DrHostConfig makeConfig()
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString();

  config.scene_miniexplain[0] = 0x70;
  config.scene_miniexplain_count = 1;
  config.scene_miniresults = 0x71;
  config.scene_miniresults_battle = 0x74;
  config.scene_miniresults_duel = 0x73;
  config.scene_item_first = 0x3B; // item mini-games (Winner's Wheel .. Swing 'n' Swipe)
  config.scene_item_last = 0x40;
  config.scene_addr = 0x800ce202; // u16

  static const uint8_t mp3_boards[] = { 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D };
  memcpy(config.scene_board_ids, mp3_boards, sizeof(mp3_boards));
  config.scene_board_id_count = sizeof(mp3_boards) / sizeof(*mp3_boards);

  static const uint8_t mp3_duel_boards[] = { 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60 };
  memcpy(config.scene_duel_board_ids, mp3_duel_boards, sizeof(mp3_duel_boards));
  config.scene_duel_board_id_count = sizeof(mp3_duel_boards) / sizeof(*mp3_duel_boards);

  config.character_addr[0] = 0x800d110b;
  config.character_addr[1] = 0x800d1143;
  config.character_addr[2] = 0x800d117b;
  config.character_addr[3] = 0x800d11b3;
  config.controller_addr[0] = 0x800d110a;
  config.controller_addr[1] = 0x800d1142;
  config.controller_addr[2] = 0x800d117a;
  config.controller_addr[3] = 0x800d11b2;
  config.difficulty_addr[0] = 0x800d1109;
  config.difficulty_addr[1] = 0x800d1141;
  config.difficulty_addr[2] = 0x800d1179;
  config.difficulty_addr[3] = 0x800d11b1;
  config.team_addr[0] = 0x800d1108;
  config.team_addr[1] = 0x800d1140;
  config.team_addr[2] = 0x800d1178;
  config.team_addr[3] = 0x800d11b0;
  config.bot_addr[0] = 0x800d110c;
  config.bot_addr[1] = 0x800d1144;
  config.bot_addr[2] = 0x800d117c;
  config.bot_addr[3] = 0x800d11b4;
  config.result_addr[0] = 0x800d1110;
  config.result_addr[1] = 0x800d1148;
  config.result_addr[2] = 0x800d1180;
  config.result_addr[3] = 0x800d11b8;
  config.bonus_result_addr[0] = 0x800d110e;
  config.bonus_result_addr[1] = 0x800d1146;
  config.bonus_result_addr[2] = 0x800d117e;
  config.bonus_result_addr[3] = 0x800d11b6;
  config.panel_color_addr[0] = 0x800d1124;
  config.panel_color_addr[1] = 0x800d115c;
  config.panel_color_addr[2] = 0x800d1194;
  config.panel_color_addr[3] = 0x800d11cc;
  config.coins_addr[0] = 0x800d1112;
  config.coins_addr[1] = 0x800d114a;
  config.coins_addr[2] = 0x800d1182;
  config.coins_addr[3] = 0x800d11ba;
  config.mg_star_addr[0] = 0x800d1130;
  config.mg_star_addr[1] = 0x800d1168;
  config.mg_star_addr[2] = 0x800d11a0;
  config.mg_star_addr[3] = 0x800d11d8;
  /* MP3 sometimes doesn't credit the mini-game star; the host adds it (bandaid). */
  config.fixup_mg_star = true;

  config.minigame_title_color_addr = 0x80100E9C;

  config.scene_board_results = 0x4f;
  config.scene_last_five_turns = 0x51;

  config.char_to_dr = MP3_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP3_CHAR_TO_DR) / sizeof(*MP3_CHAR_TO_DR);
  config.diff_to_dr = MP3_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP3_DIFF_TO_DR) / sizeof(*MP3_DIFF_TO_DR);

  config.battle_addr = 0x800cc698; // u16

  config.panel_color_to_dr = MP3_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP3_PANEL_COLOR_TO_DR) / sizeof(*MP3_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x80102C0D; // u8
  config.minigame_type_to_dr = MP3_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP3_MINIGAME_TYPE_TO_DR) / sizeof(*MP3_MINIGAME_TYPE_TO_DR);
  config.minigame_id_addr = 0x800cd068; // u8 (minigame_id_is_8bit == true)
  config.minigame_id_is_8bit = true;

  static const uint8_t blacklist[] = { 0x43, 0x44, 0x45 }; // ignore game guy
  memcpy(config.minigame_blacklist, blacklist, sizeof(blacklist));
  config.minigame_blacklist_count = 3;

  config.title_block_addr = 0x8009D724; // 0x80097724 + 0x6000
  config.title_color_addr = 0x8009DC24; // + 0x6500 (after the block)
  config.title_type_addr_duel = 0x80102BAD; // name_81 overlay's type byte
  config.cheat_title_hook = MP3_CHEAT_TITLE_HOOK_BOARD;
  config.cheat_title_hook_duel = MP3_CHEAT_TITLE_HOOK_DUEL;
  config.cheat_force_id = MP3_CHEAT_FORCE_ID_BOARD;
  config.cheat_force_id_duel = MP3_CHEAT_FORCE_ID_DUEL;
  config.scene_stack_addr = 0x800D20F0;
  config.scene_stack_count_addr = 0x800D6B60;
  config.scene_stat_board = 0x0192;
  config.scene_stat_minigame = 0x0192;
  config.scene_stat_duel = 0x4190;
  config.turn_total_addr = 0x800CD05Au; // u8
  config.turn_current_addr = 0x800CD05Bu; // u8
  config.turn_owner_addr = 0x800CD067u;   // s8 whose turn
  config.space_index_addr = 0x800CD069u;  // s8 current space index
  config.board_guard_is_8bit = true;
  config.scene_duel_slot0_addr = 0x80102BA8u; // u8
  config.cheat_regular_board = MP3_CHEAT_REGULAR_BOARD;
  config.cheat_duel_board = MP3_CHEAT_DUEL_BOARD;

  config.scene_names = MP3_SCENE_NAMES;

  return config;
}

MarioParty3Host::MarioParty3Host(QObject *parent)
  : MarioPartyN64Host(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();

        // Unlock all minigames
        m_core->cheatSet(3, true,
          "81035C00 2404"
          "+81035C02 00FF"
          "+D110AE18 1040"
          "+8111B75E 0041"
          "+D110AE18 1040"
          "+8011B761 0047");

        // Recommended Codes
        m_core->cheatSet(4, true,
          "810A12D6 0000"
          
          "+81009C10 080F"
          "+81009C12 FC00"
          "+81009C14 27BD"
          "+81009C16 FFE8"

          "+813FF000 3C05"
          "+813FF002 0013"
          "+813FF004 24A5"
          "+813FF006 0046"
          "+813FF008 1485"
          "+813FF00A 000B"
          "+813FF00C 2400"
          "+813FF010 3C04"
          "+813FF012 0013"
          "+813FF014 2484"
          "+813FF016 001C"
          "+813FF018 3C1B"
          "+813FF01A 800D"
          "+813FF01C 8365"
          "+813FF01E D058"
          "+813FF020 8366"
          "+813FF022 D059"
          "+813FF024 30A5"
          "+813FF026 0001"
          "+813FF028 1405"
          "+813FF02A 0002"
          "+813FF02C 2400"
          "+813FF030 2484"
          "+813FF032 000C"
          "+813FF034 0086"
          "+813FF036 2021"
          "+813FF038 0800"
          "+813FF03A 2706"
          "+813FF03C AFBF"
          "+813FF03E 0014"

          "+D11095AA 2484"
          "+81109348 2400");

        m_core->cheatSet(1, false, MP3_CHEAT_REGULAR_BOARD);
        m_core->cheatSet(2, false, MP3_CHEAT_DUEL_BOARD);
      }
    },
    Qt::DirectConnection);
}
