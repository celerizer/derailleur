#include "DrMinigameFilter.h"

#include <algorithm>

#include <QComboBox>
#include <QDataStream>
#include <QDir>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QSettings>
#include <QGridLayout>
#include <QGroupBox>
#include <QIODevice>
#include <QLabel>
#include <QSet>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

// Roles on leaf items: the stable network key, a marker that an item is a
// mini-game leaf (parents/group headers carry no key), and the mini-game type.
static constexpr int k_RoleKey = Qt::UserRole;
static constexpr int k_RoleIsLeaf = Qt::UserRole + 1;
static constexpr int k_RoleType = Qt::UserRole + 2;

// Saved lists live in derailleur.ini as an array, so a name can contain anything.
static const char *k_ListsArray = "minigame_lists";
static const char *k_LastKey = "settings/minigame_list_last";

static QString dr_filter_ini(void)
{
  return QDir::current().filePath("derailleur.ini");
}

// The mini-game types shown as running totals, in display order.
static const dr_minigame_type k_CountedTypes[] = {
  DR_MINIGAME_4P, DR_MINIGAME_1V3, DR_MINIGAME_2V2,
  DR_MINIGAME_BATTLE, DR_MINIGAME_DUEL, DR_MINIGAME_1P
};

namespace
{
// Mario Party games and the mini-game types each needs at least one of to be
// playable. If any listed type has a zero total, that game is unplayable.
struct MarioPartyReq
{
  const char *name;
  QList<dr_minigame_type> required;
};
const MarioPartyReq k_MarioParty[] = {
  { "Mario Party",   { DR_MINIGAME_4P, DR_MINIGAME_1V3, DR_MINIGAME_2V2, DR_MINIGAME_1P } },
  { "Mario Party 2", { DR_MINIGAME_4P, DR_MINIGAME_1V3, DR_MINIGAME_2V2, DR_MINIGAME_BATTLE } },
  { "Mario Party 3", { DR_MINIGAME_4P, DR_MINIGAME_1V3, DR_MINIGAME_2V2, DR_MINIGAME_BATTLE, DR_MINIGAME_DUEL } },
};
}

DrMinigameFilter::DrMinigameFilter(QWidget *parent)
  : QWidget(parent)
{
  setWindowTitle("Allowed Mini-games");
  resize(360, 520);

  auto *layout = new QVBoxLayout(this);

  /* Saved lists: pick one to load it, or name the current ticks and keep them. */
  auto *listRow = new QHBoxLayout;
  auto *saveButton = new QPushButton(tr("Save As..."), this);
  auto *deleteButton = new QPushButton(tr("Delete"), this);

  m_lists = new QComboBox(this);
  m_lists->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  listRow->addWidget(new QLabel(tr("List:"), this));
  listRow->addWidget(m_lists, 1);
  listRow->addWidget(saveButton);
  listRow->addWidget(deleteButton);
  layout->addLayout(listRow);

  connect(m_lists, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &DrMinigameFilter::onListSelected);
  connect(saveButton, &QPushButton::clicked, this, &DrMinigameFilter::saveList);
  connect(deleteButton, &QPushButton::clicked, this, &DrMinigameFilter::deleteList);

  m_tree = new QTreeWidget(this);
  m_tree->setHeaderHidden(true);
  m_tree->setUniformRowHeights(true);
  layout->addWidget(m_tree);

  /* Running totals of allowed mini-games per type, laid out two rows of three. */
  auto *countsBox = new QGroupBox(tr("Allowed by type"), this);
  auto *countsGrid = new QGridLayout(countsBox);
  const int perRow = 3;
  for (int i = 0; i < int(sizeof(k_CountedTypes) / sizeof(*k_CountedTypes)); i++)
  {
    const dr_minigame_type type = k_CountedTypes[i];
    auto *value = new QLabel(countsBox);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value->setTextInteractionFlags(Qt::NoTextInteraction);
    QFont f = value->font();
    f.setBold(true);
    value->setFont(f);
    m_counts[type] = value;

    auto *name = new QLabel(tr("%1:").arg(dr_minigame_type_name(type)), countsBox);
    m_countNames[type] = name;

    const int row = i / perRow;
    const int col = (i % perRow) * 2;
    countsGrid->addWidget(name, row, col);
    countsGrid->addWidget(value, row, col + 1);
  }
  layout->addWidget(countsBox);

  m_note = new QLabel(this);
  m_note->setWordWrap(true);
  m_note->setStyleSheet("QLabel { color: #c0392b; }");
  m_note->hide();
  layout->addWidget(m_note);

  /* A single click on a tristate guest box cascades check-state changes to all
   * its children, firing itemChanged once per item. Coalesce into one emit. */
  m_coalesce = new QTimer(this);
  m_coalesce->setSingleShot(true);
  m_coalesce->setInterval(0);
  connect(m_coalesce, &QTimer::timeout, this, [this]() { emit filterChanged(payload()); });

  connect(m_tree, &QTreeWidget::itemChanged, this, &DrMinigameFilter::onItemChanged);

  reloadLists(QString());
}

QMap<QString, QByteArray> DrMinigameFilter::readLists(void) const
{
  QSettings settings(dr_filter_ini(), QSettings::IniFormat);
  QMap<QString, QByteArray> lists;
  const int count = settings.beginReadArray(k_ListsArray);

  for (int i = 0; i < count; i++)
  {
    settings.setArrayIndex(i);

    const QString name = settings.value("name").toString();

    if (!name.isEmpty())
      lists.insert(name, QByteArray::fromHex(settings.value("payload").toByteArray()));
  }
  settings.endArray();

  return lists;
}

void DrMinigameFilter::writeLists(
  const QMap<QString, QByteArray> &lists, const QString &last) const
{
  QSettings settings(dr_filter_ini(), QSettings::IniFormat);
  int i = 0;

  settings.remove(k_ListsArray);
  settings.beginWriteArray(k_ListsArray, lists.size());
  for (auto it = lists.constBegin(); it != lists.constEnd(); ++it)
  {
    settings.setArrayIndex(i++);
    settings.setValue("name", it.key());
    settings.setValue("payload", QString::fromLatin1(it.value().toHex()));
  }
  settings.endArray();
  settings.setValue(k_LastKey, last);
  settings.sync();
}

void DrMinigameFilter::reloadLists(const QString &name)
{
  const QMap<QString, QByteArray> lists = readLists();
  const QSignalBlocker blocker(m_lists);
  int index;

  m_lists->clear();
  m_lists->addItem(tr("(unsaved)"), QString());
  for (auto it = lists.constBegin(); it != lists.constEnd(); ++it)
    m_lists->addItem(it.key(), it.key());

  index = name.isEmpty() ? 0 : m_lists->findData(name);
  m_lists->setCurrentIndex(index < 0 ? 0 : index);
}

void DrMinigameFilter::onListSelected(int index)
{
  const QString name = m_lists->itemData(index).toString();
  const QMap<QString, QByteArray> lists = readLists();

  /* The unsaved slot is where hand-edited ticks live; it has nothing to load. */
  if (name.isEmpty() || !lists.contains(name))
    return;

  setFromPayload(lists.value(name));

  QSettings settings(dr_filter_ini(), QSettings::IniFormat);
  settings.setValue(k_LastKey, name);
  settings.sync();

  emit filterChanged(payload());
}

void DrMinigameFilter::saveList(void)
{
  QMap<QString, QByteArray> lists = readLists();
  bool accepted = false;
  const QString name = QInputDialog::getText(this, tr("Save Mini-game List"), tr("Name:"),
    QLineEdit::Normal, m_lists->currentData().toString(), &accepted).trimmed();

  if (!accepted || name.isEmpty())
    return;

  lists.insert(name, payload());
  writeLists(lists, name);
  reloadLists(name);
}

void DrMinigameFilter::deleteList(void)
{
  const QString name = m_lists->currentData().toString();
  QMap<QString, QByteArray> lists = readLists();

  if (name.isEmpty() || !lists.contains(name))
    return;

  lists.remove(name);
  writeLists(lists, QString());
  reloadLists(QString());
}

void DrMinigameFilter::restoreLastList(void)
{
  QSettings settings(dr_filter_ini(), QSettings::IniFormat);
  const QString last = settings.value(k_LastKey).toString();

  if (last.isEmpty())
    return;

  reloadLists(last);

  /* reloadLists selects without signalling, so apply it here. */
  if (!m_lists->currentData().toString().isEmpty())
    onListSelected(m_lists->currentIndex());
}

void DrMinigameFilter::populate(const QList<DrGuest *> &guests)
{
  QList<QList<DrMinigameGroup>> groupsOf;
  QList<QPair<QString, int>> order;

  m_applying = true;
  m_tree->clear();

  /* A guest holding one game is named by that game, not by itself: a single-disc
   * Dolphin instance calls itself "Dolphin gcn-mp4" while its group carries the
   * real title. Guests with several games keep their own name as the header. */
  for (int gi = 0; gi < guests.size(); gi++)
  {
    const QList<DrMinigameGroup> groups = guests[gi]->minigameGroups();

    groupsOf.append(groups);
    order.append({ QString::fromUtf8(
                     groups.size() == 1 ? groups.first().name : guests[gi]->name()),
      gi });
  }

  /* Listed alphabetically, but the key below still counts from each guest's
   * position in `guests` -- that is what DrGuestList enumerates and what peers
   * agree on, so display order must not touch it. */
  std::sort(order.begin(), order.end(),
    [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
      return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

  for (const QPair<QString, int> &entry : order)
  {
    const int gi = entry.second;

    auto *guestItem = new QTreeWidgetItem(m_tree);
    guestItem->setText(0, entry.first);
    guestItem->setFlags(guestItem->flags() | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    guestItem->setExpanded(false);

    const QList<DrMinigameGroup> &groups = groupsOf[gi];
    const bool multiGroup = groups.size() > 1;

    /* The ordinal flattens minigameGroups() in order and increments for *every*
     * mini-game (matching DrGuestList), so keys stay stable regardless of which
     * are shown or filtered. */
    quint32 ord = 0;
    for (const DrMinigameGroup &group : groups)
    {
      // For multi-group guests (e.g. Dolphin) add a group header level; the
      // tristate aggregation propagates through it automatically.
      QTreeWidgetItem *parent = guestItem;
      if (multiGroup)
      {
        parent = new QTreeWidgetItem(guestItem);
        parent->setText(0, QString::fromUtf8(group.name));
        parent->setFlags(parent->flags() | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
      }

      for (const dr_mp_minigame_t *mg : group.minigames)
      {
        const quint32 key = (static_cast<quint32>(gi) << 16) | ord++;
        auto *leaf = new QTreeWidgetItem(parent);
        leaf->setText(0, mg->name ? QString::fromUtf8(mg->name) : QString("(unnamed)"));
        leaf->setFlags((leaf->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsAutoTristate);
        leaf->setData(0, k_RoleKey, key);
        leaf->setData(0, k_RoleIsLeaf, true);
        leaf->setData(0, k_RoleType, static_cast<int>(mg->type));
        leaf->setCheckState(0, Qt::Checked); // default: everything allowed
      }
    }
  }

  m_applying = false;
  updateCounts();
}

QByteArray DrMinigameFilter::payload() const
{
  QList<quint32> disabled;
  QTreeWidgetItemIterator it(m_tree);
  for (; *it; ++it)
  {
    QTreeWidgetItem *item = *it;
    if (!item->data(0, k_RoleIsLeaf).toBool())
      continue;
    if (item->checkState(0) != Qt::Checked)
      disabled.append(item->data(0, k_RoleKey).toUInt());
  }

  QByteArray out;
  QDataStream s(&out, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << static_cast<quint16>(disabled.size());
  for (quint32 key : disabled)
    s << key;
  return out;
}

void DrMinigameFilter::setFromPayload(const QByteArray &payload)
{
  QSet<quint32> disabled;
  QDataStream s(payload);
  s.setByteOrder(QDataStream::LittleEndian);
  quint16 count = 0;
  s >> count;
  for (quint16 i = 0; i < count && s.status() == QDataStream::Ok; i++)
  {
    quint32 key = 0;
    s >> key;
    disabled.insert(key);
  }

  m_applying = true;
  QTreeWidgetItemIterator it(m_tree);
  for (; *it; ++it)
  {
    QTreeWidgetItem *item = *it;
    if (!item->data(0, k_RoleIsLeaf).toBool())
      continue;
    const quint32 key = item->data(0, k_RoleKey).toUInt();
    item->setCheckState(0, disabled.contains(key) ? Qt::Unchecked : Qt::Checked);
  }
  m_applying = false;
  updateCounts();
}

void DrMinigameFilter::onItemChanged(QTreeWidgetItem *item, int column)
{
  (void)item;
  (void)column;
  if (m_applying)
    return;

  /* Hand-edited ticks are no longer the list that was loaded; the saved one is
   * left alone until Save As is used. */
  if (m_lists && !m_lists->currentData().toString().isEmpty())
  {
    const QSignalBlocker blocker(m_lists);

    m_lists->setCurrentIndex(0);
  }

  updateCounts();
  m_coalesce->start(); // coalesce a cascade of changes into one filterChanged
}

void DrMinigameFilter::updateCounts()
{
  int counts[DR_MINIGAME_SIZE] = {};

  QTreeWidgetItemIterator it(m_tree);
  for (; *it; ++it)
  {
    QTreeWidgetItem *item = *it;
    if (!item->data(0, k_RoleIsLeaf).toBool())
      continue;
    if (item->checkState(0) != Qt::Checked)
      continue;
    const int type = item->data(0, k_RoleType).toInt();
    if (type > 0 && type < DR_MINIGAME_SIZE)
      counts[type]++;
  }

  for (dr_minigame_type type : k_CountedTypes)
  {
    if (!m_counts[type])
      continue;
    m_counts[type]->setText(QString::number(counts[type]));
    /* Highlight an empty category: nothing of this type is allowed. */
    const QString style = counts[type] == 0 ? "QLabel { color: #c0392b; }" : QString();
    m_counts[type]->setStyleSheet(style);
    m_countNames[type]->setStyleSheet(style);
  }

  /* A Mario Party game is unplayable if any of its required types has no
   * allowed mini-games left. */
  QStringList unplayable;
  for (const MarioPartyReq &game : k_MarioParty)
  {
    for (dr_minigame_type type : game.required)
    {
      if (counts[type] == 0)
      {
        unplayable.append(QString::fromUtf8(game.name));
        break;
      }
    }
  }

  if (unplayable.isEmpty())
  {
    m_note->hide();
  }
  else
  {
    m_note->setText(tr("Cannot be played with the current selection: %1")
                      .arg(unplayable.join(tr(", "))));
    m_note->show();
  }
}
