#include "MarioPartyGcnHost.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>

MarioPartyGcnHost::MarioPartyGcnHost(const DrGcnHostConfig &config, QObject *parent)
  : DrHost(parent)
  , m_config(config)
{
  m_core = new QRetro();
  m_ownCore = true;
  m_gamePath = config.game; // so gamePath() yields the ROM (used for the netplay save name)
  if (!m_core->loadCore(config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }

  /* Read/write the save from the derailleur save dir (default cwd/save). Set before
   * loadContent so the core sees it when it reads its memory card. */
  m_core->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  if (!m_core->loadContent(config.game.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(config.game.c_str())));
    m_valid = false;
  }

  /* GameCube RAM is big-endian, so accesses can use hardware addresses directly. */
  m_endianness = DR_ENDIANNESS_BIG;

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);
}

void MarioPartyGcnHost::run(void)
{
  /* Skeleton: the state machine is in place but no transitions are driven yet. */
  switch (m_State)
  {
  case DR_GCN_HOST_STATE_INVALID:
    break;
  case DR_GCN_HOST_STATE_BEFORE_BOARD:
    break;
  case DR_GCN_HOST_STATE_BOARD:
    break;
  case DR_GCN_HOST_STATE_BEFORE_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_AFTER_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_MINIGAME:
    break;
  case DR_GCN_HOST_STATE_SIZE:
    break;
  }
}

void MarioPartyGcnHost::writeResults(DrGuest *guest)
{
  (void)guest;
}

void MarioPartyGcnHost::clearResults(void)
{
}
