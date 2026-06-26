#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QCoreApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

#include "DrCommon.h"
#include "DrDebug.h"
#include "DrGuestList.h"
#include "DrHost.h"
#include "DrInputStore.h"
#include "DrLogger.h"
#include "DrNetplay.h"
#include "DrOverlay.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void startWithHost(DrHost *host);
  void showHost();
  void showGuests();
  void launchMinigame(
    DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4]);
  void buildNetplayMenu();
  void attachNetplay();

  DrGuestList *m_Guests = nullptr;
  DrHost *m_Host = nullptr;
  QWidget *m_HostContainer = nullptr;
  QStackedWidget *m_Stack = nullptr;
  DrLogger *m_Logger = nullptr;
  DrOverlay *m_Overlay = nullptr;
  DrDebug *m_Debug = nullptr;
  DrInputStore *m_InputStore = nullptr;
  DrNetplay *m_Netplay = nullptr;

protected:
  void showEvent(QShowEvent *) override
  {
    if (m_Overlay)
      m_Overlay->trackGeometry(geometry());
  }
  void moveEvent(QMoveEvent *) override
  {
    if (m_Overlay)
      m_Overlay->trackGeometry(geometry());
  }
  void resizeEvent(QResizeEvent *) override
  {
    if (m_Overlay)
      m_Overlay->trackGeometry(geometry());
  }
  void closeEvent(QCloseEvent *e) override
  {
    if (m_Netplay)
      m_Netplay->abort();
    if (m_Logger)
      m_Logger->close();
    if (m_Debug)
      m_Debug->close();
    QMainWindow::closeEvent(e);
    // Guest/host cores run in their own top-level windows (e.g. the shared
    // Dolphin core), so closing the main window won't trigger Qt's
    // quit-on-last-window-closed. Quit explicitly so emulation stops.
    QCoreApplication::quit();
  }
};

#endif
