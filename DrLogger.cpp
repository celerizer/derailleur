#include "DrLogger.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>

DrLogger::DrLogger(QWidget *parent) : QWidget(parent)
{
  m_text = new QPlainTextEdit(this);
  m_text->setReadOnly(true);
  m_text->setMaximumBlockCount(1000);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_text);
}

void DrLogger::message(unsigned level, const QString &message)
{
  static const char *prefixes[] = { "INFO", "WARN", "ERROR" };
  const char *prefix = level < 3 ? prefixes[level] : "LOG";

  m_text->appendPlainText(QString("[%1] %2").arg(prefix).arg(message));
}
