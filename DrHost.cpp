#include "DrHost.h"

#include <QFileInfo>

DrHost::DrHost(QObject *parent)
  : DrRetro(parent)
{
  qRegisterMetaType<DrMinigameCandidate>();
  qRegisterMetaType<DrPlayerArray>();
  qRegisterMetaType<dr_minigame_type>();
}

QStringList DrHost::saveFilePatterns(void) const
{
  /* The usual case: one save file named after the ROM, whatever its extension. */
  return { QFileInfo(QString::fromStdString(gamePath())).completeBaseName() + ".*" };
}
