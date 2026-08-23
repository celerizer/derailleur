#include "DrRetro.h"

#include <QFile>

void DrRetro::init(dr_core core, const QString &rom, bool saving)
{
  m_corePath = dr_core_path(core).toStdString();
  m_gamePath = (dr_roms_directory() + "/" + rom).toStdString();

  QRetro *c = new QRetro();
  c->setSavingEnabled(saving);

  /* Content loads lazily on the first launch (see DrGuest::applyGameData); just
   * confirm the ROM is present so a missing one drops the guest at startup. */
  if (!QFile::exists(QString::fromStdString(m_gamePath)))
  {
    log(DR_LOG_ERROR,
      qPrintable(QString("rom not found: %1").arg(QString::fromStdString(m_gamePath))));
    m_valid = false;
  }

  setCore(c, true);
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
