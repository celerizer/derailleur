#include "MarioParty4Host.h"

#include <QRetroDirectories.h>

static DrGcnHostConfig makeConfig()
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 4 (USA) (Rev 1)").toStdString();

  return config;
}

MarioParty4Host::MarioParty4Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
}
