#ifndef DR_RETRO_H
#define DR_RETRO_H

#include "DrCommon.h"
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QRetro.h>
#include <QtEndian>

class DrRetro : public QObject
{
  Q_OBJECT

public:
  DrRetro(QObject *parent = nullptr)
    : QObject(parent)
  {
  }
  explicit DrRetro(QRetro *sharedCore, QObject *parent = nullptr)
    : QObject(parent)
    , m_core(sharedCore)
    , m_ownCore(false)
  {
  }
  ~DrRetro()
  {
    if (m_ownCore)
      delete m_core;
  }

  QRetro *core() const { return m_core; }
  bool isValid() const { return m_valid; }

  void setCore(QRetro *core, bool own = true)
  {
    if (m_ownCore) delete m_core;
    m_core = core;
    m_ownCore = own;
  }

  void pause()
  {
    if (m_core)
      m_core->pause();
  }
  void unpause()
  {
    if (m_core)
      m_core->unpause();
  }
  void startCore()
  {
    if (m_core)
      m_core->startCore();
  }

public:
  dr_error readu8(uint8_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error reads8(int8_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error readu16(uint16_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error reads16(int16_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error readu32(uint32_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error reads32(int32_t *out, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writeu8(uint8_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writes8(int8_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writeu16(uint16_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writes16(int16_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writeu32(uint32_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  dr_error writes32(int32_t val, size_t addr, dr_endianness endianness = DR_ENDIANNESS_LITTLE);

  void log(unsigned level, const char *message);
  void writeForFrames(
    size_t addr, const void *value, unsigned bytes, unsigned frames, dr_endianness endianness = DR_ENDIANNESS_LITTLE);
  void tickFrameWrites();

  QRetro *m_core = nullptr;
  bool m_ownCore = false;
  bool m_valid = true;

private:
  struct FrameWrite
  {
    size_t addr;
    QByteArray data;
    dr_endianness endianness;
    unsigned frames;
  };
  QList<FrameWrite> m_frameWrites;

signals:
  void logMessage(unsigned level, const QString &message);
};

#endif
