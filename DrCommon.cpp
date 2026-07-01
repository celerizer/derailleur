#include "DrCommon.h"

#include <cstdint>

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

QString dr_os_extension(void)
{
#if defined(Q_OS_WIN)
  return ".dll";
#else
  return ".so";
#endif
}

QString dr_core_path(dr_core core)
{
  switch (core)
  {
  case DR_CORE_MUPEN64PLUSNEXT:
    return s_coresDir + "/mupen64plus_next_libretro" + dr_os_extension();
  case DR_CORE_DOLPHIN:
    return s_coresDir + "/dolphin_libretro" + dr_os_extension();
  case DR_CORE_MGBA:
    return s_coresDir + "/mgba_libretro" + dr_os_extension();
  default:
    return {};
  }
}

static uint32_t s_randState = 1u;
static unsigned long s_randCount = 0;

void dr_srand(unsigned seed)
{
  s_randState = static_cast<uint32_t>(seed);
  s_randCount = 0;
}

int dr_rand(void)
{
  s_randCount++;
  s_randState = s_randState * 1103515245u + 12345u;
  return static_cast<int>((s_randState >> 16) & DR_RAND_MAX);
}

unsigned long dr_rand_count(void)
{
  return s_randCount;
}
