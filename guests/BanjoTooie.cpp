#include "BanjoTooie.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QRetro.h>

/* A freshly booted core can't be unserialized on its first frame; let it run its
 * init for a bit first, then load the state. */
static const int BT_BOOT_WARMUP_FRAMES = 16;

static const char *BT_HIRES_DIR =
  "system/Mupen64plus/hires_texture/BANJO TOOIE/GLideNHQ";

/* CI player-indicator icons shared by the two kickball mini-games, one per in-game
 * slot (controller port). */
static const char *BT_PLAYER_ICON_FILES[4] = {
  "BANJO TOOIE#124F578A#2#1#B17CA06D_ciByRGBA.png",
  "BANJO TOOIE#EB14DCFF#2#1#870FD8D2_ciByRGBA.png",
  "BANJO TOOIE#BCF739DC#2#1#789E4EF4_ciByRGBA.png",
  "BANJO TOOIE#F3AC9DD5#2#1#10841D23_ciByRGBA.png",
};

/* The native icon texture is 32x32; inject at an exact integer multiple of that
 * (2x here) so GLideN64 accepts the replacement. */
static const int BT_ICON_W = 64;
static const int BT_ICON_H = 64;

/* Per-slot outline colors: P1 blue, P2 red, P3 yellow, P4 purple. */
static const QColor BT_SLOT_COLORS[4] = {
  QColor(0x33, 0x66, 0xFF),
  QColor(0xE2, 0x2B, 0x2B),
  QColor(0xF2, 0xC5, 0x1D),
  QColor(0x9B, 0x51, 0xE0),
};

/* Thickness (in injected pixels) of the colored outline around the head. */
static const int BT_OUTLINE = 3;

/* Loads srcPath, scales the head to fit within w x h (leaving room for the
 * outline), then draws a solid colored outline hugging its silhouette. Returns a
 * null image if the source is missing. */
static QImage btOutlinedIcon(const QString &srcPath, int w, int h, const QColor &color)
{
  QImage art(srcPath);
  if (art.isNull())
    return QImage();

  const QImage head = art.scaled(
    w - 2 * BT_OUTLINE, h - 2 * BT_OUTLINE, Qt::KeepAspectRatio, Qt::FastTransformation);

  /* A solid-color copy of the head shape (color wherever the head is opaque). */
  QImage silhouette(head.size(), QImage::Format_ARGB32);
  silhouette.fill(Qt::transparent);
  {
    QPainter sp(&silhouette);
    sp.drawImage(0, 0, head);
    sp.setCompositionMode(QPainter::CompositionMode_SourceIn);
    sp.fillRect(silhouette.rect(), color);
  }

  QImage icon(w, h, QImage::Format_ARGB32);
  icon.fill(Qt::transparent);
  QPainter p(&icon);
  const int hx = (w - head.width()) / 2;
  const int hy = (h - head.height()) / 2;
  /* Stamp the silhouette all around the head to grow a solid outline, then the
   * head itself on top. */
  for (int dy = -BT_OUTLINE; dy <= BT_OUTLINE; dy++)
    for (int dx = -BT_OUTLINE; dx <= BT_OUTLINE; dx++)
      if (dx || dy)
        p.drawImage(hx + dx, hy + dy, silhouette);
  p.drawImage(hx, hy, head);
  return icon;
}

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
  , m_gamePath(dr_roms_directory() + "/Banjo-Tooie (USA).z64")
{
  m_retro = new DrRetroN64(this);
  QRetro *c = new QRetro();
  c->setSavingEnabled(false);

  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  if (!c->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }

  /* Just make sure the ROM exists -- content is loaded on the first launch so the
   * per-character hires icons can be laid down before GLideN64 scans them. */
  if (!QFile::exists(m_gamePath))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(m_gamePath)));
    m_valid = false;
  }

  m_retro->setCore(c, true);
  m_retro->applyN64Remaps();
}

QWidget *BanjoTooie::createWidget(QWidget *parent)
{
  /* Capture the window container so we can resize the core to it once the deferred
   * core boots (see run()). */
  m_container = DrGuest::createWidget(parent);
  return m_container;
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

  /* Keep the core silent while the loading overlay is up. On the first launch the
   * core boots after mainwindow's mute ran (audio() was still null), so mute here
   * each frame until the minigame starts; mainwindow unmutes on minigameStarted. */
  if (!m_minigameActive)
    if (auto *a = core()->audio())
      a->setMute(true);

  /* Once the boot warmup elapses, load the state, poke the selected mini-game, and
   * arm the confirming A press. */
  if (m_stateLoadCountdown > 0 && --m_stateLoadCountdown == 0)
  {
    core()->unserializeFromFile(dr_state_directory() + "/banjotooie.state.zip");

    uint16_t id = static_cast<uint16_t>(m_minigame ? m_minigame->minigame_id : 0);
    m_retro->writeForFrames(BT_MINIGAME_ID_ADDR, &id, sizeof(id), 60);
    m_aPressDelay = 60;
    m_stateLoaded = true;

    if (m_pendingResize)
    {
      m_pendingResize = false;
      if (m_container)
        core()->resize(m_container->width(), m_container->height());
    }
  }

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

  if (!m_stateLoaded)
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

  /* Colosseum Kickball is scored in reverse: the lowest score wins. */
  const bool lowestWins =
    m_minigame && m_minigame->minigame_id == BT_MINIGAME_COLOSSEUM_KICKBALL_CHALLENGE;

  unsigned winners = 0;
  /* Scores are indexed by in-game slot (controller port); map the winning slot(s)
   * back to the board player index. */
  if (lowestWins)
  {
    int16_t best = score[0];
    for (unsigned i = 1; i < 4; i++)
      if (score[i] < best)
        best = score[i];
    for (unsigned slot = 0; slot < 4; slot++)
      if (score[slot] == best)
        winners |= (1u << m_slotToIndex[slot]);
  }
  else
  {
    int16_t best = 0;
    for (unsigned i = 0; i < 4; i++)
      if (score[i] > best)
        best = score[i];
    if (best > 0)
      for (unsigned slot = 0; slot < 4; slot++)
        if (score[slot] == best)
          winners |= (1u << m_slotToIndex[slot]);
  }
  return winners;
}

void BanjoTooie::writePlayerIcon(unsigned slot, dr_character character)
{
  if (slot >= 4)
    return;

  /* Implant the character's face over the kickball player indicator via GLideN64's
   * hi-res texture pack feature (replace the whole CI texture with our RGBA icon). */
  const int c = static_cast<int>(character);
  const QString destDir = QString::fromUtf8(BT_HIRES_DIR);
  QDir().mkpath(destDir);

  const QImage icon = btOutlinedIcon(
    QString(":/assets/player-32px/%1.png").arg(c), BT_ICON_W, BT_ICON_H, BT_SLOT_COLORS[slot]);
  if (icon.isNull())
  {
    log(DR_LOG_WARN, qPrintable(QString("no 32px player icon for character %1").arg(c)));
    return;
  }

  const QString dest = destDir + "/" + QString::fromUtf8(BT_PLAYER_ICON_FILES[slot]);
  if (!icon.save(dest, "PNG"))
    log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(dest)));
}

void BanjoTooie::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_pointersSeen = false;
  m_stateLoaded = false;
  m_aPressDelay = 0;
  m_aReleaseDelay = 0;
  m_stateLoadCountdown = 0;

  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = data.players[i];
    m_slotToIndex[i] = i;
  }
  /* Map each in-game slot (controller port) back to its board player index, and lay
   * down that slot's kickball icon texture now, before the boot, so GLideN64 reads
   * them. State load + mini-game setup happen in run() after the boot warmup. */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = btSlot(m_players[i], i);
    m_slotToIndex[slot] = i;
    writePlayerIcon(slot, data.players[i].character);
  }

  if (!m_started)
  {
    m_started = true;
    core()->loadContent(m_gamePath.toUtf8().constData());
    startCore(); // connects frameBegin -> run() and boots
    m_stateLoadCountdown = BT_BOOT_WARMUP_FRAMES;
    m_pendingResize = true; // resize the window once it's actually up (see run)
  }
  else
  {
    m_stateLoadCountdown = 1; // already booted; load on the next frame
  }

  /* startMinigame() is deferred to run(), once the mini-game's heap objects are
   * populated -- keeps the loading overlay up and the audio muted until then. */
}

dr_minigame_result_t BanjoTooie::minigameResult(unsigned index)
{
  const unsigned winners = computeWinners();
  return { (winners & (1u << index)) ? 10 : 0, 0 };
}
