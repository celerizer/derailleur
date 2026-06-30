#include "mainwindow.h"

#include <array>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QPushButton>
#include <QRetro.h>
#include <QScreen>
#include <QSet>
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
#include "guests/MarioParty3.h"
#include "guests/CoreDolphin.h"
#include "guests/KirbyAirRide.h"
#include "guests/MarioParty4.h"
#include "guests/MarioParty5.h"
#include "guests/MarioParty6.h"
#include "guests/MarioParty7.h"
#include "guests/MarioPartyAdvance.h"
#include "guests/MarioPartyE.h"
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

  /* Randomize the shared PRNG for singleplayer; a netplay session re-seeds it
   * from the server so peers match. */
  dr_srand(QRandomGenerator::global()->generate());

  m_InputStore = new DrInputStore();
  m_Netplay = new DrNetplay(m_InputStore, this);
  setupNetplay();

  auto *dolphin = new CoreDolphin(this);
  dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
  //dolphin->addGame(new KirbyAirRide(dolphin->core(), dolphin));
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
  addGuest(new MarioParty2());
  addGuest(new MarioParty3());
  addGuest(new SmashRemix());
  addGuest(new MarioTennis());
  //addGuest(new MarioPartyAdvance());
  //addGuest(new MarioPartyE());

#if SHOW_LOGGER
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  m_Guests->logSummary();
#endif

  /* Allowed-mini-games chooser. Edits are routed through DrNetplay so the host
   * is authoritative: setMinigameFilter applies locally (host/singleplayer) and
   * broadcasts; minigameFilterReceived applies the resulting payload everywhere
   * (and reflects the host's choice on a client). */
  m_Filter = new DrMinigameFilter(nullptr);
  m_Filter->populate(m_Guests->guests());
  m_Filter->show();
  connect(m_Filter, &DrMinigameFilter::filterChanged, this,
    [this](const QByteArray &payload) { m_Netplay->setMinigameFilter(payload); });
  connect(m_Netplay, &DrNetplay::minigameFilterReceived, this, [this](QByteArray payload) {
    m_Guests->applyFilter(payload);
    m_Filter->setFromPayload(payload);
  });

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

  attachNetplay();

  /* If we are the netplay server, tell the connected clients to start this same
   * game. (On a client this is a no-op; the client got here via the server's
   * startGameRequested signal.) */
  if (m_Netplay->isServer())
    m_Netplay->startGame(static_cast<int>(host->game()));

  /* Only load guests that have at least one allowed mini-game; the rest never
   * boot (e.g. disabling every Dolphin mini-game skips the Dolphin core load
   * entirely). guestHasCandidate is derived from the host-authoritative filter,
   * so every peer loads the same set. */
  m_warmupQueue.clear();
  for (DrGuest *guest : m_Guests->guests())
    if (m_Guests->guestHasCandidate(guest))
    {
      guest->startCore();
      m_warmupQueue.append(guest);
    }
  m_Host->startCore();

  m_HostContainer = QWidget::createWindowContainer(m_Host->core(), m_Stack);
  m_HostContainer->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(m_HostContainer);

#if SHOW_DEBUG
  connect(m_Debug, &DrDebug::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players) {
      launchMinigame(guest, minigame, players.data());
    });

  connect(m_Debug, &DrDebug::cancelRequested, this, [this]() {
    if (DrGuest *guest = m_Guests->currentGuest())
      guest->cancelMinigame();
    showHost();
  });
#endif

  /* Every peer rolls its own candidates locally from the shared seeded PRNG
   * (dr_rand), so the picks are identical without a network round-trip. This
   * keeps the host state machine in lockstep — the client reaches ROULETTE on
   * the same frame as the server instead of waiting for candidates to arrive. */
  connect(m_Host, &DrHost::candidatesNeeded, this, [this](dr_minigame_type type) {
    std::array<DrMinigameCandidate, 5> candidates = {};
    for (auto &c : candidates)
    {
      const dr_mp_minigame_t *mg = nullptr;
      c.guest = m_Guests->pickMinigame(type, mg);
      c.minigame = mg;
    }
    m_Host->setCandidates(candidates);
  });

  /* Freeze the host on the exact lockstep frame the minigame is chosen, before
   * the async launchMinigame can run a different number of host frames per peer.
   * DirectConnection so it executes synchronously on the host's timing thread. */
  connect(m_Host, &DrHost::minigameRequested, m_Netplay,
    [this](DrMinigameCandidate, std::array<dr_player_t, 4>) { m_Netplay->freezeActiveContext(); },
    Qt::DirectConnection);

  connect(m_Host, &DrHost::minigameRequested, this,
    [this](DrMinigameCandidate candidate, std::array<dr_player_t, 4> players) {
      launchMinigame(candidate.guest, candidate.minigame, players.data());
    });

  m_Stack->setCurrentWidget(m_Guests);
  for (DrGuest *g : m_Guests->guests())
    g->pause();

  m_warmupIndex = 0;
  if (m_warmupQueue.isEmpty())
    showHost(); // nothing allowed to load; go straight to the host
  else
    warmupStep();
}

void MainWindow::warmupStep()
{
  DrGuest *guest = m_warmupQueue.value(m_warmupIndex, nullptr);
  if (!guest)
  {
    showHost();
    return;
  }

  // Show and run this guest while it warms up.
  m_Guests->setCurrentIndex(m_Guests->guests().indexOf(guest));
  guest->unpause();
#if SHOW_LOGGER
  m_Logger->message(DR_LOG_INFO, QString("preloading %1...").arg(guest->name()));
#endif

  m_warmupFrameCount = 0;
  m_warmupConnection = connect(guest->core(), &QRetro::frameEnd, this,
    [this, guest]() {
      if (++m_warmupFrameCount < guest->warmupFrames())
        return;
      disconnect(m_warmupConnection);
#if SHOW_LOGGER
      m_Logger->message(DR_LOG_INFO, QString("%1 ready").arg(guest->name()));
#endif
      guest->pause();
      m_warmupIndex++;
      if (m_warmupIndex < m_warmupQueue.size())
        warmupStep();
      else
        showHost();
    }, Qt::QueuedConnection);
}

void MainWindow::launchMinigame(
  DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4])
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
      players = std::array<dr_player_t, 4>{ players[0], players[1], players[2], players[3] }]() {
      m_Host->pause();
      m_Stack->setCurrentWidget(m_Guests);
      for (DrGuest *g : m_Guests->guests())
        g->pause();

      guest->core()->audio()->setVolume(0);
      guest->setMinigame(minigame);
      for (unsigned i = 0; i < 4; i++)
        guest->setPlayer(i, players[i]);

      /* A minigame state was just loaded; make this guest the foreground netplay
       * context so it resynchronizes from this sync point. Done before unpause
       * so its frame counter is reset while the core is still stopped. */
      m_Netplay->setActiveContext(guest->core());

      guest->core()->audio()->setVolume(100);
      guest->unpause();

#if SHOW_OVERLAY
      m_Overlay->fadeOut();
#endif
    });
}

void MainWindow::setupNetplay()
{
  m_NetplayUi = new DrNetplayWidget(m_Netplay, nullptr);
  m_NetplayUi->show();

  connect(m_Netplay, &DrNetplay::sessionStarted, this, [this](int index, int count) {
#if SHOW_LOGGER
    m_Logger->message(DR_LOG_INFO,
      QString("netplay started: peer %1 of %2").arg(index).arg(count));
#endif
  });
  connect(m_Netplay, &DrNetplay::peerCountChanged, this, [this](int connected, int total) {
#if SHOW_LOGGER
    m_Logger->message(DR_LOG_INFO,
      QString("netplay peers: %1/%2 connected").arg(connected).arg(total));
#endif
  });
  connect(m_Netplay, &DrNetplay::sessionError, this, [this](const QString &reason) {
#if SHOW_LOGGER
    m_Logger->message(DR_LOG_ERROR, reason);
#endif
    QMessageBox::warning(this, tr("Netplay"), reason);
  });
#if SHOW_LOGGER
  connect(m_Netplay, &DrNetplay::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  /* A client follows the server's game choice: build the matching host and
   * start it locally. */
  connect(m_Netplay, &DrNetplay::startGameRequested, this, [this](int gameId) {
    if (m_Host)
      return;
    DrHost *host = nullptr;
    switch (static_cast<dr_game>(gameId))
    {
    case DR_GAME_MARIOPARTY1:
      host = new MarioParty1Host(this);
      break;
    case DR_GAME_MARIOPARTY2:
      host = new MarioParty2Host(this);
      break;
    case DR_GAME_MARIOPARTY3:
      host = new MarioParty3Host(this);
      break;
    default:
      break;
    }
    if (host)
      startWithHost(host);
  });
}

void MainWindow::attachNetplay()
{
  /* Use the host core's existing physical backend as the local input source,
   * then install a shared backend on every distinct core so host and guests
   * read identical input from the store. */
  m_Netplay->setLocalSource(m_Host->core()->input()->backend());

  QSet<QRetro *> seen;
  m_Netplay->attachCore(m_Host->core());
  seen.insert(m_Host->core());
  for (DrGuest *guest : m_Guests->guests())
  {
    /* Skip guests with no allowed mini-game: they are not loaded, so they must
     * not consume a netplay context id either. The filter is identical across
     * peers, so every peer skips the same guests and the remaining context ids
     * line up. */
    if (!m_Guests->guestHasCandidate(guest))
      continue;
    QRetro *core = guest->core();
    if (core && !seen.contains(core))
    {
      m_Netplay->attachCore(core);
      seen.insert(core);
    }
  }
}

void MainWindow::showHost()
{
#if SHOW_OVERLAY
  {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    m_Overlay->hold(screen->grabWindow(m_Guests->currentGuest()->core()->winId()));
  }
#endif

  QTimer::singleShot(32, this, [this]() {
    for (DrGuest *guest : m_Guests->guests())
      guest->pause();
    /* The host is the foreground netplay context; set it before unpause so its
     * frame counter resets while the core is still stopped. This is the
     * post-warmup (and post-minigame) sync point that locks the two peers. */
    m_Netplay->setActiveContext(m_Host->core());
    m_Host->unpause();
    m_Stack->setCurrentWidget(m_HostContainer);
#if SHOW_OVERLAY
    m_Overlay->fadeOut();
#endif
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
