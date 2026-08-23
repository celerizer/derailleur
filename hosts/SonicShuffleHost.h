#ifndef DR_SONIC_SHUFFLE_HOST_H
#define DR_SONIC_SHUFFLE_HOST_H

#include "../DrHost.h"
#include <QByteArray>
#include <QString>
#include <array>

class SonicShuffleHand;

/// Sonic Shuffle (Dreamcast) host. Runs its own Flycast core, drives the board,
/// and hands mini-games off to guests. Skeleton: memory map and the board/roulette
/// state machine are still to be filled in -- see the @todo markers in run().
///
/// Unlike the Mario Party 1-3 hosts (which share MarioPartyN64Host), Sonic Shuffle
/// is a Dreamcast game, so it derives directly from DrHost and owns its own core.
class SonicShuffleHost : public DrHost
{
  Q_OBJECT

public:
  explicit SonicShuffleHost(QObject *parent = nullptr);
  ~SonicShuffleHost(void) override;

  dr_game game(void) const override { return DR_GAME_SONICSHUFFLE; }

  /// Flycast is a heavy GL core that crashes if its content loads before a native
  /// window surface exists, so we defer loadContent to here (called by mainwindow
  /// after the host's window container is created), mirroring the guest.
  void startCore(void) override;

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;

signals:
  /// Emitted (on the core's timing thread) when the local player's hand or its
  /// selection cursor changes, delivered queued to the hand widget on the GUI
  /// thread. highlight = index of the card to glow, or -1 for none.
  void localHandChanged(int character, QByteArray cards, int highlight);

private:
  /// Per-frame board/roulette driver, hooked to the core's frameEnd.
  void run(void);

  /// Reads the local player's character + card hand from RAM and, on change,
  /// emits localHandChanged so the hand widget redraws.
  void pollLocalHand(void);

  QString m_gamePath;
  bool m_contentLoaded = false;

  std::array<DrMinigameCandidate, 5> m_candidates = {};
  std::array<dr_player_t, 4> m_pendingPlayers = {};

  /* Last-seen scene, for edge-detecting board/roulette/results transitions. */
  int16_t m_lastScene = -1;

  /* Last hand we reported, to update only on change. num_cards 0xFF = nothing
   * yet; character 0xFF forces the first update through. */
  uint8_t m_lastCharacter = 0xFF;
  uint8_t m_lastNumCards = 0xFF;
  uint8_t m_lastHand[7] = {};
  int m_lastHighlight = -2; /* -2 forces the first update through (-1 = valid "none") */

  /* Floating panel showing our hand; owned here, top-level. */
  SonicShuffleHand *m_hand = nullptr;
};

#endif
