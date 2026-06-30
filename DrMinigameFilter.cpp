#include "DrMinigameFilter.h"

#include <QDataStream>
#include <QIODevice>
#include <QSet>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

// Roles on leaf items: the stable network key, and a marker that an item is a
// mini-game leaf (parents/group headers carry no key).
static constexpr int k_RoleKey = Qt::UserRole;
static constexpr int k_RoleIsLeaf = Qt::UserRole + 1;

DrMinigameFilter::DrMinigameFilter(QWidget *parent)
  : QWidget(parent)
{
  setWindowTitle("Allowed Mini-games");
  resize(360, 520);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_tree = new QTreeWidget(this);
  m_tree->setHeaderHidden(true);
  m_tree->setUniformRowHeights(true);
  layout->addWidget(m_tree);

  /* A single click on a tristate guest box cascades check-state changes to all
   * its children, firing itemChanged once per item. Coalesce into one emit. */
  m_coalesce = new QTimer(this);
  m_coalesce->setSingleShot(true);
  m_coalesce->setInterval(0);
  connect(m_coalesce, &QTimer::timeout, this, [this]() { emit filterChanged(payload()); });

  connect(m_tree, &QTreeWidget::itemChanged, this, &DrMinigameFilter::onItemChanged);
}

void DrMinigameFilter::populate(const QList<DrGuest *> &guests)
{
  m_applying = true;
  m_tree->clear();

  for (int gi = 0; gi < guests.size(); gi++)
  {
    DrGuest *guest = guests[gi];

    auto *guestItem = new QTreeWidgetItem(m_tree);
    guestItem->setText(0, QString::fromUtf8(guest->name()));
    guestItem->setFlags(guestItem->flags() | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    guestItem->setExpanded(false);

    const QList<DrMinigameGroup> groups = guest->minigameGroups();
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
        leaf->setCheckState(0, Qt::Checked); // default: everything allowed
      }
    }
  }

  m_applying = false;
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
}

void DrMinigameFilter::onItemChanged(QTreeWidgetItem *item, int column)
{
  (void)item;
  (void)column;
  if (m_applying)
    return;
  m_coalesce->start(); // coalesce a cascade of changes into one filterChanged
}
