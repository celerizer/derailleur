#ifndef DR_INPUT_STORE_H
#define DR_INPUT_STORE_H

#include <QMutex>
#include <cstdint>

#include "QRetroInput.h"

/**
 * Canonical per-frame input state, written once per frame by either a local
 * (singleplayer) source or by DrNetplay (committed network frames), and read
 * by every QRetroInputBackendShared during poll().
 *
 * QRetroInputJoypad is a non-copyable QObject whose getters are non-const, so
 * snapshots are copied field-by-field under a mutex rather than assigned.
 */
class DrInputStore
{
public:
  static constexpr unsigned k_MaxPorts = 4;

  DrInputStore() = default;

  /// Atomically replace the stored snapshot with `snapshots` (k_MaxPorts
  /// joypads) for the given frame.
  void commitFrame(uint64_t frame, QRetroInputJoypad *snapshots);

  /// Copy the stored snapshot into `dst` (an array of `count` joypads).
  /// Thread-safe; called by QRetroInputBackendShared::poll().
  void copyTo(QRetroInputJoypad *dst, unsigned count);

  uint64_t currentFrame() const;

  /// Direct access to a stored joypad. Not synchronized — for single-threaded
  /// callers only; cross-thread readers should use copyTo().
  QRetroInputJoypad &joypad(unsigned port) { return m_Joypads[port]; }

private:
  static void copyJoypad(QRetroInputJoypad &dst, QRetroInputJoypad &src);

  mutable QMutex m_Mutex;
  QRetroInputJoypad m_Joypads[k_MaxPorts];
  uint64_t m_Frame = 0;
};

#endif
