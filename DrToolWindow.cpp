#include "DrToolWindow.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPalette>
#include <QStackedWidget>

DrToolWindow::DrToolWindow(QWidget *parent)
  : QWidget(parent)
{
  setWindowTitle(tr("derailleur"));
  resize(760, 460);

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

  if (!m_Sidebar->currentItem())
    m_Sidebar->setCurrentItem(item);
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
}
