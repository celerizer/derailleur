#include "DrToolWindow.h"

#include "DrCommon.h"

#include <QCloseEvent>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPalette>
#include <QStackedWidget>

DrToolWindow::DrToolWindow(QWidget *parent)
  : QWidget(parent)
{
  setWindowTitle(QString("derailleur - %1 (%2)")
      .arg(QString::fromUtf8(DERAILLEUR_DATE_STRING), QString::fromUtf8(DERAILLEUR_RELEASE_STRING)));
  resize(760, 585);

  m_Sidebar = new QListWidget(this);
  m_Sidebar->setFrameShape(QFrame::NoFrame);
  m_Sidebar->setStyleSheet("QListWidget::item { padding: 6px 10px; }");
  m_Sidebar->setFixedWidth(160);

  m_Stack = new QStackedWidget(this);

  connect(m_Sidebar, &QListWidget::currentItemChanged, this,
    [this](QListWidgetItem *current, QListWidgetItem *) {
      if (!current)
        return;
      QVariant v = current->data(Qt::UserRole);
      if (v.isValid())
        m_Stack->setCurrentIndex(v.toInt());
    });

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(m_Sidebar);
  layout->addWidget(m_Stack, 1);
}

void DrToolWindow::addTool(const QString &name, QWidget *page)
{
  if (!page)
    return;

  const int index = m_Stack->addWidget(page);

  auto *item = new QListWidgetItem(name);
  item->setData(Qt::UserRole, index);
  m_Sidebar->addItem(item);

  /* Hidden tools neither show in the sidebar nor grab the initial selection. */
  if (m_DeferReveal)
    item->setHidden(true);
  else if (!m_Sidebar->currentItem())
    m_Sidebar->setCurrentItem(item);
}

QListWidgetItem *DrToolWindow::toolRow(QWidget *page) const
{
  const int index = m_Stack->indexOf(page);
  int i;

  if (index < 0)
    return nullptr;

  for (i = 0; i < m_Sidebar->count(); i++)
  {
    const QVariant v = m_Sidebar->item(i)->data(Qt::UserRole);

    if (v.isValid() && v.toInt() == index)
      return m_Sidebar->item(i);
  }

  return nullptr;
}

void DrToolWindow::setToolEnabled(QWidget *page, bool enabled)
{
  QListWidgetItem *row = toolRow(page);

  if (!row)
    return;

  if (enabled)
    row->setFlags(row->flags() | Qt::ItemIsEnabled);
  else
    row->setFlags(row->flags() & ~Qt::ItemIsEnabled);
  page->setEnabled(enabled);
}

void DrToolWindow::addDivider(const QString &text)
{
  auto *item = new QListWidgetItem(text);
  item->setFlags(Qt::ItemIsEnabled); // not selectable
  QFont f = item->font();
  f.setPointSize(qMax(f.pointSize() - 2, 7));
  item->setFont(f);
  item->setForeground(m_Sidebar->palette().color(QPalette::Mid));
  m_Sidebar->addItem(item);
  if (m_DeferReveal)
    item->setHidden(true);
}

void DrToolWindow::revealTools(const QString &selectName)
{
  m_DeferReveal = false;

  for (int i = 0; i < m_Sidebar->count(); i++)
    m_Sidebar->item(i)->setHidden(false);

  for (int i = 0; i < m_Sidebar->count(); i++)
  {
    QListWidgetItem *item = m_Sidebar->item(i);
    if (item->text() == selectName && (item->flags() & Qt::ItemIsSelectable))
    {
      m_Sidebar->setCurrentItem(item);
      break;
    }
  }
}

void DrToolWindow::closeEvent(QCloseEvent *e)
{
  /* Stay open while the game window is up */
  if (m_CloseBlocker && m_CloseBlocker->isVisible())
    e->ignore();
  else
    QWidget::closeEvent(e);
}
