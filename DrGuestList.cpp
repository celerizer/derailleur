#include "DrGuestList.h"

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

DrGuest *DrGuestList::pickMinigame(
  dr_minigame_type type, dr_mic_mode mic, const dr_mp_minigame_t *&outMinigame)
{
  struct EligibleGroup
  {
    DrGuest *guest;
    const char *name;
    QList<const dr_mp_minigame_t *> minigames;
  };
  QList<EligibleGroup> eligible;

  const bool netplay = dr_netplay_active();

  for (int i = 0; i < m_guests.size(); i++)
  {
    for (const DrMinigameGroup &group : m_guests[i]->minigameGroups())
    {
      QList<const dr_mp_minigame_t *> minigames;
      for (const dr_mp_minigame_t *mg : group.minigames)
      {
        if (mg->type == type && mg->minigame_id != 0xFF
            && !m_disabled.contains(dr_minigame_key(group.id, mg))
            && !(netplay && mg->flags.flags.no_netplay)
            && !(mic == DR_MIC_OFF && mg->flags.flags.mic)
            && !(mic == DR_MIC_ONLY && !mg->flags.flags.mic))
          minigames.append(mg);
      }
      if (!minigames.isEmpty())
        eligible.append({ m_guests[i], group.name, minigames });
    }
  }

  if (eligible.isEmpty())
    return nullptr;

  // Pick a group with equal probability, then a random minigame from that group.
  const auto &picked = eligible[dr_rand() % eligible.size()];
  outMinigame = picked.minigames[dr_rand() % picked.minigames.size()];

  return picked.guest;
}

/// Label for the log line; the plain set carries no suffix.
static const char *dr_mic_mode_suffix(dr_mic_mode mic)
{
  switch (mic)
  {
  case DR_MIC_OFF:
    return " (no mic)";
  case DR_MIC_ONLY:
    return " (mic only)";
  default:
    return "";
  }
}

void DrGuestList::rerollMinigames(void)
{
  for (unsigned t = 1; t < DR_MINIGAME_SIZE; t++)
  {
    for (unsigned m = 0; m < DR_MIC_SIZE; m++)
    {
      const dr_mic_mode mic = (dr_mic_mode)m;
      QStringList entries;

      for (DrMinigameCandidate &c : m_candidates[t][m])
      {
        const dr_mp_minigame_t *mg = nullptr;
        c.guest = pickMinigame((dr_minigame_type)t, mic, mg);
        c.minigame = mg;

        if (c.guest && c.minigame)
        {
          entries.append(QString("%1 -> %2 (0x%3)")
                         .arg(c.guest->name())
                         .arg(c.minigame->name)
                         .arg(c.minigame->minigame_id, 2, 16, QChar('0')));
        }
      }

      if (!entries.isEmpty())
      {
        log(DR_LOG_INFO, qPrintable(QString("%1 mini-games%2: %3")
                             .arg(dr_minigame_type_name((dr_minigame_type)t))
                             .arg(dr_mic_mode_suffix(mic))
                             .arg(entries.join(", "))));
      }
    }
  }
  m_rolled = true;
}

const std::array<DrMinigameCandidate, 5> &DrGuestList::minigameCandidates(
  dr_minigame_type type, dr_mic_mode mic)
{
  /* The first query rolls the whole cache; do it here so the roll lands on the
   * host's lockstepped frame and every netplay peer stays in sync. */
  if (!m_rolled)
    rerollMinigames();

  static const std::array<DrMinigameCandidate, 5> empty = {};
  if (type <= DR_MINIGAME_INVALID || type >= DR_MINIGAME_SIZE || mic >= DR_MIC_SIZE)
    return empty;
  return m_candidates[type][mic];
}

void DrGuestList::applyFilter(const QByteArray &payload)
{
  m_disabled = dr_minigame_filter_decode(payload);
  log(DR_LOG_INFO, qPrintable(QString("minigame filter: %1 disabled").arg(m_disabled.size())));

  /* The cached candidates were rolled against the old filter, so they may now be
   * disabled (or a freshly-enabled game may be missing). Invalidate the cache
   * rather than reroll here: applyFilter arrives on an async control packet, not
   * a lockstepped frame, so rerolling now could desync the shared PRNG. Clearing
   * m_rolled defers the reroll to the next query, which happens inside the host's
   * lockstepped run() -- so every netplay peer rerolls together. */
  m_rolled = false;
}

bool DrGuestList::guestHasCandidate(DrGuest *guest) const
{
  if (!m_guests.contains(guest))
    return false;

  for (const DrMinigameGroup &group : guest->minigameGroups())
    for (const dr_mp_minigame_t *mg : group.minigames)
      if (mg->minigame_id != 0xFF && !m_disabled.contains(dr_minigame_key(group.id, mg)))
        return true;
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

