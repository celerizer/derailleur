#include "DrRetro.h"
#include "DrPaths.h"
#include "DrSettings.h"

#include <QFile>
#include <QList>
#include <QPointer>
#include <QWidget>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

/* The process's memory, for the log lines around core loads and unloads */
static QString dr_memory_usage(void)
{
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc = {};

  pmc.cb = sizeof(pmc);
  if (!K32GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc)))
    return "unavailable";

  return QString("%1 MiB working set, %2 MiB private")
    .arg(static_cast<qulonglong>(pmc.WorkingSetSize >> 20))
    .arg(static_cast<qulonglong>(pmc.PrivateUsage >> 20));
#else
  QFile status("/proc/self/status");
  qlonglong rss = -1, swap = 0;

  if (!status.open(QIODevice::ReadOnly | QIODevice::Text))
    return "unavailable";

  /* Lines look like "VmRSS:    123456 kB" */
  for (const QByteArray &line : status.readAll().split('\n'))
  {
    if (line.startsWith("VmRSS:"))
      rss = line.mid(6).trimmed().split(' ').first().toLongLong();
    else if (line.startsWith("VmSwap:"))
      swap = line.mid(7).trimmed().split(' ').first().toLongLong();
  }
  if (rss < 0)
    return "unavailable";

  return QString("%1 MiB resident, %2 MiB swap").arg(rss >> 10).arg(swap >> 10);
#endif
}

typedef struct
{
  QPointer<QRetro> core;
  dr_core which;
} dr_global_options_core_t;

static QList<dr_global_options_core_t> global_options_cores;

void dr_reapply_global_core_options(void)
{
  const QList<dr_global_options_core_t> cores = global_options_cores;

  for (const dr_global_options_core_t &entry : cores)
    if (entry.core)
      dr_apply_global_core_options(entry.core, entry.which);
}

void dr_apply_global_core_options(QRetro *core, dr_core which)
{
  const bool widescreen = DrSettings::instance().widescreenHack();
  const bool widens = (which == DR_CORE_DOLPHIN || which == DR_CORE_MUPEN64PLUSNEXT);
  const dr_core_option_t options[] = {
    { "mupen64plus-aspect", widescreen ? "16:9 adjusted" : "4:3" },
    { "dolphin_widescreen_hack", widescreen ? "enabled" : "disabled" },
    { "dolphin_aspect_ratio", widescreen ? "1" : "3" },
    { nullptr, nullptr },
  };
  const dr_core_option_t *option;
  int i;

  if (!core || !core->options())
    return;

  for (i = global_options_cores.size() - 1; i >= 0; i--)
    if (!global_options_cores[i].core || global_options_cores[i].core == core)
      global_options_cores.removeAt(i);
  global_options_cores.append({ core, which });

  for (option = options; option->key; option++)
    core->options()->setOptionValue(option->key, option->value);

  /* Internal resolution. Dolphin and mupen take the multiplier as a number;
   * melonDS takes the entry it shows, so that string has to be rebuilt here. */
  switch (which)
  {
  case DR_CORE_MUPEN64PLUSNEXT:
  {
    const QByteArray scale = QByteArray::number(DrSettings::instance().resScaleN64());

    core->options()->setOptionValue("mupen64plus-EnableNativeResFactor", scale.constData());
    break;
  }
  case DR_CORE_DOLPHIN:
  {
    const QByteArray scale = QByteArray::number(DrSettings::instance().resScaleGcn());

    core->options()->setOptionValue("dolphin_efb_scale", scale.constData());
    break;
  }
  case DR_CORE_MELONDSDS:
  {
    const unsigned n = DrSettings::instance().resScaleDs();
    const QByteArray value = QString("%1x native (%2 x %3)")
                               .arg(n).arg(n * 256).arg(n * 192).toUtf8();

    core->options()->setOptionValue("melonds_opengl_resolution", value.constData());
    break;
  }
  default:
    break;
  }

  core->setForcedAspectRatio(widescreen && widens ? 16.0 / 9.0 : 0.0);
  if (!widescreen)
    core->setSourceCrop(0.0, 0.0, 0.0, 0.0);
  else
  {
    /* Crop weirdness and overscan */
    if (which == DR_CORE_DOLPHIN)
    {
      const double bar = (1.0 - (4.0 / 3.0) / (16.0 / 9.0)) / 2.0;
      core->setSourceCrop(0.0, bar, 0.0, bar);
    }
    else if (which == DR_CORE_MUPEN64PLUSNEXT)
      core->setSourceCrop(0.01, 0.01, 0.01, 0.01);
  }
}

/* The controller type the Dolphin core holds sideways, laying out its buttons
 * and tilt for that grip */
#define DR_WII_SIDEWAYS_DESC "WiiMote (sideways)"

/* Whether `port` of `core` is on the sideways WiiMote controller type */
static bool dr_wii_port_is_sideways(QRetro *core, unsigned port)
{
  const std::vector<QRetroControllerPort> &ports = core->input()->controllerPorts();
  const unsigned selected = core->input()->selectedControllerType(port);

  if (port >= ports.size())
    return false;
  for (const QRetroControllerType &type : ports[port].types)
    if (type.id == selected)
      return type.desc == DR_WII_SIDEWAYS_DESC;

  return false;
}

/* The RetroPad button that presses `upright_id`'s Wii button (as laid out on the
 * upright WiiMote: A, B, X = 1, Y = 2) on the sideways WiiMote, which lays out
 * X = A, Y = B, B = 1, A = 2 instead, and turns the d-pad 90 degrees */
static int dr_wii_sideways_button(int upright_id)
{
  switch (upright_id)
  {
  case RETRO_DEVICE_ID_JOYPAD_UP:
    return RETRO_DEVICE_ID_JOYPAD_LEFT;
  case RETRO_DEVICE_ID_JOYPAD_DOWN:
    return RETRO_DEVICE_ID_JOYPAD_RIGHT;
  case RETRO_DEVICE_ID_JOYPAD_LEFT:
    return RETRO_DEVICE_ID_JOYPAD_DOWN;
  case RETRO_DEVICE_ID_JOYPAD_RIGHT:
    return RETRO_DEVICE_ID_JOYPAD_UP;
  case RETRO_DEVICE_ID_JOYPAD_A:
    return RETRO_DEVICE_ID_JOYPAD_X;
  case RETRO_DEVICE_ID_JOYPAD_B:
    return RETRO_DEVICE_ID_JOYPAD_Y;
  case RETRO_DEVICE_ID_JOYPAD_X:
    return RETRO_DEVICE_ID_JOYPAD_B;
  case RETRO_DEVICE_ID_JOYPAD_Y:
    return RETRO_DEVICE_ID_JOYPAD_A;
  default:
    return upright_id;
  }
}

/* One left stick axis the core reads: which physical axis, and its sign */
typedef struct
{
  unsigned id;
  int sign;
} dr_wii_axis_t;

/* Applies one Wii control layout to a single player's joypad. Layouts are
 * written for the upright WiiMote, and translated for a sideways one. */
void dr_apply_wii_control(QRetro *core, unsigned port, dr_wii_control control)
{
  QRetroInputJoypad &jp = core->input()->joypads()[port];
  const bool sideways = dr_wii_port_is_sideways(core, port);
  /* Physical button -> upright Wii button, before translating */
  int buttons[RETRO_DEVICE_ID_JOYPAD_R3 + 1];
  /* Tilt the layout wants, from the physical stick, in the upright frame */
  dr_wii_axis_t tilt_x = { RETRO_DEVICE_ID_ANALOG_X, 1 };
  dr_wii_axis_t tilt_y = { RETRO_DEVICE_ID_ANALOG_Y, 1 };
  bool swap_sticks = false;
  int i;

  for (i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
    buttons[i] = i;
  buttons[RETRO_DEVICE_ID_JOYPAD_R3] = RETRO_DEVICE_ID_JOYPAD_NONE;

  /* Remove existing remaps */
  jp.clearButtonRemaps();
  jp.clearAnalogStickRemaps();
  jp.clearAnalogAxisRemaps();
  jp.setAnalogSensitivity(1.0f);

  switch (control)
  {
  case DR_WII_CONTROL_INVALID:
  case DR_WII_CONTROL_UPRIGHT:
    /* Held Wii Remote: the stick drives the d-pad. (Invalid falls back here.) */
    jp.setAnalogStickToDigitalPad(true);
    break;
  case DR_WII_CONTROL_SIDEWAYS_BUTTONS:
    /* 2 = A, 1 = B, flip dpad 90 degrees left */
    jp.setAnalogStickToDigitalPad(true);
    buttons[RETRO_DEVICE_ID_JOYPAD_A] = RETRO_DEVICE_ID_JOYPAD_Y;
    buttons[RETRO_DEVICE_ID_JOYPAD_B] = RETRO_DEVICE_ID_JOYPAD_X;
    buttons[RETRO_DEVICE_ID_JOYPAD_Y] = RETRO_DEVICE_ID_JOYPAD_A;
    buttons[RETRO_DEVICE_ID_JOYPAD_X] = RETRO_DEVICE_ID_JOYPAD_B;
    buttons[RETRO_DEVICE_ID_JOYPAD_LEFT] = RETRO_DEVICE_ID_JOYPAD_UP;
    buttons[RETRO_DEVICE_ID_JOYPAD_DOWN] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    buttons[RETRO_DEVICE_ID_JOYPAD_UP] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    break;
  case DR_WII_CONTROL_SIDEWAYS_MOTION:
    jp.setAnalogStickToDigitalPad(false);
    /* Motion tilt is twitchy at full range; halve the stick. */
    jp.setAnalogSensitivity(0.5f);
    buttons[RETRO_DEVICE_ID_JOYPAD_A] = RETRO_DEVICE_ID_JOYPAD_Y;
    buttons[RETRO_DEVICE_ID_JOYPAD_B] = RETRO_DEVICE_ID_JOYPAD_X;
    buttons[RETRO_DEVICE_ID_JOYPAD_Y] = RETRO_DEVICE_ID_JOYPAD_A;
    buttons[RETRO_DEVICE_ID_JOYPAD_X] = RETRO_DEVICE_ID_JOYPAD_B;
    buttons[RETRO_DEVICE_ID_JOYPAD_LEFT] = RETRO_DEVICE_ID_JOYPAD_UP;
    buttons[RETRO_DEVICE_ID_JOYPAD_DOWN] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    buttons[RETRO_DEVICE_ID_JOYPAD_UP] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    /* Held sideways: rotate the left stick (tilt) 90 degrees to match.
     * X <- -Y, Y <- X. */
    tilt_x.id = RETRO_DEVICE_ID_ANALOG_Y;
    tilt_x.sign = -1;
    tilt_y.id = RETRO_DEVICE_ID_ANALOG_X;
    tilt_y.sign = 1;
    break;
  case DR_WII_CONTROL_POINTER:
    /* Pointer aiming: keep the sticks analog and swap them both ways, so the
     * core's left stick (the pointer) reads the physical right stick and the
     * core's right stick reads the physical left. A is A and B is B. */
    jp.setAnalogStickToDigitalPad(false);
    swap_sticks = true;
    break;
  case DR_WII_CONTROL_WAGGLE:
    /* Shake: the stick drives the d-pad and A stands in for the shake (R2). */
    jp.setAnalogStickToDigitalPad(true);
    buttons[RETRO_DEVICE_ID_JOYPAD_A] = RETRO_DEVICE_ID_JOYPAD_R2;
    break;
  default:
    break;
  }

  for (i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
  {
    const int target = sideways ? dr_wii_sideways_button(buttons[i]) : buttons[i];

    if (target != i)
      jp.setButtonRemap(i, target);
  }

  if (swap_sticks)
  {
    jp.setAnalogStickRemap(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_INDEX_ANALOG_LEFT);
    jp.setAnalogStickRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_INDEX_ANALOG_RIGHT);
  }
  else
  {
    /* The sideways WiiMote turns tilt 90 degrees itself, so turn it back,
     * X <- Y, Y <- -X */
    if (sideways)
    {
      const dr_wii_axis_t upright_x = tilt_x;

      tilt_x = tilt_y;
      tilt_y.id = upright_x.id;
      tilt_y.sign = -upright_x.sign;
    }
    if (tilt_x.id != RETRO_DEVICE_ID_ANALOG_X || tilt_x.sign != 1 ||
        tilt_y.id != RETRO_DEVICE_ID_ANALOG_Y || tilt_y.sign != 1)
    {
      jp.setAnalogAxisRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,
        RETRO_DEVICE_INDEX_ANALOG_LEFT, tilt_x.id, tilt_x.sign);
      jp.setAnalogAxisRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,
        RETRO_DEVICE_INDEX_ANALOG_LEFT, tilt_y.id, tilt_y.sign);
    }
  }
}

void DrRetro::applyWiiControls(dr_wii_control control)
{
  unsigned i;

  if (!m_core || !m_core->input())
    return;

  for (i = 0; i < 4; i++)
    dr_apply_wii_control(m_core, i, control);

  log(DR_LOG_INFO, qPrintable(QString("[control] all ports: %1")
    .arg(dr_wii_control_name(control))));
}

void DrRetro::init(dr_core core, const QString &rom, bool saving)
{
  QRetro *c = new QRetro();

  setPaths(DrPaths::instance().corePath(core), rom);
  c->setSavingEnabled(saving);
  dr_apply_global_core_options(c, core);
  setCore(c);
}

void DrRetro::init(const QString &core, const QString &rom, bool saving)
{
  QRetro *c = new QRetro();

  /* A bare filename is looked up in the cores dir with the OS extension */
  if (core.contains('/') || core.contains('\\'))
    setPaths(core, rom);
  else
    setPaths(DrPaths::instance().coresDirectory() + "/" + core + dr_os_extension(), rom);
  c->setSavingEnabled(saving);
  setCore(c);
}

void DrRetro::setPaths(const QString &corePath, const QString &rom)
{
  m_corePath = corePath.toStdString();
  m_gamePath = (DrPaths::instance().romsDirectory() + "/" + rom).toStdString();

  /* Content loads lazily on the first launch (see DrGuest::applyGameData); just
   * confirm the ROM is present so a missing one drops the guest at startup. */
  if (!QFile::exists(QString::fromStdString(m_gamePath)))
  {
    log(DR_LOG_ERROR,
      qPrintable(QString("rom not found: %1").arg(QString::fromStdString(m_gamePath))));
    m_valid = false;
  }
}

bool DrRetro::loadCore()
{
  if (!m_core || m_corePath.empty())
    return false;
  return m_core->loadCore(m_corePath.c_str());
}

bool DrRetro::boot(unsigned frames, std::function<void()> onBooted)
{
  if (!m_core)
    return false;

  m_onBooted = std::move(onBooted);
  if (m_booted)
  {
    m_bootCountdown = 1; /* already booted; run on the next frame */
    return true;
  }

  /* Open the core now, on the first boot, rather than at construction. */
  if (!loadCore())
  {
    log(DR_LOG_ERROR, "failed to load core");
    return false;
  }
  m_booted = true;

  if (!m_gamePath.empty())
    m_core->loadContent(m_gamePath.c_str());
  startCore();

  if (!m_bootHookInstalled)
  {
    m_bootHookInstalled = true;
    connect(m_core, &QRetro::frameBegin, this, [this]() {
      if (m_bootCountdown <= 0)
        return;
      /* Mute through the boot window. On the first boot mainwindow's mute ran
       * before the core existed (audio() was still null), so re-assert it here
       * each frame; the minigameStarted handler unmutes once the game starts. */
      if (DrSettings::instance().muteWhileLoading())
        if (auto *a = m_core->audio())
          a->setMute(true);
      if (--m_bootCountdown != 0)
        return;
      /* Runs on the timing thread (safe for state loads + memory). The container
       * nudge runs on the GUI thread, after it. */
      if (m_onBooted)
        m_onBooted();
      log(DR_LOG_INFO, qPrintable("core loaded, memory: " + dr_memory_usage()));
      QMetaObject::invokeMethod(this, [this]() { resizeToContainer(); },
        Qt::QueuedConnection);
    }, Qt::DirectConnection);
  }
  m_bootCountdown = frames ? frames : 1;
  return true;
}

void DrRetro::shutdown()
{
  if (!m_core || !m_booted)
    return;

  /* The QRetro object (its window, options, remaps and netplay hookup) stays;
   * only the library, its content and its threads go. */
  m_core->unloadCore();
  m_booted = false;
  m_bootCountdown = 0;
  m_onBooted = nullptr;
  clearFrameWrites();
  log(DR_LOG_INFO, qPrintable("core unloaded, memory: " + dr_memory_usage()));
}

bool DrRetro::loadState(const char *name)
{
  if (!m_core)
    return false;
  const QString path = DrPaths::instance().stateDirectory() + "/" + name + ".state.zip";
  const bool ok = m_core->unserializeFromFile(path);
  log(ok ? DR_LOG_INFO : DR_LOG_WARN,
    qPrintable(QString("load state %1: %2").arg(path, ok ? "ok" : "failed")));
  return ok;
}

void DrRetro::applyQuirks(const dr_mp_minigame_t *minigame)
{
  if (!minigame || !m_core || !m_core->options())
    return;

  const char *option_value = nullptr;
  /* Held for as long as option_value may point into it. */
  const QByteArray efb_scale = QByteArray::number(DrSettings::instance().resScaleGcn());

  /// Graphics > Settings > Texture Cache Accuracy
  /// 128=Normal, 512=Fast, 0=Safe
  if (minigame->quirks.dolphin.needs_safe_texture_cache)
  {
    log(DR_LOG_INFO, "Mini-game requires safe texture cache, enabling...");
    option_value = "0";
  }
  else
    option_value = "128";
  m_core->options()->setOptionValue("dolphin_texture_cache_accuracy", option_value);

  /// Graphics > Hacks > Skip EFB Copy to RAM
  /// All the negative negation makes this look like a typo, but it's not
  if (minigame->quirks.dolphin.needs_efb_to_texture)
  {
    log(DR_LOG_INFO, "Mini-game requires EFB copy to RAM, enabling...");
    option_value = "disabled";
  }
  else
    option_value = "enabled";
  m_core->options()->setOptionValue("dolphin_efb_to_texture", option_value);

  /// Graphics > Settings > Internal Resolution
  /// A quirked mini-game only renders correctly at native; the rest follow the
  /// user's setting (see DrSettings::resScaleGcn).
  if (minigame->quirks.dolphin.needs_native_resolution)
  {
    log(DR_LOG_INFO, "Mini-game requires native resolution, enabling...");
    option_value = "1";
  }
  else
    option_value = efb_scale.constData();
  m_core->options()->setOptionValue("dolphin_efb_scale", option_value);
}

QWidget *DrRetro::createWidget(QWidget *parent)
{
  m_container = QWidget::createWindowContainer(m_core, parent);

  /* Accept keyboard focus without needing to click */
  m_container->setFocusPolicy(Qt::StrongFocus);

  return m_container;
}

void DrRetro::resizeToContainer()
{
  if (m_container && m_core)
    m_core->resize(m_container->width(), m_container->height());
}

dr_error DrRetro::readu8(uint8_t *out, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (endianness == DR_ENDIANNESS_WORDFLIPPED)
    addr = (addr & ~3UL) | (3 - (addr & 3));
  return m_core->memory().readValue<uint8_t>(out, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrRetro::reads8(int8_t *out, size_t addr, dr_endianness endianness)
{
  return readu8(reinterpret_cast<uint8_t *>(out), addr, endianness);
}

dr_error DrRetro::readu16(uint16_t *out, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (endianness == DR_ENDIANNESS_WORDFLIPPED)
    addr ^= 2;
  if (!m_core->memory().readValue<uint16_t>(out, addr))
    return DR_ERR_MEMORY_ACCESS_CORE;
  if (endianness == DR_ENDIANNESS_BIG)
    *out = qbswap(*out);
  return DR_OK;
}

dr_error DrRetro::reads16(int16_t *out, size_t addr, dr_endianness endianness)
{
  return readu16(reinterpret_cast<uint16_t *>(out), addr, endianness);
}

dr_error DrRetro::readu32(uint32_t *out, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (!m_core->memory().readValue<uint32_t>(out, addr))
    return DR_ERR_MEMORY_ACCESS_CORE;
  if (endianness == DR_ENDIANNESS_BIG)
    *out = qbswap(*out);
  return DR_OK;
}

dr_error DrRetro::reads32(int32_t *out, size_t addr, dr_endianness endianness)
{
  return readu32(reinterpret_cast<uint32_t *>(out), addr, endianness);
}

dr_error DrRetro::writeu8(uint8_t val, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (endianness == DR_ENDIANNESS_WORDFLIPPED)
    addr = (addr & ~3UL) | (3 - (addr & 3));
  return m_core->memory().writeValue<uint8_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrRetro::writes8(int8_t val, size_t addr, dr_endianness endianness)
{
  return writeu8(static_cast<uint8_t>(val), addr, endianness);
}

dr_error DrRetro::writeu16(uint16_t val, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (endianness == DR_ENDIANNESS_WORDFLIPPED)
    addr ^= 2;
  else if (endianness == DR_ENDIANNESS_BIG)
    val = qbswap(val);
  return m_core->memory().writeValue<uint16_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrRetro::writes16(int16_t val, size_t addr, dr_endianness endianness)
{
  return writeu16(static_cast<uint16_t>(val), addr, endianness);
}

dr_error DrRetro::writeu32(uint32_t val, size_t addr, dr_endianness endianness)
{
  if (!m_core)
    return DR_ERR_MEMORY_ACCESS_CORE;
  endianness = resolveEndianness(endianness);
  if (endianness == DR_ENDIANNESS_BIG)
    val = qbswap(val);
  return m_core->memory().writeValue<uint32_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
}

dr_error DrRetro::writes32(int32_t val, size_t addr, dr_endianness endianness)
{
  return writeu32(static_cast<uint32_t>(val), addr, endianness);
}

dr_error DrRetro::readValue(int64_t *out, const dr_value_t &value, dr_endianness endianness)
{
  dr_error err = DR_OK;
  switch (value.type)
  {
  case DR_VALUE_TYPE_S8:  { int8_t   v = 0; err = reads8(&v, value.address, endianness);  *out = v; break; }
  case DR_VALUE_TYPE_U8:  { uint8_t  v = 0; err = readu8(&v, value.address, endianness);  *out = v; break; }
  case DR_VALUE_TYPE_S16: { int16_t  v = 0; err = reads16(&v, value.address, endianness); *out = v; break; }
  case DR_VALUE_TYPE_U16: { uint16_t v = 0; err = readu16(&v, value.address, endianness); *out = v; break; }
  case DR_VALUE_TYPE_S32: { int32_t  v = 0; err = reads32(&v, value.address, endianness); *out = v; break; }
  case DR_VALUE_TYPE_U32:
  case DR_VALUE_TYPE_POINTER: { uint32_t v = 0; err = readu32(&v, value.address, endianness); *out = v; break; }
  default: return DR_ERR_INVALID_PARAMETER;
  }
  return err;
}

dr_error DrRetro::writeValue(int64_t val, const dr_value_t &value, dr_endianness endianness)
{
  switch (value.type)
  {
  case DR_VALUE_TYPE_S8:  return writes8(static_cast<int8_t>(val), value.address, endianness);
  case DR_VALUE_TYPE_U8:  return writeu8(static_cast<uint8_t>(val), value.address, endianness);
  case DR_VALUE_TYPE_S16: return writes16(static_cast<int16_t>(val), value.address, endianness);
  case DR_VALUE_TYPE_U16: return writeu16(static_cast<uint16_t>(val), value.address, endianness);
  case DR_VALUE_TYPE_S32: return writes32(static_cast<int32_t>(val), value.address, endianness);
  case DR_VALUE_TYPE_U32:
  case DR_VALUE_TYPE_POINTER: return writeu32(static_cast<uint32_t>(val), value.address, endianness);
  default: return DR_ERR_INVALID_PARAMETER;
  }
}

void DrRetro::writeForFrames(
  size_t addr, const void *value, unsigned bytes, unsigned frames, dr_endianness endianness)
{
  m_frameWrites.append(
    { addr, QByteArray(static_cast<const char *>(value), bytes), endianness, frames });
}

void DrRetro::writeValueForFrames(int64_t val, const dr_value_t &value, unsigned frames,
  dr_endianness endianness)
{
  uint64_t raw = static_cast<uint64_t>(val);
  unsigned bytes = dr_value_type_size(value.type);

  if (bytes)
    writeForFrames(value.address, &raw, bytes, frames, endianness);
}

void DrRetro::clearFrameWrites()
{
  m_frameWrites.clear();
}

void DrRetro::tickFrameWrites()
{
  for (auto it = m_frameWrites.begin(); it != m_frameWrites.end();)
  {
    switch (it->data.size())
    {
    case 1:
      writeu8(*reinterpret_cast<const uint8_t *>(it->data.constData()), it->addr, it->endianness);
      break;
    case 2:
      writeu16(*reinterpret_cast<const uint16_t *>(it->data.constData()), it->addr, it->endianness);
      break;
    case 4:
      writeu32(*reinterpret_cast<const uint32_t *>(it->data.constData()), it->addr, it->endianness);
      break;
    }
    if (--it->frames == 0)
      it = m_frameWrites.erase(it);
    else
      ++it;
  }
}

void DrRetro::log(unsigned level, const char *message)
{
  emit logMessage(level, QString::fromUtf8(message));
}
