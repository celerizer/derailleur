#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrRetro.h"
#include <QList>
#include <QString>

struct DrMinigameGroup
{
  const char                    *name;
  QList<const dr_mp_minigame_t *> minigames;
};

class DrGuest : public DrRetro
{
  Q_OBJECT

public:
  // N64-style: subclass creates its own QRetro in its constructor and assigns m_core
  DrGuest(QObject *parent = nullptr) : DrRetro(parent) {}

  // GCN-style: receives a shared QRetro it does not own
  explicit DrGuest(QRetro *sharedCore, QObject *parent = nullptr)
    : DrRetro(sharedCore, parent) {}

  virtual dr_minigame_result_t    minigameResult(unsigned index) = 0;
  virtual const dr_mp_minigame_t *minigames() const = 0;
  virtual const char             *name(void) const = 0;
  virtual QList<DrMinigameGroup>  minigameGroups() const;
  void setMinigame(const dr_mp_minigame_t *minigame);

  dr_error setPlayer(unsigned index, const dr_player_t &player);
  dr_error setPlayerCharacter(unsigned index, dr_character character);
  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port);
  dr_error setPlayerControlType(unsigned index, dr_control_type control_type);
  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty);
  dr_error setPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id);

protected:
  void startMinigame();
  void finishMinigame();

  bool                    m_minigameActive = false;
  const dr_mp_minigame_t *m_minigame       = nullptr;

private:
  virtual void doSetMinigame(const dr_mp_minigame_t *minigame) = 0;
  virtual dr_error doSetPlayerCharacter(unsigned index, dr_character character) = 0;
  virtual dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) = 0;
  virtual dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) = 0;
  virtual dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) = 0;
  virtual dr_error doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) = 0;

signals:
  void minigameFinished();
};

#endif
