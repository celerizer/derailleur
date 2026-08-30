#ifndef DR_DEBUG_H
#define DR_DEBUG_H

#include <array>
#include <QWidget>
#include <QList>
#include <QPair>
#include "DrGuest.h"

class QComboBox;
class QLabel;
class QMenu;
class QToolButton;

class DrDebug : public QWidget
{
  Q_OBJECT

public:
  DrDebug(QWidget *parent = nullptr);
  void populate(const QList<DrGuest *> &guests);

  /// The four players as configured in the dialog. With a `minigame`, the fields a
  /// launch needs (team_type and placeholder coins/stars) are filled in too; without
  /// one, only what the dialog actually shows is set.
  std::array<dr_player_t, 4> players(const dr_mp_minigame_t *minigame = nullptr) const;

  /// Point every dropdown at `players`, for pulling the running host's setup in.
  void setPlayers(const std::array<dr_player_t, 4> &players);

signals:
  void minigameRequested(
    DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players);
  void cancelRequested();
  void setTurnRequested(int turn);

  /// Read the host's player setup into the dialog. The owner does the reading and
  /// answers with setPlayers, so this widget stays independent of DrHost.
  void readPlayersRequested();

  /// Stamp the dialog's player setup into the running host.
  void writePlayersRequested(std::array<dr_player_t, 4> players);

private:
  void refreshMinis(int guestIdx);
  void selectEntry(int idx);
  /// The selected mini-game's type, or DR_MINIGAME_INVALID if none.
  dr_minigame_type selectedType() const;
  /// Relabels the per-player team dropdowns to match the selected type (Blue/Red for
  /// 2v2, Solo/Team for 1v3, Player/Non-player for duel/item/1p, else numeric).
  void updateTeamOptions();
  /// Shows the warning icon by the request button when the team split is invalid for
  /// the selected type (e.g. not one solo, not two duelers).
  void validateTeams();

  /* Per-player dropdowns. team_color and team_type are still inferred (from team_id and
   * the minigame type); everything else here is user-selectable. */
  struct PlayerControls
  {
    QComboBox *character = nullptr;
    QComboBox *controlPort = nullptr;
    QComboBox *controlType = nullptr;
    QComboBox *teamId = nullptr;
    QComboBox *difficulty = nullptr;
  };

  QComboBox *m_guestCombo = nullptr;
  /* Mini-game picker: a button whose pop-up menu groups mini-games into a
   * submenu per type, so you hover a type then pick a mini-game. */
  QToolButton *m_miniButton = nullptr;
  QMenu *m_miniMenu = nullptr;
  QLabel *m_teamWarning = nullptr; /* "⚠" shown next to Request when the team split is invalid */
  int m_selectedEntry = -1; /* index into m_entries, or -1 */
  std::array<PlayerControls, 4> m_players{};
  QList<QPair<DrGuest *, DrMinigameGroup>> m_groups;
  QList<QPair<DrGuest *, const dr_mp_minigame_t *>> m_entries;
};

#endif
