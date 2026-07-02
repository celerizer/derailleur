#include "MarioParty1Host.h"

#include <QRetroDirectories.h>

static size_t n64ByteAddr(size_t addr)
{
  return (addr & ~size_t(3)) | (3 - (addr & 3));
}

static const dr_character MP1_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO, // 0x00
  DR_CHARACTER_LUIGI, // 0x01
  DR_CHARACTER_PEACH, // 0x02
  DR_CHARACTER_YOSHI, // 0x03
  DR_CHARACTER_WARIO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
};

static const dr_difficulty MP1_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00 TODO verify
  DR_DIFFICULTY_NORMAL, // 0x01 TODO verify
  DR_DIFFICULTY_HARD, // 0x02 TODO verify
};

static const dr_team_color MP1_PANEL_COLOR_TO_DR[] = {
  DR_TEAM_COLOR_INVALID, // 0x00
  DR_TEAM_COLOR_BLUE, // 0x01
  DR_TEAM_COLOR_RED, // 0x02
  DR_TEAM_COLOR_YELLOW, // 0x03
  DR_TEAM_COLOR_GREEN, // 0x04
};

static const size_t MP1_MINIGAME_TITLE_ADDRS[6] = {
  0xB0FDBE54, 0xB0FDBE92, 0xB0FDBED6, 0xB0FDBF1C, 0xB0FDBF5A,
  0xB0FDBF98, // sentinel — TODO: verify
};

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
  { 0x06, "Same Game" },
  { 0x07, "Hot Bob-omb" },
  { 0x08, "Yoshi no Shita Awase" },
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
  { 0x3b, "Mario's Rainbow Castle" },
  { 0x3e, "First Map" }, // "rules" map
  { 0x56, "Board event" },
  { 0x61, "Intro" },
  { 0x62, "Board intro" },
  { 0x66, "Booting up" },
  { 0x67, "Booting up" },
  { 0x69, "Mushroom Village" },
  { 0x6a, "Traveling the Warp Pipe" },
  { 0x6b, "Mini-Game House" },
  { 0x6c, "Mushroom Shop" },
  { 0x6d, "Mushroom Bank" },
  { 0x6e, "Option House" },
  { 0x6f, "Mini-Game Explanation" },
  { 0x71, "Mini-Game Island loading" },
  { 0x72, "Mini-Game Island" },
  { 0x78, "Mini-Game Island intro" },
  { 0x79, "Mini-Game Island save space" },
  { 0x7c, "Mini-Game results" },
  { 0x7d, "Mini-Game Island results" },
  { 0x81, "Title Screen" },

  { -1, nullptr },
};

static DrHostConfig makeConfig()
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party (USA).z64").toStdString();

  config.scene_miniexplain[0] = 0x6F; // TODO: verify, may have multiple
  config.scene_miniexplain_count = 1;
  config.scene_miniresults = 0x7C;
  // scene_miniresults_battle / scene_miniresults_duel: not available in mp1
  config.scene_addr = 0x800C596C; // u16

  config.scene_board_ranges[0] = { 0x36, 0x3D };
  config.scene_board_range_count = 1;

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

  config.char_to_dr = MP1_CHAR_TO_DR;
  config.char_to_dr_size = sizeof(MP1_CHAR_TO_DR) / sizeof(*MP1_CHAR_TO_DR);
  config.diff_to_dr = MP1_DIFF_TO_DR;
  config.diff_to_dr_size = sizeof(MP1_DIFF_TO_DR) / sizeof(*MP1_DIFF_TO_DR);

  config.panel_color_to_dr = MP1_PANEL_COLOR_TO_DR;
  config.panel_color_to_dr_size = sizeof(MP1_PANEL_COLOR_TO_DR) / sizeof(*MP1_PANEL_COLOR_TO_DR);

  config.minigame_type_addr = 0x800D6459; // u8
  config.minigame_type_to_dr = MP1_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size = 4;

  config.next_scene_addr = 0x800F09F4; // unused
  config.minigame_id_addr = 0x800ED5DE; // u16 (minigame_id_is_8bit == false)
  config.minigame_id_is_8bit = false;

  config.title_addrs = MP1_MINIGAME_TITLE_ADDRS;
  config.title_id_base = 0x01;
  config.title_id_step = 4;
  config.title_len_offset = 2;
  config.title_addr_transform = n64ByteAddr;

  config.scene_trampoline_addr = 0x800CA9A0;

  config.scene_names = MP1_SCENE_NAMES;

  return config;
}

MarioParty1Host::MarioParty1Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();

        // Scene transition trampoline: data cell at 0x800CA9A0, code at 0x800CA9A4
        m_core->cheatSet(0, true,
          // Trampoline at 0x800CA9A0
          "810CA9A4 3C08"  // LUI  T0, 0x800D
          "+810CA9A6 800D"
          "+810CA9A8 8D08"  // LW   T0, 0xA9A0(T0)   — T0 = *0x800CA9A0
          "+810CA9AA A9A0"
          "+810CA9AC 1100"  // BEQ  T0, ZERO, +7     — if 0, passthrough
          "+810CA9AE 0007"
          "+810CA9B0 0000"  // NOP                   — (delay slot)
          "+810CA9B2 0000"
          // Override path
          "+810CA9B4 0008"  // SRL  A0, T0, 16       — A0 = scene
          "+810CA9B6 2402"
          "+810CA9B8 3105"  // ANDI A1, T0, 0xFFFF   — A1 = modifier
          "+810CA9BA FFFF"
          "+810CA9BC 0080"  // ADDU S1, A0, ZERO     — save scene to S1
          "+810CA9BE 8821"
          "+810CA9C0 3C04"  // LUI  A0, 0x800F       — load game state pointer
          "+810CA9C2 800F"
          "+810CA9C4 0801"  // J    0x8005E05C       — return
          "+810CA9C6 7817"
          "+810CA9C8 0000"  // NOP                   — (delay slot)
          "+810CA9CA 0000"
          // Passthrough path
          "+810CA9CC 0080"  // ADDU S1, A0, ZERO     — save scene arg to S1
          "+810CA9CE 8821"
          "+810CA9D0 3C04"  // LUI  A0, 0x800F       — load game state pointer
          "+810CA9D2 800F"
          "+810CA9D4 0801"  // J    0x8005E05C       — return
          "+810CA9D6 7817"
          "+810CA9D8 0000"  // NOP                   — (delay slot)
          "+810CA9DA 0000"
          // Hook at 0x8005E054: J 0x800CA9A4 + NOP
          "+8105E054 0803"  // J    0x800CA9A4
          "+8105E056 2A69"
          "+8105E058 2400"  // NOP                   — (delay slot, displaces LUI A0, 0x800F)
        );

        // Force Mini-Game Roulette IDs
        m_core->cheatSet(1, true,
          "81043AB4 0010"  // SLL  V0, S0, 2      — roulette ID offset = slot_index * 4 (step=4)
          "+81043AB6 1080"
          "+81043AB8 2442"  // ADDIU V0, V0, BASE  — roulette ID = offset + base
          "+81043ABA 0001"
          "+81043B08 2400" // NOP
          "+81043B74 2400"  // NOP
        );
      }
    },
    Qt::DirectConnection);
}
