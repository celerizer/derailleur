#include "mainwindow.h"

#include <array>
#include <memory>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QPushButton>
#include <QRetro.h>
#include <QSet>
#include <QSettings>
#include <QSize>
#include <QStackedWidget>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "DrChallenge.h"
#include "DrDebug.h"
#include "DrDownloader.h"
#include "DrSettings.h"

/* A modified host ROM found under roms/custom/mpN. `name` is the basename, which
 * is also what travels over netplay so each peer resolves its own copy. */
struct DrCustomRom
{
  int mp;
  QString path;
  QString name;
  QString icon;
};

/* N64 hosts boot .z64; the GameCube and Wii ones boot .rvz or .iso. */
static QStringList dr_custom_rom_filters(int mp)
{
  if (mp <= 3)
    return { "*.z64" };
  return { "*.rvz", "*.iso" };
}

static QList<DrCustomRom> dr_custom_roms(void)
{
  QList<DrCustomRom> roms;

  for (int mp = 1; mp <= 8; mp++)
  {
    QDir dir(dr_roms_directory() + QString("/custom/mp%1").arg(mp));

    if (!dir.exists())
      continue;

    for (const QFileInfo &info :
      dir.entryInfoList(dr_custom_rom_filters(mp), QDir::Files, QDir::Name))
    {
      const QString png = dir.filePath(info.completeBaseName() + ".png");

      roms.append({ mp, info.absoluteFilePath(), info.completeBaseName(),
        QFile::exists(png) ? png : QString() });
    }
  }

  return roms;
}

/* Boards whose battle roulette shows a picture the host can't redraw get a stock
 * one instead: the same image is copied over every icon slot the game has.
 * Dolphin loads them out of <save>/User/Load/Textures/<game id>. */
struct DrTextureAsset
{
  const char *dir;
  const char *source;
  const char *const *files;
};

static const char *const MP6_BATTLE_ICON_FILE[] = {
  "tex1_128x128_2bb955ca50dd26a6_14.png",
  "tex1_128x128_2f5e8d147ce5d7f5_14.png",
  "tex1_128x128_560c420a6be5402f_14.png",
  "tex1_128x128_722f65211251356e_14.png",
  "tex1_128x128_d3d80b92eda45477_14.png",
  "tex1_128x128_f75626c325e81470_14.png",

  nullptr
};

static const char *const MP7_BATTLE_ICON_FILE[] = {
  "tex1_128x104_2e3485dc3d6f3eb6_eea1d86ba1bfcd04_9.png",
  "tex1_128x104_a8173cf855b51046_b52e7bf246f8a16b_9.png",
  "tex1_128x104_aa12f37d6f11d30a_93ab94e37f5aabf6_9.png",
  "tex1_128x104_ddbc2b4774edce92_79b97c505c3eb0f3_9.png",
  "tex1_128x104_e2943b1807a2fede_fc117f3666cb6b2e_9.png",

  nullptr
};

static const char *const MP8_BATTLE_ICON_FILE[] = {
  "tex1_224x168_16c7623064db6ff8_14.png",

  nullptr
};

static const DrTextureAsset DR_TEXTURE_ASSETS[] = {
  { "GP6E01", ":/assets/minigame-icon/battle-mp6.png", MP6_BATTLE_ICON_FILE },
  { "GP7E01", ":/assets/minigame-icon/battle-mp7.png", MP7_BATTLE_ICON_FILE },
  { "RM8E01", ":/assets/minigame-icon/battle-mp8.png", MP8_BATTLE_ICON_FILE },

  { nullptr, nullptr, nullptr }
};

/* Refreshed every boot so an updated asset lands without anyone clearing the
 * folder by hand. A resource copies out read-only, so the mode is reset or the
 * next boot cannot overwrite it. */
static void dr_install_textures(DrLogger *logger)
{
  for (const DrTextureAsset *asset = DR_TEXTURE_ASSETS; asset->dir; asset++)
  {
    const QString dir = dr_save_directory() + "/User/Load/Textures/" + asset->dir;
    unsigned copied = 0;

    if (!asset->files[0])
      continue;

    if (!QDir().mkpath(dir))
    {
      if (logger)
        logger->message(DR_LOG_WARN, QString("textures: cannot create %1").arg(dir));
      continue;
    }

    for (const char *const *file = asset->files; *file; file++)
    {
      const QString dest = dir + "/" + *file;

      QFile::remove(dest);
      if (QFile::copy(asset->source, dest))
      {
        QFile::setPermissions(dest,
          QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
        copied++;
      }
      else if (logger)
        logger->message(DR_LOG_WARN, QString("textures: failed to write %1").arg(dest));
    }

    if (logger)
      logger->message(DR_LOG_INFO,
        QString("textures: %1 file(s) into %2").arg(copied).arg(asset->dir));
  }
}

static QString dr_custom_rom_path(int mp, const QString &name)
{
  for (const DrCustomRom &rom : dr_custom_roms())
    if (rom.mp == mp && rom.name == name)
      return rom.path;

  return QString();
}

#include "hosts/MarioParty1Host.h"
#include "hosts/MarioParty2Host.h"
#include "hosts/MarioParty3Host.h"
#include "hosts/MarioParty4Host.h"
#include "hosts/MarioParty5Host.h"
#include "hosts/MarioParty6Host.h"
#include "hosts/MarioParty7Host.h"
#include "hosts/MarioParty8Host.h"
#include "hosts/SonicShuffleHost.h"
#include "guests/MarioKart64.h"
#include "guests/MarioParty1.h"
#include "guests/MarioParty2.h"
#include "guests/MarioParty3.h"
#include "guests/BanjoTooie.h"
#include "guests/CoreDolphin.h"
#include "guests/Kirby64.h"
#include "guests/KirbyAirRide.h"
#include "guests/MarioGolf.h"
#include "guests/MarioKartDoubleDash.h"
#include "guests/MarioParty4.h"
#include "guests/MarioParty5.h"
#include "guests/MarioParty6.h"
#include "guests/MarioParty7.h"
#include "guests/MarioParty8.h"
#include "guests/MarioParty9.h"
#include "guests/MarioPartyAdvance.h"
#include "guests/MarioPartyE.h"
#include "guests/SmashRemix.h"
#include "guests/SuperMarioBros3.h"
#include "guests/YoshisIsland.h"
#include "guests/MarioTennis.h"
#include "guests/PokemonStadium2.h"
#include "guests/SonicShuffle.h"

#define SHOW_LOGGER 1
#define SHOW_OVERLAY 1
#define SHOW_DEBUG 1

/* Git hashes arrive from the .pro as bare tokens; stringize them for logging. */
#ifndef DR_GIT_HASH
#define DR_GIT_HASH unknown
#endif
#ifndef DR_GIT_HASH_FULL
#define DR_GIT_HASH_FULL unknown
#endif
#define DR_STRINGIZE_(x) #x
#define DR_STRINGIZE(x) DR_STRINGIZE_(x)

namespace
{
/* Opaque payload for a debug-menu launch relayed through netplay so every peer runs it
 * on the same gated frame: guest index, mini-game index within that guest's list, then
 * 7 bytes per player. DrNetplay prepends the target frame. */
/* Per-player bytes in a debug-launch payload: seven enum/id bytes plus the
 * signed 16-bit coin and star counts. */
#define DR_DEBUG_LAUNCH_PLAYER_SIZE (7 + 2 + 2)

QByteArray serializeDebugLaunch(int guestIndex, int minigameIndex, const dr_player_t players[4])
{
  QByteArray b;
  b.append(static_cast<char>(guestIndex));
  b.append(static_cast<char>(minigameIndex));
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = players[i];
    b.append(static_cast<char>(p.character));
    b.append(static_cast<char>(p.control_port));
    b.append(static_cast<char>(p.control_type));
    b.append(static_cast<char>(p.difficulty));
    b.append(static_cast<char>(p.team_color));
    b.append(static_cast<char>(p.team_type));
    b.append(static_cast<char>(p.team_id));
    const int16_t coins = static_cast<int16_t>(p.coins);
    const int16_t stars = static_cast<int16_t>(p.stars);
    b.append(static_cast<char>(coins & 0xFF));
    b.append(static_cast<char>((coins >> 8) & 0xFF));
    b.append(static_cast<char>(stars & 0xFF));
    b.append(static_cast<char>((stars >> 8) & 0xFF));
  }
  return b;
}
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  m_Tools = new DrToolWindow(nullptr);
  m_Tools->show();

#if SHOW_LOGGER
  m_Logger = new DrLogger(nullptr);
  m_Tools->addTool(tr("Log"), m_Logger);
  m_Logger->message(DR_LOG_INFO, QString("derailleur built %1 %2").arg(__DATE__, __TIME__));
  m_Logger->message(DR_LOG_INFO, QString("build revision %1").arg(DR_STRINGIZE(DR_GIT_HASH)));
  m_Logger->message(DR_LOG_INFO, QString("git hash %1").arg(DR_STRINGIZE(DR_GIT_HASH_FULL)));
  m_Logger->message(DR_LOG_INFO,
    QString("started %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
#endif

  /* Show only the log while starting up */
  m_Tools->setDeferReveal(true);

  /* Paths first: the host picker scans roms/custom for modified ROMs, so the
   * configured roms directory has to be in place before it is built. */
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
    dr_set_save_directory(load("paths/save", cwd.filePath("save")));
    s.sync();

    /* User settings (Settings tool page); read before the guests are built. */
    dr_settings_load();
#if SHOW_LOGGER
    if (!iniExisted)
      m_Logger->message(DR_LOG_INFO, QString("created %1").arg(iniPath));
    else
      m_Logger->message(DR_LOG_INFO, QString("loaded %1").arg(iniPath));
    m_Logger->message(DR_LOG_INFO, QString("paths/roms: %1").arg(dr_roms_directory()));
    m_Logger->message(DR_LOG_INFO, QString("paths/cores: %1").arg(dr_cores_directory()));
    m_Logger->message(DR_LOG_INFO, QString("paths/state: %1").arg(dr_state_directory()));
    m_Logger->message(DR_LOG_INFO, QString("paths/save: %1").arg(dr_save_directory()));
#endif

    DrDownloader downloader;
#if SHOW_LOGGER
    connect(&downloader, &DrDownloader::logMessage, m_Logger, &DrLogger::message);
    connect(&downloader, &DrDownloader::progressStarted, m_Logger, &DrLogger::showProgress);
    connect(&downloader, &DrDownloader::progressFinished, m_Logger, &DrLogger::hideProgress);
    connect(&downloader, &DrDownloader::progressUpdated, m_Logger, &DrLogger::setProgress);
#endif
    downloader.runBlocking(s, dr_save_directory(), dr_state_directory());
  }

  dr_install_textures(m_Logger);

  /* "Start Game" tab: the host picker. Made the default view once revealed; the
   * main window stays black until a game is started. The button handlers only
   * fire at runtime, so referencing members built further down (m_Guests,
   * m_Netplay via startWithHost) is fine. */
  {
    QWidget *startGame = new QWidget(nullptr);
    QVBoxLayout *layout = new QVBoxLayout(startGame);
    QLabel *label = new QLabel(tr("Choose Host"), startGame);
    QGridLayout *grid = new QGridLayout;
    int hosts = 0;

    label->setAlignment(Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(label);
    layout->addLayout(grid);

    /* Two rows of four: each host is its own title screen with the game's name
     * underneath. The 4:3 shots are scaled to keep their aspect. */
    auto addHostButton = [&](const QString &name, const QString &title, auto factory) {
      QToolButton *btn = new QToolButton(startGame);

      btn->setText(name);
      btn->setIcon(QIcon(QString(":/assets/titlescreen/%1.png").arg(title)));
      btn->setIconSize(QSize(160, 120));
      btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      connect(btn, &QToolButton::clicked, this, [this, factory]() {
        m_CustomRom.clear();
        startWithHost(factory());
      });
      grid->addWidget(btn, hosts / 4, hosts % 4);
      hosts++;
    };
    addHostButton("Mario Party", "marioparty1",
      [this]() -> DrHost * { return new MarioParty1Host(this); });
    addHostButton("Mario Party 2", "marioparty2",
      [this]() -> DrHost * { return new MarioParty2Host(this); });
    addHostButton("Mario Party 3", "marioparty3",
      [this]() -> DrHost * { return new MarioParty3Host(this); });
    addHostButton("Mario Party 4", "marioparty4",
      [this]() -> DrHost * { return new MarioParty4Host(this); });
    addHostButton("Mario Party 5", "marioparty5",
      [this]() -> DrHost * { return new MarioParty5Host(this); });
    addHostButton("Mario Party 6", "marioparty6",
      [this]() -> DrHost * { return new MarioParty6Host(this); });
    addHostButton("Mario Party 7", "marioparty7",
      [this]() -> DrHost * { return new MarioParty7Host(this); });
    addHostButton("Mario Party 8", "marioparty8",
      [this]() -> DrHost * { return new MarioParty8Host(this); });

    /* Modified ROMs dropped in roms/custom/mpN follow the stock eight, each named
     * after its file and using a sibling .png when one is there. */
    const QList<DrCustomRom> customRoms = dr_custom_roms();

    for (const DrCustomRom &rom : customRoms)
    {
      QToolButton *btn = new QToolButton(startGame);

      btn->setText(rom.name);
      btn->setIcon(rom.icon.isEmpty()
          ? QIcon(QString(":/assets/titlescreen/marioparty%1.png").arg(rom.mp))
          : QIcon(rom.icon));
      btn->setIconSize(QSize(160, 120));
      btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      connect(btn, &QToolButton::clicked, this, [this, rom]() {
        m_CustomRom = rom.name;
        startWithHost(makeHost(rom.mp, rom.path));
      });
      grid->addWidget(btn, hosts / 4, hosts % 4);
      hosts++;
    }

    m_StartGameTab = startGame;

    /* Custom entries push the grid past one screen, so give it a scroll area. */
    if (customRoms.isEmpty())
    {
      layout->addStretch();
      m_Tools->addTool(tr("Start Game"), startGame);
    }
    else
    {
      QScrollArea *scroll = new QScrollArea(nullptr);

      scroll->setWidget(startGame);
      scroll->setWidgetResizable(true);
      scroll->setFrameShape(QFrame::NoFrame);
      m_Tools->addTool(tr("Start Game"), scroll);
    }
  }

  /* Wipe the Mupen64Plus texture cache each boot so freshly laid-down hires
   * textures (e.g. PokemonStadium2's per-character icons) aren't masked by a
   * stale cache. */
  {
    QDir cache("system/Mupen64plus/cache");
    if (cache.exists())
    {
      const bool ok = cache.removeRecursively();
#if SHOW_LOGGER
      m_Logger->message(ok ? DR_LOG_INFO : DR_LOG_WARN,
        ok ? QString("cleared %1").arg(cache.path())
           : QString("failed to clear %1").arg(cache.path()));
#endif
    }
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

  if (!dr_settings_get().shared_gamecube_core)
  {
    /* One single-disc Dolphin per GameCube game (like the Wii ones), so no disc
     * swapping happens at all. Each needs a unique subdir for its arena tag and
     * disc list. They still load lazily -- only games launched ever boot. */
    auto addSoloGcn = [this](const QString &subdir, DolphinGuest *(*make)(QRetro *, QObject *)) {
      auto *core = new CoreDolphin(subdir, false, this);
      core->addGame(make(core->core(), core));
      core->finalizeGames();
      if (core->isValid())
        m_Guests->add(core);
    };
    addSoloGcn("gcn-mp4", [](QRetro *c, QObject *p) -> DolphinGuest * { return new MarioParty4(c, p); });
    addSoloGcn("gcn-mp5", [](QRetro *c, QObject *p) -> DolphinGuest * { return new MarioParty5(c, p); });
    addSoloGcn("gcn-mp6", [](QRetro *c, QObject *p) -> DolphinGuest * { return new MarioParty6(c, p); });
    addSoloGcn("gcn-mp7", [](QRetro *c, QObject *p) -> DolphinGuest * { return new MarioParty7(c, p); });
    addSoloGcn("gcn-mkdd", [](QRetro *c, QObject *p) -> DolphinGuest * { return new MarioKartDoubleDash(c, p); });
  }
  else
  {
    auto *dolphin = new CoreDolphin("gcn", false, this);
    dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
    dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
    dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
    dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
    //dolphin->addGame(new KirbyAirRide(dolphin->core(), dolphin));
    dolphin->addGame(new MarioKartDoubleDash(dolphin->core(), dolphin));
    dolphin->finalizeGames();
    if (dolphin->isValid())
      m_Guests->add(dolphin);
  }

  /* Setup Mario Party 8 with widescreen disabled */
  static const dr_core_option_t MP8_GUEST_OPTIONS[] = {
    { "dolphin_widescreen", "disabled" },
    { nullptr, nullptr },
  };
  auto *dolphinMp8 = new CoreDolphin("wii-mp8", true, this);
  dolphinMp8->setOptions(MP8_GUEST_OPTIONS);
  dolphinMp8->addGame(new MarioParty8(dolphinMp8->core(), dolphinMp8));
  dolphinMp8->finalizeGames();
  if (dolphinMp8->isValid())
    m_Guests->add(dolphinMp8);

  auto *dolphinMp9 = new CoreDolphin("wii-mp9", true, this);
  dolphinMp9->addGame(new MarioParty9(dolphinMp9->core(), dolphinMp9));
  dolphinMp9->finalizeGames();
  if (dolphinMp9->isValid())
    m_Guests->add(dolphinMp9);

  auto addGuest = [this](DrGuest *g) {
    if (g->isValid())
      m_Guests->add(g);
    else
      delete g;
  };
  addGuest(new MarioKart64());
  addGuest(new MarioParty1());
  addGuest(new MarioParty2());
  addGuest(new MarioParty3());
  addGuest(new SmashRemix());
  addGuest(new SuperMarioBros3());
  addGuest(new YoshisIsland());
  addGuest(new MarioTennis());
  addGuest(new PokemonStadium2());
  addGuest(new MarioGolf());
  //addGuest(new MarioPartyAdvance());
  //addGuest(new MarioPartyE());
  addGuest(new Kirby64());
  addGuest(new BanjoTooie());
  addGuest(new SonicShuffle());

#if SHOW_LOGGER
  for (DrGuest *guest : m_Guests->guests())
  {
    connect(guest, &DrGuest::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
    connectCoreLog(guest->core());
  }
  m_Guests->logSummary();
#endif

  /* Allowed-mini-games chooser. Edits are routed through DrNetplay so the host
   * is authoritative: setMinigameFilter applies locally (host/singleplayer) and
   * broadcasts; minigameFilterReceived applies the resulting payload everywhere
   * (and reflects the host's choice on a client). */
  m_Filter = new DrMinigameFilter(nullptr);
  m_Filter->populate(m_Guests->guests());
  m_Tools->addTool(tr("Minigame Filter"), m_Filter);
  connect(m_Filter, &DrMinigameFilter::filterChanged, this,
    [this](const QByteArray &payload) { m_Netplay->setMinigameFilter(payload); });
  connect(m_Netplay, &DrNetplay::minigameFilterReceived, this, [this](QByteArray payload) {
    m_Guests->applyFilter(payload);
    m_Filter->setFromPayload(payload);
  });

  /* Now that filterChanged is wired, put back the list chosen last session. */
  m_Filter->restoreLastList();

  m_Stack = new QStackedWidget(this);

  /* The game view is black until a game is started; the host picker now lives in
   * the "Start Game" tool tab. */
  QWidget *blank = new QWidget(m_Stack);
  blank->setAutoFillBackground(true);
  {
    QPalette pal = blank->palette();
    pal.setColor(QPalette::Window, Qt::black);
    blank->setPalette(pal);
  }

  m_Stack->addWidget(blank);     // index 0 — black placeholder
  m_Stack->addWidget(m_Guests);  // index 1 — guests
  // host container added in startWithHost (index 2)

  setCentralWidget(m_Stack);

#if SHOW_OVERLAY
  m_Overlay = new DrOverlay(this);
#endif

  m_Stack->setCurrentIndex(0);

#if SHOW_DEBUG
  m_Debug = new DrDebug(nullptr);
  m_Debug->populate(m_Guests->guests());
  m_Tools->addTool(tr("Debug"), m_Debug);

  /* Connected here rather than in startWithHost so mini-games can be requested
   * before a host has been chosen (launchMinigame tolerates a null host). */
  connect(m_Debug, &DrDebug::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players) {
      /* In netplay a debug launch is async on one peer, so route it through netplay to
       * start on the same gated frame everywhere; the actual launch runs from the
       * debugLaunchReady handler. Outside a session, launch directly. */
      if (m_Netplay && m_Netplay->active())
      {
        const int guestIndex = m_Guests->guests().indexOf(guest);
        int minigameIndex = 0;
        for (const dr_mp_minigame_t *mg = guest->minigames(); mg && mg->name && mg != minigame; mg++)
          minigameIndex++;
        m_Netplay->requestDebugLaunch(serializeDebugLaunch(guestIndex, minigameIndex, players.data()));
      }
      else
        launchMinigame(guest, minigame, players.data());
    });
#endif

  /* Challenge mode: beat a mini-game solo against CPUs, tracked per difficulty.
   * Like the debug picker it runs without a host. */
  m_Challenge = new DrChallenge(nullptr);
  m_Challenge->populate(m_Guests->guests());
  m_Tools->addTool(tr("Challenge"), m_Challenge);
  connect(m_Challenge, &DrChallenge::minigameRequested, this,
    [this](DrGuest *guest, const dr_mp_minigame_t *minigame, std::array<dr_player_t, 4> players) {
      launchMinigame(guest, minigame, players.data());
    });
#if SHOW_LOGGER
  connect(m_Challenge, &DrChallenge::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  /* Settings: user-facing options backed by the global dr_settings. */
  DrSettings *settingsTool = new DrSettings();
  m_Tools->addTool(tr("Settings"), settingsTool);
  connect(settingsTool, &DrSettings::redownloadSavesRequested, this, [this]() {
    QSettings s(QDir::current().filePath("derailleur.ini"), QSettings::IniFormat);
    DrDownloader downloader;
#if SHOW_LOGGER
    connect(&downloader, &DrDownloader::logMessage, m_Logger, &DrLogger::message);
    connect(&downloader, &DrDownloader::progressStarted, m_Logger, &DrLogger::showProgress);
    connect(&downloader, &DrDownloader::progressFinished, m_Logger, &DrLogger::hideProgress);
    connect(&downloader, &DrDownloader::progressUpdated, m_Logger, &DrLogger::setProgress);
#endif
    downloader.downloadSaves(s, dr_save_directory(), dr_state_directory());
  });

  connect(m_Stack, &QStackedWidget::currentChanged, this, [this](int index) {
    if (QWidget *page = m_Stack->widget(index))
      page->setFocus();

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
      /* A challenge run scores itself and must never pay out onto the board. */
      if (m_Challenge && m_Challenge->isPending())
      {
        m_Challenge->recordResult(m_Guests->currentGuest());
        /* Continuous play may have chained straight into the next mini-game,
         * which already switched the view -- don't yank it back. */
        if (m_Challenge->isPending())
          return;
        if (m_Host)
          showHost();
        else
          showChooser();
        return;
      }
      if (!m_Host) /* debug launch with no host chosen yet */
      {
        showChooser();
        return;
      }
      m_Host->writeResults(m_Guests->currentGuest());
      showHost();
    },
    Qt::QueuedConnection);

  /* Stuck-minigame timeout: zero the results (don't award anything) and return. */
  connect(
    m_Guests, &DrGuestList::minigameCanceled, this,
    [this]() {
      if (m_Challenge)
        m_Challenge->clearPending();
      if (!m_Host)
      {
        showChooser();
        return;
      }
      m_Host->clearResults();
      showHost();
    },
    Qt::QueuedConnection);

  /* Startup finished: reveal the rest of the tools and jump to Start Game. */
  m_Tools->revealTools(tr("Start Game"));

  resize(960, 540);
}

void MainWindow::connectCoreLog(QRetro *core)
{
  if (!core || !m_Logger)
    return;

  qRegisterMetaType<QRetroMessageEntry>();

  /* onCoreLog fires on the emulation thread. Some cores (e.g. Flycast) log the
   * same line every frame; marshaling each straight to the GUI-thread logger via a
   * queued connection floods the event queue and freezes the UI. So dedupe
   * consecutive identical lines here on the emu thread (a direct connection) and
   * only forward distinct ones, collapsing runs into a "repeated Nx" note. The
   * dedupe state is per-core, captured per connection. */
  auto lastMsg = std::make_shared<QString>();
  auto repeats = std::make_shared<int>(0);
  DrLogger *logger = m_Logger;
  connect(core, &QRetro::onCoreLog, this,
    [logger, lastMsg, repeats](int level, const QString &msg) {
      if (msg == *lastMsg)
      {
        ++*repeats;
        return;
      }
      const int skipped = *repeats;
      *repeats = 0;
      *lastMsg = msg;

      const unsigned lvl = level >= RETRO_LOG_ERROR ? DR_LOG_ERROR
                         : level == RETRO_LOG_WARN  ? DR_LOG_WARN
                                                    : DR_LOG_INFO;
      QMetaObject::invokeMethod(logger,
        [logger, lvl, msg, skipped]() {
          if (skipped > 0)
            logger->message(DR_LOG_INFO,
              QStringLiteral("[core] (previous line repeated %1x)").arg(skipped));
          logger->message(lvl, QStringLiteral("[core] ") + msg);
        },
        Qt::QueuedConnection);
    },
    Qt::DirectConnection);

  /* On-screen core messages (SET_MESSAGE / SET_MESSAGE_EXT) surface in the log too. */
  connect(core, &QRetro::onCoreMessage, m_Logger,
    [logger = m_Logger](const QRetroMessageEntry &entry) {
      const unsigned lvl = entry.level >= RETRO_LOG_ERROR ? DR_LOG_ERROR
                         : entry.level == RETRO_LOG_WARN  ? DR_LOG_WARN
                                                          : DR_LOG_INFO;
      logger->message(lvl, QStringLiteral("[msg] ") + entry.message);
    },
    Qt::QueuedConnection);
}

DrHost *MainWindow::makeHost(int mp, const QString &game)
{
  const std::string path = game.toStdString();

  switch (mp)
  {
  case 1:
    return new MarioParty1Host(this, path);
  case 2:
    return new MarioParty2Host(this, path);
  case 3:
    return new MarioParty3Host(this, path);
  case 4:
    return new MarioParty4Host(this, path);
  case 5:
    return new MarioParty5Host(this, path);
  case 6:
    return new MarioParty6Host(this, path);
  case 7:
    return new MarioParty7Host(this, path);
  case 8:
    return new MarioParty8Host(this, path);
  default:
    return nullptr;
  }
}

void MainWindow::startWithHost(DrHost *host)
{
  m_Host = host;

  raise();
  activateWindow();

  /* A game is running now; the host picker can't start a second one. */
  if (m_StartGameTab)
    m_StartGameTab->setEnabled(false);

  /* Tell the host which board seat is us, so it can surface our private state
   * (solo this is 0; in a session it is our netplay peer index). */
  m_Host->setLocalPlayer(m_NetplayPeerIndex);

#if SHOW_OVERLAY
  if (DrOverlay *ov = overlay())
  {
    QPixmap loading(":/assets/loading.png");
    ov->hold(loading);
  }
#endif

#if SHOW_LOGGER
  connect(m_Host, &DrRetro::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  connectCoreLog(m_Host->core());
#endif

  attachNetplay();

  /* If we are the netplay server, tell the connected clients to start this same
   * game. (On a client this is a no-op; the client got here via the server's
   * startGameRequested signal.) */
  if (m_Netplay->isServer())
  {
    m_Netplay->startGame(
      static_cast<int>(host->game()), host->saveFilePatterns(), m_CustomRom);
  }

  /* Only load guests that have at least one allowed mini-game; the rest never
   * boot (e.g. disabling every Dolphin mini-game skips the Dolphin core load
   * entirely). guestHasCandidate is derived from the host-authoritative filter,
   * so every peer loads the same set. */
  m_warmupQueue.clear();
  for (DrGuest *guest : m_Guests->guests())
    if (m_Guests->guestHasCandidate(guest) && guest->usesWarmup())
    {
      guest->startCore();
      m_warmupQueue.append(guest);
    }

  /* Create the host's window container before starting the core: a GL core
   * (Flycast/Sonic Shuffle) needs a native surface in place before it loads its
   * content, which some hosts defer into startCore(). */
  m_HostContainer = QWidget::createWindowContainer(m_Host->core(), m_Stack);
  m_HostContainer->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(m_HostContainer);

  m_Host->startCore();

  /* The host core is paused until showHost() unpauses it, and it sizes itself to
   * the game's native resolution once it starts running. Nudge it to fill its
   * container once it has actually booted a few frames (mirrors the deferred-guest
   * nudge). One-shot: disconnects itself after the first resize. */
  {
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_Host->core(), &QRetro::frameBegin, this, [this, conn]() {
      if (!m_Host || !m_Host->core() || m_Host->core()->frames() < 10)
        return;
      QObject::disconnect(*conn);
      QMetaObject::invokeMethod(this, [this]() {
        if (m_HostContainer && m_Host && m_Host->core())
          m_Host->core()->resize(m_HostContainer->width(), m_HostContainer->height());
      }, Qt::QueuedConnection);
    }, Qt::DirectConnection);
  }

#if SHOW_DEBUG
  connect(m_Debug, &DrDebug::cancelRequested, this, [this]() {
    cancelActiveMinigame();
    /* In a netplay session, cancel for everyone (no-op otherwise). */
    m_Netplay->broadcastCancelMinigame();
  });

  connect(m_Debug, &DrDebug::setTurnRequested, this,
    [this](int turn) { m_Host->setCurrentTurn(turn); });

  /* The dialog holds no host pointer, so the read happens here and goes back as a
   * plain array. Start from what the dialog already shows so fields the host doesn't
   * track keep their current value. */
  connect(m_Debug, &DrDebug::readPlayersRequested, this, [this]() {
    DrPlayerArray players = m_Debug->players();
    if (m_Host->readPlayerSetup(players))
      m_Debug->setPlayers(players);
    else
      m_Logger->message(DR_LOG_WARN, "read players: this host can't read its player setup");
  });

  connect(m_Debug, &DrDebug::writePlayersRequested, this, [this](DrPlayerArray players) {
    if (!m_Host->writePlayerSetup(players))
      m_Logger->message(DR_LOG_WARN, "write players: this host can't write its player setup");
  });
#endif

  /* The host pulls its own candidates straight from the guest list's cache when
   * it opens the roulette. Every peer rolls locally from the shared seeded PRNG
   * (dr_rand) at the same lockstepped frame, so the picks match without a network
   * round-trip and the client reaches ROULETTE alongside the server. */
  m_Host->setMinigameSource(m_Guests);

  /* Freeze the active context to wait for all netplay peers when a mini-game is started... */
  connect(m_Host, &DrHost::minigameRequested, m_Netplay,
    [this](DrMinigameCandidate, std::array<dr_player_t, 4>) { m_Netplay->freezeActiveContext(); },
    Qt::DirectConnection);

  /* ...and when it has ended and we return to the board context. */
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::minigameFinished, m_Netplay, &DrNetplay::freezeActiveContext,
      Qt::DirectConnection);

  /* A guest that couldn't run its setup deterministically (see CoreDolphin) asks to
   * be re-synced from the host, so all peers converge on one state. */
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::desyncSuspected, m_Netplay, &DrNetplay::requestResync);

  /* A guest can also proactively request a hard resync (e.g. entering a mini-game). */
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::hardResyncRequested, m_Netplay, &DrNetplay::requestResync);

  /* A guest (e.g. a golf mini-game) can request netplay "golf mode" -- one player
   * gets 0 input delay, the rest a high delay for turn-based priority. */
  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::golfModeRequested, m_Netplay, &DrNetplay::setGolfMode);

  /* The host can too (e.g. an item mini-game where only one player participates). */
  connect(m_Host, &DrHost::golfModeRequested, m_Netplay, &DrNetplay::setGolfMode);

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

/* How long holdView waits for the core to hand back a frame before covering the
 * swap with the bare page instead. Comfortably longer than a frame, short enough
 * that a stalled core doesn't leave the swap uncovered for long. */
#define DR_OVERLAY_GRAB_TIMEOUT_MS 200

void MainWindow::holdView(QRetro *core)
{
  DrOverlay *ov = overlay();
  QWidget *page = m_Stack ? m_Stack->currentWidget() : nullptr;

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

  /* The frame arrives a frame late for a hardware-rendered core, so the overlay
   * only goes up once it lands; until then the live core is still on screen and
   * the swap hasn't happened yet. Whichever of the frame or the timeout comes
   * first wins. */
  auto held = std::make_shared<bool>(false);
  auto conn = std::make_shared<QMetaObject::Connection>();

  *conn = connect(core, &QRetro::frameGrabbed, this,
    [ov, base, target, held, conn](const QImage &frame) {
      QObject::disconnect(*conn);
      if (*held)
        return;
      *held = true;

      QPixmap shot = base;
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

void MainWindow::launchMinigame(
  DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4])
{
  if (!m_Guests->activateGuest(guest))
    return;

  /* Back-to-back launches from the debug and challenge tabs can arrive while a
   * mini-game is still running, on this guest or another. Drop them all first --
   * a guest left marked active never starts its next one. */
  for (DrGuest *g : m_Guests->guests())
    if (g->minigameActive())
    {
      if (m_Logger)
        m_Logger->message(DR_LOG_INFO,
          QString("cancelling running mini-game on %1").arg(g->name()));
      g->cancelMinigame();
    }

#if SHOW_OVERLAY
  if (DrOverlay *ov = overlay())
  {
    /* Between continuous-play challenge mini-games, show the loading card with the
     * last result and what is coming up instead of the frozen frame. */
    const QString result =
      (m_Challenge && m_Challenge->isPending()) ? m_Challenge->takeContinuousResult() : QString();
    if (!result.isEmpty())
    {
      /* A guest can host several games (e.g. the shared Dolphin core), so the
       * game name is the mini-game's owning group, not the guest's own name. */
      QString game = QString::fromUtf8(guest->name());
      for (const DrMinigameGroup &group : guest->minigameGroups())
        for (const dr_mp_minigame_t *mg : group.minigames)
          if (mg == minigame)
            game = QString::fromUtf8(group.name);
      ov->showLoadingCard(
        result, game, minigame ? QString::fromUtf8(minigame->name) : QString());
    }
    else
    {
      /* Freeze what is on screen to cover the core swap. With no host chosen
       * (challenge mode, or a debug launch) there is no core to draw in, so the
       * page rasterizes on its own. */
      holdView(m_Host ? m_Host->core() : nullptr);
    }
  }
#endif

  QTimer::singleShot(32, this,
    [this, guest, minigame,
      players = std::array<dr_player_t, 4>{ players[0], players[1], players[2], players[3] }]() {
      if (m_Host)
        m_Host->pause();
      m_Stack->setCurrentWidget(m_Guests);
      for (DrGuest *g : m_Guests->guests())
        g->pause();

      /* audio() is null until a core has booted; a deferred-boot guest (PS2/Kirby)
       * hasn't started yet on its first launch, so guard this. Keep the core muted
       * until the minigame actually starts (unmuted in the connection below). */
      if (auto *a = guest->core()->audio())
        a->setMute(true);

      /* Once the guest actually starts its minigame, drop the loading overlay and
       * unmute the core. A synchronous guest fires this during applyGameData below;
       * a deferred guest (PS2/Kirby) that boots + presses A over several frames
       * keeps the overlay up and the audio muted the whole time. Self-disconnects. */
      auto conn = std::make_shared<QMetaObject::Connection>();
      *conn = connect(guest, &DrGuest::minigameStarted, this, [this, guest, conn]() {
        /* Only now — once the guest has finished booting/loading — make it the
         * foreground netplay context. Gating it any earlier deadlocks a deferred
         * guest whose load blocks the GUI thread in waitFrames() (e.g. Dolphin's
         * disc swap): the gated frame waits for peer input the blocked GUI thread
         * can't read. See launchMinigame's comment where setActiveContext used to
         * live. */
        m_Netplay->setActiveContext(guest->core());
        /* startMinigame() paused the core so it latched at a deterministic frame;
         * resume now that it is the gated foreground context, so the first frame the
         * core runs is the (input-synced) barrier frame. */
        guest->unpause();
#if SHOW_OVERLAY
        if (m_Overlay)
          m_Overlay->fadeOut();
#endif
        if (auto *a = guest->core()->audio())
          a->setMute(false);
        disconnect(*conn);
      });

      /* Hand the guest the whole setup — minigame + all four players — in one
       * call, so it can process everything at once. */
      DrGameData data;
      data.minigame = minigame;
      data.type = minigame ? minigame->type : DR_MINIGAME_INVALID;
      data.battle_pot = m_Host ? m_Host->battlePot() : 0;
      data.host_platform = m_Host ? m_Host->platform() : DR_HOST_PLATFORM_INVALID;
      for (unsigned i = 0; i < 4; i++)
        data.players[i] = players[i];
      guest->applyGameData(data);

      /* The guest becomes the foreground netplay context from its minigameStarted
       * handler above — once it has actually booted/loaded — not here. A deferred
       * guest (e.g. Dolphin) hasn't loaded yet at this point, and gating it while
       * its GUI-thread load blocks in waitFrames() deadlocks the session. */
      guest->unpause();
    });
}

void MainWindow::setupNetplay()
{
  m_NetplayUi = new DrNetplayWidget(m_Netplay, nullptr);
  m_Tools->addTool(tr("Netplay"), m_NetplayUi);

  connect(m_Netplay, &DrNetplay::sessionStarted, this, [this](int index, int count) {
#if SHOW_LOGGER
    m_Logger->message(DR_LOG_INFO,
      QString("netplay started: peer %1 of %2").arg(index).arg(count));
#endif
    /* Remember our seat, and update a running host if it already started. */
    m_NetplayPeerIndex = index;
    if (m_Host)
      m_Host->setLocalPlayer(index);
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

  /* A peer cancelled the mini-game -- cancel here too and return to the board. */
  connect(m_Netplay, &DrNetplay::cancelMinigameReceived, this, &MainWindow::cancelActiveMinigame);

  /* A debug-menu launch has reached its scheduled gated frame (in lockstep on every
   * peer). DrNetplay already froze the active context on the timing thread at that exact
   * frame, so here we only deserialize and launch on the GUI thread. */
  connect(m_Netplay, &DrNetplay::debugLaunchReady, this, [this](QByteArray payload) {
    if (payload.size() < 2 + 4 * DR_DEBUG_LAUNCH_PLAYER_SIZE)
      return;
    auto u8 = [&](int i) { return static_cast<uint8_t>(payload.at(i)); };
    DrGuest *guest = m_Guests->guests().value(u8(0), nullptr);
    if (!guest)
      return;
    const dr_mp_minigame_t *minigame = guest->minigames();
    for (int k = 0; k < u8(1) && minigame && minigame->name; k++)
      minigame++;
    if (!minigame || !minigame->name)
      return;

    std::array<dr_player_t, 4> players{};
    int off = 2;
    for (unsigned i = 0; i < 4; i++)
    {
      players[i].character = static_cast<dr_character>(u8(off++));
      players[i].control_port = static_cast<dr_control_port>(u8(off++));
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

    launchMinigame(guest, minigame, players.data());
  });

  /* A client follows the server's game choice: build the matching host and
   * start it locally. */
  connect(m_Netplay, &DrNetplay::startGameRequested, this,
    [this](int gameId, const QString &customRom) {
      if (m_Host)
        return;

      const dr_game game = static_cast<dr_game>(gameId);
      DrHost *host = nullptr;

      if (game >= DR_GAME_MARIOPARTY1 && game <= DR_GAME_MARIOPARTY8)
      {
        const int mp = 1 + (game - DR_GAME_MARIOPARTY1);
        QString path;

        /* The server named a modified ROM; find our own copy of it. Starting the
         * stock game instead would desync on the first frame, so refuse. */
        if (!customRom.isEmpty())
        {
          path = dr_custom_rom_path(mp, customRom);
          if (path.isEmpty())
          {
            if (m_Logger)
              m_Logger->message(DR_LOG_ERROR,
                QString("netplay: host uses custom ROM \"%1\", not found in custom/mp%2")
                  .arg(customRom)
                  .arg(mp));
            return;
          }
        }
        m_CustomRom = customRom;
        host = makeHost(mp, path);
      }
      else if (game == DR_GAME_SONICSHUFFLE)
        host = new SonicShuffleHost(this);

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

  /* The board core is attached first, so it is context 0 -- the one whose RNG
   * netplay cross-checks between peers (see DrNetplay::setRngProbe). */
  DrHost *host = m_Host;
  m_Netplay->setRngProbe([host]() { return host->rngValue(); });

  QSet<QRetro *> seen;
  m_Netplay->attachCore(m_Host->core(), QStringLiteral("host"));
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
      m_Netplay->attachCore(core, QString::fromUtf8(guest->name()));
      seen.insert(core);
    }
  }
}

void MainWindow::showChooser()
{
#if SHOW_OVERLAY
  if (DrGuest *guest = m_Guests->currentGuest())
  {
    if (guest->core())
      holdView(guest->core());
  }
#endif

  /* Deferred like showHost() so the finishing frame settles before we swap. */
  QTimer::singleShot(32, this, [this]() {
    for (DrGuest *guest : m_Guests->guests())
      guest->pause();
    m_Stack->setCurrentIndex(0);
#if SHOW_OVERLAY
    if (m_Overlay)
      m_Overlay->fadeOut();
#endif
  });
}

void MainWindow::cancelActiveMinigame()
{
  if (DrGuest *guest = m_Guests->currentGuest())
    guest->cancelMinigame();
  showHost();
}

void MainWindow::showHost()
{

#if SHOW_OVERLAY
  holdView(m_Guests->currentGuest()->core());
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
    if (m_Overlay)
      m_Overlay->fadeOut();
#endif
  });
}

void MainWindow::showGuests()
{
#if SHOW_OVERLAY
  holdView(m_Host->core());
#endif

  QTimer::singleShot(32, this, [this]() {
    m_Host->pause();
    m_Stack->setCurrentWidget(m_Guests);
    for (DrGuest *guest : m_Guests->guests())
      guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  });
}

MainWindow::~MainWindow() {}
