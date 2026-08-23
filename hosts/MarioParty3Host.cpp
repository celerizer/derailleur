#include "MarioParty3Host.h"

#include <cstring>

#include <asm/mp3.h>

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

  { 0x5a, "Loading (duel)" },
  { 0x5b, "Gate Guy" },
  { 0x5c, "Arrowhead" },
  { 0x5d, "Pipesqueak" },
  { 0x5e, "Blowhard" },
  { 0x5f, "Mr. Mover" },
  { 0x60, "Backtrack" },
  // { 0x61, "" },
  // { 0x62, "" },
  // { 0x63, "" },
  { 0x64, "Duel Board intro", true },
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

  config.char_to_dr = MP3_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP3_CHAR_TO_DR) / sizeof(*MP3_CHAR_TO_DR);
  config.diff_to_dr = MP3_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP3_DIFF_TO_DR) / sizeof(*MP3_DIFF_TO_DR);

  config.minigame_type_to_dr = MP3_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = sizeof(MP3_MINIGAME_TYPE_TO_DR) / sizeof(*MP3_MINIGAME_TYPE_TO_DR);

  config.cheats.cave = MP3_CAVE;
  config.cheats.cave_addr = MP3_CAVE_ADDR;
  config.cheats.cave_size = MP3_CAVE_SIZE;
  config.cheats.cheat_board = MP3_HOOK_BOARD;
  config.cheats.cheat_duel = MP3_HOOK_DUEL;

  // item / 1P mini-games (Winner's Wheel .. Swing 'n' Swipe)
  static const int mp3_1p_scenes[] = { 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, -1 };
  memcpy(config.scenes.single_player_ids, mp3_1p_scenes, sizeof(mp3_1p_scenes));

  config.scenes.minigame_explain[0] = 0x70;
  config.scenes.minigame_explain[1] = -1;

  static const int mp3_boards[] = { 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, -1 };
  memcpy(config.scenes.boards, mp3_boards, sizeof(mp3_boards));

  static const int mp3_duel_boards[] = { 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, -1 };
  memcpy(config.scenes.boards_duel, mp3_duel_boards, sizeof(mp3_duel_boards));

  config.scenes.main_menu = 0x77; // Castle Grounds
  config.scenes.board_results = 0x4f;
  config.scenes.last_five_turns = 0x51;
  config.scenes.minigame_results = 0x71;
  config.scenes.minigame_results_battle = 0x74;
  config.scenes.minigame_results_duel = 0x73;

  config.stat.board = 0x0192;
  config.stat.minigame = 0x0192;
  config.stat.duel = 0x4190;

  config.values.scene = { 0x800ce202, DR_VALUE_TYPE_U16 };
  config.values.character[0] = { 0x800d110b, DR_VALUE_TYPE_U8 };
  config.values.character[1] = { 0x800d1143, DR_VALUE_TYPE_U8 };
  config.values.character[2] = { 0x800d117b, DR_VALUE_TYPE_U8 };
  config.values.character[3] = { 0x800d11b3, DR_VALUE_TYPE_U8 };
  config.values.controller[0] = { 0x800d110a, DR_VALUE_TYPE_U8 };
  config.values.controller[1] = { 0x800d1142, DR_VALUE_TYPE_U8 };
  config.values.controller[2] = { 0x800d117a, DR_VALUE_TYPE_U8 };
  config.values.controller[3] = { 0x800d11b2, DR_VALUE_TYPE_U8 };
  config.values.difficulty[0] = { 0x800d1109, DR_VALUE_TYPE_U8 };
  config.values.difficulty[1] = { 0x800d1141, DR_VALUE_TYPE_U8 };
  config.values.difficulty[2] = { 0x800d1179, DR_VALUE_TYPE_U8 };
  config.values.difficulty[3] = { 0x800d11b1, DR_VALUE_TYPE_U8 };
  config.values.team[0] = { 0x800d1108, DR_VALUE_TYPE_U8 };
  config.values.team[1] = { 0x800d1140, DR_VALUE_TYPE_U8 };
  config.values.team[2] = { 0x800d1178, DR_VALUE_TYPE_U8 };
  config.values.team[3] = { 0x800d11b0, DR_VALUE_TYPE_U8 };
  config.values.bot[0] = { 0x800d110c, DR_VALUE_TYPE_U8 };
  config.values.bot[1] = { 0x800d1144, DR_VALUE_TYPE_U8 };
  config.values.bot[2] = { 0x800d117c, DR_VALUE_TYPE_U8 };
  config.values.bot[3] = { 0x800d11b4, DR_VALUE_TYPE_U8 };
  config.values.result[0] = { 0x800d1110, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x800d1148, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x800d1180, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x800d11b8, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[0] = { 0x800d110e, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x800d1146, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x800d117e, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x800d11b6, DR_VALUE_TYPE_S16 };
  config.values.panel_color[0] = { 0x800d1124, DR_VALUE_TYPE_U8 };
  config.values.panel_color[1] = { 0x800d115c, DR_VALUE_TYPE_U8 };
  config.values.panel_color[2] = { 0x800d1194, DR_VALUE_TYPE_U8 };
  config.values.panel_color[3] = { 0x800d11cc, DR_VALUE_TYPE_U8 };
  config.values.coins[0] = { 0x800d1112, DR_VALUE_TYPE_U16 };
  config.values.coins[1] = { 0x800d114a, DR_VALUE_TYPE_U16 };
  config.values.coins[2] = { 0x800d1182, DR_VALUE_TYPE_U16 };
  config.values.coins[3] = { 0x800d11ba, DR_VALUE_TYPE_U16 };
  config.values.stars[0] = { 0x800d1116, DR_VALUE_TYPE_U8 };
  config.values.stars[1] = { 0x800d114e, DR_VALUE_TYPE_U8 };
  config.values.stars[2] = { 0x800d1186, DR_VALUE_TYPE_U8 };
  config.values.stars[3] = { 0x800d11be, DR_VALUE_TYPE_U8 };
  config.values.mg_star[0] = { 0x800d1130, DR_VALUE_TYPE_S16 };
  config.values.mg_star[1] = { 0x800d1168, DR_VALUE_TYPE_S16 };
  config.values.mg_star[2] = { 0x800d11a0, DR_VALUE_TYPE_S16 };
  config.values.mg_star[3] = { 0x800d11d8, DR_VALUE_TYPE_S16 };
  config.values.minigame_title_color = { 0x80100E9C, DR_VALUE_TYPE_U8 };
  config.values.battle_pot = { 0x800cc698, DR_VALUE_TYPE_U16 };
  config.values.minigame_type = { 0x80102C0D, DR_VALUE_TYPE_U8 };
  config.values.minigame_id = { 0x800cd068, DR_VALUE_TYPE_S8 };
  config.values.title_block = { MP3_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.title_color = { MP3_TITLE_COLORS, DR_VALUE_TYPE_POINTER };
  config.values.title_type_duel = { 0x80102BAD, DR_VALUE_TYPE_U8 };
  config.values.scene_stack = { 0x800D20F0, DR_VALUE_TYPE_POINTER };
  config.values.scene_stack_count = { 0x800D6B60, DR_VALUE_TYPE_S16 };
  config.values.turn_total = { 0x800CD05Au, DR_VALUE_TYPE_U8 };
  config.values.turn_current = { 0x800CD05Bu, DR_VALUE_TYPE_U8 };
  config.values.turn_owner = { 0x800CD067u, DR_VALUE_TYPE_S8 };
  config.values.space_index = { 0x800CD069u, DR_VALUE_TYPE_S8 };
  config.values.rng = { 0x80097650, DR_VALUE_TYPE_U32 };

  config.host_state_addr = MP3_HOST_STATE;

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

        // Recommended Codes -- none!
        // m_core->cheatSet(0, true,
        //   );
      }
    },
    Qt::DirectConnection);
}
