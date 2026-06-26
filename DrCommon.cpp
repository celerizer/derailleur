#include "DrCommon.h"

static QString s_romsDir = "roms";
static QString s_coresDir = "cores";
static QString s_stateDir = "state";

QString dr_roms_directory(void)
{
  return s_romsDir;
}
void dr_set_roms_directory(const QString &path)
{
  s_romsDir = path;
}
QString dr_cores_directory(void)
{
  return s_coresDir;
}
void dr_set_cores_directory(const QString &path)
{
  s_coresDir = path;
}
QString dr_state_directory(void)
{
  return s_stateDir;
}
void dr_set_state_directory(const QString &path)
{
  s_stateDir = path;
}

QString dr_core_path(dr_core core)
{
  switch (core)
  {
  case DR_CORE_MUPEN64PLUSNEXT:
    return s_coresDir + "/mupen64plus_next_libretro.so";
  case DR_CORE_DOLPHIN:
    return s_coresDir + "/dolphin_libretro.so";
  case DR_CORE_MGBA:
    return s_coresDir + "/mgba_libretro.so";
  default:
    return {};
  }
}
