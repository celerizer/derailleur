#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include "DrRetro.h"
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>

struct DrMinigameGroup
{
  const char *name;
  QList<const dr_mp_minigame_t *> minigames;
};

class DrGuest : public QObject
{
  Q_OBJECT

public:
  DrGuest(QObject *parent = nullptr)
    : QObject(parent)
  {
  }

  virtual QRetro *core() const { return nullptr; }
  virtual bool isValid() const { return m_valid; }
  virtual unsigned warmupFrames() const { return 30; }

  virtual void startCore() {}
  virtual void pause() {}
  virtual void unpause() {}
  virtual QWidget *createWidget(QWidget *parent) { return QWidget::createWindowContainer(core(), parent); }

  void tick()
  {
    if (m_finishCountdown > 0 && --m_finishCountdown == 0)
    {
      finishMinigame();
      return;
    }
    run();
  }

  virtual dr_minigame_result_t minigameResult(unsigned index) = 0;
  virtual const dr_mp_minigame_t *minigames() const = 0;
  virtual const char *name(void) const = 0;
  virtual dr_guest id(void) const { return DR_GUEST_INVALID; }
  virtual QList<DrMinigameGroup> minigameGroups() const;
  void setMinigame(const dr_mp_minigame_t *minigame);
  void cancelMinigame() { m_minigameActive = false; }

  dr_error setPlayer(unsigned index, const dr_player_t &player);
  dr_error setPlayerCharacter(unsigned index, dr_character character);
  dr_error setPlayerControlPort(unsigned index, dr_control_port control_port);
  dr_error setPlayerControlType(unsigned index, dr_control_type control_type);
  dr_error setPlayerDifficulty(unsigned index, dr_difficulty difficulty);
  dr_error setPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id);

protected:
  void log(unsigned level, const char *message) { emit logMessage(level, QString::fromUtf8(message)); }
  void startMinigame();
  void finishMinigame();
  void finishMinigameInFrames(int frames) { m_finishCountdown = frames; }

  virtual void run() {}
  virtual void doSetMinigame(const dr_mp_minigame_t *minigame) { (void)minigame; }
  virtual dr_error doSetPlayerCharacter(unsigned index, dr_character character) { (void)index; (void)character; return DR_OK; }
  virtual dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) { (void)index; (void)control_port; return DR_OK; }
  virtual dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) { (void)index; (void)control_type; return DR_OK; }
  virtual dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) { (void)index; (void)difficulty; return DR_OK; }
  virtual dr_error doSetPlayerTeam(unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) { (void)index; (void)color; (void)type; (void)team_id; return DR_OK; }

  bool m_valid = true;
  bool m_minigameActive = false;
  int m_finishCountdown = 0;
  const dr_mp_minigame_t *m_minigame = nullptr;

signals:
  void logMessage(unsigned level, const QString &message);
  void minigameFinished();
};

#endif
