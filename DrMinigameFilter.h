#ifndef DR_MINIGAME_FILTER_H
#define DR_MINIGAME_FILTER_H

#include <QByteArray>
#include <QList>
#include <QWidget>

#include "DrGuest.h"

class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QLabel;

/**
 * Standalone window listing every guest game (expandable, with an all/none/some
 * tristate checkbox) and each of its mini-games as an on/off toggle. The set of
 * disabled mini-games is serialized to an opaque payload and emitted via
 * filterChanged; setFromPayload reflects a payload received from a peer.
 *
 * Mini-games are identified by a stable key — (guestIndex << 16) | ordinal,
 * where ordinal flattens the guest's minigameGroups() in order — that matches
 * DrGuestList's enumeration, so the same payload selects the same mini-games on
 * every peer regardless of pointer addresses.
 */
class DrMinigameFilter : public QWidget
{
  Q_OBJECT

public:
  explicit DrMinigameFilter(QWidget *parent = nullptr);

  void populate(const QList<DrGuest *> &guests);

  /// Current selection as an opaque payload: [u16 count][count * u32 key] of the
  /// disabled (unchecked) mini-games.
  QByteArray payload() const;

  /// Apply a payload (e.g. received from the host), updating the checkboxes
  /// without re-emitting filterChanged.
  void setFromPayload(const QByteArray &payload);

signals:
  /// Emitted when the user changes the selection. Coalesced so a single click on
  /// a guest's tristate box (which cascades to its children) emits once.
  void filterChanged(const QByteArray &payload);

private:
  void onItemChanged(QTreeWidgetItem *item, int column);

  /// Recount the allowed (checked) mini-games per type, refresh the count
  /// fields, and flag any Mario Party game that can no longer be played.
  void updateCounts();

  QTreeWidget *m_tree = nullptr;
  QTimer *m_coalesce = nullptr;
  bool m_applying = false; // suppress emits during programmatic updates

  // Count name/value labels, indexed by dr_minigame_type (unused slots stay null).
  QLabel *m_countNames[DR_MINIGAME_SIZE] = {};
  QLabel *m_counts[DR_MINIGAME_SIZE] = {};
  QLabel *m_note = nullptr;
};

#endif
