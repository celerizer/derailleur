#include "DrLogger.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
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

  /* Busy progress row pinned to the bottom, hidden until a task drives it. */
  m_progressRow = new QWidget(this);
  QHBoxLayout *prow = new QHBoxLayout(m_progressRow);
  prow->setContentsMargins(4, 2, 4, 2);
  m_progressLabel = new QLabel(m_progressRow);
  m_progress = new QProgressBar(m_progressRow);
  m_progress->setRange(0, 0); // indeterminate / busy
  m_progress->setTextVisible(false);
  prow->addWidget(m_progressLabel);
  prow->addWidget(m_progress, 1);
  m_progressRow->hide();
  layout->addWidget(m_progressRow);

  m_file.setFileName("derailleur.log");
  if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    qWarning("DrLogger: failed to open derailleur.log");
}

void DrLogger::showProgress(const QString &label)
{
  m_progressLabel->setText(label);
  m_progress->setRange(0, 0); // busy until the first byte-progress arrives
  m_progressRow->show();
}

void DrLogger::hideProgress()
{
  m_progressRow->hide();
}

void DrLogger::setProgress(qint64 received, qint64 total)
{
  if (total > 0)
  {
    /* Scale to a percentage so a huge file can't overflow the int range. */
    m_progress->setRange(0, 100);
    m_progress->setValue(static_cast<int>(received * 100 / total));
  }
  else
    m_progress->setRange(0, 0); // unknown size -> keep it busy
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
