#include "BanjoTooie.h"

#include <QRetro.h>

// s16 per-player score
static const size_t BT_SCORE_ADDRS[4] = {
  0x8012778C,
  0x8012778E,
  0x80127790,
  0x80127792,
};

// Two consecutive heap pointers the mini-game allocates while running. We wait
// for them to become non-null (the game has started), then treat them going
// null again as the mini-game being over.
static const size_t BT_HEAP_PTR_ADDRS[2] = {
  0x80127730,
  0x80127734,
};

/// @warning If the state data changes this needs to be updated, it's heap-allocated -mgmt
static const size_t BT_MINIGAME_ID_ADDR = 0x80191646;

/* In-game player slot for a board player, from their controller port. Banjo-Tooie
 * numbers its players linearly 0-3 by controller port, but Mario Party assigns
 * ports non-linearly, so scores/winners come back per port and must be mapped
 * back to the board index (falling back to the board index if out of range). */
static unsigned btSlot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

typedef enum
{
  BT_MINIGAME_TARGITZANS_TEMPLE_SHOOTOUT = 0,
  BT_MINIGAME_MAYAN_KICKBALL_CHALLENGE = 1,
  BT_MINIGAME_ORDINANCE_STORAGE_SHOOTOUT = 2,
  BT_MINIGAME_DODGEMS_CHALLENGE = 3,
  BT_MINIGAME_HOOP_HURRY_CHALLENGE = 4,
  BT_MINIGAME_BALLOON_BURST_CHALLENGE = 5,
  BT_MINIGAME_MINI_SUB_SHOOTOUT = 6,
  BT_MINIGAME_CHOMPAS_BELLY_CHALLENGE = 7,
  BT_MINIGAME_CLINKERS_CAVERN_SHOOTOUT = 8,
  BT_MINIGAME_PACKING_ROOM_CHALLENGE = 9,
  BT_MINIGAME_COLOSSEUM_KICKBALL_CHALLENGE = 10,
  BT_MINIGAME_TRASH_CAN_CHALLENGE = 11,
  BT_MINIGAME_ZUBBAS_NEST_SHOOTOUT = 12,
  BT_MINIGAME_TOWER_OF_TRAGEDY_QUIZ = 13,
} banjotooie_minigame_id;

static const dr_mp_minigame_t BT_MINIGAMES[] = {
  // { "Targitzan's Temple Shootout", DR_MINIGAME_4P, BT_MINIGAME_TARGITZANS_TEMPLE_SHOOTOUT, 0xFF, DR_NO_QUIRKS },
  { "Mayan Kickball Challenge", DR_MINIGAME_4P, BT_MINIGAME_MAYAN_KICKBALL_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  // { "Ordinance Storage Shootout", DR_MINIGAME_4P, BT_MINIGAME_ORDINANCE_STORAGE_SHOOTOUT, 0xFF, DR_NO_QUIRKS },
  { "Dodgems Challenge", DR_MINIGAME_4P, BT_MINIGAME_DODGEMS_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  { "Hoop Hurry Challenge", DR_MINIGAME_4P, BT_MINIGAME_HOOP_HURRY_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  { "Balloon Burst Challenge", DR_MINIGAME_4P, BT_MINIGAME_BALLOON_BURST_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  // { "Mini-Sub Shootout", DR_MINIGAME_4P, BT_MINIGAME_MINI_SUB_SHOOTOUT, 0xFF, DR_NO_QUIRKS },
  { "Chompa's Belly Challenge", DR_MINIGAME_4P, BT_MINIGAME_CHOMPAS_BELLY_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  // { "Clinker's Cavern Shootout", DR_MINIGAME_4P, BT_MINIGAME_CLINKERS_CAVERN_SHOOTOUT, 0xFF, DR_NO_QUIRKS },
  { "Packing Room Challenge", DR_MINIGAME_4P, BT_MINIGAME_PACKING_ROOM_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  { "Colosseum Kickball Challenge", DR_MINIGAME_4P, BT_MINIGAME_COLOSSEUM_KICKBALL_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  { "Trash Can Challenge", DR_MINIGAME_4P, BT_MINIGAME_TRASH_CAN_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  // { "Zubba's Nest Shootout", DR_MINIGAME_4P, BT_MINIGAME_ZUBBAS_NEST_SHOOTOUT, 0xFF, DR_NO_QUIRKS },
  // { "Tower of Tragedy Quiz", DR_MINIGAME_4P, BT_MINIGAME_TOWER_OF_TRAGEDY_QUIZ, 0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

BanjoTooie::BanjoTooie(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  QString gamePath = dr_roms_directory() + "/Banjo-Tooie (USA).z64";
  QRetro *c = new QRetro();
  c->setSavingEnabled(false);
  if (!c->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!c->loadContent(gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(c, true);
  m_retro->applyN64Remaps();
}

void BanjoTooie::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void BanjoTooie::run()
{
  m_retro->tickFrameWrites();

  /* A few frames after the write, force a P1 A press to confirm the menu, then
   * release it shortly after so it reads as a discrete press. */
  if (m_aPressDelay > 0 && --m_aPressDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
    m_aReleaseDelay = 8;
  }
  else if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);
  }

  if (m_finishCountdown > 0)
  {
    if (--m_finishCountdown == 0)
      finishMinigame();
    return;
  }

  if (!m_minigame)
    return;

  /* The mini-game allocates a pair of heap objects when it actually starts. Keep
   * the loading overlay up and the core muted until both are populated, then start
   * the mini-game -- startMinigame() drops the overlay and unmutes. When they clear
   * again, the mini-game is over. */
  uint32_t ptr[2] = {};
  bool ok = m_retro->readu32(&ptr[0], BT_HEAP_PTR_ADDRS[0]) == DR_OK
    && m_retro->readu32(&ptr[1], BT_HEAP_PTR_ADDRS[1]) == DR_OK;
  if (!ok)
    return;

  if (!m_pointersSeen)
  {
    if (ptr[0] && ptr[1])
    {
      m_pointersSeen = true;
      startMinigame();
    }
  }
  else if (!ptr[0] && !ptr[1])
  {
    int16_t score[4] = {};
    for (unsigned i = 0; i < 4; i++)
      m_retro->reads16(&score[i], BT_SCORE_ADDRS[i]);
    log(DR_LOG_INFO, qPrintable(QString("BT final scores (by port): %1, %2, %3, %4")
                       .arg(score[0]).arg(score[1]).arg(score[2]).arg(score[3])));
    finishMinigame();
  }
}

const dr_mp_minigame_t *BanjoTooie::minigames() const
{
  return BT_MINIGAMES;
}

unsigned BanjoTooie::computeWinners()
{
  int16_t score[4] = {};
  for (unsigned i = 0; i < 4; i++)
    m_retro->reads16(&score[i], BT_SCORE_ADDRS[i]);

  int16_t best = 0;
  for (unsigned i = 0; i < 4; i++)
    if (score[i] > best)
      best = score[i];

  /* Scores are indexed by in-game slot (controller port); map the winning slot(s)
   * back to the board player index. */
  unsigned winners = 0;
  if (best > 0)
    for (unsigned slot = 0; slot < 4; slot++)
      if (score[slot] == best)
        winners |= (1u << m_slotToIndex[slot]);
  return winners;
}

void BanjoTooie::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_pointersSeen = false;
  m_aPressDelay = 0;
  m_aReleaseDelay = 0;

  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = data.players[i];
    m_slotToIndex[i] = i;
  }
  /* Map each in-game slot (controller port) back to its board player index. */
  for (unsigned i = 0; i < 4; i++)
    m_slotToIndex[btSlot(m_players[i], i)] = i;

  core()->unserializeFromFile(dr_state_directory() + "/banjotooie.state.zip");

  /* Hold the selected mini-game in the menu for a bit, then confirm it with a P1 A
   * press a few frames later (see run()) once the menu has settled after the load. */
  uint16_t id = static_cast<uint16_t>(data.minigame->minigame_id);
  m_retro->writeForFrames(BT_MINIGAME_ID_ADDR, &id, sizeof(id), 60);
  m_aPressDelay = 60;

  /* TODO: write each player's character/controller. */

  /* startMinigame() is deferred to run(), once the mini-game's heap objects are
   * populated -- keeps the loading overlay up and the audio muted until then. */
}

dr_minigame_result_t BanjoTooie::minigameResult(unsigned index)
{
  const unsigned winners = computeWinners();
  return { (winners & (1u << index)) ? 10 : 0, 0 };
}
