#include "DrCommon.h"

#include <cstdint>

static QString roms_dir = "roms";
static QString cores_dir = "cores";
static QString state_dir = "state";
static QString save_dir = "save";

QString dr_roms_directory(void)
{
  return roms_dir;
}

void dr_set_roms_directory(const QString &path)
{
  roms_dir = path;
}

QString dr_cores_directory(void)
{
  return cores_dir;
}

void dr_set_cores_directory(const QString &path)
{
  cores_dir = path;
}

QString dr_state_directory(void)
{
  return state_dir;
}

void dr_set_state_directory(const QString &path)
{
  state_dir = path;
}

QString dr_save_directory(void)
{
  return save_dir;
}

void dr_set_save_directory(const QString &path)
{
  save_dir = path;
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
    return cores_dir + "/mupen64plus_next_libretro" + dr_os_extension();
  case DR_CORE_DOLPHIN:
    return cores_dir + "/dolphin_libretro" + dr_os_extension();
  case DR_CORE_MGBA:
    return cores_dir + "/mgba_libretro" + dr_os_extension();
  default:
    return {};
  }
}

static uint32_t rand_state = 1u;
static unsigned long rand_count = 0;

void dr_srand(unsigned seed)
{
  rand_state = static_cast<uint32_t>(seed);
  rand_count = 0;
}

int dr_rand(void)
{
  rand_count++;
  rand_state = rand_state * 1103515245u + 12345u;
  return static_cast<int>((rand_state >> 16) & 0x7FFF);
}

unsigned long dr_rand_count(void)
{
  return rand_count;
}
