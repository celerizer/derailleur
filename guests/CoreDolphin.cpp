#include "CoreDolphin.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QApplication>

static QString resolveDiscPath(const QString &base)
{
  if (QFile::exists(base + ".rvz"))
    return base + ".rvz";
  else if (QFile::exists(base + ".iso"))
    return base + ".iso";
  else
    return QString();
}

CoreDolphin::CoreDolphin(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);
  m_retro->setCore(new QRetro(), true);
  m_m3uPath = QDir::temp().filePath("derailleur_dolphin.m3u");
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
    if (!core()->loadCore(game->corePath().c_str()))
    {
      log(DR_LOG_ERROR,
        qPrintable(QString("failed to load core: %1").arg(game->corePath().c_str())));
      m_valid = false;
    }

    // Apply Dolphin settings we will need
    // See: https://github.com/classicslive/QRetro/blob/master/docs/Cores.md#Dolphin

    // Core > Dual Core Mode
    // Needs to be disabled for serialization to work.
    core()->options()->setOptionValue("dolphin_main_cpu_thread", "disabled");

    // Core > Fastmem
    // Needs to be disabled for multi-instancing to work.
    core()->options()->setOptionValue("dolphin_fastmem", "disabled");
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
  QFile m3u(m_m3uPath);
  if (m3u.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&m3u);
    for (const QString &path : m_discPaths)
      out << path << "\n";
  }

  if (!core()->loadContent(m_m3uPath.toStdString().c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(m_m3uPath)));
    m_valid = false;
  }
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
  if (discIndex != m_discIndex)
  {
    /* Expose the core so it will run frames */
    core()->show();

    /* Use the disk interface to change games */
    core()->diskControl()->setEjectState(true);
    core()->diskControl()->setImageIndex(discIndex);
    core()->diskControl()->setEjectState(false);
    m_discIndex = discIndex;

    /* Spin frames while the disc takes (MPGC needed about this much) */
    core()->unpause();
    for (int i = 0; i < 120; i++)
    {
      core()->waitFrames(1);
      QApplication::processEvents();
    }
    core()->pause();
  }

  // Load the per-game savestate
  core()->unserializeFromFile(QString::fromStdString(owner->statePath()));
  QApplication::processEvents();

  // Delegate game-specific setup (writes minigame_id, players, etc.)
  owner->applyGameData(data);
  QApplication::processEvents();

  // Spin again (this was the time needed for MP6 to draw a new frame)
  core()->unpause();
  for (int i = 0; i < 48; i++)
  {
    core()->waitFrames(1);
    QApplication::processEvents();
  }
  core()->pause();
  startMinigame();
}
