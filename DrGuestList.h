#ifndef DR_GUEST_LIST_H
#define DR_GUEST_LIST_H

#include "DrGuest.h"
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QStackedWidget>

class DrGuestList : public QStackedWidget
{
  Q_OBJECT

public:
  DrGuestList(QWidget *parent = nullptr);

  void add(DrGuest *guest);
  const QList<DrGuest *> &guests() const { return m_guests; }
  DrGuest *currentGuest() const { return m_guests.value(currentIndex()); }

  DrGuest *pickMinigame(dr_minigame_type type, const dr_mp_minigame_t *&outMinigame);
  bool activateGuest(DrGuest *guest);
  void logSummary();

  /// Replaces the set of disabled mini-games from an opaque payload produced by
  /// DrMinigameFilter ([u16 count][count * u32 key]). pickMinigame skips any
  /// mini-game whose key is disabled. Must be identical across netplay peers at
  /// selection time, so it is driven by the host.
  void applyFilter(const QByteArray &payload);

  /// True if the guest has at least one selectable mini-game (minigame_id valid)
  /// that is not disabled by the current filter. Guests with none should not be
  /// loaded at all. Derived purely from the shared filter, so it is identical
  /// across netplay peers.
  bool guestHasCandidate(DrGuest *guest) const;

signals:
  void minigameFinished();
  void minigameCanceled();
  void logMessage(unsigned level, const QString &message);

private:
  void log(unsigned level, const char *message)
  {
    emit logMessage(level, QString::fromUtf8(message));
  }
  QList<DrGuest *> m_guests;
  DrGuest *m_activeGuest = nullptr;
  QSet<quint32> m_disabled; // disabled mini-game keys: (guestIndex << 16) | ordinal
};

#endif
