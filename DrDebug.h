#ifndef DR_DEBUG_H
#define DR_DEBUG_H

#include <array>
#include <QWidget>
#include <QList>
#include <QPair>
#include "DrGuest.h"

class QComboBox;

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

private:
  void refreshMinis(int guestIdx);

  /* Per-player dropdowns. Port is fixed by player index and the team fields are
   * inferred from the minigame type, so only these are user-selectable. */
  struct PlayerControls
  {
    QComboBox *character = nullptr;
    QComboBox *controlType = nullptr;
    QComboBox *difficulty = nullptr;
  };

  QComboBox *m_guestCombo = nullptr;
  QComboBox *m_combo = nullptr;
  std::array<PlayerControls, 4> m_players{};
  QList<QPair<DrGuest *, DrMinigameGroup>> m_groups;
  QList<QPair<DrGuest *, const dr_mp_minigame_t *>> m_entries;
};

#endif
