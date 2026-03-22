#include "mainwindow.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <cstdio>
#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>
#include <QWidget>

#include "guests/MarioKart64.h"
#include "guests/MarioParty4.h"

#define N64_CORE "/media/keith/devtools/libretro/cores/mupen64plus_next_libretro.so"
#define N64_GAME "/media/keith/devtools/libretro/roms/Mario Party 3 (USA).z64"
#define N64_SCENE_ADDR 0x800ce200
#define N64_SCENE_MINIEXPLAIN 0x70
#define N64_SCENE_MINIRESULTS 0x71

#define N64_P1_CHARACTER_ADDR 0x800d1108
#define N64_P2_CHARACTER_ADDR 0x800d1140
#define N64_P3_CHARACTER_ADDR 0x800d1178
#define N64_P4_CHARACTER_ADDR 0x800d11b0

#define N64_P1_DIFFICULTY_ADDR 0x800d110a
#define N64_P2_DIFFICULTY_ADDR 0x800d1142
#define N64_P3_DIFFICULTY_ADDR 0x800d117a
#define N64_P4_DIFFICULTY_ADDR 0x800d11b2

#define N64_P1_BOT_ADDR 0x800d110f
#define N64_P2_BOT_ADDR 0x800d1147
#define N64_P3_BOT_ADDR 0x800d117f
#define N64_P4_BOT_ADDR 0x800d11b7

#define N64_P1_MINIGAMERESULT_ADDR 0x800d1112
#define N64_P2_MINIGAMERESULT_ADDR 0x800d114a
#define N64_P3_MINIGAMERESULT_ADDR 0x800d1182
#define N64_P4_MINIGAMERESULT_ADDR 0x800d11ba

static const dr_character N64_CHAR_TO_DR[] = {
  DR_CHARACTER_MARIO,       // 0x00
  DR_CHARACTER_LUIGI,       // 0x01
  DR_CHARACTER_PEACH,       // 0x02
  DR_CHARACTER_YOSHI,       // 0x03
  DR_CHARACTER_WARIO,       // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
  DR_CHARACTER_DAISY,       // 0x06
  DR_CHARACTER_WALUIGI,     // 0x07
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
  m_Guests->add(new MarioKart64());

  m_RetroB = new QRetro();
  m_RetroB->loadCore(N64_CORE);
  m_RetroB->loadContent(N64_GAME);

  for (DrGuest *guest : m_Guests->guests())
    guest->startCore();
  m_RetroB->startCore();

  QToolBar *toolbar = addToolBar("Controls");
  toolbar->setMovable(false);
  m_SwitchButton = new QPushButton("Switch to B", toolbar);
  toolbar->addWidget(m_SwitchButton);
  m_SceneLabel = new QLabel("Scene: --", toolbar);
  toolbar->addWidget(m_SceneLabel);

  m_Stack = new QStackedWidget(this);
  m_Stack->addWidget(m_Guests);
  QWidget *containerB = QWidget::createWindowContainer(m_RetroB, m_Stack);
  containerB->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(containerB);

  setCentralWidget(m_Stack);

  connect(m_SwitchButton, &QPushButton::clicked, this, &MainWindow::switchCore);

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
        if (m_Stack->currentIndex() == 1)
          m_SceneLabel->setText(QString("Scene: 0x%1").arg(val, 2, 16, QChar('0')));
        if (val == N64_SCENE_MINIEXPLAIN) {
          static const size_t N64_CHAR_ADDR[4] = { N64_P1_CHARACTER_ADDR, N64_P2_CHARACTER_ADDR, N64_P3_CHARACTER_ADDR, N64_P4_CHARACTER_ADDR };
          static const size_t N64_DIFF_ADDR[4] = { N64_P1_DIFFICULTY_ADDR, N64_P2_DIFFICULTY_ADDR, N64_P3_DIFFICULTY_ADDR, N64_P4_DIFFICULTY_ADDR };
          static const size_t N64_BOT_ADDR[4]  = { N64_P1_BOT_ADDR, N64_P2_BOT_ADDR, N64_P3_BOT_ADDR, N64_P4_BOT_ADDR };

          DrGuest *guest = m_Guests->startMinigame(DR_MINIGAME_4P);
          if (guest) {
            for (unsigned i = 0; i < 4; i++) {
              uint8_t chr, diff, bot;
              if (m_RetroB->memory().readValue<uint8_t>(&chr,  N64_CHAR_ADDR[i]) &&
                  m_RetroB->memory().readValue<uint8_t>(&diff, N64_DIFF_ADDR[i]) &&
                  m_RetroB->memory().readValue<uint8_t>(&bot,  N64_BOT_ADDR[i])) {
                dr_player_t player = {};
                if (chr  < sizeof(N64_CHAR_TO_DR)  / sizeof(*N64_CHAR_TO_DR))
                  player.character    = N64_CHAR_TO_DR[chr];
                if (diff < sizeof(N64_DIFF_TO_DR) / sizeof(*N64_DIFF_TO_DR))
                  player.difficulty   = N64_DIFF_TO_DR[diff];
                player.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
                player.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + i);
                guest->setPlayer(i, player);
              }
            }
          }

          QMetaObject::invokeMethod(this, &MainWindow::showGuests, Qt::QueuedConnection);
          writing = 30;
        }
      }
    }
  }, Qt::DirectConnection);

  showHost();
  resize(640, 510);
}

void MainWindow::showHost()
{
  for (DrGuest *guest : m_Guests->guests())
    guest->pause();
  m_RetroB->unpause();
  m_Stack->setCurrentIndex(1);
  m_SwitchButton->setText("Switch to A");
}

void MainWindow::showGuests()
{
  m_RetroB->pause();
  for (DrGuest *guest : m_Guests->guests())
    guest == m_Guests->currentGuest() ? guest->unpause() : guest->pause();
  m_Stack->setCurrentIndex(0);
  m_SwitchButton->setText("Switch to B");
}

void MainWindow::switchCore()
{
  if (m_Stack->currentIndex() == 0)
    showHost();
  else
    showGuests();
}

MainWindow::~MainWindow()
{
  delete m_RetroB;
}
