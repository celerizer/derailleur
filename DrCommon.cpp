#include "DrCommon.h"

#include <cstdint>

#include <QDir>
#include <QSettings>

static dr_settings g_settings;

static QString settings_ini_path(void)
{
  return QDir::current().filePath("derailleur.ini");
}

dr_settings &dr_settings_get(void)
{
  return g_settings;
}

void dr_settings_load(void)
{
  QSettings s(settings_ini_path(), QSettings::IniFormat);
  g_settings.shared_gamecube_core =
    s.value("settings/shared_gamecube_core", g_settings.shared_gamecube_core).toBool();
  g_settings.loading_overlay =
    s.value("settings/loading_overlay", g_settings.loading_overlay).toBool();
  g_settings.character_injection =
    s.value("settings/character_injection", g_settings.character_injection).toBool();
  g_settings.mute_while_loading =
    s.value("settings/mute_while_loading", g_settings.mute_while_loading).toBool();
  g_settings.widescreen_hack =
    s.value("settings/widescreen_hack", g_settings.widescreen_hack).toBool();
  g_settings.res_scale_n64 =
    s.value("settings/res_scale_n64", g_settings.res_scale_n64).toUInt();
  g_settings.res_scale_gcn =
    s.value("settings/res_scale_gcn", g_settings.res_scale_gcn).toUInt();
  g_settings.res_scale_ds =
    s.value("settings/res_scale_ds", g_settings.res_scale_ds).toUInt();
}

void dr_settings_save(void)
{
  QSettings s(settings_ini_path(), QSettings::IniFormat);
  s.setValue("settings/shared_gamecube_core", g_settings.shared_gamecube_core);
  s.setValue("settings/loading_overlay", g_settings.loading_overlay);
  s.setValue("settings/character_injection", g_settings.character_injection);
  s.setValue("settings/mute_while_loading", g_settings.mute_while_loading);
  s.setValue("settings/widescreen_hack", g_settings.widescreen_hack);
  s.setValue("settings/res_scale_n64", g_settings.res_scale_n64);
  s.setValue("settings/res_scale_gcn", g_settings.res_scale_gcn);
  s.setValue("settings/res_scale_ds", g_settings.res_scale_ds);
  s.sync();
}

static QString roms_dir = "roms";
static QString cores_dir = "cores";
static QString state_dir = "state";
static QString save_dir = "save";
static bool netplay_active = false;

bool dr_netplay_active(void)
{
  return netplay_active;
}

void dr_set_netplay_active(bool active)
{
  netplay_active = active;
}

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
  case DR_CORE_FLYCAST:
    return cores_dir + "/flycast_libretro" + dr_os_extension();
  case DR_CORE_FCEUMM:
    return cores_dir + "/fceumm_libretro" + dr_os_extension();
  case DR_CORE_SNES9X:
    return cores_dir + "/snes9x_libretro" + dr_os_extension();
  case DR_CORE_MELONDSDS:
    return cores_dir + "/melondsds_libretro" + dr_os_extension();
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

unsigned dr_rand_state(void)
{
  return rand_state;
}

void dr_set_rand_state(unsigned state, unsigned long count)
{
  rand_state = static_cast<uint32_t>(state);
  rand_count = count;
}
