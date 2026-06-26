#include "mainwindow.h"

#include <array>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
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

  m_InputStore = new DrInputStore();
  m_Netplay = new DrNetplay(m_InputStore, this);
  buildNetplayMenu();

  auto *dolphin = new CoreDolphin(this);
  dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
  dolphin->addGame(new KirbyAirRide(dolphin->core(), dolphin));
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
  //addGuest(new MarioParty1());
  //addGuest(new MarioParty2());
  //addGuest(new MarioParty3());
  //addGuest(new SmashRemix());
  //addGuest(new MarioTennis());
  //addGuest(new MarioPartyAdvance());
  //addGuest(new MarioPartyE());

#if SHOW_LOGGER
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
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

  attachNetplay();

  /* If we are the netplay server, tell the connected clients to start this same
   * game. (On a client this is a no-op; the client got here via the server's
   * startGameRequested signal.) */
  if (m_Netplay->isServer())
    m_Netplay->startGame(static_cast<int>(host->game()));

  for (DrGuest *guest : m_Guests->guests())
    guest->startCore();
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

  /* Map a candidate to opaque (guestIndex, minigameIndex) for the network. */
  auto candidateToPair = [this](const DrMinigameCandidate &c) -> QPair<int, int> {
    if (!c.guest || !c.minigame)
      return { -1, -1 };
    const int g = m_Guests->guests().indexOf(c.guest);
    int m = -1;
    const dr_mp_minigame_t *list = c.guest->minigames();
    for (int i = 0; list && list[i].name; i++)
      if (&list[i] == c.minigame)
      {
        m = i;
        break;
      }
    return { g, m };
  };

  auto pickCandidates = [this, candidateToPair](dr_minigame_type type) {
    /* On a client the candidates are authoritative from the server; wait for
     * the candidatesReceived signal instead of rolling our own. */
    if (m_Netplay->sessionActive() && !m_Netplay->isServer())
      return;

    std::array<DrMinigameCandidate, 5> candidates = {};
    for (auto &c : candidates)
    {
      const dr_mp_minigame_t *mg = nullptr;
      c.guest = m_Guests->pickMinigame(type, mg);
      c.minigame = mg;
    }
    m_Host->setCandidates(candidates);

    /* As server, share the exact picks so every peer rolls the same roulette. */
    if (m_Netplay->sessionActive() && m_Netplay->isServer())
    {
      QList<QPair<int, int>> picks;
      for (const auto &c : candidates)
        picks.append(candidateToPair(c));
      m_Netplay->sendCandidates(picks);
    }
  };

  connect(m_Host, &DrHost::candidatesNeeded, this,
    [this, pickCandidates](dr_minigame_type type) { pickCandidates(type); });

  /* Client: apply the server's candidate picks. */
  connect(m_Netplay, &DrNetplay::candidatesReceived, this,
    [this](QList<QPair<int, int>> picks) {
      if (!m_Host)
        return;
      const auto &guests = m_Guests->guests();
      std::array<DrMinigameCandidate, 5> candidates = {};
      for (int i = 0; i < 5 && i < picks.size(); i++)
      {
        const int g = picks[i].first;
        const int m = picks[i].second;
        if (g < 0 || g >= guests.size() || m < 0)
          continue;
        DrGuest *guest = guests[g];
        const dr_mp_minigame_t *list = guest->minigames();
        int count = 0;
        for (; list && list[count].name; count++)
          ;
        if (m < count)
        {
          candidates[i].guest = guest;
          candidates[i].minigame = &list[m];
        }
      }
      m_Host->setCandidates(candidates);
    });

  connect(m_Host, &DrHost::minigameRequested, this,
    [this](DrMinigameCandidate candidate, std::array<dr_player_t, 4> players) {
      launchMinigame(candidate.guest, candidate.minigame, players.data());
    });

  m_Stack->setCurrentWidget(m_Guests);
  for (DrGuest *g : m_Guests->guests())
    if (g != m_Guests->currentGuest())
      g->pause();

#if SHOW_LOGGER
  m_Logger->message(DR_LOG_INFO, QString("preloading %1...").arg(m_Guests->currentGuest()->name()));
#endif

  QTimer *warmupTimer = new QTimer(this);
  warmupTimer->setInterval(5000);
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

void MainWindow::buildNetplayMenu()
{
  QMenu *menu = menuBar()->addMenu(tr("Netplay"));

  menu->addAction(tr("Host session..."), this, [this]() {
    bool ok = false;
    int port = QInputDialog::getInt(this, tr("Host Session"), tr("Port:"), 55435, 1, 65535, 1, &ok);
    if (!ok)
      return;
    int players = QInputDialog::getInt(this, tr("Host Session"), tr("Players:"), 2, 2,
      DrNetplay::k_MaxPeers, 1, &ok);
    if (!ok)
      return;
    m_Netplay->hostSession(static_cast<quint16>(port), players);
  });

  menu->addAction(tr("Join session..."), this, [this]() {
    bool ok = false;
    QString addr =
      QInputDialog::getText(this, tr("Join Session"), tr("Server address:"), QLineEdit::Normal,
        QStringLiteral("127.0.0.1"), &ok);
    if (!ok || addr.isEmpty())
      return;
    int port = QInputDialog::getInt(this, tr("Join Session"), tr("Port:"), 55435, 1, 65535, 1, &ok);
    if (!ok)
      return;
    m_Netplay->joinSession(addr, static_cast<quint16>(port));
  });

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
