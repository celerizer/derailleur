#include "mainwindow.h"
#include "DrRandom.h"
#include "DrRetro.h"

#include <array>
#include <cstdlib>
#include <memory>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRandomGenerator>
#include <QRetro.h>
#include <QScrollArea>
#include <QShortcut>
#include <QTimer>

#ifdef _WIN32
#include <windows.h>
#endif

#include "DrSettingsPage.h"
#include "DrStartup.h"
#include "guests/DrGuests.h"
#include "hosts/DrHosts.h"

/* Delay before a view switch, so the frame on screen settles first */
#define DR_VIEW_SWITCH_MS 32

/* How long holdView waits for a core frame before covering with the bare page */
#define DR_OVERLAY_GRAB_TIMEOUT_MS 200

/* Debug launch payload: guest index, mini-game index, then per player seven
 * enum/id bytes and the s16 coin and star counts. */
#define DR_DEBUG_LAUNCH_PLAYER_SIZE (7 + 2 + 2)
#define DR_DEBUG_LAUNCH_SIZE (2 + 4 * DR_DEBUG_LAUNCH_PLAYER_SIZE)

static QByteArray dr_debug_launch_encode(
  int guestIndex, int minigameIndex, const dr_player_t players[4])
{
  QByteArray b;

  b.append(static_cast<char>(guestIndex));
  b.append(static_cast<char>(minigameIndex));
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = players[i];
    const int16_t coins = static_cast<int16_t>(p.coins);
    const int16_t stars = static_cast<int16_t>(p.stars);

    b.append(static_cast<char>(p.character));
    b.append(static_cast<char>(p.control_port));
    b.append(static_cast<char>(p.control_type));
    b.append(static_cast<char>(p.difficulty));
    b.append(static_cast<char>(p.team_color));
    b.append(static_cast<char>(p.team_type));
    b.append(static_cast<char>(p.team_id));
    b.append(static_cast<char>(coins & 0xFF));
    b.append(static_cast<char>((coins >> 8) & 0xFF));
    b.append(static_cast<char>(stars & 0xFF));
    b.append(static_cast<char>((stars >> 8) & 0xFF));
  }

  return b;
}

static bool dr_debug_launch_decode(const QByteArray &payload, int *guestIndex,
  int *minigameIndex, std::array<dr_player_t, 4> &players)
{
  auto u8 = [&](int i) { return static_cast<uint8_t>(payload.at(i)); };
  int off = 2;

  if (payload.size() < DR_DEBUG_LAUNCH_SIZE)
    return false;

  *guestIndex = u8(0);
  *minigameIndex = u8(1);
  for (unsigned i = 0; i < 4; i++)
  {
    players[i] = {};
    players[i].character = static_cast<dr_character>(u8(off++));
    players[i].control_port = u8(off++);
    players[i].control_type = static_cast<dr_control_type>(u8(off++));
    players[i].difficulty = static_cast<dr_difficulty>(u8(off++));
    players[i].team_color = static_cast<dr_team_color>(u8(off++));
    players[i].team_type = static_cast<dr_team_type>(u8(off++));
    players[i].team_id = u8(off++);
    players[i].coins = static_cast<int16_t>(u8(off) | (u8(off + 1) << 8));
    off += 2;
    players[i].stars = static_cast<int16_t>(u8(off) | (u8(off + 1) << 8));
    off += 2;
  }

  return true;
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  m_Tools = new DrToolWindow(nullptr);
  m_Tools->show();

  m_Logger = new DrLogger(nullptr);
  m_Tools->addTool(tr("Log"), m_Logger);
  m_Tools->setDeferReveal(true);

  dr_startup(m_Logger);

  m_Picker = new DrHostPicker(nullptr);
  m_Tools->addTool(tr("Start Game"), m_Picker);
  connect(m_Picker, &DrHostPicker::hostChosen, this, &MainWindow::startHost);

  DrRandom::instance().seed(QRandomGenerator::global()->generate());

  m_InputStore = new DrInputStore();
  m_Netplay = new DrNetplay(m_InputStore, this);
  setupNetplay();

  m_Guests = new DrGuestList(this);
  setupGuests();
  setupFilter();
  setupStack();
  setupFullscreen();
  m_Overlay = new DrOverlay(this);
  setupChallenge();
  setupSettings();
  setupDebug();

  m_Tools->revealTools(tr("Start Game"));

  resize(1040, 585);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
  m_Tools->close();
  QMainWindow::closeEvent(e);

#ifdef _WIN32
  TerminateProcess(GetCurrentProcess(), 0);
#else
  std::_Exit(0);
#endif
}

void MainWindow::setupStack()
{
  QPalette pal;

  m_Stack = new QStackedWidget(this);
  m_Blank = new QWidget(m_Stack);
  m_Blank->setAutoFillBackground(true);
  pal = m_Blank->palette();
  pal.setColor(QPalette::Window, Qt::black);
  m_Blank->setPalette(pal);

  m_Stack->addWidget(m_Blank);
  m_Stack->addWidget(m_Guests);
  m_Stack->setCurrentWidget(m_Blank);
  setCentralWidget(m_Stack);

  connect(m_Stack, &QStackedWidget::currentChanged, this, [this](int index) {
    QWidget *page = m_Stack->widget(index);
    DrGuest *guest = m_Guests->currentGuest();

    if (page)
      page->setFocus();
    if (page == m_Guests && guest && guest->core())
      if (QWidget *container = m_Guests->currentWidget())
        guest->core()->resize(container->width(), container->height());
  });
}

void MainWindow::setupFullscreen(void)
{
  QShortcut *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);

  escape->setEnabled(false);
  connect(escape, &QShortcut::activated, this, [this, escape]() {
    escape->setEnabled(false);
    showNormal();
  });

  auto toggle = [this, escape]() {
    if (isFullScreen())
      showNormal();
    else
      showFullScreen();
    escape->setEnabled(isFullScreen());
  };

  connect(new QShortcut(QKeySequence(Qt::Key_F11), this), &QShortcut::activated, this, toggle);
  connect(new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Return), this), &QShortcut::activated,
    this, toggle);
  connect(new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Enter), this), &QShortcut::activated,
    this, toggle);
}

void MainWindow::setupGuests()
{
  m_Logger->attach(m_Guests);
  dr_create_guests(m_Guests);

  for (DrGuest *guest : m_Guests->guests())
  {
    m_Logger->attach(guest);
    m_Logger->attachCore(guest->core());

    connect(guest, &DrGuest::minigameFinished, m_Netplay, &DrNetplay::freezeActiveContext,
      Qt::DirectConnection);
    connect(guest, &DrGuest::hardResync, m_Netplay, &DrNetplay::requestResync);
    connect(guest, &DrGuest::golfModeRequested, m_Netplay, &DrNetplay::setGolfMode);
  }
  m_Guests->logSummary();

  {
    QSet<int> owned;

    for (DrGuest *guest : m_Guests->guests())
      owned.insert(guest->id());
    m_Netplay->setOwnedGuests(owned);
  }

  connect(m_Guests, &DrGuestList::minigameFinished, this, &MainWindow::onMinigameFinished,
    Qt::QueuedConnection);
}

void MainWindow::setupFilter()
{
  m_Filter = new DrMinigameFilter(nullptr);
  m_Filter->populate(m_Guests->guests());
  m_Tools->addTool(tr("Minigame Filter"), m_Filter);

  connect(m_Filter, &DrMinigameFilter::filterChanged, this,
    [this](const QByteArray &payload) { m_Netplay->setMinigameFilter(payload); });
  connect(m_Netplay, &DrNetplay::minigameFilterReceived, this, [this](QByteArray payload) {
    m_Guests->applyFilter(payload);
    m_Filter->setFromPayload(payload);
  });

  connect(m_Netplay, &DrNetplay::missingGuestsChanged, this, [this](QSet<int> guests) {
    QSet<dr_guest> forced;

    for (int guest : guests)
      forced.insert(static_cast<dr_guest>(guest));
    m_Filter->setForcedOff(forced);
  });
  connect(m_Netplay, &DrNetplay::hostGuestsReceived, this, [this](QSet<int> guests) {
    QSet<dr_guest> excluded;

    for (DrGuest *guest : m_Guests->guests())
      if (!guests.contains(guest->id()))
        excluded.insert(guest->id());
    m_Guests->setExcludedGuests(excluded);
  });
  connect(m_Netplay, &DrNetplay::lobbyJoined, this,
    [this]() { m_Tools->setToolEnabled(m_Filter, false); });
  connect(m_Netplay, &DrNetplay::sessionError, this, [this]() {
    m_Tools->setToolEnabled(m_Filter, true);
    m_Guests->setExcludedGuests(QSet<dr_guest>());
    m_Filter->setForcedOff(QSet<dr_guest>());
  });

  m_Filter->restoreLastList();
}

void MainWindow::setupDebug()
{
  QScrollArea *debugScroll;

  m_Debug = new DrDebug(nullptr);
  m_Debug->populate(m_Guests->guests());

  debugScroll = new QScrollArea;
  debugScroll->setWidget(m_Debug);
  debugScroll->setWidgetResizable(true);
  debugScroll->setFrameShape(QFrame::NoFrame);
  m_Tools->addTool(tr("Debug"), debugScroll);

  connect(m_Debug, &DrDebug::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players) {
      int minigameIndex = 0;

      if (!m_Netplay->active())
      {
        launchMinigame(guest, minigame, players.data());
        return;
      }

      for (const dr_mp_minigame_t *mg = guest->minigames(); mg && mg->name && mg != minigame; mg++)
        minigameIndex++;
      m_Netplay->requestDebugLaunch(dr_debug_launch_encode(
        m_Guests->guests().indexOf(guest), minigameIndex, players.data()));
    });

  connect(m_Debug, &DrDebug::cancelRequested, this, [this]() {
    cancelActiveMinigame();
    m_Netplay->broadcastCancelMinigame();
  });

  connect(m_Debug, &DrDebug::setTurnRequested, this, [this](int turn) {
    if (m_Host)
      m_Host->setCurrentTurn(turn);
  });

  connect(m_Debug, &DrDebug::readPlayersRequested, this, [this]() {
    DrPlayerArray players = m_Debug->players();

    if (m_Host && m_Host->readPlayerSetup(players))
      m_Debug->setPlayers(players);
    else
      m_Logger->message(DR_LOG_WARN, "read players: this host can't read its player setup");
  });

  connect(m_Debug, &DrDebug::writePlayersRequested, this, [this](DrPlayerArray players) {
    if (!m_Host || !m_Host->writePlayerSetup(players))
      m_Logger->message(DR_LOG_WARN, "write players: this host can't write its player setup");
  });

  m_DebugCore = new DrDebugCore(m_Logger, this);
  connect(m_Debug, &DrDebug::launchCoreRequested, m_DebugCore,
    [this](const QString &corePath, const QString &contentPath) {
      m_DebugCore->launch(corePath, contentPath);
    });
}

void MainWindow::setupChallenge()
{
  m_Challenge = new DrChallenge(nullptr);
  m_Challenge->populate(m_Guests->guests());
  m_Tools->addTool(tr("Challenge"), m_Challenge);
  m_Logger->attach(m_Challenge);

  connect(m_Challenge, &DrChallenge::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players) {
      launchMinigame(guest, minigame, players.data());
    });
}

void MainWindow::setupSettings()
{
  DrSettingsPage *settings = new DrSettingsPage();

  m_Settings = settings;

  m_Tools->addTool(tr("Settings"), settings);
  connect(settings, &DrSettingsPage::redownloadSavesRequested, this,
    [this]() { dr_redownload_saves(m_Logger); });
  connect(settings, &DrSettingsPage::settingChanged, m_Netplay, [this](dr_setting_id id) {
    if (DrSettings::table[id].host_inherited)
      m_Netplay->broadcastSettings();
  });
  connect(m_Netplay, &DrNetplay::hostSettingsReceived, settings,
    [settings]() { settings->setNetplayClient(true); });
  connect(m_Netplay, &DrNetplay::sessionError, settings, [settings]() {
    DrSettings::instance().clearHost();
    settings->setNetplayClient(false);
  });
}

void MainWindow::setupNetplay()
{
  m_NetplayUi = new DrNetplayWidget(m_Netplay, nullptr);
  m_Tools->addTool(tr("Netplay"), m_NetplayUi);
  m_Logger->attach(m_Netplay);

  connect(m_Netplay, &DrNetplay::sessionStarted, this, [this](int index, int count) {
    m_Logger->message(DR_LOG_INFO, QString("netplay started: peer %1 of %2").arg(index).arg(count));
    m_Tools->setToolEnabled(m_Filter, false);
    m_Tools->setToolEnabled(m_Challenge, false);
    m_NetplayPeerIndex = index;
    if (m_Host)
      m_Host->setLocalPlayer(index);
  });

  connect(m_Netplay, &DrNetplay::peerCountChanged, this, [this](int connected, int total) {
    m_Logger->message(DR_LOG_INFO, QString("netplay peers: %1/%2 connected").arg(connected).arg(total));
  });

  connect(m_Netplay, &DrNetplay::sessionError, this, [this](const QString &reason) {
    m_Tools->setToolEnabled(m_Filter, true);
    m_Tools->setToolEnabled(m_Challenge, true);
    m_Logger->message(DR_LOG_ERROR, reason);
    QMessageBox::warning(this, tr("Netplay"), reason);
  });

  connect(m_Netplay, &DrNetplay::cancelMinigameReceived, this, &MainWindow::cancelActiveMinigame);

  connect(m_Netplay, &DrNetplay::debugLaunchReady, this, [this](QByteArray payload) {
    std::array<dr_player_t, 4> players;
    int guestIndex, minigameIndex;
    const dr_mp_minigame_t *minigame;
    DrGuest *guest;

    if (!dr_debug_launch_decode(payload, &guestIndex, &minigameIndex, players))
      return;
    guest = m_Guests->guests().value(guestIndex, nullptr);
    if (!guest)
      return;
    minigame = guest->minigames();
    for (int i = 0; i < minigameIndex && minigame && minigame->name; i++)
      minigame++;
    if (minigame && minigame->name)
      launchMinigame(guest, minigame, players.data());
  });

  connect(m_Netplay, &DrNetplay::startGameRequested, this,
    [this](int gameId, const QString &customRom) {
      const dr_game game = static_cast<dr_game>(gameId);
      const DrHostEntry *entry = dr_host_entry(game);
      QString rom;

      if (m_Host || !entry)
        return;
      if (!customRom.isEmpty())
      {
        rom = dr_custom_rom_path(game, customRom);
        if (rom.isEmpty())
        {
          m_Logger->message(DR_LOG_ERROR,
            QString("netplay: host uses custom ROM \"%1\", which isn't installed").arg(customRom));
          return;
        }
      }
      startHost(entry, rom, customRom);
    });
}

void MainWindow::startHost(const DrHostEntry *entry, const QString &rom, const QString &customRom)
{
  m_CustomRom = customRom;
  startWithHost(entry->create(this, rom.toStdString()));
}

void MainWindow::startWithHost(DrHost *host)
{
  m_Host = host;

  raise();
  activateWindow();
  m_Picker->setEnabled(false);
  m_Settings->setGameRunning(true);
  m_Host->setLocalPlayer(m_NetplayPeerIndex);

  if (DrOverlay *ov = overlay())
    ov->hold(QPixmap(":/assets/loading.png"));

  m_Logger->attach(m_Host);
  m_Logger->attachCore(m_Host->core());

  attachNetplay();
  if (m_Netplay->isServer())
    m_Netplay->startGame(static_cast<int>(m_Host->game()), m_Host->saveFilePatterns(), m_CustomRom);

  m_HostContainer = m_Host->createWidget(m_Stack);
  m_HostContainer->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(m_HostContainer);
  m_Host->startCore();

  m_Host->setMinigameSource(m_Guests);
  connect(m_Host, &DrHost::minigameRequested, m_Netplay,
    [this](DrMinigameCandidate, DrPlayerArray) { m_Netplay->freezeActiveContext(); },
    Qt::DirectConnection);
  connect(m_Host, &DrHost::minigameRequested, this,
    [this](DrMinigameCandidate candidate, DrPlayerArray players) {
      launchMinigame(candidate.guest, candidate.minigame, players.data());
    });
  connect(m_Host, &DrHost::golfModeRequested, m_Netplay, &DrNetplay::setGolfMode);

  m_Stack->setCurrentWidget(m_Guests);
  pauseGuests();
  showHost();
}

void MainWindow::attachNetplay()
{
  QRetroInputBackend *local = m_Host->core() ? m_Host->core()->input()->backend() : nullptr;
  DrHost *host = m_Host;

  for (DrGuest *guest : m_Guests->guests())
  {
    if (local)
      break;
    if (m_Guests->guestHasCandidate(guest) && guest->core())
      local = guest->core()->input()->backend();
  }
  m_Netplay->setLocalSource(local);

  m_Netplay->attachCore(m_Host->core(), QStringLiteral("host"),
    [host]() { return host->syncValidator(); });

  for (DrGuest *guest : m_Guests->guests())
    if (m_Guests->guestHasCandidate(guest) && guest->core())
      m_Netplay->attachCore(guest->core(), QString::fromUtf8(guest->name()),
        [guest]() { return guest->syncValidator(); });
}

void MainWindow::launchMinigame(
  DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4])
{
  const DrPlayerArray launchPlayers = { players[0], players[1], players[2], players[3] };

  if (!m_Guests->activateGuest(guest))
    return;

  for (DrGuest *g : m_Guests->guests())
    if (g->minigameActive())
    {
      m_Logger->message(DR_LOG_INFO, QString("cancelling running mini-game on %1").arg(g->name()));
      g->cancelMinigame();
    }

  if (DrOverlay *ov = overlay())
  {
    const QString result =
      m_Challenge->isPending() ? m_Challenge->takeContinuousResult() : QString();

    if (!result.isEmpty())
      ov->showLoadingCard(result, QString::fromUtf8(guest->name()),
        minigame ? QString::fromUtf8(minigame->name) : QString());
    else
      holdView(m_Host ? m_Host->core() : nullptr);
  }

  switchView([this, guest, minigame, launchPlayers]() {
    DrGameData data;

    m_Stack->setCurrentWidget(m_Guests);
    if (DrSettings::instance().muteWhileLoading())
      if (auto *a = guest->core()->audio())
        a->setMute(true);

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(guest, &DrGuest::minigameStarted, this, [this, guest, conn]() {
      disconnect(*conn);
      m_Netplay->setActiveContext(guest->core());
      guest->unpause();
      fadeOverlay();
      if (auto *a = guest->core()->audio())
        a->setMute(false);
    });

    data.minigame = minigame;
    data.type = minigame ? minigame->type : DR_MINIGAME_INVALID;
    data.battle_pot = m_Host ? m_Host->battlePot() : 0;
    data.host_platform = m_Host ? m_Host->platform() : DR_HOST_PLATFORM_INVALID;
    for (unsigned i = 0; i < 4; i++)
      data.players[i] = launchPlayers[i];
    guest->applyGameData(data);
    guest->unpause();
  });
}

void MainWindow::onMinigameFinished()
{
  if (m_Challenge->isPending())
  {
    m_Challenge->recordResult(m_Guests->currentGuest());
    if (!m_Challenge->isPending())
      showBoard();
    return;
  }

  if (m_Host)
    m_Host->writeResults(m_Guests->currentGuest());
  showBoard();
}

void MainWindow::cancelActiveMinigame()
{
  DrGuest *guest = m_Guests->currentGuest();

  m_Challenge->clearPending();
  if (guest && guest->minigameActive())
  {
    guest->cancelMinigame();
    if (m_Host)
      m_Host->clearResults();
  }
  showBoard();
}

void MainWindow::showBoard()
{
  if (m_Host)
    showHost();
  else
    showChooser();
}

void MainWindow::showHost()
{
  if (DrGuest *guest = m_Guests->currentGuest())
    holdView(guest->core());

  switchView([this]() {
    /* Free idle guests while the overlay still covers the switch and the board is
     * held; every peer does it at this point, and the barrier below absorbs any
     * difference in how long it takes. The overlay shows a frame grabbed from the
     * guest already (holdView), and its QRetro window survives the unload. */
    m_Guests->unloadIdleGuests(DrSettings::instance().residentGuests());
    m_Netplay->setActiveContext(m_Host->core());
    m_Host->unpause();
    m_Stack->setCurrentWidget(m_HostContainer);
    fadeOverlay();
  });
}

void MainWindow::showChooser()
{
  if (DrGuest *guest = m_Guests->currentGuest())
    holdView(guest->core());

  switchView([this]() {
    m_Stack->setCurrentWidget(m_Blank);
    fadeOverlay();
  });
}

void MainWindow::switchView(std::function<void()> show)
{
  QTimer::singleShot(DR_VIEW_SWITCH_MS, this, [this, show]() {
    if (m_Host)
      m_Host->pause();
    pauseGuests();
    show();
  });
}

void MainWindow::pauseGuests()
{
  for (DrGuest *guest : m_Guests->guests())
    guest->pause();
}

void MainWindow::holdView(QRetro *core)
{
  DrOverlay *ov = overlay();
  QWidget *page = m_Stack->currentWidget();

  if (!ov || !page || page->size().isEmpty())
    return;

  const qreal dpr = page->devicePixelRatioF();
  QPixmap base(page->size() * dpr);

  base.setDevicePixelRatio(dpr);
  base.fill(Qt::black);
  page->render(&base, QPoint(), QRegion(), QWidget::DrawChildren);

  if (!core)
  {
    ov->hold(base);
    return;
  }

  const QRect target(QPoint(0, 0), page->size());
  auto held = std::make_shared<bool>(false);
  auto conn = std::make_shared<QMetaObject::Connection>();

  *conn = connect(core, &QRetro::frameGrabbed, this,
    [ov, base, target, held, conn](const QImage &frame) {
      QPixmap shot = base;

      QObject::disconnect(*conn);
      if (*held)
        return;
      *held = true;
      if (!frame.isNull())
      {
        QPainter painter(&shot);
        painter.drawImage(target, frame);
      }
      ov->hold(shot);
    });

  QTimer::singleShot(DR_OVERLAY_GRAB_TIMEOUT_MS, this, [ov, base, held, conn]() {
    if (*held)
      return;
    *held = true;
    QObject::disconnect(*conn);
    ov->hold(base);
  });

  core->grabFrame();
}

void MainWindow::fadeOverlay()
{
  if (m_Overlay)
    m_Overlay->fadeOut();
}

void MainWindow::trackOverlay()
{
  if (m_Overlay)
    m_Overlay->trackGeometry(geometry());
}
