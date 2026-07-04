#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdlib>

#include <QCloseEvent>
#include <QMainWindow>
#include <QMetaObject>
#include <QStackedWidget>
#include <QTimer>

#include "DrCommon.h"
#include "DrDebug.h"
#include "DrGuestList.h"
#include "DrHost.h"
#include "DrInputStore.h"
#include "DrLogger.h"
#include "DrMinigameFilter.h"
#include "DrNetplay.h"
#include "DrNetplayWidget.h"
#include "DrOverlay.h"
#include "DrToolWindow.h"

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
  void warmupStep();
  void launchMinigame(
    DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4]);
  void setupNetplay();
  void attachNetplay();

  QMetaObject::Connection m_warmupConnection;
  unsigned m_warmupFrameCount = 0;
  QList<DrGuest *> m_warmupQueue; // guests with an allowed mini-game, warmed in order
  int m_warmupIndex = 0;

  DrGuestList *m_Guests = nullptr;
  DrHost *m_Host = nullptr;
  QWidget *m_HostContainer = nullptr;
  QStackedWidget *m_Stack = nullptr;
  DrLogger *m_Logger = nullptr;
  DrOverlay *m_Overlay = nullptr;
  DrDebug *m_Debug = nullptr;
  DrMinigameFilter *m_Filter = nullptr;
  DrInputStore *m_InputStore = nullptr;
  DrNetplay *m_Netplay = nullptr;
  DrNetplayWidget *m_NetplayUi = nullptr;
  DrToolWindow *m_Tools = nullptr;

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
    // The tool pages are children of m_Tools, so closing it closes them all.
    if (m_Tools)
      m_Tools->close();
    QMainWindow::closeEvent(e);
    // The emulator cores run background threads (timing, SDL, libusb) that fire
    // DirectConnection signals into our objects. Unwinding all of that during
    // normal teardown races with object destruction and crashes (notably under
    // Wine). There is no exit-time state to flush, so terminate immediately.
    std::_Exit(0);
  }
};

#endif
