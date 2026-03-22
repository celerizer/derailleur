#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include <QRetro.h>

class DrGuest : public QRetro
{
  Q_OBJECT

public:
  DrGuest(QWindow *parent = nullptr) : QRetro(parent) {}

  virtual dr_minigame_result_t minigameResult(unsigned index) const = 0;

  virtual dr_error setPlayerCharacter(unsigned index, dr_character character) = 0;
  virtual dr_error setPlayerControlPort(unsigned index, dr_control_port control_port) = 0;
  virtual dr_error setPlayerControlType(unsigned index, dr_control_type control_type) = 0;
  virtual dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty) = 0;
  virtual dr_error setPlayerTeam(unsigned index, dr_team team) = 0;

signals:
  void minigameFinished();
};

#endif
