#include "DrNetplayWidget.h"

#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

#include "DrNetplay.h"

namespace
{
/* Same derailleur.ini MainWindow uses for paths. */
QSettings netplaySettings()
{
  return QSettings(QDir::current().filePath("derailleur.ini"), QSettings::IniFormat);
}
}

DrNetplayWidget::DrNetplayWidget(DrNetplay *netplay, QWidget *parent)
  : QWidget(parent), m_netplay(netplay)
{
  setWindowTitle(tr("Netplay"));

  QSettings settings = netplaySettings();

  QGroupBox *hostBox = new QGroupBox(tr("Host"), this);
  QFormLayout *hostForm = new QFormLayout(hostBox);
  m_hostPort = new QSpinBox(hostBox);
  m_hostPort->setRange(1, 65535);
  m_hostPort->setValue(settings.value("netplay/host_port", 55435).toInt());
  m_hostPlayers = new QSpinBox(hostBox);
  m_hostPlayers->setRange(2, DR_NETPLAY_MAX_PEERS);
  m_hostPlayers->setValue(2);
  m_hostButton = new QPushButton(tr("Host session"), hostBox);
  hostForm->addRow(tr("Port:"), m_hostPort);
  hostForm->addRow(tr("Players:"), m_hostPlayers);
  hostForm->addRow(m_hostButton);

  QGroupBox *joinBox = new QGroupBox(tr("Join"), this);
  QFormLayout *joinForm = new QFormLayout(joinBox);
  m_joinAddress =
    new QLineEdit(settings.value("netplay/address", "127.0.0.1").toString(), joinBox);
  m_joinPort = new QSpinBox(joinBox);
  m_joinPort->setRange(1, 65535);
  m_joinPort->setValue(settings.value("netplay/port", 55435).toInt());
  m_joinButton = new QPushButton(tr("Join session"), joinBox);
  joinForm->addRow(tr("Address:"), m_joinAddress);
  joinForm->addRow(tr("Port:"), m_joinPort);
  joinForm->addRow(m_joinButton);

  QGroupBox *tuningBox = new QGroupBox(tr("Tuning"), this);
  QFormLayout *tuningForm = new QFormLayout(tuningBox);
  m_buffer = new QSpinBox(tuningBox);
  m_buffer->setRange(0, 30);
  m_buffer->setValue(m_netplay ? m_netplay->inputDelay() : 2);
  m_buffer->setToolTip(tr("Input delay in frames. Higher hides more latency but "
                          "adds input lag; lower is snappier but stalls on jitter. "
                          "Can be changed mid-session and is applied to all peers."));
  tuningForm->addRow(tr("Buffer frames:"), m_buffer);

  m_resyncButton = new QPushButton(tr("Hard resync"), tuningBox);
  m_resyncButton->setToolTip(tr("Host only: pause all peers, send them this "
                                "instance's savestate, and resume in lockstep."));
  m_resyncButton->setEnabled(false);
  tuningForm->addRow(m_resyncButton);

  m_status = new QLabel(tr("Not connected"), this);

  QHBoxLayout *connectRow = new QHBoxLayout;
  connectRow->setContentsMargins(0, 0, 0, 0);
  connectRow->addWidget(hostBox, 1);
  connectRow->addWidget(joinBox, 1);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(6);
  layout->addLayout(connectRow);
  layout->addWidget(tuningBox);
  layout->addWidget(m_status);
  layout->addStretch();

  if (m_netplay)
  {
    connect(m_buffer, QOverload<int>::of(&QSpinBox::valueChanged), this,
      [this](int frames) { m_netplay->changeInputDelay(frames); });

    /* Reflect a delay change that originated from another peer without echoing
     * it back out (which would loop). */
    connect(m_netplay, &DrNetplay::inputDelayChanged, this, [this](int frames) {
      QSignalBlocker block(m_buffer);
      m_buffer->setValue(frames);
    });

    connect(m_hostButton, &QPushButton::clicked, this, [this]() {
      QSettings settings = netplaySettings();
      settings.setValue("netplay/host_port", m_hostPort->value());
      m_netplay->hostSession(
        static_cast<quint16>(m_hostPort->value()), m_hostPlayers->value());
      setStatus(tr("Hosting on port %1, waiting for players...").arg(m_hostPort->value()));
      lockControls(true);
    });

    connect(m_joinButton, &QPushButton::clicked, this, [this]() {
      if (m_joinAddress->text().isEmpty())
        return;
      QSettings settings = netplaySettings();
      settings.setValue("netplay/address", m_joinAddress->text());
      settings.setValue("netplay/port", m_joinPort->value());
      m_netplay->joinSession(
        m_joinAddress->text(), static_cast<quint16>(m_joinPort->value()));
      setStatus(tr("Connecting to %1...").arg(m_joinAddress->text()));
      lockControls(true);
    });

    connect(m_resyncButton, &QPushButton::clicked, this,
      [this]() { m_netplay->requestHardResync(); });

    connect(m_netplay, &DrNetplay::sessionStarted, this, [this](int index, int count) {
      setStatus(tr("Connected as peer %1 of %2").arg(index + 1).arg(count));
      lockControls(true);
      /* Only the host (peer 0) can drive a resync. */
      m_resyncButton->setEnabled(index == 0);
    });
    connect(m_netplay, &DrNetplay::peerCountChanged, this, [this](int connected, int total) {
      setStatus(tr("%1 of %2 peers connected").arg(connected).arg(total));
    });
    connect(m_netplay, &DrNetplay::lobbyJoined, this, [this](int index, int count) {
      setStatus(tr("Connected as peer %1 of %2, waiting for host to start...")
                  .arg(index + 1).arg(count));
    });
    connect(m_netplay, &DrNetplay::sessionError, this, [this](const QString &reason) {
      setStatus(tr("Error: %1").arg(reason));
      lockControls(false);
      m_resyncButton->setEnabled(false);
    });
  }
}

void DrNetplayWidget::setStatus(const QString &text)
{
  m_status->setText(text);
}

void DrNetplayWidget::lockControls(bool locked)
{
  /* Host/join can only be set before a session; the buffer stays editable so it
   * can be retuned live. */
  m_hostPort->setEnabled(!locked);
  m_hostPlayers->setEnabled(!locked);
  m_hostButton->setEnabled(!locked);
  m_joinAddress->setEnabled(!locked);
  m_joinPort->setEnabled(!locked);
  m_joinButton->setEnabled(!locked);
}
