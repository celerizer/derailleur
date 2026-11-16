#ifndef DR_RETRO_H
#define DR_RETRO_H

#include "DrCommon.h"
#include <functional>
#include <string>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QRetro.h>
#include <QtEndian>

class QWidget;

/// Applies one Wii Remote control layout to `port`'s joypad on `core`: button and
/// stick remaps that make a pad stand in for how the game expects the remote to
/// be held (see dr_wii_control). Layouts are written for the upright WiiMote
/// controller type and translated when the port is on the sideways one.
void dr_apply_wii_control(QRetro *core, unsigned port, dr_wii_control control);

/// Pushes the global, user-facing core options (see DrSettings) into `core`.
/// Call it once per QRetro, right after it is constructed -- values for options
/// the core hasn't registered yet are held and applied when it does. Every
/// setting is written both ways (on *and* off), because a value that lands in
/// the core's own ini would otherwise outlive the user turning it back off.
void dr_apply_global_core_options(QRetro *core, dr_core which);

/// Pushes the current global core options into every QRetro that has had them
/// applied, so a changed setting reaches cores that already exist.
void dr_reapply_global_core_options(void);

class DrRetro : public QObject
{
  Q_OBJECT

public:
  DrRetro(QObject *parent = nullptr)
    : QObject(parent)
  {
  }
  ~DrRetro() { delete m_core; }

  QRetro *core() const { return m_core; }
  bool isValid() const { return m_valid; }

  /// Takes ownership of `core`, replacing any previous one.
  virtual void setCore(QRetro *core)
  {
    delete m_core;
    m_core = core;
  }

  /// Standard setup for a guest that owns a core: creates the QRetro, records the
  /// core (a dr_core) and ROM (a filename under the roms dir) so the base can load
  /// them lazily on the first launch, verifies the ROM exists (dropping isValid()
  /// if not), and installs the core. Plain DrRetro; DrRetroN64 also applies its
  /// N64 button remaps. `saving` toggles the core's SRAM writes (off by default).
  virtual void init(dr_core core, const QString &rom, bool saving = false);

  /// As above, but loads the core by filename instead of a dr_core: a bare name
  /// (no extension) is looked up in the cores dir, a path is used as-is. Global
  /// core options aren't applied, since they're keyed by dr_core.
  void init(const QString &core, const QString &rom, bool saving = false);

  /// Core library and content paths, loaded lazily by boot(). Set by init(), or
  /// directly for cores set up by hand.
  std::string corePath() const { return m_corePath; }
  std::string gamePath() const { return m_gamePath; }
  void setCorePath(const std::string &path) { m_corePath = path; }
  void setGamePath(const std::string &path) { m_gamePath = path; }

  /// Loads the core library, once, before the first content load. The default
  /// dlopens corePath(); override for cores that need custom loading (e.g.
  /// DrRetroDolphin patches its library first). Returns false if it can't load.
  virtual bool loadCore();

  /// Boots the core lazily and calls `onBooted` once it has run `frames` frames.
  /// The first call loads the core and content and starts it; later calls just
  /// re-run `onBooted` on the next frame. Audio is held muted through the boot
  /// window when the user asked for it. `onBooted` runs on the timing thread;
  /// the window container is resized on the GUI thread after it. Returns false
  /// if the core can't load.
  bool boot(unsigned frames, std::function<void()> onBooted);

  /// Whether a boot() countdown is in progress.
  bool booting() const { return m_bootCountdown > 0; }

  /// Whether the core is loaded (boot() has run and shutdown() hasn't since).
  bool booted() const { return m_booted; }

  /// Unloads a booted core and its content to give back their memory; the next
  /// boot() loads both again from scratch. GUI thread only, and only while the
  /// core isn't in use (not the current context).
  void shutdown();

  /// Loads <state directory>/<name>.state.zip (see DrPaths), logs the result, and
  /// returns whether it loaded.
  bool loadState(const char *name);

  /// Pushes a mini-game's emulation quirks (see dr_emulation_quirk_t) into the
  /// core's options. Options the core doesn't have are ignored.
  void applyQuirks(const dr_mp_minigame_t *minigame);

  /// A window container around the core, focusable without a click.
  QWidget *createWidget(QWidget *parent);

  /// Resizes the core's window to fill its container. Must run on the GUI thread.
  void resizeToContainer();

  virtual void pause()
  {
    if (m_core)
      m_core->pause();
  }
  virtual void unpause()
  {
    if (m_core)
      m_core->unpause();
  }
  virtual void startCore()
  {
    if (m_core)
      m_core->startCore();
  }

  /// Puts every port on one Wii control layout, for a context that doesn't vary
  /// it per player (a board, a menu). Guests that split solo/team layouts per
  /// mini-game use DrGuestWii::applyControlRemap instead.
  void applyWiiControls(dr_wii_control control);

public:
  dr_error readu8(uint8_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error reads8(int8_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error readu16(uint16_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error reads16(int16_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error readu32(uint32_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error reads32(int32_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writeu8(uint8_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writes8(int8_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writeu16(uint16_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writes16(int16_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writeu32(uint32_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writes32(int32_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_INVALID);

  dr_error readValue(int64_t *out, const dr_value_t &value, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  dr_error writeValue(int64_t val, const dr_value_t &value, dr_endianness endianness = DR_ENDIANNESS_INVALID);

  void log(unsigned level, const char *message);
  void writeForFrames(
    size_t addr, const void *value, unsigned bytes, unsigned frames, dr_endianness endianness = DR_ENDIANNESS_INVALID);
  void writeValueForFrames(int64_t val, const dr_value_t &value, unsigned frames,
    dr_endianness endianness = DR_ENDIANNESS_INVALID);
  void clearFrameWrites();
  void tickFrameWrites();

  QRetro *m_core = nullptr;
  bool m_valid = true;
  std::string m_corePath; // core library path, for the lazy first-launch load
  std::string m_gamePath; // content path, for the lazy first-launch load
  QWidget *m_container = nullptr; // window container from createWidget()

protected:
  /// The endianness used for memory r/w
  dr_endianness m_endianness = DR_ENDIANNESS_LITTLE;
  dr_endianness resolveEndianness(dr_endianness e) const
  {
    return e == DR_ENDIANNESS_INVALID ? m_endianness : e;
  }

private:
  struct FrameWrite
  {
    size_t addr;
    QByteArray data;
    dr_endianness endianness;
    unsigned frames;
  };
  QList<FrameWrite> m_frameWrites;

  /// Records the core and content paths for init(), dropping isValid() if the
  /// ROM is missing.
  void setPaths(const QString &corePath, const QString &rom);

  /* boot() state */
  bool m_booted = false;
  bool m_bootHookInstalled = false;
  int m_bootCountdown = 0;
  std::function<void()> m_onBooted;

signals:
  void logMessage(unsigned level, const QString &message);
};

#endif
