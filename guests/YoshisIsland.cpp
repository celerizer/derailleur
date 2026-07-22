#include "YoshisIsland.h"

#include <QFile>

static const dr_mp_minigame_t YI_MINIGAMES[] = {
  /* @todo fill in real mini-games (e.g. one of the bonus mini-games). */
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

YoshisIsland::YoshisIsland(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);

  QString corePath = dr_core_path(DR_CORE_SNES9X);
  QString gamePath = dr_roms_directory() + "/Super Mario World 2 - Yoshi's Island (USA) (Rev 1).sfc";
  m_gamePath = gamePath.toStdString();

  QRetro *core = new QRetro();
  core->setSavingEnabled(false);
  if (!core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  /* Content is loaded lazily on the first launch (see DrGuest::applyGameData). */
  if (!QFile::exists(gamePath))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(core, true);
}

void YoshisIsland::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

const dr_mp_minigame_t *YoshisIsland::minigames() const
{
  return YI_MINIGAMES;
}

void YoshisIsland::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  for (unsigned i = 0; i < 4; i++)
    m_players[i] = data.players[i];

  /* @todo load a savestate and write the selected mini-game id + player setup. */

  startMinigame();
}

void YoshisIsland::run()
{
  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* @todo detect the mini-game finishing and call finishMinigame(). */
}

dr_minigame_result_t YoshisIsland::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  (void)index;

  /* @todo award coins based on the mini-game outcome. */
  return result;
}
