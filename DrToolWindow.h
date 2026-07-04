#ifndef DR_TOOL_WINDOW_H
#define DR_TOOL_WINDOW_H

#include <QWidget>

class QListWidget;
class QStackedWidget;

/**
 * A single helper window that hosts derailleur's auxiliary tool widgets (log,
 * minigame filter, debug, netplay) as pages selected from a category sidebar,
 * mirroring QRetroConfig's QListWidget + QStackedWidget layout. Each page is a
 * plain QWidget reparented into the stack, so the tools stay independent.
 */
class DrToolWindow : public QWidget
{
  Q_OBJECT

public:
  explicit DrToolWindow(QWidget *parent = nullptr);

  /// Adds `page` as a new category, reparenting it into the stack. The first
  /// page added becomes the current selection.
  void addTool(const QString &name, QWidget *page);

  /// Adds a non-selectable section header to the sidebar.
  void addDivider(const QString &text);

private:
  QListWidget *m_Sidebar = nullptr;
  QStackedWidget *m_Stack = nullptr;
};

#endif
