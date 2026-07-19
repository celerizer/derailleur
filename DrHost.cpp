#include "DrHost.h"

DrHost::DrHost(QObject *parent)
  : DrRetro(parent)
{
  qRegisterMetaType<DrMinigameCandidate>();
  qRegisterMetaType<DrPlayerArray>();
  qRegisterMetaType<dr_minigame_type>();
}
