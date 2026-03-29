#include "DrGuestList.h"

#include <QRandomGenerator>
#include <QWidget>

DrGuestList::DrGuestList(QWidget *parent) : QStackedWidget(parent)
{
}

void DrGuestList::add(DrGuest *guest)
{
  m_guests.append(guest);
  addWidget(QWidget::createWindowContainer(guest, this));
  connect(guest, &DrGuest::minigameFinished, this, [this, guest]() {
    if (guest == m_activeGuest)
      emit minigameFinished();
  });
}

DrGuest* DrGuestList::pickMinigame(dr_minigame_type type, const dr_mp_minigame_t *&outMinigame)
{
  // Collect eligible guests and their matching minigame pointers
  struct EligibleGuest { DrGuest *guest; int index; QList<const dr_mp_minigame_t*> minigames; };
  QList<EligibleGuest> eligible;

  for (int i = 0; i < m_guests.size(); i++) {
    QList<const dr_mp_minigame_t*> minigames;
    for (const dr_mp_minigame_t *mg = m_guests[i]->minigames(); mg->name; mg++)
      if (mg->type == type && mg->minigame_id != 0xFF)
        minigames.append(mg);
    if (!minigames.isEmpty())
      eligible.append({m_guests[i], i, minigames});
  }

  if (eligible.isEmpty())
    return nullptr;

  // Pick a guest with equal probability, then a random minigame from that guest
  const auto &picked = eligible[QRandomGenerator::global()->bounded(eligible.size())];
  outMinigame = picked.minigames[QRandomGenerator::global()->bounded(picked.minigames.size())];

  log(DR_LOG_INFO, qPrintable(QString("guest: %1").arg(picked.guest->name())));
  log(DR_LOG_INFO, qPrintable(QString("minigame: %1 (0x%2)").arg(outMinigame->name).arg(outMinigame->minigame_id, 2, 16, QChar('0'))));

  m_activeGuest = picked.guest;
  setCurrentIndex(picked.index);
  return picked.guest;
}

void DrGuestList::logSummary()
{
  static const char *typeNames[DR_MINIGAME_SIZE] = {
    nullptr, "4P", "1v3", "2v2", "1P", "Battle", "Duel", "Item"
  };

  QStringList gameNames;
  unsigned typeCounts[DR_MINIGAME_SIZE] = {};
  unsigned total = 0;

  for (DrGuest *guest : m_guests) {
    gameNames.append(QString::fromUtf8(guest->name()));
    for (const dr_mp_minigame_t *mg = guest->minigames(); mg->name; mg++) {
      if (mg->type < DR_MINIGAME_SIZE)
        typeCounts[mg->type]++;
      total++;
    }
  }

  log(DR_LOG_INFO, qPrintable(QString("%1 game(s) loaded: %2")
    .arg(m_guests.size()).arg(gameNames.join(", "))));

  QStringList typeParts;
  for (unsigned t = 1; t < DR_MINIGAME_SIZE; t++)
    if (typeCounts[t] > 0)
      typeParts.append(QString("%1x %2").arg(typeCounts[t]).arg(typeNames[t]));

  log(DR_LOG_INFO, qPrintable(QString("%1 minigame(s): %2")
    .arg(total).arg(typeParts.join(", "))));
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

DrGuest* DrGuestList::startMinigame(dr_minigame_type type)
{
  const dr_mp_minigame_t *minigame = nullptr;
  DrGuest *guest = pickMinigame(type, minigame);
  if (guest)
    guest->setMinigame(minigame);
  return guest;
}
