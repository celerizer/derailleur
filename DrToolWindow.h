#ifndef DR_TOOL_WINDOW_H
#define DR_TOOL_WINDOW_H

#include <QWidget>

class QListWidget;
class QListWidgetItem;
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

  /// Greys out (or restores) the sidebar row and page for `page`.
  void setToolEnabled(QWidget *page, bool enabled);

  /// Adds a non-selectable section header to the sidebar.
  void addDivider(const QString &text);

  /// While enabled, tools/dividers added are hidden from the sidebar (their pages
  /// still exist). Used to show only the Log during startup; revealTools() undoes
  /// it. Does not affect tools already added.
  void setDeferReveal(bool on) { m_DeferReveal = on; }

  /// Shows every hidden sidebar row and selects the tool named `selectName` (if
  /// present). Ends the startup deferral.
  void revealTools(const QString &selectName);

  /// While `window` is visible, attempts to close this window are ignored.
  /// Pass null to allow closing again.
  void setCloseBlocker(QWidget *window) { m_CloseBlocker = window; }

protected:
  void closeEvent(QCloseEvent *e) override;

private:
  /// The sidebar row showing `page`, or null.
  QListWidgetItem *toolRow(QWidget *page) const;

  QListWidget *m_Sidebar = nullptr;
  QStackedWidget *m_Stack = nullptr;
  bool m_DeferReveal = false;
  QWidget *m_CloseBlocker = nullptr;
};

#endif
