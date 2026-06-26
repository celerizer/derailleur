#include "DrInputStore.h"

#include <libretro.h>

void DrInputStore::copyJoypad(QRetroInputJoypad &dst, QRetroInputJoypad &src)
{
  for (unsigned id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; id++)
  {
    dst.setDigitalButton(id, src.digitalButton(id));
    dst.setAnalogButton(id, src.analogButton(id));
  }
  for (unsigned index = 0; index < 2; index++)
  {
    dst.setAnalogStick(
      index, RETRO_DEVICE_ID_ANALOG_X, src.analogStick(index, RETRO_DEVICE_ID_ANALOG_X));
    dst.setAnalogStick(
      index, RETRO_DEVICE_ID_ANALOG_Y, src.analogStick(index, RETRO_DEVICE_ID_ANALOG_Y));
  }
}

void DrInputStore::commitFrame(uint64_t frame, QRetroInputJoypad *snapshots)
{
  QMutexLocker lock(&m_Mutex);
  for (unsigned i = 0; i < k_MaxPorts; i++)
    copyJoypad(m_Joypads[i], snapshots[i]);
  m_Frame = frame;
}

void DrInputStore::copyTo(QRetroInputJoypad *dst, unsigned count)
{
  QMutexLocker lock(&m_Mutex);
  for (unsigned i = 0; i < count && i < k_MaxPorts; i++)
    copyJoypad(dst[i], m_Joypads[i]);
}

uint64_t DrInputStore::currentFrame() const
{
  QMutexLocker lock(&m_Mutex);
  return m_Frame;
}
