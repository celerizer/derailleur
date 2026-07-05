#ifndef DR_LOGGER_H
#define DR_LOGGER_H

#include "DrCommon.h"
#include <QFile>
#include <QWidget>
#include <QString>

class QLabel;
class QPlainTextEdit;
class QProgressBar;

class DrLogger : public QWidget
{
  Q_OBJECT

public:
  DrLogger(QWidget *parent = nullptr);

public slots:
  void message(unsigned level, const QString &message);

  /// Show a busy progress row along the bottom of the log with `label`. Call
  /// again to retitle it; hideProgress() removes it.
  void showProgress(const QString &label);
  void hideProgress();

private:
  QPlainTextEdit *m_text = nullptr;
  QWidget *m_progressRow = nullptr;
  QLabel *m_progressLabel = nullptr;
  QProgressBar *m_progress = nullptr;
  QFile m_file;
};

#endif
