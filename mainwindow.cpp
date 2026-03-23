#include "mainwindow.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <cstdio>
#include <QString>
#include <QStackedWidget>
#include <QDockWidget>
#include <QWidget>

#include "guests/MarioKart64.h"
#include "guests/MarioParty4.h"
#include "guests/SmashRemix.h"

#define SHOW_LOGGER 0

#define N64_CORE "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so"
#define N64_GAME "/media/keith/devtools/libretro/roms/Mario Party 3 (USA).z64"
#define N64_SCENE_ADDR 0x800ce200
#define N64_SCENE_MINIEXPLAIN 0x70
#define N64_SCENE_MINIRESULTS 0x71

#define N64_P1_CHARACTER_ADDR 0x800d1108
#define N64_P2_CHARACTER_ADDR 0x800d1140
#define N64_P3_CHARACTER_ADDR 0x800d1178
#define N64_P4_CHARACTER_ADDR 0x800d11b0

#define N64_P1_CONTROLLER_ADDR 0x800d1109
#define N64_P2_CONTROLLER_ADDR 0x800d1141
#define N64_P3_CONTROLLER_ADDR 0x800d1179
#define N64_P4_CONTROLLER_ADDR 0x800d11b1

#define N64_P1_DIFFICULTY_ADDR 0x800d110a
#define N64_P2_DIFFICULTY_ADDR 0x800d1142
#define N64_P3_DIFFICULTY_ADDR 0x800d117a
#define N64_P4_DIFFICULTY_ADDR 0x800d11b2

#define N64_P1_TEAM_ADDR 0x800d110b
#define N64_P2_TEAM_ADDR 0x800d1143
#define N64_P3_TEAM_ADDR 0x800d117b
#define N64_P4_TEAM_ADDR 0x800d11b3

#define N64_P1_BOT_ADDR 0x800d110f
#define N64_P2_BOT_ADDR 0x800d1147
#define N64_P3_BOT_ADDR 0x800d117f
#define N64_P4_BOT_ADDR 0x800d11b7

#define N64_P1_MINIGAMERESULT_ADDR 0x800d1112
#define N64_P2_MINIGAMERESULT_ADDR 0x800d114a
#define N64_P3_MINIGAMERESULT_ADDR 0x800d1182
#define N64_P4_MINIGAMERESULT_ADDR 0x800d11ba

#define N64_P1_PANEL_COLOR_ADDR 0x800d1127
#define N64_P2_PANEL_COLOR_ADDR 0x800d115f
#define N64_P3_PANEL_COLOR_ADDR 0x800d1197
#define N64_P4_PANEL_COLOR_ADDR 0x800d11cf

static const dr_character N64_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO,       // 0x00
  DR_CHARACTER_LUIGI,       // 0x01
  DR_CHARACTER_PEACH,       // 0x02
  DR_CHARACTER_YOSHI,       // 0x03
  DR_CHARACTER_WARIO,       // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
  DR_CHARACTER_WALUIGI,     // 0x06
  DR_CHARACTER_DAISY,       // 0x07
};

static const dr_difficulty N64_DIFF_TO_DR[] = {
  DR_DIFFICULTY_EASY,       // 0x00
  DR_DIFFICULTY_NORMAL,     // 0x01
  DR_DIFFICULTY_HARD,       // 0x02
};

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  m_Guests = new DrGuestList(this);
  m_Guests->add(new MarioParty4());
  //m_Guests->add(new MarioKart64());
  //m_Guests->add(new SmashRemix());

  m_RetroB = new QRetro();
  m_RetroB->loadCore(N64_CORE);
  m_RetroB->loadContent(N64_GAME);

  for (DrGuest *guest : m_Guests->guests())
    guest->startCore();
  m_RetroB->startCore();

  m_Stack = new QStackedWidget(this);
  m_Stack->addWidget(m_Guests);
  QWidget *containerB = QWidget::createWindowContainer(m_RetroB, m_Stack);
  containerB->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(containerB);

  setCentralWidget(m_Stack);

  // Cycle through each guest for 2 seconds, then switch to the host
  m_Stack->setCurrentIndex(0);
  QTimer *warmupTimer = new QTimer(this);
  warmupTimer->setInterval(2000);
  connect(warmupTimer, &QTimer::timeout, this, [this, warmupTimer]() {
    int next = m_Guests->currentIndex() + 1;
    if (next < m_Guests->count()) {
      m_Guests->setCurrentIndex(next);
    } else {
      warmupTimer->stop();
      warmupTimer->deleteLater();
      showHost();
    }
  });
  warmupTimer->start();

#if SHOW_LOGGER
  m_Logger = new DrLogger(this);
  QDockWidget *logDock = new QDockWidget("Log", this);
  logDock->setWidget(m_Logger);
  addDockWidget(Qt::BottomDockWidgetArea, logDock);
  logDock->setFloating(true);

  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
#endif

  auto recalc = [this]() {
    resize(width() + 1, height());
    resize(width() - 1, height());
  };

  for (DrGuest *guest : m_Guests->guests()) {
    QMetaObject::Connection *conn = new QMetaObject::Connection;
    *conn = connect(guest, &QRetro::frameBegin, this, [recalc, conn]() {
      recalc();
      disconnect(*conn);
      delete conn;
    });
  }

  QMetaObject::Connection *connB = new QMetaObject::Connection;
  *connB = connect(m_RetroB, &QRetro::frameBegin, this, [recalc, connB]() {
    recalc();
    disconnect(*connB);
    delete connB;
  });

  connect(m_Guests, &DrGuestList::minigameFinished, this, [this]() {
    DrGuest *guest = m_Guests->currentGuest();
    for (unsigned i = 0; i < 4; i++) {
      auto result = guest->minigameResult(i);
      m_RetroB->memory().writeValue<uint8_t>(
        static_cast<uint8_t>(result.coins),
        (size_t[]){ N64_P1_MINIGAMERESULT_ADDR, N64_P2_MINIGAMERESULT_ADDR,
                    N64_P3_MINIGAMERESULT_ADDR, N64_P4_MINIGAMERESULT_ADDR }[i]);
    }
    showHost();
  }, Qt::QueuedConnection);

  connect(m_RetroB, &QRetro::frameBegin, this, [this, last = uint8_t(0xFF), writing = 0]() mutable {
    if (m_RetroB->frames() > 120) {
      if (writing > 0) {
        m_RetroB->memory().writeValue<uint8_t>(N64_SCENE_MINIRESULTS, N64_SCENE_ADDR);
        if (--writing == 0)
          last = N64_SCENE_MINIRESULTS;
        return;
      }
      uint8_t val;
      if (m_RetroB->memory().readValue<uint8_t>(&val, N64_SCENE_ADDR) && val != last) {
        printf("N64_SCENE_ADDR: 0x%02X\n", val);
        last = val;
        if (val == N64_SCENE_MINIEXPLAIN) {
          static const size_t N64_CHAR_ADDR[4] = { N64_P1_CHARACTER_ADDR, N64_P2_CHARACTER_ADDR, N64_P3_CHARACTER_ADDR, N64_P4_CHARACTER_ADDR };
          static const size_t N64_CTRL_ADDR[4] = { N64_P1_CONTROLLER_ADDR, N64_P2_CONTROLLER_ADDR, N64_P3_CONTROLLER_ADDR, N64_P4_CONTROLLER_ADDR };
          static const size_t N64_DIFF_ADDR[4] = { N64_P1_DIFFICULTY_ADDR, N64_P2_DIFFICULTY_ADDR, N64_P3_DIFFICULTY_ADDR, N64_P4_DIFFICULTY_ADDR };
          static const size_t N64_BOT_ADDR[4]  = { N64_P1_BOT_ADDR, N64_P2_BOT_ADDR, N64_P3_BOT_ADDR, N64_P4_BOT_ADDR };
          static const size_t N64_TEAM_ADDR[4] = { N64_P1_TEAM_ADDR, N64_P2_TEAM_ADDR, N64_P3_TEAM_ADDR, N64_P4_TEAM_ADDR };

          static const size_t N64_PANEL_COLOR_ADDR[4] = { N64_P1_PANEL_COLOR_ADDR, N64_P2_PANEL_COLOR_ADDR, N64_P3_PANEL_COLOR_ADDR, N64_P4_PANEL_COLOR_ADDR };

          uint8_t teamBytes[4];
          uint8_t panelColors[4];
          for (unsigned i = 0; i < 4; i++) {
            if (!m_RetroB->memory().readValue<uint8_t>(&teamBytes[i], N64_TEAM_ADDR[i]))
              teamBytes[i] = 0xFF;
            if (!m_RetroB->memory().readValue<uint8_t>(&panelColors[i], N64_PANEL_COLOR_ADDR[i]))
              panelColors[i] = 0xFF;
          }

          auto log = [this](unsigned level, QString msg) {
#if SHOW_LOGGER
            QMetaObject::invokeMethod(m_Logger, "message", Qt::QueuedConnection,
              Q_ARG(unsigned, level), Q_ARG(QString, msg));
#else
            (void)level; (void)msg;
#endif
          };

          // Check panel colors — skip if any is 0
          int zeroPanelPlayer = -1;
          for (unsigned i = 0; i < 4; i++)
            if (panelColors[i] == 0) { zeroPanelPlayer = (int)i; break; }

          if (zeroPanelPlayer != -1) {
            log(DR_LOG_WARN, QString("skipping minigame: panel color for P%1 is 0").arg(zeroPanelPlayer + 1));
          } else {
            // Count occurrences of each distinct team value
            unsigned teamCount[4] = {};
            uint8_t  teamVals[4]  = {};
            unsigned numTeams     = 0;
            for (unsigned i = 0; i < 4; i++) {
              unsigned j = 0;
              for (; j < numTeams; j++)
                if (teamVals[j] == teamBytes[i]) { teamCount[j]++; break; }
              if (j == numTeams) { teamVals[numTeams] = teamBytes[i]; teamCount[numTeams++] = 1; }
            }

            // Determine minigame type:
            //   4 distinct values (0,1,2,3) → 4P free-for-all
            //   2 distinct values, 2+2 → 2v2
            //   2 distinct values, 1+3 or 3+1 → 1v3
            //   anything else → ignore
            dr_minigame_type mgType = DR_MINIGAME_INVALID;
            if (numTeams == 4) {
              mgType = DR_MINIGAME_4P;
            } else if (numTeams == 2) {
              unsigned a = teamCount[0], b = teamCount[1];
              if      (a == 2 && b == 2) mgType = DR_MINIGAME_2V2;
              else if (a == 1 || b == 1) mgType = DR_MINIGAME_1V3;
            }

            if (mgType == DR_MINIGAME_INVALID) {
              log(DR_LOG_WARN, QString("skipping minigame: teams = [%1, %2, %3, %4]")
                .arg(teamBytes[0]).arg(teamBytes[1]).arg(teamBytes[2]).arg(teamBytes[3]));
            } else {
              log(DR_LOG_INFO, QString("starting minigame: teams = [%1, %2, %3, %4]")
                .arg(teamBytes[0]).arg(teamBytes[1]).arg(teamBytes[2]).arg(teamBytes[3]));

              DrGuest *guest = m_Guests->startMinigame(mgType);
              if (guest) {
                for (unsigned i = 0; i < 4; i++) {
                  uint8_t chr, ctrl, diff, bot;
                  if (m_RetroB->memory().readValue<uint8_t>(&chr,  N64_CHAR_ADDR[i]) &&
                      m_RetroB->memory().readValue<uint8_t>(&ctrl, N64_CTRL_ADDR[i]) &&
                      m_RetroB->memory().readValue<uint8_t>(&diff, N64_DIFF_ADDR[i]) &&
                      m_RetroB->memory().readValue<uint8_t>(&bot,  N64_BOT_ADDR[i])) {
                    dr_player_t player = {};
                    if (chr  < sizeof(N64_CHAR_TO_DR) / sizeof(*N64_CHAR_TO_DR))
                      player.character   = N64_CHAR_TO_DR[chr];
                    if (diff < sizeof(N64_DIFF_TO_DR) / sizeof(*N64_DIFF_TO_DR))
                      player.difficulty  = N64_DIFF_TO_DR[diff];
                    player.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
                    player.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
                    player.team         = (teamBytes[i] == 0x01) ? DR_TEAM_RED : DR_TEAM_BLUE;
                    guest->setPlayer(i, player);
                  }
                }
              }

              QMetaObject::invokeMethod(this, &MainWindow::showGuests, Qt::QueuedConnection);
              writing = 30;
            }
          }
        }
      }
    }
  }, Qt::DirectConnection);

  resize(640, 480);
}

void MainWindow::showHost()
{
  for (DrGuest *guest : m_Guests->guests())
    guest->pause();
  m_RetroB->unpause();
  m_Stack->setCurrentIndex(1);
}

void MainWindow::showGuests()
{
  m_RetroB->pause();
  for (DrGuest *guest : m_Guests->guests())
    guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  m_Stack->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
  delete m_RetroB;
}
