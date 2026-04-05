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
    m_core->loadCore(game->config().core);

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
      out << game->config().game << "\n";
  }

  m_core->loadContent(m_m3uPath.toStdString().c_str());
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
    m_core->swapDisc(discIndex);
    m_discIndex = discIndex;
    for (int i = 0; i < 60; i++)
    {
      m_core->waitFrames(1);
      QApplication::processEvents();
    }
  }

  // Load the per-game savestate
  m_core->unserializeFromFile(owner->config().state);
  QApplication::processEvents();

  // Delegate game-specific setup (writes minigame_id etc.)
  owner->setMinigame(minigame);
  QApplication::processEvents();
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
