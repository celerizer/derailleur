#include "CoreDolphin.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QUuid>
#include <QApplication>
#include <QRetroDirectories.h>

static QString resolveDiscPath(const QString &base)
{
  if (QFile::exists(base + ".rvz"))
    return base + ".rvz";
  else if (QFile::exists(base + ".iso"))
    return base + ".iso";
  else
    return QString();
}

/* Generate a unique name for the Dolphin instance */
static QByteArray dolphinArenaTag(const QString &subdir)
{
  QByteArray base = subdir.toUtf8().left(8);
  while (base.size() < 8)
    base.append('-');
  const uint hash = qHash(subdir) & 0xFFF;
  return base + QByteArray::number(hash, 16).rightJustified(3, '0').right(3);
}

/**
 * ABSOLUTELY DERANGED WINDOWS HACK!!
 * Scan the binary for the text "dolphin-emu\0" and replace it with a unique
 * marker.
 *
 * See https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Common/MemArenaWin.cpp#L121
 *
 * The problem comes from this function on Windows creating a uniquely named
 * handle based on the process ID... but since the GCN and Wii Dolphins both
 * belong to derailleur they both try to take the same name, which is allowed
 * on Linux but not on Windows.
 */
static QString writePatchedDolphinCore(const QString &originalPath, const QString &subdir)
{
  QFile origFile(originalPath);
  if (!origFile.open(QIODevice::ReadOnly))
    return QString();
  QByteArray data = origFile.readAll();
  origFile.close();

  const QByteArray needle("dolphin-emu", 12);
  const QByteArray marker("Memory::Init()");
  int firstIdx = -1;
  for (int from = 0; (from = data.indexOf(needle, from)) >= 0; from++)
  {
    const int windowStart = from + needle.size();
    const int windowEnd = qMin(windowStart + 64, data.size());
    if (data.mid(windowStart, windowEnd - windowStart).contains(marker))
    {
      firstIdx = from;
      break;
    }
  }
  if (firstIdx < 0)
    return QString();

  const QByteArray tag = dolphinArenaTag(subdir);
  data.replace(firstIdx, tag.size(), tag);

  QFileInfo origInfo(originalPath);
  const QString destPath = QString("%1/dolphin_libretro_%2_%3.%4")
    .arg(QDir::tempPath(), subdir, QUuid::createUuid().toString(QUuid::Id128), origInfo.suffix());

  QFile destFile(destPath);
  if (!destFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return QString();
  destFile.write(data);
  destFile.close();

  return destPath;
}

CoreDolphin::CoreDolphin(const QString &subdir, QObject *parent)
  : DrGuest(parent)
{
  m_subdir = subdir;
  m_retro = new DrRetro(this);
  m_retro->setCore(new QRetro(), true);
  m_name = ("Dolphin " + subdir).toUtf8();

  /* startMinigame() runs on the GUI thread here (doDelegateLaunch), so the base's
   * GUI-thread pause would race the free-running timing thread. doDelegateLaunch
   * pauses deterministically on the timing thread instead (see below). */
  m_pauseOnStart = false;

  /* Pretend to not support gyro/accel so we can use the sticks */
  core()->setEnvironmentCallbackSupported(RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE, false);

  /* All Dolphin instances share the default system/save dirs; only the disc-list
   * needs to stay per-instance, e.g. system/discs-gcn.m3u. */
  const QString system =
    QString::fromUtf8(core()->directories()->get(QRetroDirectories::System));
  m_m3uPath = system + "/discs-" + subdir + ".m3u";
}

bool CoreDolphin::loadCore()
{
  /* Patch the library so this instance gets a unique memory-arena name (see
   * writePatchedDolphinCore), then dlopen it. QRetro copies the file into its own
   * temp before loading, so the patched copy can be removed right after. */
  const QString patchedPath = writePatchedDolphinCore(m_baseCorePath, m_subdir);
  const QString loadPath = patchedPath.isEmpty() ? m_baseCorePath : patchedPath;

  const bool ok = core() && core()->loadCore(loadPath.toUtf8().constData());
  if (!ok)
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(loadPath)));

  if (!patchedPath.isEmpty())
    QFile::remove(patchedPath);

  return ok;
}

void CoreDolphin::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

CoreDolphin::~CoreDolphin()
{
  delete[] m_flatList;
}

void CoreDolphin::addGame(DolphinGuest *game)
{
  QString base = QString::fromStdString(game->discPath());
  QString discPath = resolveDiscPath(base);
  if (discPath.isEmpty())
  {
    log(DR_LOG_WARN,
      qPrintable(
        QString("skipping %1: rom not found (tried .rvz, .iso): %2").arg(game->name()).arg(base)));
    return;
  }

  if (m_games.isEmpty())
  {
    // Apply Dolphin settings we will need
    // See: https://github.com/classicslive/QRetro/blob/master/docs/Cores.md#Dolphin

    // Core > Dual Core Mode
    // Needs to be disabled for serialization to work.
    core()->options()->setOptionValue("dolphin_main_cpu_thread", "disabled");

    // Core > Fastmem
    // Needs to be disabled for multi-instancing to work.
    core()->options()->setOptionValue("dolphin_fastmem", "disabled");

    /* The library is dlopen'd lazily on the first launch; see loadCore(). */
    m_baseCorePath = QString::fromStdString(game->corePath());
  }

  m_games.append(game);
  m_discPaths.append(discPath);

  connect(game, &DrGuest::minigameFinished, this, [this]() { finishMinigame(); });
  connect(game, &DrGuest::logMessage, this, &DrGuest::logMessage);

  // Collect all mini-games from this game
  for (const dr_mp_minigame_t *mg = game->minigames(); mg && mg->name; mg++)
    m_entries.append({ game, mg });

  rebuildFlatList();
}

void CoreDolphin::finalizeGames()
{
  /* Only write the disc-list m3u here; the base loads it lazily on the first
   * launch (gamePath() returns m_m3uPath) so the two Dolphin cores don't both
   * boot at startup. */
  QFile m3u(m_m3uPath);
  if (m3u.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&m3u);
    for (const QString &path : m_discPaths)
      out << path << "\n";
  }
  else
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to write disc list: %1").arg(m_m3uPath)));
    m_valid = false;
  }

  /* With a single disc there is nothing to swap to -- assume the one disc */
  if (m_games.size() == 1)
    m_discIndex = 0;
}

void CoreDolphin::rebuildFlatList()
{
  delete[] m_flatList;
  int count = m_entries.size();
  m_flatList = new dr_mp_minigame_t[count + 1];

  for (int i = 0; i < count; i++)
    m_flatList[i] = *m_entries[i].second;

  // Null-terminate sentinel
  m_flatList[count] = { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS };
}

QList<DrMinigameGroup> CoreDolphin::minigameGroups() const
{
  QList<DrMinigameGroup> result;
  int i = 0;
  for (DolphinGuest *game : m_games)
  {
    DrMinigameGroup group;
    group.name = game->name();
    while (i < m_entries.size() && m_entries[i].first == game)
      group.minigames.append(&m_flatList[i++]);
    result.append(group);
  }
  return result;
}

void CoreDolphin::run()
{
  if (m_delegate)
    m_delegate->tick();
}

dr_minigame_result_t CoreDolphin::minigameResult(unsigned index)
{
  if (m_delegate)
    return m_delegate->minigameResult(index);
  return { 0, 0 };
}

void CoreDolphin::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;

  // Find which child game owns this minigame entry
  DolphinGuest *owner = nullptr;
  for (int i = 0; i < m_entries.size(); i++)
  {
    if (&m_flatList[i] == minigame)
    {
      owner = m_entries[i].first;
      break;
    }
  }

  if (!owner)
    return;

  m_delegate = owner;

  // Swap to the disc for this game (index matches order in the .m3u)
  int discIndex = m_games.indexOf(owner);
  log(DR_LOG_INFO, qPrintable(QString("disc change: %1 -> disc %2 (was %3, %4 disc(s))")
                                .arg(owner->name())
                                .arg(discIndex)
                                .arg(m_discIndex)
                                .arg(m_games.size())));

  // Always (re)insert on a multi-disc core; single-disc cores never hot-swap
  if (m_games.size() > 1)
  {
    core()->show();
    core()->unpause();

    static const int stepFrames = 120;

    /* Run each disc change step for two seconds */
    auto spin = [this](int frames) {
      for (int i = 0; i < frames; i++)
      {
        core()->waitFrames(1);
        QApplication::processEvents();
      }
    };

    /* Use the disk interface to change games, letting each step take */
    log(DR_LOG_INFO, "disc change: ejecting");
    core()->diskControl()->setEjectState(true);
    spin(stepFrames);

    log(DR_LOG_INFO, qPrintable(QString("disc change: setting image index %1").arg(discIndex)));
    core()->diskControl()->setImageIndex(discIndex);
    spin(stepFrames);

    log(DR_LOG_INFO, "disc change: inserting");
    core()->diskControl()->setEjectState(false);
    m_discIndex = discIndex;
    spin(stepFrames);

    core()->pause();
    log(DR_LOG_INFO, "disc change: disc settled");
  }
  else
  {
    log(DR_LOG_INFO, "disc change: single-disc core, keeping the booted disc");
  }

  // Load the per-game savestate
  const QString statePath = QString::fromStdString(owner->statePath());
  log(DR_LOG_INFO, qPrintable(QString("disc change: loading savestate %1").arg(statePath)));
  const bool stateOk = core()->unserializeFromFile(statePath);
  QApplication::processEvents();
  log(stateOk ? DR_LOG_INFO : DR_LOG_ERROR,
    qPrintable(QString("disc change: savestate %1")
                 .arg(stateOk ? "loaded" : "FAILED to load")));

  // Cleanup any leftover mini-game state
  owner->cancelMinigame();

  // Delegate game-specific setup (writes minigame_id, players, etc.)
  log(DR_LOG_INFO, "disc change: applying game-specific setup");
  owner->applyGameData(data);
  QApplication::processEvents();

  core()->unpause();

  // Wait for the delegate to actually start its mini-game before revealing it
  static const int maxSetupFrames = 60 * 30;
  int setupFrames = 0;
  while (!owner->minigameActive() && setupFrames < maxSetupFrames)
  {
    core()->waitFrames(1);
    QApplication::processEvents();
    setupFrames++;
  }
  if (setupFrames > 0)
    log(owner->minigameActive() ? DR_LOG_INFO : DR_LOG_WARN,
      qPrintable(QString("disc change: waited %1 frames for delegate setup%2")
                   .arg(setupFrames)
                   .arg(owner->minigameActive() ? "" : " (timed out)")));

  /* A timed-out delegate means our setup diverged from the peers'; ask for a hard
   * resync (once) so we realign. Only meaningful during a netplay session. */
  if (dr_netplay_active() && !owner->minigameActive() && !m_resyncRequested)
  {
    m_resyncRequested = true;
    emit desyncSuspected();
  }

  /* Stop on an exact frame gate */
  core()->execOnTimingThread([c = core()]() { c->pause(); });

  log(DR_LOG_INFO, "disc change: starting minigame");
  startMinigame();
}
