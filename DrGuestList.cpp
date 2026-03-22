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
  struct Candidate { DrGuest *guest; int index; unsigned id; };
  QList<Candidate> candidates;

  for (int i = 0; i < m_guests.size(); i++)
    for (const dr_mp_minigame_t *mg = m_guests[i]->minigames(); mg->name; mg++)
      if (mg->type == type && mg->minigame_id != 0xFF)
        candidates.append({m_guests[i], i, mg->minigame_id});

  if (candidates.isEmpty())
    return nullptr;

  const auto &chosen = candidates[QRandomGenerator::global()->bounded(candidates.size())];

  for (const dr_mp_minigame_t *mg = chosen.guest->minigames(); mg->name; mg++)
    if (mg->minigame_id == chosen.id)
      printf("minigame: %s (0x%02X)\n", mg->name, chosen.id);

  chosen.guest->setMinigame(chosen.id);
  setCurrentIndex(chosen.index);
  return chosen.guest;
}
