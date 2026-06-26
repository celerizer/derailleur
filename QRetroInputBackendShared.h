#ifndef QRETRO_INPUT_BACKEND_SHARED_H
#define QRETRO_INPUT_BACKEND_SHARED_H

#include "QRetroInputBackend.h"

class DrInputStore;

/**
 * A QRetroInputBackend that sources its input from a shared DrInputStore rather
 * than from hardware. Every QRetro instance in derailleur uses one of these,
 * all pointed at the same store, so host and guests observe identical input.
 *
 * poll() only copies the store snapshot into the joypad array — it never blocks,
 * makes network calls, or stalls. Frame gating is handled separately by
 * DrNetplay via the frameBegin signal.
 */
class QRetroInputBackendShared : public QRetroInputBackend
{
  Q_OBJECT

public:
  explicit QRetroInputBackendShared(DrInputStore *store, QObject *parent = nullptr);

  void init(QRetroInputJoypad *joypads, unsigned maxUsers) override;
  void poll() override;

private:
  DrInputStore *m_Store = nullptr;
  QRetroInputJoypad *m_Joypads = nullptr;
  unsigned m_MaxUsers = 0;
};

#endif
