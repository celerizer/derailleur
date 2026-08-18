#include "MarioParty2Host.h"

#include <cstring>

#include <asm/gameshark/mp2.h>

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

  config.scenes.main_menu = 0x5B; // Information Center
  config.scenes.minigame_explain[0] = 0x5F;
  config.scenes.minigame_explain[1] = 0x60;
  config.scenes.minigame_explain[2] = -1;
  config.scenes.minigame_results = 0x70;
  config.scenes.minigame_results_battle = 0x6f;
  // scenes.minigame_results_duel: not available in mp2
  static const int mp2_1p_scenes[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, -1 };
  memcpy(config.scenes.single_player_ids, mp2_1p_scenes, sizeof(mp2_1p_scenes));
  config.scenes.board_results = 0x52;
  config.scenes.last_five_turns = 0x40;
  config.values.scene = { 0x800FA63E, DR_VALUE_TYPE_U16 }; // u16

  static const int mp2_boards[] = { 0x3E, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, -1 };
  memcpy(config.scenes.boards, mp2_boards, sizeof(mp2_boards));

  config.values.character[0] = { 0x800fd2c4, DR_VALUE_TYPE_U8 };
  config.values.character[1] = { 0x800fd2f8, DR_VALUE_TYPE_U8 };
  config.values.character[2] = { 0x800fd32c, DR_VALUE_TYPE_U8 };
  config.values.character[3] = { 0x800fd360, DR_VALUE_TYPE_U8 };
  config.values.controller[0] = { 0x800fd2c3, DR_VALUE_TYPE_U8 };
  config.values.controller[1] = { 0x800fd2f7, DR_VALUE_TYPE_U8 };
  config.values.controller[2] = { 0x800fd32b, DR_VALUE_TYPE_U8 };
  config.values.controller[3] = { 0x800fd35f, DR_VALUE_TYPE_U8 };
  config.values.difficulty[0] = { 0x800fd2c2, DR_VALUE_TYPE_U8 };
  config.values.difficulty[1] = { 0x800fd2f6, DR_VALUE_TYPE_U8 };
  config.values.difficulty[2] = { 0x800fd32a, DR_VALUE_TYPE_U8 };
  config.values.difficulty[3] = { 0x800fd35e, DR_VALUE_TYPE_U8 };
  config.values.team[0] = { 0x800fd2c0, DR_VALUE_TYPE_U8 };
  config.values.team[1] = { 0x800fd2f4, DR_VALUE_TYPE_U8 };
  config.values.team[2] = { 0x800fd328, DR_VALUE_TYPE_U8 };
  config.values.team[3] = { 0x800fd35c, DR_VALUE_TYPE_U8 };
  config.values.bot[0] = { 0x800fd2c7, DR_VALUE_TYPE_U8 };
  config.values.bot[1] = { 0x800fd2fb, DR_VALUE_TYPE_U8 };
  config.values.bot[2] = { 0x800fd32f, DR_VALUE_TYPE_U8 };
  config.values.bot[3] = { 0x800fd363, DR_VALUE_TYPE_U8 };
  config.values.result[0] = { 0x800fd2cc, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x800fd300, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x800fd334, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x800fd368, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[0] = { 0x800fd2ca, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x800fd2fe, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x800fd332, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x800fd366, DR_VALUE_TYPE_S16 };
  config.values.panel_color[0] = { 0x800fd2db, DR_VALUE_TYPE_U8 };
  config.values.panel_color[1] = { 0x800fd30f, DR_VALUE_TYPE_U8 };
  config.values.panel_color[2] = { 0x800fd343, DR_VALUE_TYPE_U8 };
  config.values.panel_color[3] = { 0x800fd377, DR_VALUE_TYPE_U8 };

  config.values.minigame_title_color = { 0x800C8D78, DR_VALUE_TYPE_U8 };

  config.char_to_dr = MP2_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP2_CHAR_TO_DR) / sizeof(*MP2_CHAR_TO_DR);
  config.diff_to_dr = MP2_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR);

  config.values.battle = { 0x800F9208, DR_VALUE_TYPE_S16 }; // u16

  config.values.turn_total = { 0x800F93AF, DR_VALUE_TYPE_U8 };   // u8
  config.values.turn_current = { 0x800F93B1, DR_VALUE_TYPE_U8 }; // u8
  config.values.turn_owner = { 0x800F93C6, DR_VALUE_TYPE_S16 };  // whose turn
  config.values.space_index = { 0x800F93CA, DR_VALUE_TYPE_S16 }; // current space index

  config.values.minigame_type = { 0x800DF6C5, DR_VALUE_TYPE_U8 }; // u8
  config.minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR);
  config.values.minigame_id = { 0x800F93C8, DR_VALUE_TYPE_S16 };

  config.values.title_block = { MP2_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.title_color = { MP2_TITLE_COLORS, DR_VALUE_TYPE_POINTER };
  config.cheats.cave = MP2_CAVE;
  config.cheats.cave_addr = MP2_CAVE_ADDR;
  config.cheats.cave_size = MP2_CAVE_SIZE;
  config.cheats.cheat_board = MP2_HOOK_BOARD;

  config.values.scene_stack = { 0x800E1F58, DR_VALUE_TYPE_POINTER };       // 5 x { s32 scene, s16 event, s16 stat }
  config.values.scene_stack_count = { 0x800E1F52, DR_VALUE_TYPE_S16 }; // s16 element count
  config.stat.board = 0x0192;
  config.stat.minigame = 0x0094;
  // stat.duel: no duels in MP2

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
        m_core->cheatSet(0, true,
          /* Force always save on... */
          "800F93CC 0000"
          "+800F93CE 0000"
          /* ...except for Mini-Game Trial. */
          "+D10FA63E 004B"
          "+800F93CC 0002"

          /* Advance "START" prompt */
          "+8104F0FC 2400"

          /* Honestly don't remember. These may both be speed */
          "+810657EE 0005"
          "+81062D8C 1000");
      }
    },
    Qt::DirectConnection);
}

