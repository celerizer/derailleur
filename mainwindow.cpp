#include "mainwindow.h"

#include <array>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QRetro.h>
#include <QScreen>
#include <QSettings>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "DrDebug.h"
#include "hosts/MarioParty1Host.h"
#include "hosts/MarioParty2Host.h"
#include "hosts/MarioParty3Host.h"
#include "guests/MarioKart64.h"
#include "guests/MarioParty1.h"
#include "guests/MarioParty2.h"
#include "guests/CoreDolphin.h"
#include "guests/MarioParty4.h"
#include "guests/MarioParty5.h"
#include "guests/MarioParty6.h"
#include "guests/MarioParty7.h"
#include "guests/SmashRemix.h"
#include "guests/MarioTennis.h"

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
      if (!s.contains(key))
        s.setValue(key, def);
      return s.value(key).toString();
    };
    dr_set_roms_directory(load("paths/roms", cwd.filePath("roms")));
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

  auto *dolphin = new CoreDolphin(this);
  dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
  dolphin->finalizeGames();
  if (dolphin->isValid())
    m_Guests->add(dolphin);

  auto addGuest = [this](DrGuest *g) {
    if (g->isValid())
      m_Guests->add(g);
    else
      delete g;
  };
  //addGuest(new MarioKart64());
  addGuest(new MarioParty1());
  //addGuest(new MarioParty2());
  addGuest(new SmashRemix());
  addGuest(new MarioTennis());

#if SHOW_LOGGER
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrRetro::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  m_Guests->logSummary();
#endif

  m_Stack = new QStackedWidget(this);

  // Host chooser — shown once at boot, never again
  QWidget *chooser = new QWidget(m_Stack);
  QVBoxLayout *chooserLayout = new QVBoxLayout(chooser);
  QLabel *chooserLabel = new QLabel("Choose Host", chooser);
  chooserLabel->setAlignment(Qt::AlignCenter);
  chooserLayout->addStretch();
  chooserLayout->addWidget(chooserLabel);

  auto addHostButton = [&](const QString &name, auto factory) {
    QPushButton *btn = new QPushButton(name, chooser);
    connect(btn, &QPushButton::clicked, this, [this, factory]() { startWithHost(factory()); });
    chooserLayout->addWidget(btn);
  };
  addHostButton("Mario Party 1", [this]() -> DrHost * { return new MarioParty1Host(this); });
  addHostButton("Mario Party 2", [this]() -> DrHost * { return new MarioParty2Host(this); });
  addHostButton("Mario Party 3", [this]() -> DrHost * { return new MarioParty3Host(this); });
  chooserLayout->addStretch();

  m_Stack->addWidget(chooser); // index 0 — chooser
  m_Stack->addWidget(m_Guests); // index 1 — guests
  // host container added in startWithHost (index 2)

  setCentralWidget(m_Stack);

#if SHOW_OVERLAY
  m_Overlay = new DrOverlay(this);
#endif

  m_Stack->setCurrentIndex(0);

#if SHOW_DEBUG
  m_Debug = new DrDebug(nullptr);
  m_Debug->setWindowTitle("Debug");
  m_Debug->resize(300, 200);
  m_Debug->populate(m_Guests->guests());
  m_Debug->show();
#endif

  connect(m_Stack, &QStackedWidget::currentChanged, this, [this](int index) {
    const auto &guests = m_Guests->guests();
    if (m_Stack->widget(index) == m_Guests && m_Guests->currentIndex() >= 0 &&
        m_Guests->currentIndex() < guests.size())
    {
      if (QWidget *container = m_Guests->widget(m_Guests->currentIndex()))
        guests[m_Guests->currentIndex()]->core()->resize(container->width(), container->height());
    }
  });

  connect(
    m_Guests, &DrGuestList::minigameFinished, this,
    [this]() {
      m_Host->writeResults(m_Guests->currentGuest());
      showHost();
    },
    Qt::QueuedConnection);

  resize(704, 528);
}

void MainWindow::startWithHost(DrHost *host)
{
  m_Host = host;

#if SHOW_OVERLAY
  {
    QPixmap loading(":/assets/loading.png");
    m_Overlay->hold(loading);
  }
#endif

#if SHOW_LOGGER
  connect(m_Host, &DrRetro::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  for (DrGuest *guest : m_Guests->guests())
    guest->startCore();
  m_Host->startCore();

  m_HostContainer = QWidget::createWindowContainer(m_Host->core(), m_Stack);
  m_HostContainer->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(m_HostContainer);

#if SHOW_DEBUG
  connect(m_Debug, &DrDebug::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players,
      std::array<bool, 4> playerValid) {
      launchMinigame(guest, minigame, players.data(), playerValid.data());
    });

  connect(m_Debug, &DrDebug::cancelRequested, this, [this]() {
    if (DrGuest *guest = m_Guests->currentGuest())
      guest->cancelMinigame();
    showHost();
  });
#endif

  auto pickCandidates = [this](dr_minigame_type type) {
    std::array<DrMinigameCandidate, 5> candidates = {};
    for (auto &c : candidates)
    {
      const dr_mp_minigame_t *mg = nullptr;
      c.guest = m_Guests->pickMinigame(type, mg);
      c.minigame = mg;
    }
    m_Host->setCandidates(candidates);
  };

  connect(m_Host, &DrHost::candidatesNeeded, this,
    [this, pickCandidates](dr_minigame_type type) { pickCandidates(type); });

  connect(m_Host, &DrHost::candidatesRequested, this,
    [this, pickCandidates](
      dr_minigame_type type, std::array<dr_player_t, 4> players, std::array<bool, 4> playerValid) {
      pickCandidates(type);
      m_Host->startMinigame(0);
    });

  connect(m_Host, &DrHost::minigameRequested, this,
    [this](DrMinigameCandidate candidate, std::array<dr_player_t, 4> players,
      std::array<bool, 4> playerValid) {
      launchMinigame(candidate.guest, candidate.minigame, players.data(), playerValid.data());
    });

  m_Stack->setCurrentWidget(m_Guests);
  for (DrGuest *g : m_Guests->guests())
    if (g != m_Guests->currentGuest())
      g->pause();

#if SHOW_LOGGER
  m_Logger->message(DR_LOG_INFO, QString("preloading %1...").arg(m_Guests->currentGuest()->name()));
#endif

  QTimer *warmupTimer = new QTimer(this);
  warmupTimer->setInterval(2000);
  connect(warmupTimer, &QTimer::timeout, this, [this, warmupTimer]() {
    int next = m_Guests->currentIndex() + 1;
    if (next < m_Guests->count())
    {
#if SHOW_LOGGER
      m_Logger->message(DR_LOG_INFO, QString("%1 ready").arg(m_Guests->currentGuest()->name()));
#endif
      m_Guests->currentGuest()->pause();
      m_Guests->setCurrentIndex(next);
      m_Guests->currentGuest()->unpause();
#if SHOW_LOGGER
      m_Logger->message(
        DR_LOG_INFO, QString("preloading %1...").arg(m_Guests->currentGuest()->name()));
#endif
    }
    else
    {
#if SHOW_LOGGER
      m_Logger->message(DR_LOG_INFO, QString("%1 ready").arg(m_Guests->currentGuest()->name()));
#endif
      warmupTimer->stop();
      warmupTimer->deleteLater();
      showHost();
    }
  });
  warmupTimer->start();
}

void MainWindow::launchMinigame(DrGuest *guest, const dr_mp_minigame_t *minigame,
  const dr_player_t players[4], const bool playerValid[4])
{
  if (!m_Guests->activateGuest(guest))
    return;

#if SHOW_OVERLAY
  {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    m_Overlay->hold(screen->grabWindow(m_Host->core()->winId()));
  }
#endif

  QTimer::singleShot(32, this,
    [this, guest, minigame,
      players = std::array<dr_player_t, 4>{ players[0], players[1], players[2], players[3] },
      playerValid =
        std::array<bool, 4>{ playerValid[0], playerValid[1], playerValid[2], playerValid[3] }]() {
      m_Host->pause();
      m_Stack->setCurrentWidget(m_Guests);
      for (DrGuest *g : m_Guests->guests())
        g->pause();

      guest->core()->audio()->setVolume(0);
      guest->setMinigame(minigame);
      for (unsigned i = 0; i < 4; i++)
        if (playerValid[i])
          guest->setPlayer(i, players[i]);
      guest->core()->audio()->setVolume(100);
      guest->unpause();

#if SHOW_OVERLAY
      m_Overlay->fadeOut();
#endif
    });
}

void MainWindow::launchMinigame(
  dr_minigame_type type, const dr_player_t players[4], const bool playerValid[4])
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

  QTimer::singleShot(32, this,
    [this, guest, minigame,
      players = std::array<dr_player_t, 4>{ players[0], players[1], players[2], players[3] },
      playerValid =
        std::array<bool, 4>{ playerValid[0], playerValid[1], playerValid[2], playerValid[3] }]() {
      m_Host->pause();
      m_Stack->setCurrentWidget(m_Guests);
      for (DrGuest *g : m_Guests->guests())
        g->pause();

      guest->core()->audio()->setVolume(0);
      guest->setMinigame(minigame);
      for (unsigned i = 0; i < 4; i++)
        if (playerValid[i])
          guest->setPlayer(i, players[i]);
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
    m_Stack->setCurrentWidget(m_HostContainer);
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
    m_Stack->setCurrentWidget(m_Guests);
    for (DrGuest *guest : m_Guests->guests())
      guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  });
}

MainWindow::~MainWindow() {}
