#ifndef DR_LOGGER_H
#define DR_LOGGER_H

#include "DrCommon.h"
#include <QFile>
#include <QWidget>
#include <QString>

class QPlainTextEdit;

class DrLogger : public QWidget
{
  Q_OBJECT

public:
  DrLogger(QWidget *parent = nullptr);

public slots:
  void message(unsigned level, const QString &message);

private:
  QPlainTextEdit *m_text = nullptr;
  QFile           m_file;
};

#endif
