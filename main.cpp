#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  /* Hint to use X11/XWayland on Linux for OpenGL context juggling */
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
