#ifndef DR_MINIGAME_FILTER_H
#define DR_MINIGAME_FILTER_H

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "DrGuest.h"

class QComboBox;
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
 * Mini-games are identified by dr_minigame_key: the owning game's dr_guest id
 * plus the mini-game's own ids, so a payload or saved list selects the same
 * mini-games regardless of which guests are loaded or in what order.
 *
 * Saved lists are one JSON file each, minigame_lists/<name>.json.
 */
class DrMinigameFilter : public QWidget
{
  Q_OBJECT

public:
  explicit DrMinigameFilter(QWidget *parent = nullptr);

  void populate(const QList<DrGuest *> &guests);

  /// Current selection as an opaque payload of the disabled (unchecked)
  /// mini-games; see dr_minigame_filter_encode.
  QByteArray payload() const;

  /// Apply a payload (e.g. received from the host), updating the checkboxes
  /// without re-emitting filterChanged.
  void setFromPayload(const QByteArray &payload);

  /// Select and apply the list chosen last session, emitting filterChanged so it
  /// reaches the guests. Call once the caller has connected that signal --
  /// populate() alone leaves everything allowed.
  void restoreLastList(void);

signals:
  /// Emitted when the user changes the selection. Coalesced so a single click on
  /// a guest's tristate box (which cascades to its children) emits once.
  void filterChanged(const QByteArray &payload);

private:
  void onItemChanged(QTreeWidgetItem *item, int column);

  /// Names of the saved list files, sorted.
  QStringList listNames(void) const;

  /// Read a saved list file into a disabled set. False if missing or malformed.
  bool readList(const QString &name, QSet<dr_minigame_key_t> &disabled) const;

  /// Write the current selection to a saved list file.
  bool writeList(const QString &name) const;

  /// Refill the dropdown from the list files and select `name` ("" = the unsaved slot).
  void reloadLists(const QString &name);

  /// Every disabled key: unchecked leaves plus keys for games not loaded here.
  QSet<dr_minigame_key_t> disabledKeys(void) const;

  /// Tick every leaf not in `disabled`, keeping the keys no leaf matches.
  void applyDisabled(const QSet<dr_minigame_key_t> &disabled);

  /// Apply the list at `index`, or do nothing for the unsaved slot.
  void onListSelected(int index);

  void saveList(void);
  void deleteList(void);

  /// Recount the allowed (checked) mini-games per type, refresh the count
  /// fields, and flag any Mario Party game that can no longer be played.
  void updateCounts();

  QComboBox *m_lists = nullptr;
  QTreeWidget *m_tree = nullptr;
  QTimer *m_coalesce = nullptr;
  bool m_applying = false; // suppress emits during programmatic updates

  /* Disabled keys with no leaf here (a game this install has not loaded). */
  QSet<dr_minigame_key_t> m_hidden;

  // Count name/value labels, indexed by dr_minigame_type (unused slots stay null).
  QLabel *m_countNames[DR_MINIGAME_SIZE] = {};
  QLabel *m_counts[DR_MINIGAME_SIZE] = {};
  QLabel *m_note = nullptr;
};

#endif
