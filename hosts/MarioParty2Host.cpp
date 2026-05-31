#include "MarioParty2Host.h"

#include <QRetroDirectories.h>
#include <cstring>

/**
 * Starting at 0xB11570D9 is an unaligned struct of mini-game titles
 * in this format:
 * u8 : length of text 
 * u8 : unknown, always 0b
 * char array of specified size
 * we want to replace up to five mini-game names with our titles instead.
 * as a test, make a function that writes test names of maximum 0x0f characters
 * into this struct. put it in marioparty2host only
 */

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

static const size_t MP2_SLOT_ADDRS[5] = {
  0x800df6c7,
  0x800df6c3,
  0x800df6c2,
  0x800df6c1,
  0x800df6c0,
};

static DrHostConfig makeConfig()
{
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party 2 (USA).z64").toStdString(),

    .scene_miniexplain = { 0x5F, 0x60 },
    .scene_miniexplain_count = 2,
    .scene_miniresults = 0x70,
    .scene_miniresults_battle = 0x6f,
    .scene_addr = 0x800FA63C,

    .scene_board_ranges = {}, // TODO
    .scene_board_range_count = 0, // TODO
    .scene_duel_board_range = {}, // TODO

    .character_addr = { 0x800fd2c7, 0x800fd2fb, 0x800fd32f, 0x800fd363 },
    .controller_addr = { 0x800fd2c0, 0x800fd2f4, 0x800fd328, 0x800fd35c },
    .difficulty_addr = { 0x800fd2c1, 0x800fd2f5, 0x800fd329, 0x800fd35d },
    .team_addr = { 0x800fd2c3, 0x800fd2f7, 0x800fd32b, 0x800fd35f },
    .bot_addr = { 0x800fd2c4, 0x800fd2f8, 0x800fd32c, 0x800fd360 },
    .result_addr = { 0x800fd2ce, 0x800fd302, 0x800fd336, 0x800fd36a },
    .bonus_result_addr = { 0x800fd2c8, 0x800fd2fc, 0x800fd330, 0x800fd364 },
    .panel_color_addr = { 0x800fd2d8, 0x800fd30c, 0x800fd340, 0x800fd374 },

    .char_to_dr = MP2_CHAR_TO_DR,
    .char_to_dr_size = sizeof(MP2_CHAR_TO_DR) / sizeof(*MP2_CHAR_TO_DR),
    .diff_to_dr = MP2_DIFF_TO_DR,
    .diff_to_dr_size = sizeof(MP2_DIFF_TO_DR) / sizeof(*MP2_DIFF_TO_DR),

    .battle_addr = 0x800F920A,

    .panel_color_to_dr = MP2_PANEL_COLOR_TO_DR,
    .panel_color_to_dr_size = sizeof(MP2_PANEL_COLOR_TO_DR) / sizeof(*MP2_PANEL_COLOR_TO_DR),

    .minigame_type_addr = 0x800DF6C6,
    .minigame_type_to_dr = MP2_MINIGAME_TYPE_TO_DR,
    .minigame_type_to_dr_size = sizeof(MP2_MINIGAME_TYPE_TO_DR) / sizeof(*MP2_MINIGAME_TYPE_TO_DR),
    .next_scene_addr = 0,
    .minigame_id_addr = 0x800F93CA,
    .minigame_id_is_8bit = false,
    .minigame_blacklist = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },
    .minigame_blacklist_count = 6,
  };
}

MarioParty2Host::MarioParty2Host(QObject *parent)
  : DrHost(makeConfig(), parent)
{
  connect(
    m_core, &QRetro::frameEnd, this,
    [this, called = false]() mutable {
      if (!called)
      {
        called = true;
        m_core->cheatReset();
        m_core->cheatSet(0, true,
          "8104AF9C 2602"
          "+8104AF9E 0025"
          "+8104B020 2400");
        writeMinigameNames();
      }
    },
    Qt::DirectConnection);
}

static size_t n64ByteAddr(size_t addr)
{
  return (addr & ~size_t(3)) | (3 - (addr & 3));
}

void MarioParty2Host::writeMinigameNames()
{
  static const char *names[5] = {
    "ackman83",
    "little mac",
    "ranga time",
    "fuck keith",
    "reredeadpants",
  };

  size_t addr = 0xB1157363;
  for (unsigned i = 0; i < 5; i++)
  {
    uint8_t oldLen = 0;
    uint8_t marker = 0;
    if (readu8(&oldLen, n64ByteAddr(addr)) != DR_OK)
      return;
    if (readu8(&marker, n64ByteAddr(addr + 1)) != DR_OK)
      return;
    if (marker != 0x0B)
    {
      log(DR_LOG_WARN,
        qPrintable(QString("writeMinigameNames[%1]: expected 0x0B at +1, got 0x%2; backseek")
                     .arg(i)
                     .arg(marker, 2, 16, QChar('0'))));
      bool found = false;
      for (unsigned back = 1; back <= 8; back++)
      {
        addr--;
        if (readu8(&oldLen, n64ByteAddr(addr)) != DR_OK ||
            readu8(&marker, n64ByteAddr(addr + 1)) != DR_OK)
          return;
        if (marker == 0x0B)
        {
          found = true;
          break;
        }
      }
      if (!found)
        return;
    }

    uint8_t nameLen = (uint8_t)strnlen(names[i], oldLen > 0 ? oldLen - 1 : 0);
    for (uint8_t j = 0; j < nameLen; j++)
      writeu8((uint8_t)names[i][j], n64ByteAddr(addr + 2 + j));
    writeu8(0, n64ByteAddr(addr + 2 + nameLen));

    addr += 2 + oldLen;
  }
}
