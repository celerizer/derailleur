#include "MarioParty1Host.h"

#include <cstring>

#include <QRetroDirectories.h>

static const dr_character MP1_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO, // 0x00
  DR_CHARACTER_LUIGI, // 0x01
  DR_CHARACTER_PEACH, // 0x02
  DR_CHARACTER_YOSHI, // 0x03
  DR_CHARACTER_WARIO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty MP1_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD, // 0x02
  DR_DIFFICULTY_VERY_HARD // 0x03
};

static const dr_team_color MP1_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE, // 0x01
  DR_TEAM_COLOR_RED, // 0x02
  DR_TEAM_COLOR_YELLOW, // 0x03
  DR_TEAM_COLOR_GREEN, // 0x04
};

/* Roulette-title trampolines + hooks (see DrHostConfig::cheat_title_hook). Two copies
 * of the same trampoline redirect the title loader to our glyph block at 0x80400080 +
 * (type*5 + slot)*32: A at 0x80400000 for the list-create pass (slot in S0) and B at
 * 0x80400028 for the re-read on selection (slot in V1). Both read the type byte at
 * 0x800D6459 and tail-call the loader at 0x8006D7D8. */
static const char MP1_CHEAT_TITLE_HOOK[] =
  // A: list-create (slot in S0)
  "81400000 3C01"   // LUI   AT, 0x800D
  "+81400002 800D"
  "+81400004 9021"  // LBU   AT, 0x6459(AT)    — type byte
  "+81400006 6459"
  "+81400008 0001"  // SLL   V0, AT, 2
  "+8140000A 1080"
  "+8140000C 0041"  // ADDU  V0, V0, AT        — type * 5
  "+8140000E 1021"
  "+81400010 0050"  // ADDU  V0, V0, S0        — + slot
  "+81400012 1021"
  "+81400014 0002"  // SLL   V0, V0, 5         — * 32
  "+81400016 1140"
  "+81400018 3C01"  // LUI   AT, 0x8040
  "+8140001A 8040"
  "+8140001C 0022"  // ADDU  AT, AT, V0
  "+8140001E 0821"
  "+81400020 0801"  // J     0x8006D7D8
  "+81400022 B5F6"
  "+81400024 2425"  // ADDIU A1, AT, 0x80      — block base (delay slot)
  "+81400026 0080"
  // B: re-read (cursor slot in V1). V1 is the 0-4 cursor position and already the block
  // index -- the force-id +1 only shifts the stored id (read by onMiniexplainDetected),
  // not the cursor -- so index V1 directly with no decrement.
  "+81400028 3C01"  // LUI   AT, 0x800D
  "+8140002A 800D"
  "+8140002C 9021"  // LBU   AT, 0x6459(AT)
  "+8140002E 6459"
  "+81400030 0001"  // SLL   V0, AT, 2
  "+81400032 1080"
  "+81400034 0041"  // ADDU  V0, V0, AT        — type * 5
  "+81400036 1021"
  "+81400038 0043"  // ADDU  V0, V0, V1        — + cursor slot
  "+8140003A 1021"
  "+8140003C 0002"  // SLL   V0, V0, 5         — * 32
  "+8140003E 1140"
  "+81400040 3C01"  // LUI   AT, 0x8040
  "+81400042 8040"
  "+81400044 0022"  // ADDU  AT, AT, V0
  "+81400046 0821"
  "+81400048 0801"  // J     0x8006D7D8
  "+8140004A B5F6"
  "+8140004C 2425"  // ADDIU A1, AT, 0x80      — block base
  "+8140004E 0080"
  // Hooks
  "+81043CB0 0C10"  // JAL 0x80400000 — list-create -> A
  "+81043CB2 0000"
  "+81043100 0C10"  // JAL 0x80400028 — re-read -> B
  "+81043102 000A";

static const char MP1_CHEAT_FORCE_ID[] =
  "81043AB8 2602"   // ADDIU V0, S0, 1 ; ID = slot index + 1 (1-5)
  "+81043ABA 0001"
  "+81043AC8 1000"  // BEQ  ZERO, ZERO, 0x80043C90
  "+81043ACA 0071";

static const dr_minigame_type MP1_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_1P // 0x03
};

static const dr_scene_name_t MP1_SCENE_NAMES[] =
{
  { 0x00, "Memory Match" },
  { 0x01, "Chance Time" },
  { 0x02, "Slot Machine" },
  { 0x03, "Buried Treasure" },
  { 0x04, "Treasure Divers" },
  { 0x05, "Shell Game" },
  { 0x06, "Same Game" }, // unused
  { 0x07, "Hot Bob-omb" },
  { 0x08, "Yoshi no Shita Awase" }, // unused
  { 0x09, "Pipe Maze" },
  { 0x0a, "Ghost Guess" },
  { 0x0b, "Musical Mushroom" },
  { 0x0c, "Pedal Power" },
  { 0x0d, "Crazy Cutter" },
  { 0x0e, "Face Lift" },
  { 0x0f, "Whack-a-Plant" },
  { 0x10, "Bash 'n' Cash" },
  { 0x11, "Bowl Over" },
  { 0x12, "Ground Pound" },
  { 0x13, "Balloon Burst" },
  { 0x14, "Coin Block Blitz" },
  { 0x15, "Coin Block Bash" },
  { 0x16, "Skateboard Scamper" },
  { 0x17, "Box Mountain Mayhem" },
  { 0x18, "Platform Peril" },
  { 0x19, "Teetering Towers" },
  { 0x1a, "Mushroom Mix-up" },
  { 0x1b, "Hammer Drop" },
  { 0x1c, "Grab Bag" },
  { 0x1d, "Bobsled Run" },
  { 0x1e, "Bumper Balls" },
  { 0x1f, "Tightrope Treachery" },
  { 0x20, "Knock Block Tower" },
  { 0x21, "Tipsy Tourney" },
  { 0x22, "Bombs Away" },
  { 0x23, "Crane Game" },
  { 0x24, "Coin Shower Flower" },
  { 0x25, "Slot Car Derby" },
  { 0x26, "Mario Bandstand" },
  { 0x27, "Desert Dash" },
  { 0x28, "Shy Guy Says" },
  { 0x29, "Limbo Dance" },
  { 0x2a, "Bombsketball" },
  { 0x2b, "Cast Aways" },
  { 0x2c, "Key-pa-way" },
  { 0x2d, "Running of the Bulb" },
  { 0x2e, "Hot Rope Jump" },
  { 0x2f, "Handcar Havoc" },
  { 0x30, "Deep Sea Divers" },
  { 0x31, "Piranha's Pursuit" },
  { 0x32, "Tug o' War" },
  { 0x33, "Paddle Battle" },
  { 0x34, "Bumper Ball Maze" },

  { 0x35, "Loading" },
  { 0x36, "DK's Jungle Adventure" },
  { 0x37, "Peach's Birthday Cake" },
  { 0x38, "Yoshi's Tropical Island" },
  { 0x39, "Wario's Battle Canyon" },
  { 0x3a, "Luigi's Engine Room" },
  { 0x3b, "Mario's Rainbow Castle" },
  { 0x3c, "Bowser's Magma Mountain" },
  { 0x3d, "Eternal Star" },
  { 0x3e, "First Map" }, // "rules" map
  { 0x3f, "Last 5 Turns" },
  // { 0x40, "" },
  // { 0x41, "" },
  // { 0x42, "" },
  // { 0x43, "" },
  { 0x44, "Visiting Toad" }, // generic
  // { 0x45, "" },
  { 0x46, "Visiting Bowser" }, // generic
  { 0x47, "DK's Jungle Adventure" }, // talking to whomp
  // { 0x48, "" },
  { 0x49, "Peach's Birthday Cake" }, // bowser visit
  // { 0x4a, "" },
  { 0x4b, "Peach's Birthday Cake" }, // goomba visit
  // { 0x4c, "" },
  { 0x4d, "Yoshi's Tropical Island" }, // thwomp visit
  { 0x4e, "Yoshi's Tropical Island" }, // bubba event
  { 0x4f, "Yoshi's Tropical Island" }, // bowser visit
  // { 0x50, "" },
  // { 0x51, "" },
  // { 0x52, "" },
  // { 0x53, "" },
  // { 0x54, "" },
  // { 0x55, "" },
  // { 0x56, "" },
  { 0x57, "Mario's Rainbow Castle" }, // talking to toad/bowser
  // { 0x58, "" },
  { 0x59, "Bowser's Magma Mountain" }, // junction
  // { 0x5a, "" },
  // { 0x5b, "" },
  // { 0x5c, "" },
  // { 0x5d, "" },
  { 0x5e, "Eternal Star" }, // baby bowser visit
  { 0x5f, "Visiting Koopa Troopa" },
  // { 0x60, "" },
  { 0x61, "Intro" },
  { 0x62, "Board intro" },
  // { 0x63, "" },
  // { 0x64, "" },
  { 0x65, "Visiting Boo" },
  { 0x66, "Booting up" },
  { 0x67, "Booting up" },
  { 0x68, "Save data corrupted" },
  { 0x69, "Mushroom Village" },
  { 0x6a, "Traveling the Warp Pipe" },
  { 0x6b, "Mini-Game House" },
  { 0x6c, "Mushroom Shop" },
  { 0x6d, "Mushroom Bank" },
  { 0x6e, "Option House" },
  { 0x6f, "Mini-Game Explanation" },
  { 0x70, "Test" }, // unused
  { 0x71, "Mini-Game Island loading" },
  { 0x72, "Mini-Game Island" },
  // { 0x73, "" },
  // { 0x74, "" },
  // { 0x75, "" },
  // { 0x76, "" },
  { 0x77, "Mini-Game Island ending" },
  { 0x78, "Mini-Game Island intro" },
  { 0x79, "Mini-Game Island save space" },
  { 0x7a, "Random Play" }, // unused
  { 0x7b, "Mini-Game results" }, // stadium
  { 0x7c, "Mini-Game results" },
  { 0x7d, "Mini-Game Island results" },
  { 0x7e, "Sequential Play" }, // unused
  { 0x7f, "Mini-Game Stadium intro" },
  { 0x80, "Mini-Game Stadium results" },
  { 0x81, "Title Screen" },
  { 0x82, "Mini-Game Stadium intro" }, // again?
  { 0x83, "Debug menu" }, // unused

  { -1, nullptr },
};

static DrHostConfig makeConfig()
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString();

  config.scene_miniexplain[0] = 0x6F;
  config.scene_miniexplain_count = 1;
  config.scene_miniresults = 0x7C;
  
  config.scene_addr = 0x800C596C; // u16

  static const uint8_t mp1_boards[] = { 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D };
  memcpy(config.scene_board_ids, mp1_boards, sizeof(mp1_boards));
  config.scene_board_id_count = sizeof(mp1_boards) / sizeof(*mp1_boards);

  config.character_addr[0] = 0x800f32b4;
  config.character_addr[1] = 0x800f32e4;
  config.character_addr[2] = 0x800f3314;
  config.character_addr[3] = 0x800f3344;
  config.controller_addr[0] = 0x800f32b3;
  config.controller_addr[1] = 0x800f32e3;
  config.controller_addr[2] = 0x800f3313;
  config.controller_addr[3] = 0x800f3343;
  config.difficulty_addr[0] = 0x800f32b2;
  config.difficulty_addr[1] = 0x800f32e2;
  config.difficulty_addr[2] = 0x800f3312;
  config.difficulty_addr[3] = 0x800f3342;
  config.team_addr[0] = 0x800f32b0;
  config.team_addr[1] = 0x800f32e0;
  config.team_addr[2] = 0x800f3310;
  config.team_addr[3] = 0x800f3340;
  config.bot_addr[0] = 0x800f32b7;
  config.bot_addr[1] = 0x800f32e7;
  config.bot_addr[2] = 0x800f3317;
  config.bot_addr[3] = 0x800f3347;
  config.result_addr[0] = 0x800f32ba;
  config.result_addr[1] = 0x800f32ea;
  config.result_addr[2] = 0x800f331a;
  config.result_addr[3] = 0x800f334a;
  // bonus_result_addr: not available in mp1
  config.panel_color_addr[0] = 0x800f32c7;
  config.panel_color_addr[1] = 0x800f32f7;
  config.panel_color_addr[2] = 0x800f3327;
  config.panel_color_addr[3] = 0x800f3357;

  config.minigame_title_color_addr = 0x800C4DD0;

  config.char_to_dr = MP1_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP1_CHAR_TO_DR) / sizeof(*MP1_CHAR_TO_DR);
  config.diff_to_dr = MP1_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP1_DIFF_TO_DR) / sizeof(*MP1_DIFF_TO_DR);

  config.panel_color_to_dr = MP1_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP1_PANEL_COLOR_TO_DR) / sizeof(*MP1_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x800D6459; // u8
  config.minigame_type_to_dr = MP1_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = 4;

  config.minigame_id_addr = 0x800ED5DE; // u16 (minigame_id_is_8bit == false)
  config.minigame_id_is_8bit = false;

  config.title_block_addr = 0x80400080;
  config.cheat_title_hook = MP1_CHEAT_TITLE_HOOK;
  config.cheat_force_id = MP1_CHEAT_FORCE_ID;

  config.scene_stack_addr = 0x800D86B8;       // 5 x { s32 scene, s16 event, s16 stat }
  config.scene_stack_count_addr = 0x800D86B2; // s16 element count
  config.scene_stat_board = 0x0092;
  config.scene_stat_minigame = 0x0094;
  // scene_stat_duel: no duels in MP1

  config.scene_names = MP1_SCENE_NAMES;

  config.turn_total_addr = 0x800ED5C7;   // u8
  config.turn_current_addr = 0x800ED5C9; // u8

  return config;
}

MarioParty1Host::MarioParty1Host(QObject *parent)
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
        m_core->cheatSet(2, true,
          /* Board speed? */
          "81057852 4218"

          /* Force always save... */
          "+800ED5E2 0002"
          "+800ED5E4 0002"
          /* ...except for Mini-Game Stadium */
          "+D10F09F6 007F"
          "+800ED5E2 0000"

          /* Don't proceed on results? */
          "+D10F09F6 0064"
          "+810FB1AC 2400"

          /* Don't proceed on results */
          "+D10F09F6 0040"
          "+810FB1AC 2400"

          /* Advance "START" prompt */
          "+81046DDC 2400"
        );
      }
    },
    Qt::DirectConnection);
}
