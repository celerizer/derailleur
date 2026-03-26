#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include <QRetro.h>
#include <QString>
#include <QtEndian>

class DrGuest : public QRetro
{
  Q_OBJECT

public:
  DrGuest(QWindow *parent = nullptr) : QRetro(parent) {}

  virtual dr_minigame_result_t minigameResult(unsigned index) = 0;
  virtual const dr_mp_minigame_t* minigames() const = 0;
  virtual const char* name(void) const = 0;
  virtual void setMinigame(unsigned id) = 0;

  dr_error setPlayer(unsigned index, const dr_player_t &player);
  dr_error setPlayerCharacter(unsigned index, dr_character character);
  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port);
  dr_error setPlayerControlType(unsigned index, dr_control_type control_type);
  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty);
  dr_error setPlayerTeam(unsigned index, dr_team team);

protected:
  dr_error read8(uint8_t *out, size_t addr, bool big_endian = false);
  dr_error read16(uint16_t *out, size_t addr, bool big_endian = false);
  dr_error read32(uint32_t *out, size_t addr, bool big_endian = false);
  dr_error write8(uint8_t val, size_t addr, bool big_endian = false);
  dr_error write16(uint16_t val, size_t addr, bool big_endian = false);
  dr_error write32(uint32_t val, size_t addr, bool big_endian = false);

  void log(unsigned level, const char *message);
  void startMinigame();
  void finishMinigame();

  bool m_minigameActive = false;

  virtual dr_error doSetPlayerCharacter(unsigned index, dr_character character) = 0;
  virtual dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) = 0;
  virtual dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) = 0;
  virtual dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) = 0;
  virtual dr_error doSetPlayerTeam(unsigned index, dr_team team) = 0;

signals:
  void minigameFinished();
  void logMessage(unsigned level, const QString &message);
};

#endif
