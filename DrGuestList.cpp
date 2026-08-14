#include "DrGuestList.h"

#include <QDataStream>
#include <QWidget>

DrGuestList::DrGuestList(QWidget *parent)
  : QStackedWidget(parent)
{
  /* Move focus to the next widget */
  connect(this, &QStackedWidget::currentChanged, this, [this](int index) {
    QWidget *w = widget(index);
    setFocusProxy(w);
    if (w)
      w->setFocus();
  });
}

void DrGuestList::add(DrGuest *guest)
{
  m_guests.append(guest);
  addWidget(guest->createWidget(this));
  connect(guest, &DrGuest::minigameFinished, this, [this, guest]() {
    if (guest == m_activeGuest)
      emit minigameFinished();
  });
  connect(guest, &DrGuest::minigameCanceled, this, [this, guest]() {
    if (guest == m_activeGuest)
      emit minigameCanceled();
  });
}

DrGuest *DrGuestList::pickMinigame(dr_minigame_type type, const dr_mp_minigame_t *&outMinigame)
{
  struct EligibleGroup
  {
    DrGuest *guest;
    const char *name;
    QList<const dr_mp_minigame_t *> minigames;
  };
  QList<EligibleGroup> eligible;

  for (int i = 0; i < m_guests.size(); i++)
  {
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
        eligible.append({ m_guests[i], group.name, minigames });
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

  /* Picking does not activate the guest -- rerollMinigames rolls every type up
   * front and must not disturb what is on screen. The chosen guest is activated
   * only when its mini-game actually launches (see activateGuest). */
  return picked.guest;
}

void DrGuestList::rerollMinigames(void)
{
  for (unsigned t = 1; t < DR_MINIGAME_SIZE; t++)
    for (DrMinigameCandidate &c : m_candidates[t])
    {
      const dr_mp_minigame_t *mg = nullptr;
      c.guest = pickMinigame((dr_minigame_type)t, mg);
      c.minigame = mg;
    }
  m_rolled = true;
}

const std::array<DrMinigameCandidate, 5> &DrGuestList::minigameCandidates(dr_minigame_type type)
{
  /* The first query rolls the whole cache; do it here so the roll lands on the
   * host's lockstepped frame and every netplay peer stays in sync. */
  if (!m_rolled)
    rerollMinigames();

  static const std::array<DrMinigameCandidate, 5> empty = {};
  if (type <= DR_MINIGAME_INVALID || type >= DR_MINIGAME_SIZE)
    return empty;
  return m_candidates[type];
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

