#include "WiiParty.h"
#include "DrNetplay.h"
#include "DrPaths.h"

#include <QRetro.h>
#include <QStringList>

static const dr_value_t WP_MINIGAME_ID = { 0x80235D8C, DR_VALUE_TYPE_S32 };

/// s32 - Game mode; mini-games check it to decide what, if anything, to write to the
/// result field. Free play (menu.rel) sets it from the mini-game id via 0x80007960.
static const dr_value_t WP_MODE = { 0x80235D90, DR_VALUE_TYPE_S32 };

/// 4P outside free play: win flags
static const int32_t WP_MODE_4P = 0;
/// Pair outside free play, and some pair games in free play: pass/fail flag
static const int32_t WP_MODE_PAIR = 1;
/// 1v3 in free play. 1v3 games write win flags in every mode.
static const int32_t WP_MODE_FREE_1V3 = 2;
/// Duel in free play: win flags. Some duels write no result in any other mode.
static const int32_t WP_MODE_FREE_DUEL = 3;
/// 4P in free play: 0-based placements
static const int32_t WP_MODE_FREE_4P = 5;
/// Pair in free play: grade 0-3. Pair games write 0 in modes other than 1 and 6.
static const int32_t WP_MODE_FREE_PAIR = 6;

/// Pair games free play still runs in WP_MODE_PAIR (list at 0x80208078)
static const int32_t WP_PAIR_FLAG_MINIGAMES[] = { 204, 210, 211, 216, 218, 222, 223 };

/// u8 - Number of players taking part; slots past it sit out
static const dr_value_t WP_PLAYER_COUNT = { 0x80235D94, DR_VALUE_TYPE_U8 };

/// Player table: 0x30-byte entries, players 0-3 (slots 4-7 unused)
static const size_t WP_PLAYERS_ADDR = 0x80231980;
static const size_t WP_PLAYER_SIZE = 0x30;

/// s32 - Wii Remote channel driving this player
static const size_t WP_PLAYER_CONTROLLER = 0x00;
/// s32 - Team; -1 = not playing. 4P: player index, 1v3: 0 = solo / 1 = group, 2v2/1v1: 0 or 1
static const size_t WP_PLAYER_TEAM = 0x04;
/// s32 - CPU strength 0-99. The explanation screen rewrites it for CPUs from the
/// global setting (x20) right before loading the mini-game.
static const size_t WP_PLAYER_CPU_LEVEL = 0x08;
/// u8 - 1 if a CPU
static const size_t WP_PLAYER_IS_CPU = 0x0C;
/// s16 - Mii source; 0 = system Mii database
static const size_t WP_PLAYER_MII_TYPE = 0x10;
/// s16 - Mii index within its source
static const size_t WP_PLAYER_MII_INDEX = 0x12;
/// s32 - Mini-game result: 0 = lost, 1 = won; pair games grade 0-4. 4P games
/// outside mode 0 write placement instead (0 = 1st).
static const size_t WP_PLAYER_RESULT = 0x28;

/// Head of the OS loaded-module (REL) list; each OSModuleInfo starts with its id
/// followed by the next link.
static const dr_value_t WP_OS_MODULE_LIST_HEAD = { 0x800030C8, DR_VALUE_TYPE_POINTER };

/// Give up walking the module list after this many links
static const unsigned WP_MODULE_LIST_MAX = 32;

/// OSModuleInfo: u32 - section count, pointer - section table ({ addr | exec bit, size }), u32 - bss size
static const size_t WP_MODULE_NUM_SECTIONS = 0x0C;
static const size_t WP_MODULE_SECTION_INFO = 0x10;
static const size_t WP_MODULE_BSS_SIZE = 0x20;

/// Skip module sections bigger than this when looking for the color table
static const unsigned WP_MODULE_SECTION_MAX = 0x100000;

/// u32[8][2] - RGBA { bright, dark } per player; drives rings, pointers and other highlights
static const size_t WP_PLAYER_COLORS_ADDR = 0x801DE148;

/// u32[8] - Each mini-game REL keeps its own copy of the bright colors for Mii outfits.
/// We match the first four to find it.
static const uint32_t WP_MODULE_COLORS[4] = { 0x008CFFFF, 0xFF3838FF, 0x10BD0DFF, 0xFF9C00FF };

/// The explanation screen (inst.rel) builds start/practice/back, with B bound to back, only in
/// freeplay. Patching its `li r30, 1` to `li r30, 0` makes it build the set without back.
/// The boot state has menu.rel at the same address, so only write once inst is there.
static const dr_value_t WP_INST_MODULE = { 0x803388A0, DR_VALUE_TYPE_U32 };
static const int64_t WP_INST_MODULE_ID = 0x6A;
static const dr_value_t WP_BACK_BUTTON = { 0x8034481C, DR_VALUE_TYPE_U32 };
static const int64_t WP_BACK_BUTTON_VALUE_OLD = 0x3BC00001;
static const int64_t WP_BACK_BUTTON_VALUE_NEW = 0x3BC00000;

static dr_value_t wp_player_value(unsigned slot, size_t offset, dr_value_type type)
{
  return { WP_PLAYERS_ADDR + slot * WP_PLAYER_SIZE + offset, type };
}

/// RGBA from a character's color; dark matches the game's shades at 60%
static uint32_t wp_rgba(dr_character character, bool dark)
{
  const dr_color_t color = dr_character_color(character);
  const unsigned scale = dark ? 6 : 10;

  return ((color.red * scale / 10) << 24) | ((color.green * scale / 10) << 16) |
    ((color.blue * scale / 10) << 8) | 0xFF;
}

/// The mode free play would set for this mini-game (0x80007960 with the free play scene)
static int32_t wp_mode(const dr_mp_minigame_t *minigame)
{
  unsigned i;

  switch (minigame->type)
  {
  case DR_MINIGAME_DUEL:
    return WP_MODE_FREE_DUEL;
  case DR_MINIGAME_PAIR:
    for (i = 0; i < sizeof(WP_PAIR_FLAG_MINIGAMES) / sizeof(WP_PAIR_FLAG_MINIGAMES[0]); i++)
      if (minigame->minigame_id == WP_PAIR_FLAG_MINIGAMES[i])
        return WP_MODE_PAIR;
    return WP_MODE_FREE_PAIR;
  case DR_MINIGAME_1V3:
    return WP_MODE_FREE_1V3;
  default:
    return WP_MODE_FREE_4P;
  }
}

static int32_t wp_cpu_level(dr_difficulty difficulty)
{
  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
    return 0;
  case DR_DIFFICULTY_EASY:
    return 20;
  case DR_DIFFICULTY_NORMAL:
    return 40;
  case DR_DIFFICULTY_HARD:
    return 60;
  case DR_DIFFICULTY_VERY_HARD:
    return 80;
  default:
    return 40;
  }
}

static const dr_mp_minigame_t WP_MINIGAMES[] =
{
  { "Commuter Count", DR_MINIGAME_DUEL, 100, 22, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Timber Topple", DR_MINIGAME_DUEL, 101, 23, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Delivery Duel", DR_MINIGAME_DUEL, 102, 24, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Roll to the Goal", DR_MINIGAME_DUEL, 104, 26, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Channel Changers", DR_MINIGAME_DUEL, 106, 27, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Spring Time", DR_MINIGAME_DUEL, 107, 28, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Jangle Wranglers", DR_MINIGAME_DUEL, 108, 29, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Poppin' Pilots", DR_MINIGAME_DUEL, 109, 30, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Flag Footrace", DR_MINIGAME_DUEL, 110, 68, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Pop-Up Bandits", DR_MINIGAME_DUEL, 111, 74, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Fruit Focus", DR_MINIGAME_DUEL, 112, 31, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },

  //{ "Flycycle Team", DR_MINIGAME_PAIR, 200, 32, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS }, difficult to do with emulated motion controls
  { "Fishin' Buddies", DR_MINIGAME_PAIR, 201, 33, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Bumper Crops", DR_MINIGAME_PAIR, 202, 34, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Robot Factory", DR_MINIGAME_PAIR, 203, 35, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Flashlight Frights", DR_MINIGAME_PAIR, 204, 36, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Basket Bonanza", DR_MINIGAME_PAIR, 205, 37, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Meet and Greet", DR_MINIGAME_PAIR, 206, 38, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Paddle Pals", DR_MINIGAME_PAIR, 208, 39, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Pop Coaster", DR_MINIGAME_PAIR, 209, 40, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Tippy Traverse", DR_MINIGAME_PAIR, 210, 41, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Pump Cart Panic", DR_MINIGAME_PAIR, 211, 42, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Sheep Crossing", DR_MINIGAME_PAIR, 212, 43, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Banana Blockade", DR_MINIGAME_PAIR, 213, 44, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Spring Ringers", DR_MINIGAME_PAIR, 214, 45, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Can Poppers", DR_MINIGAME_PAIR, 215, 66, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Clover Hunt", DR_MINIGAME_PAIR, 216, 46, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Rodent Rundown", DR_MINIGAME_PAIR, 218, 47, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  //{ "Jump Rope Jam", DR_MINIGAME_PAIR, 219, 48, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS }, difficult to do with emulated motion controls
  { "Bobsled Highway", DR_MINIGAME_PAIR, 221, 49, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Color Coordination", DR_MINIGAME_PAIR, 222, 50, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Teamwork Temple", DR_MINIGAME_PAIR, 223, 51, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Blastronauts", DR_MINIGAME_PAIR, 224, 52, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },

  { "Flying Fruit", DR_MINIGAME_1V3, 300, 53, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Lumber Whacks", DR_MINIGAME_1V3, 301, 54, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Splash Bash", DR_MINIGAME_1V3, 302, 55, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Poserobics", DR_MINIGAME_1V3, 303, 56, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Hide-and-Peek", DR_MINIGAME_1V3, 304, 57, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },

  { "Derby Dash", DR_MINIGAME_4P, 400, 58, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Smile Snap", DR_MINIGAME_4P, 401, 59, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Chop Chops", DR_MINIGAME_4P, 402, 60, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Walk-Off", DR_MINIGAME_4P, 403, 61, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shifty Gifts", DR_MINIGAME_4P, 404, 62, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Flap Hurdles", DR_MINIGAME_4P, 405, 63, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Jumbo Jump", DR_MINIGAME_4P, 406, 64, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Moon Landing", DR_MINIGAME_4P, 407, 65, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Popgun Posse", DR_MINIGAME_4P, 408, 66, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Friendly Face-Off", DR_MINIGAME_4P, 410, 67, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY }, // needs pointer + motion tilt controls
  { "Flag Fracas", DR_MINIGAME_4P, 411, 68, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Shutterpup", DR_MINIGAME_4P, 412, 69, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Lucky Launch", DR_MINIGAME_4P, 413, 70, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Saucer Snap", DR_MINIGAME_4P, 414, 71, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Maze Daze", DR_MINIGAME_4P, 415, 72, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  //{ "Quicker Chipper", DR_MINIGAME_4P, 416, 73, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS }, difficult to do with emulated motion controls
  { "Barrel Daredevil", DR_MINIGAME_4P, 418, 25, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Face Flip", DR_MINIGAME_4P, 419, 75, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Goal Getters", DR_MINIGAME_4P, 420, 76, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS },
  { "Stop Watchers", DR_MINIGAME_4P, 421, 77, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Chopper Hoppers", DR_MINIGAME_4P, 422, 78, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Puzzle Pick-Up", DR_MINIGAME_4P, 423, 79, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Feathered Frenzy", DR_MINIGAME_4P, 424, 80, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Follow Your Face", DR_MINIGAME_4P, 426, 81, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Strategy Steps", DR_MINIGAME_4P, 427, 82, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_NO_FLAGS }, // uses dpad of an upright Wiimote
  { "Tropical Punch", DR_MINIGAME_4P, 428, 83, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Zombie Tag", DR_MINIGAME_4P, 430, 84, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Pearl Plunder", DR_MINIGAME_4P, 431, 85, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Hammer Heads", DR_MINIGAME_4P, 432, 86, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Crash Balls", DR_MINIGAME_4P, 433, 87, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_MOTION), DR_NO_FLAGS },
  { "Rope Sling", DR_MINIGAME_4P, 434, 88, DR_WII_CONTROL(DR_WII_CONTROL_UPRIGHT), DR_NO_FLAGS },
  { "Space Brawl", DR_MINIGAME_4P, 436, 89, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Dicey Descent", DR_MINIGAME_4P, 437, 90, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Cosmic Capers", DR_MINIGAME_4P, 438, 91, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  //{ "Cry Babies", DR_MINIGAME_4P, 439, 92, DR_WII_CONTROL(DR_WII_CONTROL_WAGGLE), DR_NO_FLAGS }, bad one with odd motion controls
  { "Ram Jam", DR_MINIGAME_4P, 440, 93, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Spotlight Fight", DR_MINIGAME_4P, 441, 94, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Chin-Up Champ", DR_MINIGAME_4P, 442, 95, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_NO_FLAGS }, // uses A+B buttons of an upright Wiimote
  { "Risky Railroad", DR_MINIGAME_4P, 443, 96, DR_WII_CONTROL(DR_WII_CONTROL_POINTER), DR_FLAG_NO_NETPLAY },
  { "Balloon Buggies", DR_MINIGAME_4P, 445, 97, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
  { "Back Attack", DR_MINIGAME_4P, 446, 98, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

WiiParty::WiiParty(QObject *parent)
  : DrGuestWii("wii-wp", (DrPaths::instance().romsDirectory() + "/Wii Party (USA) (En,Fr,Es)").toStdString(), parent)
{
}

const dr_mp_minigame_t *WiiParty::minigames() const
{
  return WP_MINIGAMES;
}

size_t WiiParty::moduleAddress(unsigned module_id)
{
  int64_t node = 0;
  unsigned i;

  if (m_retro->readValue(&node, WP_OS_MODULE_LIST_HEAD) != DR_OK)
    return 0;

  for (i = 0; i < WP_MODULE_LIST_MAX && node; i++)
  {
    int64_t id = 0;

    if (m_retro->readValue(&id, { static_cast<size_t>(node), DR_VALUE_TYPE_U32 }) != DR_OK)
      return 0;
    if (id == static_cast<int64_t>(module_id))
      return static_cast<size_t>(node);
    if (m_retro->readValue(&node, { static_cast<size_t>(node) + 4, DR_VALUE_TYPE_POINTER }) != DR_OK)
      return 0;
  }

  return 0;
}

void WiiParty::writeColors(void)
{
  unsigned i;

  for (i = 0; i < 4; i++)
  {
    const unsigned slot = m_slotOf[i];
    const size_t entry = WP_PLAYER_COLORS_ADDR + slot * 8;

    m_retro->writeValue(wp_rgba(m_players[i].character, false), { entry, DR_VALUE_TYPE_U32 });
    m_retro->writeValue(wp_rgba(m_players[i].character, true), { entry + 4, DR_VALUE_TYPE_U32 });
  }
}

void WiiParty::writeModuleColors(size_t module)
{
  int64_t sections = 0;
  int64_t table = 0;
  int64_t bss_size = 0;
  unsigned found = 0;
  unsigned i;

  if (m_retro->readValue(&sections, { module + WP_MODULE_NUM_SECTIONS, DR_VALUE_TYPE_U32 }) != DR_OK ||
      m_retro->readValue(&table, { module + WP_MODULE_SECTION_INFO, DR_VALUE_TYPE_POINTER }) != DR_OK ||
      m_retro->readValue(&bss_size, { module + WP_MODULE_BSS_SIZE, DR_VALUE_TYPE_U32 }) != DR_OK)
    return;

  for (i = 0; i < static_cast<unsigned>(sections) && i < 32; i++)
  {
    int64_t addr = 0;
    int64_t size = 0;
    size_t pos;

    if (m_retro->readValue(&addr, { static_cast<size_t>(table) + i * 8, DR_VALUE_TYPE_U32 }) != DR_OK ||
        m_retro->readValue(&size, { static_cast<size_t>(table) + i * 8 + 4, DR_VALUE_TYPE_U32 }) != DR_OK)
      return;

    /* Data sections only: skip empty, code (low bit set) and bss */
    if (!addr || !size || (addr & 1) || size == bss_size || size > WP_MODULE_SECTION_MAX)
      continue;

    for (pos = 0; pos + 16 <= static_cast<size_t>(size); pos += 4)
    {
      const size_t at = static_cast<size_t>(addr) + pos;
      unsigned j;

      for (j = 0; j < 4; j++)
      {
        int64_t value = 0;

        if (m_retro->readValue(&value, { at + j * 4, DR_VALUE_TYPE_U32 }) != DR_OK ||
            static_cast<uint32_t>(value) != WP_MODULE_COLORS[j])
          break;
      }
      if (j < 4)
        continue;

      for (j = 0; j < 4; j++)
        m_retro->writeValue(wp_rgba(m_players[j].character, false), { at + m_slotOf[j] * 4, DR_VALUE_TYPE_U32 });
      found++;
      pos += 12;
    }
  }

  log(DR_LOG_INFO, qPrintable(QString("recolored %1 module color table(s)").arg(found)));
}

void WiiParty::assignSlots(void)
{
  const dr_minigame_type type = m_minigame ? m_minigame->type : DR_MINIGAME_4P;
  bool taken[4] = { false, false, false, false };
  int32_t wanted[4] = { -1, -1, -1, -1 };
  int32_t next_duelist = 0;
  int32_t next_partner = 0;
  unsigned i;

  for (i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];

    m_slotOf[i] = p.control_port;
    switch (type)
    {
    case DR_MINIGAME_1V3:
      m_teamOf[i] = p.team_type == DR_TEAM_TYPE_1V3_GROUP ? 1 : 0;
      break;
    case DR_MINIGAME_2V2:
      m_teamOf[i] = static_cast<int32_t>(p.team_id);
      break;
    case DR_MINIGAME_DUEL:
      if (p.team_type == DR_TEAM_TYPE_DUEL_INITIATOR)
        m_teamOf[i] = 0;
      else if (p.team_type == DR_TEAM_TYPE_DUEL_TARGET)
        m_teamOf[i] = 1;
      else if (dr_team_type_participates(p.team_type) && next_duelist < 2)
        m_teamOf[i] = next_duelist++;
      else
        m_teamOf[i] = -1;
      wanted[i] = m_teamOf[i];
      break;
    case DR_MINIGAME_PAIR:
      if (dr_team_type_participates(p.team_type) && next_partner < 2)
      {
        m_teamOf[i] = 0;
        wanted[i] = next_partner++;
      }
      else
        m_teamOf[i] = -1;
      break;
    default:
      m_teamOf[i] = static_cast<int32_t>(m_slotOf[i]);
      break;
    }
  }

  if (type != DR_MINIGAME_DUEL && type != DR_MINIGAME_PAIR)
    return;

  /* Duelists or partners take slots 0 and 1, the players sitting out fill the rest */
  for (i = 0; i < 4; i++)
    if (wanted[i] >= 0 && !taken[wanted[i]])
    {
      m_slotOf[i] = static_cast<unsigned>(wanted[i]);
      taken[m_slotOf[i]] = true;
    }
    else
      m_slotOf[i] = 4;
  for (i = 0; i < 4; i++)
    if (m_slotOf[i] == 4)
    {
      unsigned slot = 0;

      while (taken[slot])
        slot++;
      m_slotOf[i] = slot;
      taken[slot] = true;
    }
}

void WiiParty::writePlayers(void)
{
  const bool two_player = m_minigame &&
    (m_minigame->type == DR_MINIGAME_DUEL || m_minigame->type == DR_MINIGAME_PAIR);
  unsigned i;

  m_retro->writeValue(two_player ? 2 : 4, WP_PLAYER_COUNT);

  for (i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    const unsigned slot = m_slotOf[i];
    const unsigned port = p.control_port;

    m_retro->writeValue(port < 4 ? port : slot,
      wp_player_value(slot, WP_PLAYER_CONTROLLER, DR_VALUE_TYPE_S32));
    m_retro->writeValue(m_teamOf[i], wp_player_value(slot, WP_PLAYER_TEAM, DR_VALUE_TYPE_S32));
    m_retro->writeValue(p.control_type == DR_CONTROL_TYPE_CPU ? 1 : 0,
      wp_player_value(slot, WP_PLAYER_IS_CPU, DR_VALUE_TYPE_U8));
    m_retro->writeValue(wp_cpu_level(p.difficulty),
      wp_player_value(slot, WP_PLAYER_CPU_LEVEL, DR_VALUE_TYPE_S32));
    m_retro->writeValue(0, wp_player_value(slot, WP_PLAYER_MII_TYPE, DR_VALUE_TYPE_S16));
    m_retro->writeValue(static_cast<int64_t>(p.character),
      wp_player_value(slot, WP_PLAYER_MII_INDEX, DR_VALUE_TYPE_S16));
  }
}

void WiiParty::logResults(void)
{
  QStringList results;
  int64_t mode = 0;
  unsigned i;

  for (i = 0; i < 4; i++)
  {
    int64_t team = 0;
    int64_t result = 0;

    m_retro->readValue(&team, wp_player_value(i, WP_PLAYER_TEAM, DR_VALUE_TYPE_S32));
    m_retro->readValue(&result, wp_player_value(i, WP_PLAYER_RESULT, DR_VALUE_TYPE_S32));
    results << QString("slot %1 team %2 result %3").arg(i).arg(team).arg(result);
  }
  m_retro->readValue(&mode, WP_MODE);
  log(DR_LOG_INFO, qPrintable(QString("mini-game results (mode %1): %2")
    .arg(mode).arg(results.join(", "))));
}

void WiiParty::recolorNewModules(bool recolor)
{
  QSet<unsigned> loaded;
  int64_t node = 0;
  unsigned i;

  if (m_retro->readValue(&node, WP_OS_MODULE_LIST_HEAD) != DR_OK)
    return;

  for (i = 0; i < WP_MODULE_LIST_MAX && node; i++)
  {
    int64_t id = 0;

    if (m_retro->readValue(&id, { static_cast<size_t>(node), DR_VALUE_TYPE_U32 }) != DR_OK)
      return;
    loaded.insert(static_cast<unsigned>(id));
    if (!m_recoloredModules.contains(static_cast<unsigned>(id)))
    {
      m_recoloredModules.insert(static_cast<unsigned>(id));
      if (recolor)
      {
        log(DR_LOG_INFO, qPrintable(QString("module %1 loaded at frame %2")
          .arg(id).arg(m_minigameFrames)));
        writeModuleColors(static_cast<size_t>(node));
      }
    }
    if (m_retro->readValue(&node, { static_cast<size_t>(node) + 4, DR_VALUE_TYPE_POINTER }) != DR_OK)
      return;
  }

  /* Forget unloaded modules; inst.rel unloads for the mini-game and reloads stock for the results */
  m_recoloredModules.intersect(loaded);
}

void WiiParty::disableBackButton(void)
{
  int64_t id = 0, instruction = 0;

  if (m_retro->readValue(&id, WP_INST_MODULE) == DR_OK && id == WP_INST_MODULE_ID &&
      m_retro->readValue(&instruction, WP_BACK_BUTTON) == DR_OK &&
      instruction == WP_BACK_BUTTON_VALUE_OLD)
    m_retro->writeValue(WP_BACK_BUTTON_VALUE_NEW, WP_BACK_BUTTON);
}

void WiiParty::run()
{
  /* Nothing to drive until the state is loaded */
  if (m_retro->booting())
    return;

  disableBackButton();

  if (!m_minigameActive)
  {
    if (++m_minigameFrames >= 48)
    {
      m_minigameFrames = 0;
      startMinigame();
    }
    return;
  }

  m_minigameFrames++;
  recolorNewModules(true);

  /* Resync peers */
  if (m_resyncCountdown > 0 && --m_resyncCountdown == 0 && DrNetplay::sessionServer())
    emit hardResync();

  if (!m_minigame || m_finishScheduled)
    return;

  if (!m_moduleSeen)
  {
    writePlayers();

    const size_t module = moduleAddress(static_cast<unsigned>(m_minigame->scene_id));

    if (module)
    {
      m_moduleSeen = true;
      log(DR_LOG_INFO, qPrintable(QString("mini-game module %1 loaded at frame %2")
        .arg(m_minigame->scene_id).arg(m_minigameFrames)));
      applyControlRemap(m_minigame->quirks, m_players);
      if (DrNetplay::sessionActive())
        m_resyncCountdown = 30;
    }
  }
  else if (!moduleAddress(static_cast<unsigned>(m_minigame->scene_id)))
  {
    /* The mini-game unloads once results are in; let the results screen show */
    logResults();
    m_finishScheduled = true;
    finishMinigameInFrames(540);
  }
}

void WiiParty::doApplyGameData(const DrGameData &data)
{
  unsigned i;

  m_retro->loadState(state());

  m_minigameFrames = 0;
  m_finishScheduled = false;
  m_resyncCountdown = 0;
  m_moduleSeen = false;
  m_recoloredModules.clear();
  recolorNewModules(false);

  m_retro->writeValue(data.minigame->minigame_id, WP_MINIGAME_ID);
  m_retro->writeValue(wp_mode(data.minigame), WP_MODE);
  assignSlots();
  writePlayers();
  writeColors();
  applyControlProfile(DR_WII_CONTROL_POINTER);

  for (i = 0; i < 4; i++)
  {
    int64_t controller = -1;

    m_retro->readValue(&controller, wp_player_value(i, WP_PLAYER_CONTROLLER, DR_VALUE_TYPE_S32));
    log(DR_LOG_INFO, qPrintable(QString("slot %1 controller %2").arg(i).arg(controller)));
  }
}

dr_minigame_result_t WiiParty::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t mode = 0;
  int64_t mine = 0;
  unsigned i;

  if (index >= 4)
    return result;

  if (m_retro->readValue(&mine, wp_player_value(m_slotOf[index],
        WP_PLAYER_RESULT, DR_VALUE_TYPE_S32)) != DR_OK)
    return result;

  m_retro->readValue(&mode, WP_MODE);

  if (m_minigame && m_minigame->type == DR_MINIGAME_4P && mode == WP_MODE_FREE_4P)
  {
    /* Placements: everyone in 1st wins */
    if (mine == 0)
      result.coins = 10;
  }
  else
  {
    int64_t best = 0;

    /* Win flags, pass/fail or pair grades: the highest result wins */
    for (i = 0; i < 4; i++)
    {
      int64_t value = 0;

      if (m_retro->readValue(&value, wp_player_value(i, WP_PLAYER_RESULT, DR_VALUE_TYPE_S32)) == DR_OK &&
          value > best)
        best = value;
    }

    if (best > 0 && mine == best)
      result.coins = 10;
  }

  log(DR_LOG_INFO, qPrintable(QString("player %1 result %2 (mode %3)").arg(index).arg(mine).arg(mode)));

  return result;
}
