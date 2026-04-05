#include "mainwindow.h"

#include <array>
#include <QRandomGenerator>
#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QGuiApplication>
#include <QScreen>
#include <cstdio>
#include <QString>
#include <QStackedWidget>
#include <QWidget>

#include "DrDebug.h"
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
  auto *dolphin = new CoreDolphin(this);
  dolphin->addGame(new MarioParty4(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty5(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty6(dolphin->core(), dolphin));
  dolphin->addGame(new MarioParty7(dolphin->core(), dolphin));
  dolphin->finalizeGames();
  m_Guests->add(dolphin);
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

#if SHOW_LOGGER
  m_Logger = new DrLogger(nullptr);
  m_Logger->setWindowTitle("Log");
  m_Logger->resize(600, 300);
  m_Logger->show();

  for (DrGuest *guest : m_Guests->guests())
    connect(guest, &DrGuest::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  connect(m_Guests, &DrGuestList::logMessage, m_Logger, &DrLogger::message, Qt::QueuedConnection);
  m_Guests->logSummary();
#endif

#if SHOW_DEBUG
  m_Debug = new DrDebug(nullptr);
  m_Debug->setWindowTitle("Debug");
  m_Debug->resize(300, 200);
  m_Debug->populate(m_Guests->guests());
  m_Debug->show();

  connect(m_Debug, &DrDebug::minigameRequested, this, [this](DrGuest *guest, const dr_mp_minigame_t *minigame) {
    dr_player_t players[4] = {};
    bool playerValid[4] = { true, true, true, true };
    players[0] = { DR_CHARACTER_MARIO,       DR_CONTROL_PORT_P1, DR_CONTROL_TYPE_HUMAN, DR_DIFFICULTY_NORMAL, DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 0 };
    players[1] = { DR_CHARACTER_LUIGI,       DR_CONTROL_PORT_P2, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_NORMAL, DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 1 };
    players[2] = { DR_CHARACTER_PEACH,       DR_CONTROL_PORT_P3, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_NORMAL, DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 2 };
    players[3] = { DR_CHARACTER_DONKEY_KONG, DR_CONTROL_PORT_P4, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_NORMAL, DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 3 };
    launchMinigame(guest, minigame, players, playerValid);
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
    static const size_t N64_CHAR_ADDR[4] = {
      N64_P1_CHARACTER_ADDR, N64_P2_CHARACTER_ADDR,
      N64_P3_CHARACTER_ADDR, N64_P4_CHARACTER_ADDR
    };
    static const size_t N64_RESULT_ADDR[4] = {
      N64_P1_MINIGAMERESULT_ADDR, N64_P2_MINIGAMERESULT_ADDR,
      N64_P3_MINIGAMERESULT_ADDR, N64_P4_MINIGAMERESULT_ADDR
    };

    DrGuest *guest = m_Guests->currentGuest();
    for (unsigned i = 0; i < 4; i++) {
      auto result = guest->minigameResult(i);

      uint8_t chr = 0;
      m_RetroB->memory().readValue<uint8_t>(&chr, N64_CHAR_ADDR[i]);
      dr_character character = (chr < sizeof(N64_CHAR_TO_DR) / sizeof(*N64_CHAR_TO_DR))
        ? N64_CHAR_TO_DR[chr] : DR_CHARACTER_INVALID;

#if SHOW_LOGGER
      if (m_Logger)
        m_Logger->message(DR_LOG_INFO,
          QString("%1 gets %2 coins").arg(dr_character_name(character)).arg(result.coins));
#endif

      m_RetroB->memory().writeValue<uint8_t>(
        static_cast<uint8_t>(result.coins), N64_RESULT_ADDR[i]);
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
        QMetaObject::invokeMethod(m_Logger, "message", Qt::QueuedConnection,
          Q_ARG(unsigned, DR_LOG_INFO),
          Q_ARG(QString, QString("N64_SCENE_ADDR: 0x%1").arg(val, 2, 16, QChar('0'))));
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

          // Check panel colors — 0 is uninitialized/invalid
          int badPanelPlayer = -1;
          for (unsigned i = 0; i < 4; i++)
            if (panelColors[i] == 0) { badPanelPlayer = (int)i; break; }

          if (badPanelPlayer != -1) {
            log(DR_LOG_WARN, QString("skipping minigame: panel color for P%1 is 0x%2").arg(badPanelPlayer + 1).arg(panelColors[badPanelPlayer], 2, 16, QChar('0')));
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

              // Read N64 memory on the timing thread, then hand off to GUI thread
              dr_player_t players[4] = {};
              bool playerValid[4] = {};
              for (unsigned i = 0; i < 4; i++) {
                uint8_t chr, ctrl, diff, bot;
                if (m_RetroB->memory().readValue<uint8_t>(&chr,  N64_CHAR_ADDR[i]) &&
                    m_RetroB->memory().readValue<uint8_t>(&ctrl, N64_CTRL_ADDR[i]) &&
                    m_RetroB->memory().readValue<uint8_t>(&diff, N64_DIFF_ADDR[i]) &&
                    m_RetroB->memory().readValue<uint8_t>(&bot,  N64_BOT_ADDR[i])) {
                  dr_player_t &p = players[i];
                  if (chr  < sizeof(N64_CHAR_TO_DR) / sizeof(*N64_CHAR_TO_DR))
                    p.character   = N64_CHAR_TO_DR[chr];
                  if (diff < sizeof(N64_DIFF_TO_DR) / sizeof(*N64_DIFF_TO_DR))
                    p.difficulty  = N64_DIFF_TO_DR[diff];
                  p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
                  p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
                  p.team_color   = static_cast<dr_team_color>(panelColors[i]);
                  p.team_id      = teamBytes[i];
                  playerValid[i] = true;
                }
              }

              // Determine team_type for each player
              if (mgType == DR_MINIGAME_1V3) {
                // Count which color appears once — that's the solo
                unsigned colorCount[DR_TEAM_COLOR_SIZE] = {};
                for (unsigned i = 0; i < 4; i++)
                  if (playerValid[i]) colorCount[players[i].team_color]++;
                dr_team_color soloColor = DR_TEAM_COLOR_INVALID;
                for (unsigned c = DR_TEAM_COLOR_INVALID + 1; c < DR_TEAM_COLOR_SIZE; c++)
                  if (colorCount[c] == 1) { soloColor = static_cast<dr_team_color>(c); break; }
                for (unsigned i = 0; i < 4; i++)
                  if (playerValid[i])
                    players[i].team_type = (players[i].team_color == soloColor)
                      ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
              } else if (mgType == DR_MINIGAME_2V2) {
                for (unsigned i = 0; i < 4; i++)
                  if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_2V2;
              } else if (mgType == DR_MINIGAME_4P) {
                for (unsigned i = 0; i < 4; i++)
                  if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_4P;
              } else {
                for (unsigned i = 0; i < 4; i++)
                  if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_SOLO;
              }

              // All Qt widget and guest-core operations must run on the GUI thread
              QMetaObject::invokeMethod(this, [this, mgType, players, playerValid]() {
                launchMinigame(mgType, players, playerValid);
              }, Qt::QueuedConnection);
              writing = 30;
            }
          }
        }
      }
    }
  }, Qt::DirectConnection);

  resize(704, 528);
}

void MainWindow::launchMinigame(DrGuest *guest, const dr_mp_minigame_t *minigame, const dr_player_t players[4], const bool playerValid[4])
{
  if (!m_Guests->activateGuest(guest))
    return;

  showGuests();
  guest->core()->audio()->setVolume(0);
  QTimer::singleShot(100, this, [this, guest, minigame, players = std::array<dr_player_t, 4>{players[0], players[1], players[2], players[3]}, playerValid = std::array<bool, 4>{playerValid[0], playerValid[1], playerValid[2], playerValid[3]}]() {
    guest->setMinigame(minigame);
    guest->core()->audio()->setVolume(100);
    for (unsigned i = 0; i < 4; i++)
      if (playerValid[i]) guest->setPlayer(i, players[i]);
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

  showGuests();
  guest->core()->audio()->setVolume(0);
  QTimer::singleShot(100, this, [this, guest, minigame, players = std::array<dr_player_t, 4>{players[0], players[1], players[2], players[3]}, playerValid = std::array<bool, 4>{playerValid[0], playerValid[1], playerValid[2], playerValid[3]}]() {
    guest->setMinigame(minigame);
    guest->core()->audio()->setVolume(100);
    for (unsigned i = 0; i < 4; i++)
      if (playerValid[i]) guest->setPlayer(i, players[i]);
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
    m_RetroB->unpause();
    m_Stack->setCurrentIndex(1);
  });
}

void MainWindow::showGuests()
{
#if SHOW_OVERLAY
  QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
  m_Overlay->hold(screen->grabWindow(m_RetroB->winId()));
#endif

  QTimer::singleShot(32, this, [this]() {
    m_RetroB->pause();
    m_Stack->setCurrentIndex(0);
    for (DrGuest *guest : m_Guests->guests())
      guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  });
}

MainWindow::~MainWindow()
{
  delete m_RetroB;
}
