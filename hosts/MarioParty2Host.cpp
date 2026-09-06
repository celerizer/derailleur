#include "MarioParty2Host.h"

#include <cstring>

#include <asm/mp2.h>

#include <QRetroDirectories.h>

static dr_character mp2_char_to_dr(unsigned chr)
{
  switch (chr)
  {
  case 0x00: return DR_CHARACTER_MARIO;
  case 0x01: return DR_CHARACTER_LUIGI;
  case 0x02: return DR_CHARACTER_PEACH;
  case 0x03: return DR_CHARACTER_YOSHI;
  case 0x04: return DR_CHARACTER_WARIO;
  case 0x05: return DR_CHARACTER_DONKEY_KONG;
  }

  return DR_CHARACTER_INVALID;
}

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
  { 0x00, "Booting up", true },

  { 0x01, "Bowser Slots", false },
  { 0x02, "Roll Out the Barrels", false },
  { 0x03, "Coffin Congestion", false },
  { 0x04, "Hammer Slammer", false },
  { 0x05, "Give Me a Brake!", false },
  { 0x06, "Mallet-Go-Round", false },
  { 0x07, "Grab Bag", false },
  { 0x08, "Lava Tile Isle", false },
  { 0x09, "Bumper Balloon Cars", false },
  { 0x0a, "Rakin' 'em In", false },
  { 0x0b, "Day at the Races", false },
  { 0x0c, "Hot Rope Jump", false },
  { 0x0d, "Hot Bob-omb", false },
  { 0x0e, "Bowl Over", false },
  { 0x0f, "Rainbow Run", false },
  { 0x10, "Crane Game", false },
  { 0x11, "Move to the Music", false },
  { 0x12, "Bob-omb Barrage", false },
  { 0x13, "Look Away", false },
  { 0x14, "Shock, Drop or Roll", false },
  { 0x15, "Lights Out", false },
  { 0x16, "Filet Relay", false },
  { 0x17, "Archer-ival", false },
  { 0x18, "Toad Bandstand", false },
  { 0x19, "Bobsled Run", false },
  { 0x1a, "Handcar Havoc", false },
  { 0x1b, "Balloon Burst", false },
  { 0x1c, "Sky Pilots", false },
  { 0x1d, "Speed Hockey", false },
  { 0x1e, "Cake Factory", false },
  { 0x1f, "Dungeon Dash", false },
  { 0x20, "Magnet Carta", false },
  { 0x21, "Face Lift", false },
  { 0x22, "Shell Shocked", false },
  { 0x23, "Crazy Cutters", false },
  { 0x24, "Toad in the Box", false },
  { 0x25, "Mecha-Marathon", false },
  { 0x26, "Roll Call", false },
  { 0x27, "Abandon Ship", false },
  { 0x28, "Platform Peril", false },
  { 0x29, "Totem Pole Pound", false },
  { 0x2a, "Bumper Balls", false },
  { 0x2b, "Bombs Away", false },
  { 0x2c, "Tipsy Tourney", false },
  { 0x2d, "Honeycomb Havoc", false },
  { 0x2e, "Hexagon Heat", false },
  { 0x2f, "Skateboard Scamper", false },
  { 0x30, "Slot Car Derby", false },
  { 0x31, "Shy Guy Says", false },
  { 0x32, "Sneak 'n' Snore", false },
  { 0x33, "Driver's Ed", false },
  { 0x34, "Chance Time", true },
  { 0x35, "Looney Lumberjacks", false },
  { 0x36, "Dizzy Dancing", false },
  { 0x37, "Tile Driver", false },
  { 0x38, "Quicksand Cache", false },
  { 0x39, "Bowser's Big Blast", false },
  { 0x3a, "Torpedo Targets", false },
  { 0x3b, "Destruction Duet", false },
  { 0x3c, "Deep Sea Salvage", false },
  { 0x3d, "Loading", true },
  { 0x3e, "Western Land", false },
  { 0x3f, "Quick Draw Corks", false },
  { 0x40, "Last 5 Turns", true },
  { 0x41, "Pirate Land", false },
  { 0x42, "Saber Swipes", false },
  { 0x43, "Horror Land", false },
  { 0x44, "Mushroom Brew", false },
  { 0x45, "Space Land", false },
  { 0x46, "Time Bomb", false },
  { 0x47, "Mystery Land", false },
  { 0x48, "Psychic Safari", false },
  { 0x49, "Bowser Land", false },
  { 0x4a, "Rock, Paper, Mario", false },
  { 0x4b, "Mini-Game Trial", false },
  { 0x4c, "Rules Land", false },
  { 0x4d, "Rules Land intro", true },
  { 0x4e, "Battle Mode / Duel Mode", false },
  { 0x4f, "Board ending", true },
  { 0x50, "Board ending", true },
  { 0x51, "Board results", true },
  { 0x52, "The Adventure Ends", true },
  { 0x53, "Bowser Event", true },
  // { 0x54, "" },
  { 0x55, "Board intro", false },
  // { 0x56, "" },
  { 0x57, "Booting up", false }, // N64/Hudson logos
  { 0x58, "In the Pipe", false },
  // { 0x59, "" },
  // { 0x5a, "" },
  { 0x5b, "Information Center", false },
  { 0x5c, "Mini-Game Land", false },
  { 0x5d, "Mini-Game Park", false },
  { 0x5e, "Mini-Game Park loading", true },
  { 0x5f, "Mini-Game explanation", true },
  { 0x60, "Mini-Game explanation", true },
  { 0x61, "Game ending", false },
  { 0x62, "Title screen", false }, // and game intro
  { 0x63, "Mini-Game Coaster loading", true },
  { 0x64, "Mini-Game Coaster", false },
  // { 0x65, "" },
  // { 0x66, "" },
  // { 0x67, "" },
  // { 0x68, "" },
  // { 0x69, "" },
  // { 0x6a, "" },
  // { 0x6b, "" },
  // { 0x6c, "" },
  { 0x6d, "Mini-Game Coaster ending", false },
  { 0x6e, "Mini-Game Coaster intro", false },
  { 0x6f, "Battle Game results", true },
  { 0x70, "Mini-Game results", true },
  { 0x71, "Message test", true },
  { 0x72, "Credits", false },

  { -1, nullptr },
};

static DrHostConfig makeConfig(const std::string &game)
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString();
  if (!game.empty())
    config.game = game;

  config.char_to_dr = mp2_char_to_dr;
  config.diff_to_dr = MP2_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR);

  config.minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR);

  config.cheats.cave = MP2_CAVE;
  config.cheats.cave_addr = MP2_CAVE_ADDR;
  config.cheats.cave_size = MP2_CAVE_SIZE;
  config.cheats.cheat_board = MP2_HOOK_BOARD;

  static const int mp2_1p_scenes[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, -1 };
  memcpy(config.scenes.single_player_ids, mp2_1p_scenes, sizeof(mp2_1p_scenes));

  config.scenes.minigame_explain[0] = 0x5F;
  config.scenes.minigame_explain[1] = 0x60;
  config.scenes.minigame_explain[2] = -1;

  static const int mp2_boards[] = { 0x3E, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, -1 };
  memcpy(config.scenes.boards, mp2_boards, sizeof(mp2_boards));

  config.scenes.main_menu = 0x5B; // Information Center
  config.scenes.board_results = 0x52;
  config.scenes.last_five_turns = 0x40;
  config.scenes.minigame_results = 0x70;
  config.scenes.minigame_results_battle = 0x6f;
  // scenes.minigame_results_duel: not available in mp2

  config.stat.board = 0x0192;
  config.stat.minigame = 0x0094;
  // stat.duel: no duels in MP2

  config.values.scene = { 0x800FA63E, DR_VALUE_TYPE_U16 };
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
  config.values.result[0] = { 0x800fd2cc, DR_VALUE_TYPE_S16 };
  config.values.result[1] = { 0x800fd300, DR_VALUE_TYPE_S16 };
  config.values.result[2] = { 0x800fd334, DR_VALUE_TYPE_S16 };
  config.values.result[3] = { 0x800fd368, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[0] = { 0x800fd2ca, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x800fd2fe, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x800fd332, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x800fd366, DR_VALUE_TYPE_S16 };
  config.values.panel_color[0] = { 0x800fd2db, DR_VALUE_TYPE_S8 };
  config.values.panel_color[1] = { 0x800fd30f, DR_VALUE_TYPE_S8 };
  config.values.panel_color[2] = { 0x800fd343, DR_VALUE_TYPE_S8 };
  config.values.panel_color[3] = { 0x800fd377, DR_VALUE_TYPE_S8 };
  config.values.coins[0] = { 0x800fd2c8, DR_VALUE_TYPE_S16 };
  config.values.coins[1] = { 0x800fd2fc, DR_VALUE_TYPE_S16 };
  config.values.coins[2] = { 0x800fd330, DR_VALUE_TYPE_S16 };
  config.values.coins[3] = { 0x800fd364, DR_VALUE_TYPE_S16 };
  config.values.stars[0] = { 0x800fd2ce, DR_VALUE_TYPE_S16 };
  config.values.stars[1] = { 0x800fd302, DR_VALUE_TYPE_S16 };
  config.values.stars[2] = { 0x800fd336, DR_VALUE_TYPE_S16 };
  config.values.stars[3] = { 0x800fd36a, DR_VALUE_TYPE_S16 };
  config.values.minigame_title_color = { 0x800C8D78, DR_VALUE_TYPE_U8 };
  config.values.battle_pot = { 0x800f9208, DR_VALUE_TYPE_U16 };
  config.values.minigame_type = { 0x800DF6C5, DR_VALUE_TYPE_U8 };
  config.values.minigame_id = { 0x800F93C8, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP2_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.title_color = { MP2_TITLE_COLORS, DR_VALUE_TYPE_POINTER };
  config.values.scene_stack = { 0x800E1F58, DR_VALUE_TYPE_POINTER };
  config.values.scene_stack_count = { 0x800E1F52, DR_VALUE_TYPE_S16 };
  config.values.turn_total = { 0x800F93AF, DR_VALUE_TYPE_U8 };
  config.values.turn_current = { 0x800F93B1, DR_VALUE_TYPE_U8 };
  config.values.turn_owner = { 0x800F93C6, DR_VALUE_TYPE_S16 };
  config.values.space_index = { 0x800F93CA, DR_VALUE_TYPE_S16 };
  config.values.rng = { 0x800c99b4, DR_VALUE_TYPE_U32 };

  config.host_state_addr = MP2_HOST_STATE;

  config.scene_names = MP2_SCENE_NAMES;

  return config;
}

MarioParty2Host::MarioParty2Host(QObject *parent, const std::string &game)
  : MarioPartyN64Host(makeConfig(game), parent)
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

