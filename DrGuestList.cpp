#include "DrGuestList.h"

#include <QDataStream>
#include <QWidget>

DrGuestList::DrGuestList(QWidget *parent)
  : QStackedWidget(parent)
{
}

void DrGuestList::add(DrGuest *guest)
{
  m_guests.append(guest);
  addWidget(guest->createWidget(this));
  connect(guest, &DrGuest::minigameFinished, this, [this, guest]() {
    if (guest == m_activeGuest)
      emit minigameFinished();
  });
}

DrGuest *DrGuestList::pickMinigame(dr_minigame_type type, const dr_mp_minigame_t *&outMinigame)
{
  struct EligibleGroup
  {
    DrGuest *guest;
    int guestIndex;
    const char *name;
    QList<const dr_mp_minigame_t *> minigames;
  };
  QList<EligibleGroup> eligible;

  for (int i = 0; i < m_guests.size(); i++)
  {
    /* The ordinal flattens minigameGroups() in order and increments for every
     * mini-game (matching DrMinigameFilter), so disabled-keys line up. */
    quint32 ord = 0;
    for (const DrMinigameGroup &group : m_guests[i]->minigameGroups())
    {
      QList<const dr_mp_minigame_t *> minigames;
      for (const dr_mp_minigame_t *mg : group.minigames)
      {
        const quint32 key = (static_cast<quint32>(i) << 16) | ord++;
        if (mg->type == type && mg->minigame_id != 0xFF && !m_disabled.contains(key))
          minigames.append(mg);
      }
      if (!minigames.isEmpty())
        eligible.append({ m_guests[i], i, group.name, minigames });
    }
  }

  log(DR_LOG_INFO, qPrintable(QString("pick type=%1 eligible=%2 randcount=%3")
                       .arg((int)type).arg(eligible.size()).arg(dr_rand_count())));

  if (eligible.isEmpty())
    return nullptr;

  // Pick a group with equal probability, then a random minigame from that group.
  const auto &picked = eligible[dr_rand() % eligible.size()];
  outMinigame = picked.minigames[dr_rand() % picked.minigames.size()];

  log(DR_LOG_INFO, qPrintable(QString("guest: %1").arg(picked.name)));
  log(DR_LOG_INFO, qPrintable(QString("minigame: %1 (0x%2)")
                       .arg(outMinigame->name)
                       .arg(outMinigame->minigame_id, 2, 16, QChar('0'))));

  m_activeGuest = picked.guest;
  setCurrentIndex(picked.guestIndex);
  return picked.guest;
}

void DrGuestList::applyFilter(const QByteArray &payload)
{
  m_disabled.clear();
  QDataStream s(payload);
  s.setByteOrder(QDataStream::LittleEndian);
  quint16 count = 0;
  s >> count;
  for (quint16 i = 0; i < count && s.status() == QDataStream::Ok; i++)
  {
    quint32 key = 0;
    s >> key;
    m_disabled.insert(key);
  }
  log(DR_LOG_INFO, qPrintable(QString("minigame filter: %1 disabled").arg(m_disabled.size())));
}

bool DrGuestList::guestHasCandidate(DrGuest *guest) const
{
  const int gi = m_guests.indexOf(guest);
  if (gi < 0)
    return false;

  // Same ordinal scheme as pickMinigame / DrMinigameFilter.
  quint32 ord = 0;
  for (const DrMinigameGroup &group : guest->minigameGroups())
    for (const dr_mp_minigame_t *mg : group.minigames)
    {
      const quint32 key = (static_cast<quint32>(gi) << 16) | ord++;
      if (mg->minigame_id != 0xFF && !m_disabled.contains(key))
        return true;
    }
  return false;
}

void DrGuestList::logSummary()
{
  QStringList gameNames;
  unsigned typeCounts[DR_MINIGAME_SIZE] = {};
  unsigned total = 0;

  for (DrGuest *guest : m_guests)
  {
    for (const DrMinigameGroup &group : guest->minigameGroups())
    {
      gameNames.append(QString::fromUtf8(group.name));
      for (const dr_mp_minigame_t *mg : group.minigames)
      {
        if (mg->type < DR_MINIGAME_SIZE)
          typeCounts[mg->type]++;
        total++;
      }
    }
  }

  log(DR_LOG_INFO,
    qPrintable(QString("%1 game(s) loaded: %2").arg(gameNames.size()).arg(gameNames.join(", "))));

  QStringList typeParts;
  for (unsigned t = 1; t < DR_MINIGAME_SIZE; t++)
    if (typeCounts[t] > 0)
      typeParts.append(QString("%1x %2").arg(typeCounts[t]).arg(dr_minigame_type_name((dr_minigame_type)t)));

  log(DR_LOG_INFO, qPrintable(QString("%1 minigame(s): %2").arg(total).arg(typeParts.join(", "))));
}

bool DrGuestList::activateGuest(DrGuest *guest)
{
  int idx = m_guests.indexOf(guest);
  if (idx < 0)
    return false;
  m_activeGuest = guest;
  setCurrentIndex(idx);
  return true;
}

DrGuest *DrGuestList::startMinigame(dr_minigame_type type)
{
  const dr_mp_minigame_t *minigame = nullptr;
  DrGuest *guest = pickMinigame(type, minigame);
  if (guest)
    guest->setMinigame(minigame);
  return guest;
}
