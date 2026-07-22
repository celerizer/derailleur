#ifndef DR_DEBUG_H
#define DR_DEBUG_H

#include <array>
#include <QWidget>
#include <QList>
#include <QPair>
#include "DrGuest.h"

class QComboBox;
class QMenu;
class QToolButton;

class DrDebug : public QWidget
{
  Q_OBJECT

public:
  DrDebug(QWidget *parent = nullptr);
  void populate(const QList<DrGuest *> &guests);

signals:
  void minigameRequested(
    DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players);
  void cancelRequested();
  void setTurnRequested(int turn);

private:
  void refreshMinis(int guestIdx);
  void selectEntry(int idx);

  /* Per-player dropdowns. Port is fixed by player index and the team fields are
   * inferred from the minigame type, so only these are user-selectable. */
  struct PlayerControls
  {
    QComboBox *character = nullptr;
    QComboBox *controlType = nullptr;
    QComboBox *difficulty = nullptr;
  };

  QComboBox *m_guestCombo = nullptr;
  /* Mini-game picker: a button whose pop-up menu groups mini-games into a
   * submenu per type, so you hover a type then pick a mini-game. */
  QToolButton *m_miniButton = nullptr;
  QMenu *m_miniMenu = nullptr;
  int m_selectedEntry = -1; /* index into m_entries, or -1 */
  std::array<PlayerControls, 4> m_players{};
  QList<QPair<DrGuest *, DrMinigameGroup>> m_groups;
  QList<QPair<DrGuest *, const dr_mp_minigame_t *>> m_entries;
};

#endif
