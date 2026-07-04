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

  /**
   * Points this backend at the real hardware backend so that device management
   * (and rumble/sensors) transparently target physical controllers. Input for
   * emulation still comes from the store via poll(); only out-of-band device
   * queries are forwarded. Without this, every shared core reports zero
   * gamepads, which makes QRetroConfig fall back to keyboard input.
   */
  void setDeviceBackend(QRetroInputBackend *backend) { m_DeviceBackend = backend; }

  QString deviceName(unsigned port) const override;
  QVector<QRetroGamepadInfo> connectedGamepads() const override;
  bool assignGamepad(unsigned port, unsigned deviceId) override;
  unsigned assignedDeviceId(unsigned port) const override;
  bool setRumble(unsigned port, retro_rumble_effect effect, uint16_t strength) override;
  bool setSensorState(unsigned port, retro_sensor_action action, unsigned rate) override;
  float getSensorInput(unsigned port, unsigned id) override;
  bool sensorActive(unsigned port, unsigned id) override;

private:
  DrInputStore *m_Store = nullptr;
  QRetroInputJoypad *m_Joypads = nullptr;
  unsigned m_MaxUsers = 0;
  QRetroInputBackend *m_DeviceBackend = nullptr;
};

#endif
