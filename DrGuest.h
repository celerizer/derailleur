#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include <QList>
#include <QObject>
#include <QRetro.h>
#include <QString>
#include <QtEndian>

struct DrMinigameGroup
{
  const char                   *name;
  QList<const dr_mp_minigame_t*> minigames;
};

class DrGuest : public QObject
{
  Q_OBJECT

public:
  // N64-style: subclass creates its own QRetro in its constructor and assigns m_core
  DrGuest(QObject *parent = nullptr) : QObject(parent) {}

  // GCN-style: receives a shared QRetro it does not own
  explicit DrGuest(QRetro *sharedCore, QObject *parent = nullptr)
    : QObject(parent), m_core(sharedCore), m_ownCore(false) {}

  ~DrGuest() { if (m_ownCore) delete m_core; }

  QRetro *core() const { return m_core; }

  virtual dr_minigame_result_t    minigameResult(unsigned index) = 0;
  virtual const dr_mp_minigame_t* minigames() const = 0;
  virtual const char*             name(void) const = 0;
  virtual QList<DrMinigameGroup>  minigameGroups() const;
  void setMinigame(const dr_mp_minigame_t *minigame);

  dr_error setPlayer(unsigned index, const dr_player_t &player);
  dr_error setPlayerCharacter(unsigned index, dr_character character);
  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port);
  dr_error setPlayerControlType(unsigned index, dr_control_type control_type);
  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty);
  dr_error setPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id);

  void pause(void)
  {
    if (m_core)
      m_core->pause();
  }
  
  void unpause(void)
  {
    if (m_core)
      m_core->unpause();
  }
  
  void startCore(void)
  {
    if (m_core)
      m_core->startCore();
  }

protected:
  dr_error readu8(uint8_t *out, size_t addr, bool big_endian = false);
  dr_error reads8(int8_t *out, size_t addr, bool big_endian = false);
  dr_error readu16(uint16_t *out, size_t addr, bool big_endian = false);
  dr_error reads16(int16_t *out, size_t addr, bool big_endian = false);
  dr_error readu32(uint32_t *out, size_t addr, bool big_endian = false);
  dr_error reads32(int32_t *out, size_t addr, bool big_endian = false);
  dr_error writeu8(uint8_t val, size_t addr, bool big_endian = false);
  dr_error writes8(int8_t val, size_t addr, bool big_endian = false);
  dr_error writeu16(uint16_t val, size_t addr, bool big_endian = false);
  dr_error writes16(int16_t val, size_t addr, bool big_endian = false);
  dr_error writeu32(uint32_t val, size_t addr, bool big_endian = false);
  dr_error writes32(int32_t val, size_t addr, bool big_endian = false);

  void log(unsigned level, const char *message);
  void startMinigame();
  void finishMinigame();
  void writeForFrames(size_t addr, const void *value, unsigned bytes, bool big_endian, int frames);
  void tickFrameWrites(void);

  QRetro                  *m_core          = nullptr;
  bool                     m_ownCore       = false;
  bool                     m_minigameActive = false;
  const dr_mp_minigame_t  *m_minigame       = nullptr;

private:
  struct FrameWrite { size_t addr; QByteArray data; bool big_endian; int frames; };
  QList<FrameWrite> m_frameWrites;

  virtual void doSetMinigame(const dr_mp_minigame_t *minigame) = 0;
  virtual dr_error doSetPlayerCharacter(unsigned index, dr_character character) = 0;
  virtual dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) = 0;
  virtual dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) = 0;
  virtual dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) = 0;
  virtual dr_error doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) = 0;

signals:
  void minigameFinished();
  void logMessage(unsigned level, const QString &message);
};

#endif
