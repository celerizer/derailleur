#include "SonicShuffleHost.h"
#include "SonicShuffleHand.h"

#include <QRetro.h>
#include <QString>

/* ------------------------------------------------------------------------- *
 *  Memory map (Flycast RAM, little-endian -- DrRetro's default endianness).
 *  @todo Fill these in. The guest side (guests/SonicShuffle.cpp) already maps a
 *  lot of the mini-game setup block around 0x21D8xx and the 0x200-byte player
 *  structs at 0x21D940; the board-side addresses the host needs live elsewhere.
 * ------------------------------------------------------------------------- */

/* u16 - current scene/board id. @todo find the real address. */
// static const size_t SSH_SCENE_ADDR = 0x000000;

/* @todo per-player board state (character / rings / precioustones / cpu), the
 * roulette / mini-game-type trigger, and where a chosen mini-game's results are
 * written back. */

/* Per-player board state: 0x200-byte structs from SSH_PLAYER_BASE. Mirrors the
 * subset of the guest's ss_player_t (guests/SonicShuffle.cpp) we need here. */
static const size_t SSH_PLAYER_BASE = 0x21D940;
static const size_t SSH_PLAYER_STRIDE = 0x200;
static const size_t SSH_CHARACTER_OFFSET = 0x04;     /* u8: 1 Sonic, 2 Tails, ... */
static const size_t SSH_CARDS_IN_HAND_OFFSET = 0x1C; /* u8[7] */
static const size_t SSH_NUM_CARDS_OFFSET = 0x23;     /* u8 */

/* Card-selection cursor while choosing a card (you can pick from another player's
 * hand, so the target player is tracked separately). */
static const size_t SSH_SEL_CARD_INDEX_ADDR = 0x0032E4F0;  /* u32: card index in hand */
static const size_t SSH_SEL_CARD_PLAYER_ADDR = 0x0032E4F4; /* u32: whose hand (0-indexed) */
static const size_t SSH_SEL_CARD_CHOSEN_ADDR = 0x0032E538; /* u32 bool: selection confirmed */

/* Address of `off` within player p's struct (p = 0..3). */
#define SSH_PLAYER_ADDR(p, off) \
  (SSH_PLAYER_BASE + static_cast<size_t>(p) * SSH_PLAYER_STRIDE + (off))

SonicShuffleHost::SonicShuffleHost(QObject *parent)
  : DrHost(parent)
{
  const QString corePath = dr_core_path(DR_CORE_FLYCAST);
  m_gamePath = dr_roms_directory() + "/Sonic Shuffle (USA).chd";

  m_core = new QRetro();
  m_ownCore = true;

  if (!m_core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
    return;
  }

  /* Dreamcast RAM is little-endian, which is DrRetro's default -- no override.
   * Content is loaded lazily in startCore(); see the header for why. */

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);

  /* The hand panel lives on the GUI thread; pollLocalHand runs on the core's
   * timing thread, so the update is delivered as a queued signal. */
  m_hand = new SonicShuffleHand();
  connect(this, &SonicShuffleHost::localHandChanged, m_hand, &SonicShuffleHand::setHand,
    Qt::QueuedConnection);
  m_hand->show();
}

SonicShuffleHost::~SonicShuffleHost(void)
{
  delete m_hand;
}

void SonicShuffleHost::startCore(void)
{
  /* Load content now, with a native window surface already in place (mainwindow
   * creates the host container before calling this). Flycast crashes if content
   * loads earlier, the same reason the guest defers it to its first launch. */
  if (!m_contentLoaded && m_core)
  {
    m_contentLoaded = true;
    if (!m_core->loadContent(m_gamePath.toUtf8().constData()))
    {
      log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(m_gamePath)));
      m_valid = false;
      return;
    }
  }

  DrRetro::startCore();
}

void SonicShuffleHost::pollLocalHand(void)
{
  const int p = m_LocalPlayer;
  if (p < 0 || p > 3)
    return;

  uint8_t character = 0;
  readu8(&character, SSH_PLAYER_ADDR(p, SSH_CHARACTER_OFFSET));

  uint8_t num = 0;
  readu8(&num, SSH_PLAYER_ADDR(p, SSH_NUM_CARDS_OFFSET));

  uint8_t hand[7] = {};
  for (unsigned i = 0; i < 7; i++)
    readu8(&hand[i], SSH_PLAYER_ADDR(p, SSH_CARDS_IN_HAND_OFFSET + i));

  /* Which of our cards to glow: only while a selection is in progress (not yet
   * confirmed) and the cursor is on our own hand. -1 = no glow. */
  uint32_t selIndex = 0, selPlayer = 0, selChosen = 0;
  readu32(&selIndex, SSH_SEL_CARD_INDEX_ADDR);
  readu32(&selPlayer, SSH_SEL_CARD_PLAYER_ADDR);
  readu32(&selChosen, SSH_SEL_CARD_CHOSEN_ADDR);
  const int highlight =
    (selChosen == 0 && selPlayer == static_cast<uint32_t>(p)) ? static_cast<int>(selIndex) : -1;

  /* Only report when something changed, so this doesn't spam every frame. */
  bool changed = (character != m_lastCharacter) || (num != m_lastNumCards) ||
    (highlight != m_lastHighlight);
  for (unsigned i = 0; i < 7 && !changed; i++)
    changed = (hand[i] != m_lastHand[i]);
  if (!changed)
    return;

  m_lastCharacter = character;
  m_lastNumCards = num;
  m_lastHighlight = highlight;
  for (unsigned i = 0; i < 7; i++)
    m_lastHand[i] = hand[i];

  /* Hand order, clamped to the 7-card array (num can read as garbage pre-boot). */
  const int count = qBound(0, static_cast<int>(num), 7);
  const QByteArray cards(reinterpret_cast<const char *>(hand), count);

  /* Queued to the widget on the GUI thread (see the constructor). */
  emit localHandChanged(static_cast<int>(character), cards, highlight);
}

void SonicShuffleHost::run(void)
{
  tickFrameWrites();

  pollLocalHand();

  /* @todo Board/roulette state machine. Rough shape (see MarioPartyN64Host::run
   * for the reference implementation):
   *
   *   1. Read the current scene id; edge-detect changes against m_lastScene.
   *   2. While on the board, watch for the mini-game roulette trigger and emit
   *      candidatesNeeded(type) so the picker fills m_candidates.
   *   3. Once the game commits to a slot, resolve the players and emit
   *      minigameRequested({guest, minigame}, players) to launch the guest.
   *   4. When the guest reports back, writeResults() pushes coins/rings onto the
   *      board and control returns here.
   */
}

void SonicShuffleHost::setCandidates(std::array<DrMinigameCandidate, 5> candidates)
{
  m_candidates = candidates;

  /* @todo Inject the five candidates' names/ids into the roulette, the way
   * MarioPartyN64Host::injectMinigameTitles does for the N64 hosts. */
}

void SonicShuffleHost::writeResults(DrGuest *guest)
{
  (void)guest;

  /* @todo Read each player's result from `guest->minigameResult(i)` and add the
   * coins/rings onto the board's per-player totals. */
}

void SonicShuffleHost::clearResults(void)
{
  /* @todo Zero the per-player result fields so a canceled mini-game doesn't leave
   * stale winnings for the board to read. */
}
