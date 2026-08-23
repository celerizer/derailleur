#ifndef DR_GUEST_LIST_H
#define DR_GUEST_LIST_H

#include "DrGuest.h"
#include "DrHost.h"
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QStackedWidget>
#include <array>

class DrGuestList : public QStackedWidget, public DrMinigameSource
{
  Q_OBJECT

public:
  DrGuestList(QWidget *parent = nullptr);

  void add(DrGuest *guest);
  const QList<DrGuest *> &guests() const { return m_guests; }
  DrGuest *currentGuest() const { return m_guests.value(currentIndex()); }

  /// DrMinigameSource: the cached candidates for a type, and a full reroll. The
  /// cache is empty until the first query (see rerollMinigames), which keeps
  /// netplay peers rolling in lockstep. See DrMinigameSource for the contract.
  const std::array<DrMinigameCandidate, 5> &minigameCandidates(dr_minigame_type type) override;
  void rerollMinigames(void) override;

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

  /* Rolled candidate cache, indexed by dr_minigame_type. Empty (m_rolled false)
   * until the first query or reroll. */
  std::array<std::array<DrMinigameCandidate, 5>, DR_MINIGAME_SIZE> m_candidates = {};
  bool m_rolled = false;
};

#endif
