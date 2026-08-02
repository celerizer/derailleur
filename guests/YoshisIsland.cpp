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
  m_retro->init(coreId(), rom());
}

const dr_mp_minigame_t *YoshisIsland::minigames() const
{
  return YI_MINIGAMES;
}

void YoshisIsland::doApplyGameData(const DrGameData &data)
{
  (void)data; /* players cached by the base; m_minigame set by the base */
  m_minigameFrames = 0;

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
