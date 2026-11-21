#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
  /* Hint to use X11/XWayland on Linux for OpenGL context juggling */
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

  QApplication a(argc, argv);
  a.setWindowIcon(QIcon(":/assets/icon.png"));
  MainWindow w;
  w.show();
  return a.exec();
}
