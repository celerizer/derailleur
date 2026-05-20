#include "CoreDolphin.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QApplication>

CoreDolphin::CoreDolphin(QObject *parent)
  : DrGuest(parent)
{
  m_core = new QRetro();
  m_ownCore = true;
  m_m3uPath = QDir::temp().filePath("derailleur_dolphin.m3u");
}

CoreDolphin::~CoreDolphin()
{
  delete[] m_flatList;
}

void CoreDolphin::addGame(MarioPartyGcn *game)
{
  if (m_games.isEmpty())
  {
    if (!m_core->loadCore(game->config().core.c_str())) {
      log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(game->config().core.c_str())));
      m_valid = false;
    }

    // Apply Dolphin settings we will need
    // See: https://github.com/classicslive/QRetro/blob/master/docs/Cores.md#Dolphin

    // Core > Dual Core Mode
    // Needs to be disabled for serialization to work.
    m_core->options()->setOptionValue("dolphin_main_cpu_thread", "disabled");
    
    // Core > Fastmem
    // Needs to be disabled for multi-instancing to work.
    m_core->options()->setOptionValue("dolphin_fastmem", "disabled");
  }

  m_games.append(game);

  connect(game, &DrGuest::minigameFinished, this, [this]() { finishMinigame(); });

  // Collect all (non-sentinel) minigames from this game
  for (const dr_mp_minigame_t *mg = game->config().minigames; mg && mg->name; mg++)
    m_entries.append({ game, mg });

  rebuildFlatList();
}

void CoreDolphin::finalizeGames()
{
  QFile m3u(m_m3uPath);
  if (m3u.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&m3u);
    for (MarioPartyGcn *game : m_games)
      out << game->config().game.c_str() << "\n";
  }

  if (!m_core->loadContent(m_m3uPath.toStdString().c_str())) {
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
  for (MarioPartyGcn *game : m_games) {
    DrMinigameGroup group;
    group.name = game->name();
    while (i < m_entries.size() && m_entries[i].first == game)
      group.minigames.append(&m_flatList[i++]);
    result.append(group);
  }
  return result;
}

dr_minigame_result_t CoreDolphin::minigameResult(unsigned index)
{
  if (m_delegate)
    return m_delegate->minigameResult(index);
  return { 0, 0 };
}

void CoreDolphin::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  // Find which child game owns this minigame entry
  MarioPartyGcn *owner = nullptr;
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
  startMinigame();

  // Swap to the disc for this game (index matches order in the .m3u)
  int discIndex = m_games.indexOf(owner);
  if (discIndex != m_discIndex)
  {
    m_core->show();
    m_core->diskControl()->setEjectState(true);
    m_core->diskControl()->setImageIndex(discIndex);
    m_core->diskControl()->setEjectState(false);
    m_discIndex = discIndex;

    // Spin for 60 frames while the disc takes (MPGC needed about this much)
    m_core->unpause();
    for (int i = 0; i < 60; i++)
    {
      m_core->waitFrames(1);
      QApplication::processEvents();
    }
    m_core->pause();
  }

  // Load the per-game savestate
  m_core->unserializeFromFile(QString::fromStdString(owner->config().state));
  QApplication::processEvents();

  // Delegate game-specific setup (writes minigame_id etc.)
  owner->setMinigame(minigame);
  QApplication::processEvents();

  // Spin again (this was the time needed for MP6 to draw a new frame)
  m_core->unpause();
  for (int i = 0; i < 48; i++)
  {
    m_core->waitFrames(1);
    QApplication::processEvents();
  }
  m_core->pause();
}

dr_error CoreDolphin::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return m_delegate ? m_delegate->doSetPlayerCharacter(index, character) : DR_ERR_INVALID_PARAMETER;
}

dr_error CoreDolphin::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return m_delegate ? m_delegate->doSetPlayerControlPort(index, control_port) : DR_ERR_INVALID_PARAMETER;
}

dr_error CoreDolphin::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  return m_delegate ? m_delegate->doSetPlayerControlType(index, control_type) : DR_ERR_INVALID_PARAMETER;
}

dr_error CoreDolphin::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  return m_delegate ? m_delegate->doSetPlayerDifficulty(index, difficulty) : DR_ERR_INVALID_PARAMETER;
}

dr_error CoreDolphin::doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  return m_delegate ? m_delegate->doSetPlayerTeam(index, color, type, team_id) : DR_ERR_INVALID_PARAMETER;
}
