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

  void showProgress(const QString &label);
  void hideProgress();
  void setProgress(qint64 received, qint64 total);

private:
  QPlainTextEdit *m_text = nullptr;
  QWidget *m_progressRow = nullptr;
  QLabel *m_progressLabel = nullptr;
  QProgressBar *m_progress = nullptr;
  QFile m_file;
};

#endif
