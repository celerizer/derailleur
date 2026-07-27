#ifndef DR_CHALLENGE_H
#define DR_CHALLENGE_H

#include <array>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>
#include "DrGuest.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QTreeWidget;

/// Per-mini-game completion for one character. The Unfair challenge (a 2v2/1v3
/// with very-easy team-mates against very-hard opponents) is tracked separately
/// from the five normal difficulties, not as a sixth tier.
struct DrClearStatus
{
  int tier = 0;         ///< highest normal difficulty cleared (1-5, 0 = none)
  bool unfair = false;  ///< whether the Unfair challenge was beaten
};

/**
 * Challenge mode: pick one of the first eight characters, a difficulty, and any
 * loaded mini-game, then try to win it single-handedly. The other three players
 * are CPUs with random characters. A win is persisted per mini-game and shown
 * back as a star rating (see DrClearStatus).
 *
 * Tiers 1-5 are Very Easy..Very Hard. "Unfair" (tier value 6) is a separate
 * challenge available only for team mini-games, where your team-mates are Very
 * Easy and your opponents Very Hard; in a 1v3 the player is normally the solo
 * "1", but under Unfair they join the trio instead. An Unfair run picks only
 * 2v2/1v3 mini-games and its clears are recorded apart from the five tiers,
 * showing as an extra sixth star rather than raising the tier.
 */
class DrChallenge : public QWidget
{
  Q_OBJECT

public:
  DrChallenge(QWidget *parent = nullptr);

  void populate(const QList<DrGuest *> &guests);

  /// Whether a challenge launch is still awaiting its result.
  bool isPending() const { return m_pending.minigame != nullptr; }

  /// Scores a finished challenge, recording a win if the player placed.
  void recordResult(DrGuest *guest);

  /// Drops a pending challenge without scoring it (cancelled / timed out).
  void clearPending()
  {
    m_pending = Pending();
    m_continuousResult.clear();
  }

  /// For a continuous-play transition, the banner for the mini-game just finished
  /// ("SUCCESS"/"FAILURE"). Empty on the first launch of a run or a one-off play.
  /// Reading it consumes it, so it only applies to the launch it precedes.
  QString takeContinuousResult()
  {
    const QString r = m_continuousResult;
    m_continuousResult.clear();
    return r;
  }

signals:
  void minigameRequested(
    DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players);

  /// Diagnostic log (level matches DrLogger::message / DR_LOG_*).
  void logMessage(unsigned level, const QString &message);

private:
  struct Pending
  {
    DrGuest *guest = nullptr;
    const dr_mp_minigame_t *minigame = nullptr;
    int tier = 0;
    /* Records go to the character that actually played, even if the combo is
     * changed while the mini-game is running. */
    dr_character character = DR_CHARACTER_INVALID;
    /* The launched line-up, so scoring knows the team split (needed to tell who
     * won a 2v2/1v3). */
    std::array<dr_player_t, 4> players{};
  };

  void start();
  /// Picks a random still-uncleared mini-game at m_continuousTier and launches it,
  /// skipping excludeIdx (the one just played) unless it is the only one left.
  /// Does nothing once everything is cleared, ending the continuous run.
  void launchNext(int excludeIdx);
  /// Emits a launch for one entry at the given effective tier.
  void launchEntry(int idx, int playTier);
  int indexOfEntry(const DrGuest *guest, const dr_mp_minigame_t *minigame) const;
  void refreshStars();
  /// Per-type star tally across the whole list, counting only the five base
  /// tiers (an Unfair clear is a bonus and does not add to the count).
  void updateTotals();
  void updateDifficultyChoices();
  const dr_mp_minigame_t *selectedMinigame(DrGuest **guestOut = nullptr) const;
  std::array<dr_player_t, 4> buildPlayers(
    const DrGuest *guest, const dr_mp_minigame_t *minigame, int tier) const;

  /// Loads the selected character's records and refreshes the view.
  void reload();
  dr_character currentCharacter() const;

  /* Completion is tracked per character, one file each. */
  static QString filePathFor(dr_character character);
  static QHash<QString, DrClearStatus> loadFor(dr_character character);
  static void saveFor(dr_character character, const QHash<QString, DrClearStatus> &records);
  static QString recordKey(const DrGuest *guest, const dr_mp_minigame_t *minigame);

  QComboBox *m_character = nullptr;
  QComboBox *m_difficulty = nullptr;
  QCheckBox *m_continuous = nullptr;
  QTreeWidget *m_tree = nullptr;
  QLabel *m_totals = nullptr;

  /// The tier selected when a continuous run began. Held fixed across the run so
  /// an Unfair run stays Unfair even while non-team minis play at Very Hard.
  int m_continuousTier = 0;
  /// Result banner for the next continuous launch; see takeContinuousResult().
  QString m_continuousResult;

  /// Flat list behind the tree; items carry an index into this.
  QList<QPair<DrGuest *, const dr_mp_minigame_t *>> m_entries;
  /// recordKey() -> clear status, for the selected character only.
  QHash<QString, DrClearStatus> m_cleared;
  Pending m_pending;
};

#endif
