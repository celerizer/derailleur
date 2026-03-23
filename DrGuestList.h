#ifndef DR_GUEST_LIST_H
#define DR_GUEST_LIST_H

#include "DrGuest.h"
#include <QList>
#include <QStackedWidget>

class DrGuestList : public QStackedWidget
{
  Q_OBJECT

public:
  DrGuestList(QWidget *parent = nullptr);

  void add(DrGuest *guest);
  const QList<DrGuest*>& guests() const { return m_guests; }
  DrGuest* currentGuest() const { return m_guests.value(currentIndex()); }

  DrGuest* startMinigame(dr_minigame_type type);

signals:
  void minigameFinished();
  void logMessage(unsigned level, const QString &message);

private:
  void log(unsigned level, const char *message) { emit logMessage(level, QString::fromUtf8(message)); }
  QList<DrGuest*> m_guests;
};

#endif
