#include "DrGuestList.h"

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
    for (const DrMinigameGroup &group : m_guests[i]->minigameGroups())
    {
      QList<const dr_mp_minigame_t *> minigames;
      for (const dr_mp_minigame_t *mg : group.minigames)
        if (mg->type == type && mg->minigame_id != 0xFF)
          minigames.append(mg);
      if (!minigames.isEmpty())
        eligible.append({ m_guests[i], i, group.name, minigames });
    }
  }

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
