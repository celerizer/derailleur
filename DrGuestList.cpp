#include "DrGuestList.h"

#include <QRandomGenerator>
#include <QWidget>
#include <cstdio>

DrGuestList::DrGuestList(QWidget *parent) : QStackedWidget(parent)
{
}

void DrGuestList::add(DrGuest *guest)
{
  m_guests.append(guest);
  addWidget(QWidget::createWindowContainer(guest, this));
  connect(guest, &DrGuest::minigameFinished, this, &DrGuestList::minigameFinished);
}

DrGuest* DrGuestList::startMinigame(dr_minigame_type type)
{
  // Collect eligible guests (those with at least one matching minigame)
  struct EligibleGuest { DrGuest *guest; int index; QList<unsigned> ids; };
  QList<EligibleGuest> eligible;

  for (int i = 0; i < m_guests.size(); i++) {
    QList<unsigned> ids;
    for (const dr_mp_minigame_t *mg = m_guests[i]->minigames(); mg->name; mg++)
      if (mg->type == type && mg->minigame_id != 0xFF)
        ids.append(mg->minigame_id);
    if (!ids.isEmpty())
      eligible.append({m_guests[i], i, ids});
  }

  if (eligible.isEmpty())
    return nullptr;

  // Pick a guest with equal probability, then a random minigame from that guest
  const auto &picked = eligible[QRandomGenerator::global()->bounded(eligible.size())];
  unsigned id = picked.ids[QRandomGenerator::global()->bounded(picked.ids.size())];

  for (const dr_mp_minigame_t *mg = picked.guest->minigames(); mg->name; mg++)
    if (mg->minigame_id == id)
      printf("minigame: %s (0x%02X)\n", mg->name, id);

  picked.guest->setMinigame(id);
  setCurrentIndex(picked.index);
  return picked.guest;
}
