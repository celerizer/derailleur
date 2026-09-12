#include "MarioParty3Host.h"

#include <cstring>

#include <asm/mp3.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QUuid>
#include <QtEndian>

/**
 * Mupen64Plus-Next compiles its ROM database into the library rather than reading
 * system/Mupen64plus/mupen64plus.ini, so a modified Mario Party 3 is an unknown
 * ROM and boots with the wrong save type. Overwrite a spare Mario Party 3 entry
 * in a temp copy of the core with this ROM's md5 and crcs, so it inherits the
 * retail settings the entry already refers to. Both fields are replaced in place
 * at the same length, so nothing in the library shifts.
 */
static const char MP3_DB_MD5[] = "BDD79F498F37D01B8958F56EC6FFA097";
static const char MP3_DB_CRC[] = "65DB63E3 64357A65";

/// Temp core built for the running custom ROM, removed once the core has opened it.
static QString &mp3PatchedCore(void)
{
  static QString path;

  return path;
}

/// What the patch attempt did, logged once the host's log signal is connected.
static QString &mp3PatchStatus(void)
{
  static QString status;

  return status;
}

static QString writePatchedMupenCore(const QString &corePath, const QString &romPath)
{
  const int md5Len = sizeof(MP3_DB_MD5) - 1;
  const int crcLen = sizeof(MP3_DB_CRC) - 1;
  QFile rom(romPath);
  QByteArray header;
  QByteArray md5;

  if (!rom.open(QIODevice::ReadOnly))
  {
    mp3PatchStatus() = QString("could not read %1").arg(romPath);
    return QString();
  }
  header = rom.read(0x18).mid(0x10);
  rom.seek(0);
  md5 = QCryptographicHash::hash(rom.readAll(), QCryptographicHash::Md5).toHex().toUpper();
  rom.close();

  if (header.size() < 8 || md5.size() != md5Len)
  {
    mp3PatchStatus() = QString("%1 is too short to be a rom").arg(romPath);
    return QString();
  }

  const QByteArray crc = QString("%1 %2")
                           .arg(qFromBigEndian<quint32>(header.constData()), 8, 16, QChar('0'))
                           .arg(qFromBigEndian<quint32>(header.constData() + 4), 8, 16, QChar('0'))
                           .toUpper()
                           .toLatin1();

  if (crc.size() != crcLen)
  {
    mp3PatchStatus() = "computed crc was the wrong width";
    return QString();
  }

  QFile core(corePath);
  QByteArray data;

  if (!core.open(QIODevice::ReadOnly))
  {
    mp3PatchStatus() = QString("could not read core %1").arg(corePath);
    return QString();
  }
  data = core.readAll();
  core.close();

  const int md5Idx = data.indexOf(MP3_DB_MD5);

  if (md5Idx < 0)
  {
    mp3PatchStatus() = "core has no Mario Party 3 (U) [f1] database entry to borrow";
    return QString();
  }

  /* The crc belongs to the same entry, a fixed distance below the md5. */
  const int crcIdx = data.indexOf(MP3_DB_CRC, md5Idx);

  if (crcIdx < 0 || crcIdx - md5Idx > 128)
  {
    mp3PatchStatus() = "database entry found but its crc field was not where expected";
    return QString();
  }

  data.replace(md5Idx, md5Len, md5);
  data.replace(crcIdx, crcLen, crc);

  const QString destPath = QString("%1/mupen64plus_next_%2.%3")
                             .arg(QDir::tempPath(), QUuid::createUuid().toString(QUuid::Id128),
                               QFileInfo(corePath).suffix());
  QFile dest(destPath);

  if (!dest.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    mp3PatchStatus() = QString("could not write patched core to %1").arg(destPath);
    return QString();
  }
  dest.write(data);
  dest.close();

  mp3PatchStatus() =
    QString("patched core database entry to md5 %1, crc %2").arg(QString(md5), QString(crc));

  return destPath;
}

/* Native character id -> dr_character */
static dr_character mp3_char_to_dr(unsigned chr)
{
  switch (chr)
  {
  case 0x00: return DR_CHARACTER_MARIO;
  case 0x01: return DR_CHARACTER_LUIGI;
  case 0x02: return DR_CHARACTER_PEACH;
  case 0x03: return DR_CHARACTER_YOSHI;
  case 0x04: return DR_CHARACTER_WARIO;
  case 0x05: return DR_CHARACTER_DONKEY_KONG;
  case 0x06: return DR_CHARACTER_WALUIGI;
  case 0x07: return DR_CHARACTER_DAISY;
  }

  return DR_CHARACTER_INVALID;
}

static const dr_difficulty MP3_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY, // 0x00
  DR_DIFFICULTY_NORMAL, // 0x01
  DR_DIFFICULTY_HARD, // 0x02
  DR_DIFFICULTY_VERY_HARD, // 0x03
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
  { 0x00, "Booting up", true },

  { 0x01, "Hand, Line and Sinker", false },
  { 0x02, "Coconut Conk", false },
  { 0x03, "Spotlight Swim", false },
  { 0x04, "Boulder Ball", false },
  { 0x05, "Crazy Cogs", false },
  { 0x06, "Hide and Sneak", false },
  { 0x07, "Ridiculous Relay", false },
  { 0x08, "Thwomp Pull", false },
  { 0x09, "River Raiders", false },
  { 0x0a, "Tidal Toss", false },
  { 0x0b, "Eatsa Pizza", false },
  { 0x0c, "Baby Bowser Broadside", false },
  { 0x0d, "Pump, Pump and Away", false },
  { 0x0e, "Hyper Hydrants", false },
  { 0x0f, "Picking Panic", false },
  { 0x10, "Cosmic Coaster", false },
  { 0x11, "Puddle Paddle", false },
  { 0x12, "Etch 'n' Catch", false },
  { 0x13, "Log Jam", false },
  { 0x14, "Slot Synch", false },
  { 0x15, "Treadmill Grill", false },
  { 0x16, "Toadstool Titan", false },
  { 0x17, "Aces High", false },
  { 0x18, "Bounce 'n' Trounce", false },
  { 0x19, "Ice Rink Risk", false },
  { 0x1a, "Locked Out", false },
  { 0x1b, "Chip Shot Challenge", false },
  { 0x1c, "Parasol Plummet", false },
  { 0x1d, "Messy Memory", false },
  { 0x1e, "Picture Imperfect", false },
  { 0x1f, "Mario's Puzzle Party", false },
  { 0x20, "The Beat Goes On", false },
  { 0x21, "M.P.I.Q.", false },
  { 0x22, "Curtain Call", false },
  { 0x23, "Water Whirled", false },
  { 0x24, "Frigid Bridges", false },
  { 0x25, "Awful Tower", false },
  { 0x26, "Cheep Cheep Chase", false },
  { 0x27, "Pipe Cleaners", false },
  { 0x28, "Snowball Summit", false },
  { 0x29, "All Fired Up", false },
  { 0x2a, "Stacked Deck", false },
  { 0x2b, "Three Door Monty", false },
  { 0x2c, "Rockin' Raceway", false },
  { 0x2d, "Merry-Go-Chomp", false },
  { 0x2e, "Slap Down", false },
  { 0x2f, "Storm Chasers", false },
  { 0x30, "Eye Sore", false },
  { 0x31, "Vine With Me", false },
  { 0x32, "Popgun Pick-Off", false },
  { 0x33, "End of the Line", false },
  { 0x34, "Bowser Toss", false },
  { 0x35, "Baby Bowser Bonkers", false },
  { 0x36, "Motor Rooter", false },
  { 0x37, "Silly Screws", false },
  { 0x38, "Crowd Cover", false },
  { 0x39, "Tick Tock Hop", false },
  { 0x3a, "Fowl Play", false },
  { 0x3b, "Winner's Wheel", false },
  { 0x3c, "Hey, Batter, Batter!", false },
  { 0x3d, "Bobbing Bow-loons", false },
  { 0x3e, "Dorrie Dip", false },
  { 0x3f, "Swinging with Sharks", false },
  { 0x40, "Swing 'n' Swipe", false },
  { 0x41, "Stardust Battle", false },
  { 0x42, "Game Guy's Roulette", false },
  { 0x43, "Game Guy's Lucky 7", false },
  { 0x44, "Game Guy's Magic Boxes", false },
  { 0x45, "Game Guy's Sweet Surprise", false },
  { 0x46, "Dizzy Dinghies", false },

  { 0x47, "Loading", true },
  { 0x48, "Chilly Waters", false },
  { 0x49, "Deep Bloober Sea", false },
  { 0x4a, "Spiny Desert", false },
  { 0x4b, "Woody Woods", false },
  { 0x4c, "Creepy Cavern", false },
  { 0x4d, "Waluigi's Island", false },
  { 0x4e, "Battle Royal Rule Map", false },
  { 0x4f, "Board result", false }, // result cutscene
  { 0x50, "Bowser Event", true },
  { 0x51, "Last 5 Turns", true },
  { 0x52, "Mushroom Genie", true },
  { 0x53, "Board intro", false },
  { 0x54, "Battle Royal Rule Map intro", false },
  { 0x55, "Board result", false }, // result screen
  { 0x56, "mchar", true },
  { 0x57, "mchar2", true }, // unused
  { 0x58, "Booting up", false }, // Nintendo/Hudson logos
  { 0x59, "sldebug", true }, // unused

  { 0x5a, "Loading (duel)", true },
  { 0x5b, "Gate Guy", false },
  { 0x5c, "Arrowhead", false },
  { 0x5d, "Pipesqueak", false },
  { 0x5e, "Blowhard", false },
  { 0x5f, "Mr. Mover", false },
  { 0x60, "Backtrack", false },
  // { 0x61, "" },
  // { 0x62, "" },
  // { 0x63, "" },
  { 0x64, "Duel Board intro", false },
  // { 0x65, "" },
  // { 0x66, "" },
  { 0x67, "Initializing save file", true },
  // { 0x68, "" },
  { 0x69, "Mini-Game Room", false },
  { 0x6a, "Chance Time", false },
  // { 0x6b, "" },
  // { 0x6c, "" },
  // { 0x6d, "" },
  // { 0x6e, "" },
  // { 0x6f, "" },
  { 0x70, "Mini-Game explanation", true },
  { 0x71, "Mini-Game results", true },
  { 0x72, "Game Guy results", true },
  { 0x73, "Duel Game results", true },
  { 0x74, "Battle Game results", true },
  // { 0x75, "" },
  // { 0x76, "" },
  { 0x77, "Castle Grounds", false },
  { 0x78, "Star Lift", false },
  { 0x79, "File select", false },
  { 0x7a, "Cutscene", false },
  { 0x7b, "Princess Peach's Castle", false },
  { 0x7c, "Credits", false },
  { 0x7d, "Story Mode result", false },
  // { 0x7e, "" },
  { 0x7f, "selmenu", true }, // unused

  { -1, nullptr },
};

static DrHostConfig makeConfig(const std::string &game)
{
  DrHostConfig config = {};

  config.core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString();
  if (!game.empty())
  {
    config.game = game;
    mp3PatchedCore() = writePatchedMupenCore(
      QString::fromStdString(config.core), QString::fromStdString(config.game));
    if (!mp3PatchedCore().isEmpty())
      config.core = mp3PatchedCore().toStdString();
  }

  config.char_to_dr = mp3_char_to_dr;
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
  config.values.result[0] = { 0x800d1110, DR_VALUE_TYPE_S16 };
  config.values.result[1] = { 0x800d1148, DR_VALUE_TYPE_S16 };
  config.values.result[2] = { 0x800d1180, DR_VALUE_TYPE_S16 };
  config.values.result[3] = { 0x800d11b8, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[0] = { 0x800d110e, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x800d1146, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x800d117e, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x800d11b6, DR_VALUE_TYPE_S16 };
  config.values.panel_color[0] = { 0x800d1124, DR_VALUE_TYPE_U8 };
  config.values.panel_color[1] = { 0x800d115c, DR_VALUE_TYPE_U8 };
  config.values.panel_color[2] = { 0x800d1194, DR_VALUE_TYPE_U8 };
  config.values.panel_color[3] = { 0x800d11cc, DR_VALUE_TYPE_U8 };
  config.values.coins[0] = { 0x800d1112, DR_VALUE_TYPE_S16 };
  config.values.coins[1] = { 0x800d114a, DR_VALUE_TYPE_S16 };
  config.values.coins[2] = { 0x800d1182, DR_VALUE_TYPE_S16 };
  config.values.coins[3] = { 0x800d11ba, DR_VALUE_TYPE_S16 };
  config.values.stars[0] = { 0x800d1116, DR_VALUE_TYPE_S8 };
  config.values.stars[1] = { 0x800d114e, DR_VALUE_TYPE_S8 };
  config.values.stars[2] = { 0x800d1186, DR_VALUE_TYPE_S8 };
  config.values.stars[3] = { 0x800d11be, DR_VALUE_TYPE_S8 };
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

MarioParty3Host::MarioParty3Host(QObject *parent, const std::string &game)
  : MarioPartyN64Host(makeConfig(game), parent)
{
  /* QRetro copies the core into its own temp before opening it, so ours is done. */
  if (!mp3PatchedCore().isEmpty())
  {
    QFile::remove(mp3PatchedCore());
    mp3PatchedCore().clear();
  }

  /* Deferred: the host's log signal is only connected after we are constructed. */
  if (!mp3PatchStatus().isEmpty())
  {
    const bool ok = mp3PatchStatus().startsWith("patched");

    QMetaObject::invokeMethod(
      this,
      [this, ok, status = mp3PatchStatus()]() {
        log(ok ? DR_LOG_INFO : DR_LOG_WARN,
          qPrintable(QString("custom rom: %1%2")
                       .arg(status)
                       .arg(ok ? "" : "; using the stock core, save type may be wrong")));
      },
      Qt::QueuedConnection);
    mp3PatchStatus().clear();
  }

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
