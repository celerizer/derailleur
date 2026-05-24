#include "mainwindow.h"

#include <array>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QRetro.h>
#include <QScreen>
#include <QSettings>
#include <QStackedWidget>
#include <QString>
#include <QWidget>

#include "DrDebug.h"
#include "hosts/MarioParty3.h"
#include "guests/MarioKart64.h"
#include "guests/CoreDolphin.h"
#include "guests/MarioParty4.h"
#include "guests/MarioParty5.h"
#include "guests/MarioParty6.h"
#include "guests/MarioParty7.h"
#include "guests/SmashRemix.h"

#define SHOW_LOGGER 1
#define SHOW_OVERLAY 1
#define SHOW_DEBUG 1


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
#if SHOW_LOGGER
  m_Logger = new DrLogger(nullptr);
  m_Logger->setWindowTitle("Log");
  m_Logger->resize(600, 300);
  m_Logger->show();
#endif

  {
    QDir cwd = QDir::current();
    QString iniPath = cwd.filePath("derailleur.ini");
    bool iniExisted = QFile::exists(iniPath);
    QSettings s(iniPath, QSettings::IniFormat);
    auto load = [&](const char *key, const QString &def) {
      if (!s.contains(key)) s.setValue(key, def);
      return s.value(key).toString();
    };
    dr_set_roms_directory(load("paths/roms",   cwd.filePath("roms")));
    dr_set_cores_directory(load("paths/cores", cwd.filePath("cores")));
    dr_set_state_directory(load("paths/state", cwd.filePath("state")));
    s.sync();
#if SHOW_LOGGER
    if (!iniExisted)
      m_Logger->message(DR_LOG_INFO, QString("created %1").arg(iniPath));
    else
      m_Logger->message(DR_LOG_INFO, QString("loaded %1").arg(iniPath));
    m_Logger->message(DR_LOG_INFO, QString("paths/roms: %1").arg(dr_roms_directory()));
    m_Logger->message(DR_LOG_INFO, QString("paths/cores: %1").arg(dr_cores_directory()));
    m_Logger->message(DR_LOG_INFO, QString("paths/state: %1").arg(dr_state_directory()));
#endif
  }

  m_Guests = new DrGuestList(this);
#if SHOW_LOGGER
  connect(m_Guests, &DrGuestList::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  /*auto *dolphin = new CoreDolphin(this);
  dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
  dolphin->finalizeGames();
  if (dolphin->isValid())
    m_Guests->add(dolphin);
    */

  auto addGuest = [this](DrGuest *g) {
    if (g->isValid()) m_Guests->add(g);
    else delete g;
  };
  addGuest(new MarioKart64());
  //addGuest(new SmashRemix());

#if SHOW_LOGGER
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrRetro::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  m_Guests->logSummary();
#endif

  m_Host = new MarioParty3(this);
#if SHOW_LOGGER
  connect(m_Host, &DrRetro::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  for (DrGuest *guest : m_Guests->guests())
    guest->startCore();
  m_Host->startCore();

  m_Stack = new QStackedWidget(this);
  m_Stack->addWidget(m_Guests);
  QWidget *containerB = QWidget::createWindowContainer(m_Host->core(), m_Stack);
  containerB->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(containerB);

  setCentralWidget(m_Stack);
#if SHOW_OVERLAY
  m_Overlay = new DrOverlay(this);
  {
    QPixmap loading(":/assets/loading.png");
    m_Overlay->hold(loading);
  }
#endif

  // Cycle through each guest for 2 seconds, then switch to the host
  m_Stack->setCurrentIndex(0);
  for (DrGuest *g : m_Guests->guests())
    if (g != m_Guests->currentGuest()) g->pause();

  QTimer *warmupTimer = new QTimer(this);
  warmupTimer->setInterval(2000);
  connect(warmupTimer, &QTimer::timeout, this, [this, warmupTimer]() {
    int next = m_Guests->currentIndex() + 1;
    if (next < m_Guests->count()) {
      m_Guests->currentGuest()->pause();
      m_Guests->setCurrentIndex(next);
      m_Guests->currentGuest()->unpause();
    } else {
      warmupTimer->stop();
      warmupTimer->deleteLater();
      showHost();
    }
  });
  warmupTimer->start();

#if SHOW_DEBUG
  m_Debug = new DrDebug(nullptr);
  m_Debug->setWindowTitle("Debug");
  m_Debug->resize(300, 200);
  m_Debug->populate(m_Guests->guests());
  m_Debug->show();

  connect(m_Debug, &DrDebug::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame,
           std::array<dr_player_t, 4> players, std::array<bool, 4> playerValid) {
      launchMinigame(guest, minigame, players.data(), playerValid.data());
    });

  connect(m_Debug, &DrDebug::cancelRequested, this, [this]() {
    if (DrGuest *guest = m_Guests->currentGuest())
      guest->cancelMinigame();
    showHost();
  });
#endif

  connect(m_Guests, &QStackedWidget::currentChanged, this, [this](int index) {
    const auto &guests = m_Guests->guests();
    if (index >= 0 && index < guests.size()) {
      if (QWidget *container = m_Guests->widget(index))
        guests[index]->core()->resize(container->width(), container->height());
    }
  });

  connect(m_Guests, &DrGuestList::minigameFinished, this, [this]() {
    m_Host->writeResults(m_Guests->currentGuest());
    showHost();
  }, Qt::QueuedConnection);

  connect(m_Host, &DrHost::minigameRequested, this,
    [this](dr_minigame_type type, std::array<dr_player_t, 4> players, std::array<bool, 4> playerValid) {
      launchMinigame(type, players.data(), playerValid.data());
    });

  resize(704, 528);
}

void MainWindow::launchMinigame(DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4], const bool playerValid[4])
{
  if (!m_Guests->activateGuest(guest))
    return;

#if SHOW_OVERLAY
  {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    m_Overlay->hold(screen->grabWindow(m_Host->core()->winId()));
  }
#endif

  QTimer::singleShot(32, this, [this, guest, minigame, players = std::array<dr_player_t, 4>{players[0], players[1], players[2], players[3]}, playerValid = std::array<bool, 4>{playerValid[0], playerValid[1], playerValid[2], playerValid[3]}]() {
    m_Host->pause();
    m_Stack->setCurrentIndex(0);
    for (DrGuest *g : m_Guests->guests())
      g->pause();

    guest->core()->audio()->setVolume(0);
    guest->setMinigame(minigame);
    for (unsigned i = 0; i < 4; i++)
      if (playerValid[i]) guest->setPlayer(i, players[i]);
    guest->core()->audio()->setVolume(100);
    guest->unpause();

#if SHOW_OVERLAY
    m_Overlay->fadeOut();
#endif
  });
}

void MainWindow::launchMinigame(dr_minigame_type type, const dr_player_t players[4], const bool playerValid[4])
{
  const dr_mp_minigame_t *minigame = nullptr;
  DrGuest *guest = m_Guests->pickMinigame(type, minigame);
  if (!guest)
    return;

#if SHOW_OVERLAY
  {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    m_Overlay->hold(screen->grabWindow(m_Host->core()->winId()));
  }
#endif

  QTimer::singleShot(32, this, [this, guest, minigame, players = std::array<dr_player_t, 4>{players[0], players[1], players[2], players[3]}, playerValid = std::array<bool, 4>{playerValid[0], playerValid[1], playerValid[2], playerValid[3]}]() {
    m_Host->pause();
    m_Stack->setCurrentIndex(0);
    for (DrGuest *g : m_Guests->guests())
      g->pause();

    guest->core()->audio()->setVolume(0);
    guest->setMinigame(minigame);
    for (unsigned i = 0; i < 4; i++)
      if (playerValid[i]) guest->setPlayer(i, players[i]);
    guest->core()->audio()->setVolume(100);
    guest->unpause();

#if SHOW_OVERLAY
    m_Overlay->fadeOut();
#endif
  });
}

void MainWindow::showHost()
{
#if SHOW_OVERLAY
  QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
  m_Overlay->flash(screen->grabWindow(m_Guests->currentGuest()->core()->winId()));
#endif

  QTimer::singleShot(32, this, [this]() {
    for (DrGuest *guest : m_Guests->guests())
      guest->pause();
    m_Host->unpause();
    m_Stack->setCurrentIndex(1);
  });
}

void MainWindow::showGuests()
{
#if SHOW_OVERLAY
  QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
  m_Overlay->hold(screen->grabWindow(m_Host->core()->winId()));
#endif

  QTimer::singleShot(32, this, [this]() {
    m_Host->pause();
    m_Stack->setCurrentIndex(0);
    for (DrGuest *guest : m_Guests->guests())
      guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  });
}

MainWindow::~MainWindow()
{
}
