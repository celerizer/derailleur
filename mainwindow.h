#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

#include <QRetro.h>
#include "DrGuestList.h"
#include "DrLogger.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void showHost();
  void showGuests();

  DrGuestList    *m_Guests = nullptr;
  QRetro         *m_RetroB = nullptr;
  QStackedWidget *m_Stack = nullptr;
  DrLogger       *m_Logger = nullptr;
};

#endif
