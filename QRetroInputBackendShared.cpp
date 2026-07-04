#include "QRetroInputBackendShared.h"

#include "DrInputStore.h"

QRetroInputBackendShared::QRetroInputBackendShared(DrInputStore *store, QObject *parent)
  : QRetroInputBackend(parent)
  , m_Store(store)
{
}

void QRetroInputBackendShared::init(QRetroInputJoypad *joypads, unsigned maxUsers)
{
  m_Joypads = joypads;
  m_MaxUsers = maxUsers;
}

void QRetroInputBackendShared::poll()
{
  if (m_Store && m_Joypads)
    m_Store->copyTo(m_Joypads, m_MaxUsers);
}

/* Device management is forwarded to the real hardware backend: every core in
 * derailleur runs a shared backend, but there is only one set of physical
 * controllers, owned by the local source backend. */

QString QRetroInputBackendShared::deviceName(unsigned port) const
{
  return m_DeviceBackend ? m_DeviceBackend->deviceName(port) : QString();
}

QVector<QRetroGamepadInfo> QRetroInputBackendShared::connectedGamepads() const
{
  return m_DeviceBackend ? m_DeviceBackend->connectedGamepads() : QVector<QRetroGamepadInfo>();
}

bool QRetroInputBackendShared::assignGamepad(unsigned port, unsigned deviceId)
{
  return m_DeviceBackend && m_DeviceBackend->assignGamepad(port, deviceId);
}

unsigned QRetroInputBackendShared::assignedDeviceId(unsigned port) const
{
  return m_DeviceBackend ? m_DeviceBackend->assignedDeviceId(port) : QRETRO_NO_DEVICE;
}

bool QRetroInputBackendShared::setRumble(unsigned port, retro_rumble_effect effect, uint16_t strength)
{
  return m_DeviceBackend && m_DeviceBackend->setRumble(port, effect, strength);
}

bool QRetroInputBackendShared::setSensorState(unsigned port, retro_sensor_action action, unsigned rate)
{
  return m_DeviceBackend && m_DeviceBackend->setSensorState(port, action, rate);
}

float QRetroInputBackendShared::getSensorInput(unsigned port, unsigned id)
{
  return m_DeviceBackend ? m_DeviceBackend->getSensorInput(port, id) : 0.0f;
}

bool QRetroInputBackendShared::sensorActive(unsigned port, unsigned id)
{
  return m_DeviceBackend && m_DeviceBackend->sensorActive(port, id);
}
