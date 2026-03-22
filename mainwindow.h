#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>

#include <QRetro.h>
#include "DrGuestList.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void switchCore();

private:
  void showHost();
  void showGuests();

  DrGuestList    *m_Guests = nullptr;
  QRetro         *m_RetroB = nullptr;
  QStackedWidget *m_Stack = nullptr;
  QPushButton    *m_SwitchButton = nullptr;
  QLabel         *m_SceneLabel = nullptr;
};

#endif
