#ifndef DR_NETPLAY_WIDGET_H
#define DR_NETPLAY_WIDGET_H

#include <QWidget>

class DrNetplay;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

/**
 * Standalone window exposing the netplay host/join controls and the input
 * buffer (delay) setting, driving a DrNetplay instance.
 */
class DrNetplayWidget : public QWidget
{
  Q_OBJECT

public:
  explicit DrNetplayWidget(DrNetplay *netplay, QWidget *parent = nullptr);

private:
  void setStatus(const QString &text);
  void lockControls(bool locked);

  DrNetplay *m_netplay = nullptr;

  QSpinBox *m_hostPort = nullptr;
  QSpinBox *m_hostPlayers = nullptr;
  QPushButton *m_hostButton = nullptr;

  QLineEdit *m_joinAddress = nullptr;
  QSpinBox *m_joinPort = nullptr;
  QPushButton *m_joinButton = nullptr;

  QSpinBox *m_buffer = nullptr;
  QPushButton *m_resyncButton = nullptr;
  QLabel *m_status = nullptr;
};

#endif
