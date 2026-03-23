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
  virtual void setMinigame(unsigned id) = 0;

  dr_error setPlayer(unsigned index, const dr_player_t &player)
  {
    if (index >= 4) return DR_ERR_INVALID_PARAMETER;
    setPlayerCharacter(index, player.character);
    setPlayerControlPort(index, player.control_port);
    setPlayerControlType(index, player.control_type);
    setPlayerDifficulty(index, player.difficulty);
    setPlayerTeam(index, player.team);
    return DR_OK;
  }

  dr_error setPlayerCharacter(unsigned index, dr_character character)
  {
    if (index >= 4 || character >= DR_CHARACTER_SIZE) return DR_ERR_INVALID_PARAMETER;
    return doSetPlayerCharacter(index, character);
  }

  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port)
  {
    if (index >= 4 || control_port == DR_CONTROL_PORT_INVALID || control_port >= DR_CONTROL_PORT_SIZE)
      return DR_ERR_INVALID_PARAMETER;
    return doSetPlayerControlPort(index, control_port);
  }

  dr_error setPlayerControlType(unsigned index, dr_control_type control_type)
  {
    if (index >= 4 || control_type == DR_CONTROL_TYPE_INVALID || control_type >= DR_CONTROL_TYPE_SIZE)
      return DR_ERR_INVALID_PARAMETER;
    return doSetPlayerControlType(index, control_type);
  }

  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
  {
    if (index >= 4 || difficulty >= DR_DIFFICULTY_SIZE) return DR_ERR_INVALID_PARAMETER;
    return doSetPlayerDifficulty(index, difficulty);
  }

  dr_error setPlayerTeam(unsigned index, dr_team team)
  {
    if (index >= 4 || team >= DR_TEAM_SIZE) return DR_ERR_INVALID_PARAMETER;
    return doSetPlayerTeam(index, team);
  }

protected:
  dr_error read8(uint8_t *out, size_t addr, bool big_endian = false) {
    (void)big_endian;
    return memory().readValue<uint8_t>(out, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
  }

  dr_error read16(uint16_t *out, size_t addr, bool big_endian = false) {
    if (!memory().readValue<uint16_t>(out, addr)) return DR_ERR_MEMORY_ACCESS_CORE;
    if (big_endian) *out = qbswap(*out);
    return DR_OK;
  }

  dr_error read32(uint32_t *out, size_t addr, bool big_endian = false) {
    if (!memory().readValue<uint32_t>(out, addr)) return DR_ERR_MEMORY_ACCESS_CORE;
    if (big_endian) *out = qbswap(*out);
    return DR_OK;
  }

  dr_error write8(uint8_t val, size_t addr, bool big_endian = false) {
    (void)big_endian;
    return memory().writeValue<uint8_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
  }

  dr_error write16(uint16_t val, size_t addr, bool big_endian = false) {
    if (big_endian) val = qbswap(val);
    return memory().writeValue<uint16_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
  }

  dr_error write32(uint32_t val, size_t addr, bool big_endian = false) {
    if (big_endian) val = qbswap(val);
    return memory().writeValue<uint32_t>(val, addr) ? DR_OK : DR_ERR_MEMORY_ACCESS_CORE;
  }

  virtual dr_error doSetPlayerCharacter(unsigned index, dr_character character) = 0;
  virtual dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) = 0;
  virtual dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) = 0;
  virtual dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) = 0;
  virtual dr_error doSetPlayerTeam(unsigned index, dr_team team) = 0;

signals:
  void minigameFinished();
  void logMessage(unsigned level, const QString &message);

protected:
  void log(unsigned level, const char *message)
  {
    emit logMessage(level, QString::fromUtf8(message));
  }
};

#endif
