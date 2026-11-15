#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <functional>

#include <QMainWindow>
#include <QStackedWidget>

#include "DrCommon.h"
#include "DrChallenge.h"
#include "DrDebug.h"
#include "DrDebugCore.h"
#include "DrGuestList.h"
#include "DrHost.h"
#include "DrHostPicker.h"
#include "DrInputStore.h"
#include "DrLogger.h"
#include "DrMinigameFilter.h"
#include "DrNetplay.h"
#include "DrNetplayWidget.h"
#include "DrOverlay.h"
#include "DrSettingsPage.h"
#include "DrToolWindow.h"

class QRetro;
struct DrHostEntry;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

protected:
  void showEvent(QShowEvent *) override { trackOverlay(); }
  void moveEvent(QMoveEvent *) override { trackOverlay(); }
  void resizeEvent(QResizeEvent *) override { trackOverlay(); }
  void closeEvent(QCloseEvent *e) override;

private:
  void setupStack();
  void setupFullscreen(void);
  void setupGuests();
  void setupFilter();
  void setupDebug();
  void setupChallenge();
  void setupSettings();
  void setupNetplay();

  /// Builds the host for `entry` and starts it. `rom` overrides the stock
  /// content; `customRom` is its basename when it is a custom ROM.
  void startHost(const DrHostEntry *entry, const QString &rom, const QString &customRom);
  void startWithHost(DrHost *host);
  void attachNetplay();

  void launchMinigame(
    DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4]);
  void onMinigameFinished();

  /// Cancels the active mini-game locally and returns to the board.
  void cancelActiveMinigame();

  /// Returns to the board, or to the picker when there is no host.
  void showBoard();
  void showHost();
  void showChooser();

  /// Pauses every core, then runs `show` once the current frame has settled.
  void switchView(std::function<void()> show);
  void pauseGuests();

  /// Freezes the game view into the loading overlay. `core` is the core on
  /// screen, or null.
  void holdView(QRetro *core);
  void fadeOverlay();
  void trackOverlay();

  /// The loading overlay, or null when the user has turned it off.
  DrOverlay *overlay(void) { return DrSettings::instance().loadingOverlay() ? m_Overlay : nullptr; }

  int m_NetplayPeerIndex = 0;
  QString m_CustomRom;

  DrGuestList *m_Guests = nullptr;
  DrHost *m_Host = nullptr;
  QWidget *m_HostContainer = nullptr;
  QWidget *m_Blank = nullptr;
  QStackedWidget *m_Stack = nullptr;

  DrToolWindow *m_Tools = nullptr;
  DrLogger *m_Logger = nullptr;
  DrHostPicker *m_Picker = nullptr;
  DrMinigameFilter *m_Filter = nullptr;
  DrSettingsPage *m_Settings = nullptr;
  DrDebug *m_Debug = nullptr;
  DrDebugCore *m_DebugCore = nullptr;
  DrChallenge *m_Challenge = nullptr;
  DrOverlay *m_Overlay = nullptr;

  DrInputStore *m_InputStore = nullptr;
  DrNetplay *m_Netplay = nullptr;
  DrNetplayWidget *m_NetplayUi = nullptr;
};

#endif
