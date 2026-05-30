#include "DrLogger.h"

#include <QFile>
#include <QPlainTextEdit>
#include <QVBoxLayout>

DrLogger::DrLogger(QWidget *parent)
  : QWidget(parent)
{
  m_text = new QPlainTextEdit(this);
  m_text->setReadOnly(true);
  m_text->setMaximumBlockCount(1000);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_text);

  m_file.setFileName("derailleur.log");
  m_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
}

void DrLogger::message(unsigned level, const QString &message)
{
  static const char *prefixes[] = { "INFO", "WARN", "ERROR" };
  const char *prefix = level < 3 ? prefixes[level] : "LOG";

  QString line = QString("[%1] %2").arg(prefix).arg(message);
  m_text->appendPlainText(line);

  if (m_file.isOpen())
  {
    m_file.write((line + '\n').toUtf8());
    m_file.flush();
  }
}
