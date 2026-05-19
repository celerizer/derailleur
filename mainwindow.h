#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

#include "DrCommon.h"
#include "DrDebug.h"
#include "DrGuestList.h"
#include "DrHost.h"
#include "DrLogger.h"
#include "DrOverlay.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void showHost();
  void showGuests();
  void launchMinigame(dr_minigame_type type, const dr_player_t players[4], const bool playerValid[4]);
  void launchMinigame(DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4], const bool playerValid[4]);

  DrGuestList    *m_Guests  = nullptr;
  DrHost         *m_Host    = nullptr;
  QStackedWidget *m_Stack   = nullptr;
  DrLogger       *m_Logger  = nullptr;
  DrOverlay      *m_Overlay = nullptr;
  DrDebug        *m_Debug   = nullptr;

protected:
  void showEvent(QShowEvent *) override     { if (m_Overlay) { m_Overlay->trackGeometry(geometry()); m_Overlay->show(); } }
  void moveEvent(QMoveEvent *) override     { if (m_Overlay) m_Overlay->trackGeometry(geometry()); }
  void resizeEvent(QResizeEvent *) override { if (m_Overlay) m_Overlay->trackGeometry(geometry()); }
  void closeEvent(QCloseEvent *e) override  { if (m_Logger) m_Logger->close(); if (m_Debug) m_Debug->close(); QMainWindow::closeEvent(e); }
};

#endif
