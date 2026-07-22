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

// Hardware addresses (accessed wordflipped via MarioPartyN64Host). u8 fields unflip ^3.
static const size_t MP3_SLOT_ADDRS[5] = {
  0x80102C08, // slot 0
  0x80102C09, // slot 1
  0x80102C0A, // slot 2
  0x80102C0B, // slot 3
  0x80102C0C, // slot 4
};

// TABLE_BASE=0xB122D74A, HEADER=0x124 (0x48 entries), ENTRY=0x30
// Slots use odd entries (1,3,5,7,9), each spanning into the next entry for capacity
static const size_t MP3_MINIGAME_TITLE_ADDRS[6] = {
  0xB122D89F, // entry  1 len byte
  0xB122D8FF, // entry  3 len byte
  0xB122D95F, // entry  5 len byte
  0xB122D9BF, // entry  7 len byte
  0xB122DA1F, // entry  9 len byte
  0xB122DA7F, // entry 11 len byte (sentinel)
};

// 72 game guy results
/* duel results on duel map scene 0x73 */

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
  config.next_scene_addr = 0x800D2032; // u16, unused
  config.next_scene_modifier_addr = 0x800D2034; // u16, unused
  config.minigame_id_addr = 0x800cd068; // u8 (minigame_id_is_8bit == true)
  config.minigame_id_is_8bit = true;

  static const uint8_t blacklist[] = { 0x43, 0x44, 0x45 }; // ignore game guy
  memcpy(config.minigame_blacklist, blacklist, sizeof(blacklist));
  config.minigame_blacklist_count = 3;

  config.title_addrs = MP3_MINIGAME_TITLE_ADDRS;
  config.title_id_base = 2;
  config.title_id_step = 2;
  config.title_len_offset = 3;
  config.slot_addrs = MP3_SLOT_ADDRS;
  config.scene_trampoline_addr = 0x800A7A54;
  config.turn_total_addr = 0x800CD05Au; // u8
  config.turn_current_addr = 0x800CD05Bu; // u8
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

        // Hook decompressor at 0x8000B004: if A0 == 0x0122D74A (title table),
        // set A3 = 0 to skip decompression and use our patched copy in place
        m_core->cheatSet(0, true,
          // Hook: J 0x800A7A30 + NOP at 0x8000B004
          "8100B004 0802"
          "+8100B006 9E8C"
          "+8100B008 0000"
          "+8100B00A 0000"
          // Trampoline at 0x800A7A30
          "+810A7A30 AFBF"  // SW   RA, 32(SP)          — displaced original insn from 0x8000B004
          "+810A7A32 0020"
          "+810A7A34 AFA4"  // SW   A0, 20(SP)          — displaced original insn from 0x8000B008
          "+810A7A36 0014"
          "+810A7A38 3C01"  // LUI  AT, 0x0123          — AT = 0x01230000
          "+810A7A3A 0123"
          "+810A7A3C 2421"  // ADDIU AT, AT, -0x28B6    — AT = 0x0122D74A (title table ROM offset)
          "+810A7A3E D74A"
          "+810A7A40 1481"  // BNE  A0, AT, +2          — if A0 != title table, skip A3 = 0
          "+810A7A42 0002"
          "+810A7A44 0000"  // NOP                      — (delay slot)
          "+810A7A46 0000"
          "+810A7A48 0000"  // OR   A3, R0, R0          — A3 = 0: signal decompressor to skip
          "+810A7A4A 3825"
          "+810A7A4C 0800"  // J    0x8000B00C          — return past the hooked instructions
          "+810A7A4E 2C03"
          "+810A7A50 0000"  // NOP                      — (delay slot)
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
        m_core->cheatSet(3, true,
          "81035C00 2404"
          "+81035C02 00FF"
          "+D110AE18 1040"
          "+8111B75E 0041"
          "+D110AE18 1040"
          "+8011B761 0047");

        m_core->cheatSet(1, false, MP3_CHEAT_REGULAR_BOARD);
        m_core->cheatSet(2, false, MP3_CHEAT_DUEL_BOARD);

        // Write the minigame-title table to ROM at 0xB122D74A. Entries are indexed
        // by (minigame_id - 1); MarioParty3.cpp uses ids 0x01-0x48, so the table
        // needs 0x48 entries (strings 0x00-0x47).
        //   0x00-0x0F : 48-byte roulette slots (names injected at runtime)
        //   named     : item + special names, each at its (id - 1) string index
        //   all others: share one 6-byte filler entry
        static constexpr size_t   TABLE_BASE        = 0xB122D74A;
        static constexpr uint32_t ENTRY_COUNT       = 0x48; // strings 0x00-0x47
        static constexpr uint32_t FULL_COUNT        = 16;
        static constexpr uint32_t ENTRY_SIZE        = 48;
        static constexpr uint32_t SMALL_ENTRY_SIZE  = 6;  // single shared filler
        static constexpr uint32_t NAMED_ENTRY_SIZE  = 24; // longest name is 20 chars + 3
        static constexpr uint32_t HEADER_SIZE       = 4 + ENTRY_COUNT * 4;
        static constexpr uint32_t FULL_AREA_SIZE    = FULL_COUNT * ENTRY_SIZE;
        static constexpr uint32_t FILLER_OFFSET     = HEADER_SIZE + FULL_AREA_SIZE;
        static constexpr uint32_t NAMED_AREA_OFFSET = FULL_AREA_SIZE + SMALL_ENTRY_SIZE;

        // Static (non-roulette) names, placed at string index = minigame_id - 1.
        struct NamedEntry { uint32_t index; const char *name; };
        static const NamedEntry NAMED[] = {
          { 0x3A, "Winner's Wheel" },       // id 0x3B
          { 0x3B, "Hey, Batter, Batter!" }, // id 0x3C
          { 0x3C, "Bobbing Bow-loons" },    // id 0x3D
          { 0x3D, "Dorrie Dip" },           // id 0x3E
          { 0x3E, "Swinging with Sharks" }, // id 0x3F
          { 0x3F, "Swing 'n' Swipe" },      // id 0x40
          { 0x41, "Stardust Battle" },      // id 0x42
        };
        static constexpr uint32_t NAMED_COUNT = sizeof(NAMED) / sizeof(*NAMED);

        auto xw = [this](uint8_t val, size_t addr) {
          writeu8(val, addr);
        };
        auto xw32 = [&xw](uint32_t val, size_t addr) {
          xw(uint8_t(val >> 24), addr);
          xw(uint8_t(val >> 16), addr + 1);
          xw(uint8_t(val >>  8), addr + 2);
          xw(uint8_t(val      ), addr + 3);
        };

        xw32(ENTRY_COUNT, TABLE_BASE);
        for (uint32_t i = 0; i < FULL_COUNT; i++)
          xw32(HEADER_SIZE + i * ENTRY_SIZE, TABLE_BASE + 4 + i * 4);
        for (uint32_t i = FULL_COUNT; i < ENTRY_COUNT; i++)
          xw32(FILLER_OFFSET, TABLE_BASE + 4 + i * 4); // default: shared filler
        for (uint32_t n = 0; n < NAMED_COUNT; n++)
          xw32(HEADER_SIZE + NAMED_AREA_OFFSET + n * NAMED_ENTRY_SIZE,
               TABLE_BASE + 4 + NAMED[n].index * 4);

        for (uint32_t i = 0; i < FULL_COUNT; i++)
        {
          size_t base = TABLE_BASE + HEADER_SIZE + i * ENTRY_SIZE;
          xw(0x00, base);     // alignment
          xw(0x03, base + 1); // len (empty: strlen 0 + offset 3)
          xw(0x0B, base + 2); // marker
          for (uint32_t j = 3; j < ENTRY_SIZE; j++)
            xw(0x00, base + j);
        }

        {
          // One filler entry shared by every unnamed id.
          size_t base = TABLE_BASE + FILLER_OFFSET;
          xw(0x00, base);     // alignment
          xw(0x04, base + 1); // len (offset 3 + strlen 1)
          xw(0x0B, base + 2); // marker
          xw('A',  base + 3);
          xw(0x00, base + 4);
          xw(0x00, base + 5);
        }

        for (uint32_t n = 0; n < NAMED_COUNT; n++)
        {
          size_t base = TABLE_BASE + HEADER_SIZE + NAMED_AREA_OFFSET + n * NAMED_ENTRY_SIZE;
          const char *name = NAMED[n].name;
          uint8_t encoded[32] = {};
          uint8_t nameLen = 0;
          for (size_t k = 0, srcLen = strlen(name); k < srcLen && nameLen < NAMED_ENTRY_SIZE - 3; k++)
          {
            char c = name[k];
            uint8_t enc;
            if (isalpha((unsigned char)c) || isdigit((unsigned char)c) || c == ' ')
              enc = (uint8_t)c;
            else if (c == '\'') enc = 0x5C;
            else if (c == '-')  enc = 0x3D;
            else if (c == ',')  enc = 0x82;
            else if (c == '!')  enc = 0xC2;
            else continue;
            encoded[nameLen++] = enc;
          }
          xw(0x00, base);
          xw(nameLen + 3, base + 1); // len = offset 3 + strlen
          xw(0x0B, base + 2);        // marker
          for (uint32_t j = 0; j < NAMED_ENTRY_SIZE - 3; j++)
            xw(j < nameLen ? encoded[j] : 0, base + 3 + j);
        }
      }
    },
    Qt::DirectConnection);
}

