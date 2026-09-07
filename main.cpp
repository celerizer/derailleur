#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#ifdef Q_OS_MACOS
/**
 * A bundled app is launched with its working directory set to "/", but every
 * path in derailleur and QRetro is resolved against it, and the bundle itself
 * is not writable. Anchor it in Application Support instead.
 *
 * Running the binary straight out of a build tree keeps the working directory
 * it was given, so a checkout with its own roms/cores next to it still works.
 */
static void dr_anchor_working_directory(void)
{
  const QString exe = QCoreApplication::applicationDirPath();

  if (!exe.endsWith(".app/Contents/MacOS"))
    return;

  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  if (!dir.isEmpty() && QDir().mkpath(dir))
    QDir::setCurrent(dir);
}
#endif

int main(int argc, char *argv[])
{
  /* Hint to use X11/XWayland on Linux for OpenGL context juggling */
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

  QApplication a(argc, argv);

#ifdef Q_OS_MACOS
  dr_anchor_working_directory();
#endif

  MainWindow w;
  w.show();
  return a.exec();
}
