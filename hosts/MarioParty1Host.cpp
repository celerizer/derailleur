#include "MarioParty1Host.h"

#include <cstring>

#include <asm/mp1.h>

#include <QRetroDirectories.h>

static dr_character mp1_char_to_dr(unsigned chr)
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

static const dr_difficulty MP1_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD, // 0x02
  DR_DIFFICULTY_VERY_HARD // 0x03
};

static const dr_minigame_type MP1_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, // 0x00
  DR_MINIGAME_1V3, // 0x01
  DR_MINIGAME_2V2, // 0x02
  DR_MINIGAME_1P // 0x03
};

static const dr_scene_name_t MP1_SCENE_NAMES[] =
{
  { 0x00, "Memory Match", false },
  { 0x01, "Chance Time", false },
  { 0x02, "Slot Machine", false },
  { 0x03, "Buried Treasure", false },
  { 0x04, "Treasure Divers", false },
  { 0x05, "Shell Game", false },
  { 0x06, "Same Game", false }, // unused
  { 0x07, "Hot Bob-omb", false },
  { 0x08, "Yoshi no Shita Awase", false }, // unused
  { 0x09, "Pipe Maze", false },
  { 0x0a, "Ghost Guess", false },
  { 0x0b, "Musical Mushroom", false },
  { 0x0c, "Pedal Power", false },
  { 0x0d, "Crazy Cutter", false },
  { 0x0e, "Face Lift", false },
  { 0x0f, "Whack-a-Plant", false },
  { 0x10, "Bash 'n' Cash", false },
  { 0x11, "Bowl Over", false },
  { 0x12, "Ground Pound", false },
  { 0x13, "Balloon Burst", false },
  { 0x14, "Coin Block Blitz", false },
  { 0x15, "Coin Block Bash", false },
  { 0x16, "Skateboard Scamper", false },
  { 0x17, "Box Mountain Mayhem", false },
  { 0x18, "Platform Peril", false },
  { 0x19, "Teetering Towers", false },
  { 0x1a, "Mushroom Mix-up", false },
  { 0x1b, "Hammer Drop", false },
  { 0x1c, "Grab Bag", false },
  { 0x1d, "Bobsled Run", false },
  { 0x1e, "Bumper Balls", false },
  { 0x1f, "Tightrope Treachery", false },
  { 0x20, "Knock Block Tower", false },
  { 0x21, "Tipsy Tourney", false },
  { 0x22, "Bombs Away", false },
  { 0x23, "Crane Game", false },
  { 0x24, "Coin Shower Flower", false },
  { 0x25, "Slot Car Derby", false },
  { 0x26, "Mario Bandstand", false },
  { 0x27, "Desert Dash", false },
  { 0x28, "Shy Guy Says", false },
  { 0x29, "Limbo Dance", false },
  { 0x2a, "Bombsketball", false },
  { 0x2b, "Cast Aways", false },
  { 0x2c, "Key-pa-way", false },
  { 0x2d, "Running of the Bulb", false },
  { 0x2e, "Hot Rope Jump", false },
  { 0x2f, "Handcar Havoc", false },
  { 0x30, "Deep Sea Divers", false },
  { 0x31, "Piranha's Pursuit", false },
  { 0x32, "Tug o' War", false },
  { 0x33, "Paddle Battle", false },
  { 0x34, "Bumper Ball Maze", false },

  { 0x35, "Loading", true },
  { 0x36, "DK's Jungle Adventure", false },
  { 0x37, "Peach's Birthday Cake", false },
  { 0x38, "Yoshi's Tropical Island", false },
  { 0x39, "Wario's Battle Canyon", false },
  { 0x3a, "Luigi's Engine Room", false },
  { 0x3b, "Mario's Rainbow Castle", false },
  { 0x3c, "Bowser's Magma Mountain", false },
  { 0x3d, "Eternal Star", false },
  { 0x3e, "First Map", false }, // "rules" map
  { 0x3f, "Last 5 Turns", true },
  // { 0x40, "" },
  // { 0x41, "" },
  // { 0x42, "" },
  // { 0x43, "" },
  { 0x44, "Visiting Toad", true }, // generic
  // { 0x45, "" },
  { 0x46, "Visiting Bowser", true }, // generic
  { 0x47, "DK's Jungle Adventure", true }, // talking to whomp
  // { 0x48, "" },
  { 0x49, "Peach's Birthday Cake", true }, // bowser visit
  // { 0x4a, "" },
  { 0x4b, "Peach's Birthday Cake", true }, // goomba visit
  // { 0x4c, "" },
  { 0x4d, "Yoshi's Tropical Island", true }, // thwomp visit
  { 0x4e, "Yoshi's Tropical Island", true }, // bubba event
  { 0x4f, "Yoshi's Tropical Island", true }, // bowser visit
  // { 0x50, "" },
  // { 0x51, "" },
  // { 0x52, "" },
  // { 0x53, "" },
  // { 0x54, "" },
  // { 0x55, "" },
  // { 0x56, "" },
  { 0x57, "Mario's Rainbow Castle", true }, // talking to toad/bowser
  // { 0x58, "" },
  { 0x59, "Bowser's Magma Mountain", true }, // junction
  // { 0x5a, "" },
  // { 0x5b, "" },
  // { 0x5c, "" },
  // { 0x5d, "" },
  { 0x5e, "Eternal Star", true }, // baby bowser visit
  { 0x5f, "Visiting Koopa Troopa", true },
  // { 0x60, "" },
  { 0x61, "Intro", true },
  { 0x62, "Board intro", true },
  // { 0x63, "" },
  // { 0x64, "" },
  { 0x65, "Visiting Boo", true },
  { 0x66, "Booting up", true },
  { 0x67, "Booting up", true },
  { 0x68, "Save data corrupted", true },
  { 0x69, "Mushroom Village", false },
  { 0x6a, "Traveling the Warp Pipe", false },
  { 0x6b, "Mini-Game House", false },
  { 0x6c, "Mushroom Shop", false },
  { 0x6d, "Mushroom Bank", false },
  { 0x6e, "Option House", false },
  { 0x6f, "Mini-Game Explanation", true },
  { 0x70, "Test", true }, // unused
  { 0x71, "Mini-Game Island loading", true },
  { 0x72, "Mini-Game Island", false },
  // { 0x73, "" },
  // { 0x74, "" },
  // { 0x75, "" },
  // { 0x76, "" },
  { 0x77, "Mini-Game Island ending", true },
  { 0x78, "Mini-Game Island intro", true },
  { 0x79, "Mini-Game Island save space", true },
  { 0x7a, "Random Play", true }, // unused
  { 0x7b, "Mini-Game results", true }, // stadium
  { 0x7c, "Mini-Game results", true },
  { 0x7d, "Mini-Game Island results", true },
  { 0x7e, "Sequential Play", true }, // unused
  { 0x7f, "Mini-Game Stadium intro", true },
  { 0x80, "Mini-Game Stadium results", true },
  { 0x81, "Title Screen", true },
  { 0x82, "Mini-Game Stadium intro", true }, // again?
  { 0x83, "Debug menu", true }, // unused

  { -1, nullptr },
};

static DrHostConfig makeConfig()
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString();

  config.char_to_dr = mp1_char_to_dr;
  config.diff_to_dr = MP1_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP1_DIFF_TO_DR) / sizeof(*MP1_DIFF_TO_DR);

  config.minigame_type_to_dr = MP1_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = 4;

  config.cheats.cave = MP1_CAVE;
  config.cheats.cave_addr = MP1_CAVE_ADDR;
  config.cheats.cave_size = MP1_CAVE_SIZE;
  config.cheats.cheat_board = MP1_HOOK_BOARD;

  config.scenes.minigame_explain[0] = 0x6F;
  config.scenes.minigame_explain[1] = -1;

  static const int mp1_boards[] = { 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, -1 };
  memcpy(config.scenes.boards, mp1_boards, sizeof(mp1_boards));

  config.scenes.main_menu = 0x69; // Mushroom Village
  config.scenes.minigame_results = 0x7C;

  config.stat.board = 0x0092;
  config.stat.minigame = 0x0094;
  // stat.duel: no duels in MP1

  config.values.scene = { 0x800C596C, DR_VALUE_TYPE_U16 };
  config.values.character[0] = { 0x800f32b4, DR_VALUE_TYPE_U8 };
  config.values.character[1] = { 0x800f32e4, DR_VALUE_TYPE_U8 };
  config.values.character[2] = { 0x800f3314, DR_VALUE_TYPE_U8 };
  config.values.character[3] = { 0x800f3344, DR_VALUE_TYPE_U8 };
  config.values.controller[0] = { 0x800f32b3, DR_VALUE_TYPE_U8 };
  config.values.controller[1] = { 0x800f32e3, DR_VALUE_TYPE_U8 };
  config.values.controller[2] = { 0x800f3313, DR_VALUE_TYPE_U8 };
  config.values.controller[3] = { 0x800f3343, DR_VALUE_TYPE_U8 };
  config.values.difficulty[0] = { 0x800f32b2, DR_VALUE_TYPE_U8 };
  config.values.difficulty[1] = { 0x800f32e2, DR_VALUE_TYPE_U8 };
  config.values.difficulty[2] = { 0x800f3312, DR_VALUE_TYPE_U8 };
  config.values.difficulty[3] = { 0x800f3342, DR_VALUE_TYPE_U8 };
  config.values.team[0] = { 0x800f32b0, DR_VALUE_TYPE_U8 };
  config.values.team[1] = { 0x800f32e0, DR_VALUE_TYPE_U8 };
  config.values.team[2] = { 0x800f3310, DR_VALUE_TYPE_U8 };
  config.values.team[3] = { 0x800f3340, DR_VALUE_TYPE_U8 };
  config.values.bot[0] = { 0x800f32b7, DR_VALUE_TYPE_U8 };
  config.values.bot[1] = { 0x800f32e7, DR_VALUE_TYPE_U8 };
  config.values.bot[2] = { 0x800f3317, DR_VALUE_TYPE_U8 };
  config.values.bot[3] = { 0x800f3347, DR_VALUE_TYPE_U8 };
  config.values.result[0] = { 0x800f32ba, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x800f32ea, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x800f331a, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x800f334a, DR_VALUE_TYPE_U16 };
  // bonus_result: not available in mp1
  config.values.panel_color[0] = { 0x800f32c7, DR_VALUE_TYPE_U8 };
  config.values.panel_color[1] = { 0x800f32f7, DR_VALUE_TYPE_U8 };
  config.values.panel_color[2] = { 0x800f3327, DR_VALUE_TYPE_U8 };
  config.values.panel_color[3] = { 0x800f3357, DR_VALUE_TYPE_U8 };
  config.values.coins[0] = { 0x800f32b8, DR_VALUE_TYPE_U16 };
  config.values.coins[1] = { 0x800f32e8, DR_VALUE_TYPE_U16 };
  config.values.coins[2] = { 0x800f3318, DR_VALUE_TYPE_U16 };
  config.values.coins[3] = { 0x800f3348, DR_VALUE_TYPE_U16 };
  config.values.stars[0] = { 0x800f32bc, DR_VALUE_TYPE_U16 };
  config.values.stars[1] = { 0x800f32ec, DR_VALUE_TYPE_U16 };
  config.values.stars[2] = { 0x800f331c, DR_VALUE_TYPE_U16 };
  config.values.stars[3] = { 0x800f334c, DR_VALUE_TYPE_U16 };
  config.values.minigame_title_color = { 0x800C4DD0, DR_VALUE_TYPE_U8 };
  config.values.minigame_type = { 0x800D6459, DR_VALUE_TYPE_U8 };
  config.values.minigame_id = { 0x800ED5DE, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP1_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.title_color = { MP1_TITLE_COLORS, DR_VALUE_TYPE_POINTER };
  config.values.scene_stack = { 0x800D86B8, DR_VALUE_TYPE_POINTER };
  config.values.scene_stack_count = { 0x800D86B2, DR_VALUE_TYPE_S16 };
  config.values.turn_total = { 0x800ED5C7, DR_VALUE_TYPE_U8 };
  config.values.turn_current = { 0x800ED5C9, DR_VALUE_TYPE_U8 };
  config.values.turn_owner = { 0x800ED5DC, DR_VALUE_TYPE_S16 };
  config.values.space_index = { 0x800ED5E0, DR_VALUE_TYPE_S16 };
  config.values.rng = { 0x800c2ff4, DR_VALUE_TYPE_U32 };

  config.host_state_addr = MP1_HOST_STATE;

  config.scene_names = MP1_SCENE_NAMES;

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
        m_core->cheatSet(0, true,
          /* Board speed? */
          "81057852 4218"

          /* Force always save... */
          "+800ED5E2 0002"
          "+800ED5E4 0002"
          /* ...except for Mini-Game Stadium */
          "+D10F09F6 007F"
          "+800ED5E2 0000"

          /* Advance "START" prompt */
          "+81046DDC 2400"
        );
      }
    },
    Qt::DirectConnection);
}
