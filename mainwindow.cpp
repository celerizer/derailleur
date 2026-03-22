#include "mainwindow.h"

#include <QRetro.h>
#include <cstdio>
#include <QList>
#include <QLabel>
#include <QRandomGenerator>
#include <QString>
#include <QPushButton>
#include <QTimer>
#include <QStackedWidget>
#include <QToolBar>
#include <QWidget>

typedef enum
{
  DR_MINIGAME_INVALID,
  DR_MINIGAME_4P,
  DR_MINIGAME_1V3,
  DR_MINIGAME_2V2,
  DR_MINIGAME_BATTLE,
  DR_MINIGAME_DUEL,
  DR_MINIGAME_ITEM,
} dr_minigame_type;

typedef struct
{
  const char *name;
  dr_minigame_type type;
  unsigned minigame_id;
  unsigned scene_id;
} dr_mp_minigame_t;

static const dr_mp_minigame_t gcn_minigames[] =
{
  {"Manta Rings",              DR_MINIGAME_4P, 0x00, 0x09},
  {"Slime Time",               DR_MINIGAME_4P, 0x01, 0x0A},
  {"Booksquirm",               DR_MINIGAME_4P, 0x02, 0x0B},
  {"Trace Race",               DR_MINIGAME_BATTLE, 0x03, 0x0C},
  {"Mario Medley",             DR_MINIGAME_4P, 0x04, 0x0D},
  {"Avalanche!",               DR_MINIGAME_4P, 0x05, 0x0E},
  {"Domination",               DR_MINIGAME_4P, 0x06, 0x0F},
  {"Paratrooper Plunge",       DR_MINIGAME_4P, 0x07, 0x10},
  {"Toad's Quick Draw",        DR_MINIGAME_4P, 0x08, 0x11},
  {"Three Throw",              DR_MINIGAME_4P, 0x09, 0x12},
  {"Photo Finish",             DR_MINIGAME_4P, 0x0A, 0x13},
  {"Mr. Blizzard's Brigade",   DR_MINIGAME_4P, 0x0B, 0x14},
  {"Bob-omb Breakers",         DR_MINIGAME_4P, 0x0C, 0x15},
  {"Long Claw of the Law",     DR_MINIGAME_4P, 0x0D, 0x16},
  {"Stamp Out!",               DR_MINIGAME_4P, 0x0E, 0x17},
  {"Candlelight Fright",       DR_MINIGAME_1V3, 0x0F, 0x18},
  {"Makin' Waves",             DR_MINIGAME_1V3, 0x10, 0x19},
  {"Hide and Go BOOM!",        DR_MINIGAME_1V3, 0x11, 0x1A},
  {"Tree Stomp",               DR_MINIGAME_1V3, 0x12, 0x1B},
  {"Fish n' Drips",            DR_MINIGAME_1V3, 0x13, 0x1C},
  {"Hop or Pop",               DR_MINIGAME_1V3, 0x14, 0x1D},
  {"Money Belts",              DR_MINIGAME_1V3, 0x15, 0x1E},
  {"GOOOOOOOAL!!",             DR_MINIGAME_1V3, 0x16, 0x1F},
  {"Blame it on the Crane",    DR_MINIGAME_1V3, 0x17, 0x20},
  {"The Great Deflate",        DR_MINIGAME_2V2, 0x18, 0x21},
  {"Revers-a-Bomb",            DR_MINIGAME_2V2, 0x19, 0x22},
  {"Right Oar Left?",          DR_MINIGAME_2V2, 0x1A, 0x23},
  {"Cliffhangers",             DR_MINIGAME_2V2, 0x1B, 0x24},
  {"Team Treasure Trek",       DR_MINIGAME_2V2, 0x1C, 0x25},
  {"Pair-a-sailing",           DR_MINIGAME_2V2, 0x1D, 0x26},
  {"Order Up",                 DR_MINIGAME_2V2, 0x1E, 0x27},
  {"Dungeon Duos",             DR_MINIGAME_2V2, 0x1F, 0x28},
  {"Beach Volley Folley",      DR_MINIGAME_DUEL, 0x20, 0x29},
  {"Cheep Cheep Sweep",        DR_MINIGAME_2V2, 0x21, 0x2A},
  {"Darts of Doom",            DR_MINIGAME_BATTLE, 0x22, 0x2B},
  {"Fruits of Doom",           DR_MINIGAME_BATTLE, 0x23, 0x2C},
  {"Balloon of Doom",          DR_MINIGAME_BATTLE, 0x24, 0x2D},
  {"Chain Chomp Fever",        DR_MINIGAME_BATTLE, 0x25, 0x2E},
  {"Paths of Peril",           DR_MINIGAME_BATTLE, 0x26, 0x2F},
  {"Bowser's Bigger Blast",    DR_MINIGAME_BATTLE, 0x27, 0x30},
  {"Butterfly Blitz",          DR_MINIGAME_BATTLE, 0x28, 0x31},
  {"Barrel Baron",             DR_MINIGAME_DUEL, 0x29, 0x32},
  {"Mario Speedwagons",        DR_MINIGAME_4P, 0x2A, 0x33},
  /* 0x2B unknown */
  {"Bowser Bop",               DR_MINIGAME_ITEM, 0x2C, 0x35},
  {"Mystic Match 'Em",         DR_MINIGAME_ITEM, 0x2D, 0x36},
  {"Archaeologuess",           DR_MINIGAME_ITEM, 0x2E, 0x37},
  {"Goomba's Chip Flip",       DR_MINIGAME_ITEM, 0x2F, 0x38},
  {"Kareening Koopas",         DR_MINIGAME_ITEM, 0x30, 0x39},
  {"The Final Battle!",        DR_MINIGAME_INVALID, 0x31, 0x3A},
  {"Jigsaw Jitters",           DR_MINIGAME_INVALID, 0xFF, 0x3B},
  {"Challenge Booksquirm",     DR_MINIGAME_INVALID, 0xFF, 0x3C},
  {"Rumble Fishing",           DR_MINIGAME_BATTLE, 0xFF, 0x3D},
  {"Take a Breather",          DR_MINIGAME_4P, 0xFF, 0x3E},
  {"Bowser Wrestling",         DR_MINIGAME_DUEL, 0xFF, 0x3F},
  {"Mushroom Medic",           DR_MINIGAME_ITEM, 0xFF, 0x41},
  {"Doors of Doom",            DR_MINIGAME_ITEM, 0xFF, 0x42},
  {"Bob-omb X-ing",            DR_MINIGAME_ITEM, 0xFF, 0x43},
  {"Goomba Stomp",             DR_MINIGAME_ITEM, 0xFF, 0x44},
  {"Panel Panic",              DR_MINIGAME_ITEM, 0xFF, 0x45},
  {nullptr,                    DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

typedef struct
{
  const char *name;
  const char *core_path;
  const char *game_path;
  
  unsigned player_values[6];
  size_t player_addresses[4];
} dr_game_slave_t;

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

#define N64_P1_BOT_ADDR 0x800d110f
#define N64_P2_BOT_ADDR 0x800d1147
#define N64_P3_BOT_ADDR 0x800d117f
#define N64_P4_BOT_ADDR 0x800d11b7

#define N64_P1_MINIGAMERESULT_ADDR 0x800d1112
#define N64_P2_MINIGAMERESULT_ADDR 0x800d114a
#define N64_P3_MINIGAMERESULT_ADDR 0x800d1182
#define N64_P4_MINIGAMERESULT_ADDR 0x800d11ba

#define GCN_CORE "/media/keith/devtools/libretro/cores/dolphin_libretro.so"
#define GCN_GAME "/media/keith/devtools/libretro/roms/Mario Party 4 (USA) (Rev 1).rvz"
#define GCN_SCENE_ADDR 0x801d3ce3
#define GCN_MINIGAME_ADDR 0x8018FD2d
#define GCN_SCENE_MINIEXPLAIN 0x03
#define GCN_SCENE_MINIRESULTS 0x54

#define GCN_P1_CHARACTER_ADDR 0x8018fc11
#define GCN_P2_CHARACTER_ADDR 0x8018fc1b
#define GCN_P3_CHARACTER_ADDR 0x8018fc25
#define GCN_P4_CHARACTER_ADDR 0x8018fc2f

#define GCN_P1_CONTROLLER_ADDR 0x8018fc13
#define GCN_P2_CONTROLLER_ADDR 0x8018fc1d
#define GCN_P3_CONTROLLER_ADDR 0x8018fc27
#define GCN_P4_CONTROLLER_ADDR 0x8018fc31

#define GCN_P1_DIFFICULTY_ADDR 0x8018fc15
#define GCN_P2_DIFFICULTY_ADDR 0x8018fc1f
#define GCN_P3_DIFFICULTY_ADDR 0x8018fc29
#define GCN_P4_DIFFICULTY_ADDR 0x8018fc33

#define GCN_P1_BOT_ADDR 0x8018fc19
#define GCN_P2_BOT_ADDR 0x8018fc23
#define GCN_P3_BOT_ADDR 0x8018fc2d
#define GCN_P4_BOT_ADDR 0x8018fc37

#define GCN_P1_MINIGAMERESULT_ADDR 0x8018fc61
#define GCN_P2_MINIGAMERESULT_ADDR 0x8018fc91
#define GCN_P3_MINIGAMERESULT_ADDR 0x8018fcc1
#define GCN_P4_MINIGAMERESULT_ADDR 0x8018fcf1

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  m_RetroA = new QRetro();
  m_RetroB = new QRetro();

  m_RetroA->loadCore(GCN_CORE);
  m_RetroA->loadContent(GCN_GAME);

  m_RetroB->loadCore(N64_CORE);
  m_RetroB->loadContent(N64_GAME);

  m_RetroA->startCore();
  m_RetroB->startCore();

  QToolBar *toolbar = addToolBar("Controls");
  toolbar->setMovable(false);
  m_SwitchButton = new QPushButton("Switch to B", toolbar);
  toolbar->addWidget(m_SwitchButton);
  m_SceneLabel = new QLabel("Scene: --", toolbar);
  toolbar->addWidget(m_SceneLabel);

  m_Stack = new QStackedWidget(this);
  QWidget *containerA = QWidget::createWindowContainer(m_RetroA, m_Stack);
  containerA->setFocusPolicy(Qt::StrongFocus);
  QWidget *containerB = QWidget::createWindowContainer(m_RetroB, m_Stack);
  containerB->setFocusPolicy(Qt::StrongFocus);
  m_Stack->addWidget(containerA);
  m_Stack->addWidget(containerB);

  setCentralWidget(m_Stack);

  connect(m_SwitchButton, &QPushButton::clicked, this, &MainWindow::switchCore);

  auto recalc = [this]() {
    resize(width() + 1, height());
    resize(width() - 1, height());
  };

  QMetaObject::Connection *connA = new QMetaObject::Connection;
  *connA = connect(m_RetroA, &QRetro::frameBegin, this, [&, recalc, connA]() {
    recalc();
    disconnect(*connA);
    delete connA;
  });

  QMetaObject::Connection *connB = new QMetaObject::Connection;
  *connB = connect(m_RetroB, &QRetro::frameBegin, this, [&, recalc, connB]() {
    recalc();
    disconnect(*connB);
    delete connB;
  });

  connect(m_RetroA, &QRetro::frameBegin, this, [this, last = uint8_t(0xFF)]() mutable {
    if (m_RetroA->frames() > 120) {
      if (m_PendingMinigameFrames > 0) {
        m_RetroA->memory().writeValue<uint8_t>(m_PendingMinigameId, GCN_MINIGAME_ADDR);
        --m_PendingMinigameFrames;
      }
      uint8_t val;
      if (m_RetroA->memory().readValue<uint8_t>(&val, GCN_SCENE_ADDR) && val != last) {
        printf("GCN_SCENE_ADDR: 0x%02X\n", val);
        last = val;
        if (m_Stack->currentIndex() == 0)
          m_SceneLabel->setText(QString("Scene: 0x%1").arg(val, 2, 16, QChar('0')));
        if (val == GCN_SCENE_MINIRESULTS) {
          uint8_t r1, r2, r3, r4;
          if (m_RetroA->memory().readValue<uint8_t>(&r1, GCN_P1_MINIGAMERESULT_ADDR) &&
              m_RetroA->memory().readValue<uint8_t>(&r2, GCN_P2_MINIGAMERESULT_ADDR) &&
              m_RetroA->memory().readValue<uint8_t>(&r3, GCN_P3_MINIGAMERESULT_ADDR) &&
              m_RetroA->memory().readValue<uint8_t>(&r4, GCN_P4_MINIGAMERESULT_ADDR)) {
            m_RetroB->memory().writeValue<uint8_t>(r1, N64_P1_MINIGAMERESULT_ADDR);
            m_RetroB->memory().writeValue<uint8_t>(r2, N64_P2_MINIGAMERESULT_ADDR);
            m_RetroB->memory().writeValue<uint8_t>(r3, N64_P3_MINIGAMERESULT_ADDR);
            m_RetroB->memory().writeValue<uint8_t>(r4, N64_P4_MINIGAMERESULT_ADDR);
          }
          m_Stack->setCurrentIndex(1);
          m_SwitchButton->setText("Switch to A");
        }
      }
    }
  }, Qt::DirectConnection);

  connect(m_RetroA, &QRetro::frameEnd, this, [this]() {
    if (m_RetroA->frames() > 120) {
    }
  }, Qt::DirectConnection);

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
          auto filename = QString(m_RetroA->directories()->get(QRetroDirectories::State)) + "/gcn.state";
          m_RetroA->unserializeFromFile(filename);

          struct { size_t n64char, n64ctrl, n64diff, n64bot; size_t gcnchar, gcnctrl, gcndiff, gcnbot; } players[] = {
            {N64_P1_CHARACTER_ADDR, N64_P1_CONTROLLER_ADDR, N64_P1_DIFFICULTY_ADDR, N64_P1_BOT_ADDR,
             GCN_P1_CHARACTER_ADDR, GCN_P1_CONTROLLER_ADDR, GCN_P1_DIFFICULTY_ADDR, GCN_P1_BOT_ADDR},
            {N64_P2_CHARACTER_ADDR, N64_P2_CONTROLLER_ADDR, N64_P2_DIFFICULTY_ADDR, N64_P2_BOT_ADDR,
             GCN_P2_CHARACTER_ADDR, GCN_P2_CONTROLLER_ADDR, GCN_P2_DIFFICULTY_ADDR, GCN_P2_BOT_ADDR},
            {N64_P3_CHARACTER_ADDR, N64_P3_CONTROLLER_ADDR, N64_P3_DIFFICULTY_ADDR, N64_P3_BOT_ADDR,
             GCN_P3_CHARACTER_ADDR, GCN_P3_CONTROLLER_ADDR, GCN_P3_DIFFICULTY_ADDR, GCN_P3_BOT_ADDR},
            {N64_P4_CHARACTER_ADDR, N64_P4_CONTROLLER_ADDR, N64_P4_DIFFICULTY_ADDR, N64_P4_BOT_ADDR,
             GCN_P4_CHARACTER_ADDR, GCN_P4_CONTROLLER_ADDR, GCN_P4_DIFFICULTY_ADDR, GCN_P4_BOT_ADDR},
          };
          for (auto &p : players) {
            uint8_t chr, ctrl, diff, bot, gcnbot;
            if (m_RetroB->memory().readValue<uint8_t>(&chr,  p.n64char) &&
                m_RetroB->memory().readValue<uint8_t>(&ctrl, p.n64ctrl) &&
                m_RetroB->memory().readValue<uint8_t>(&diff, p.n64diff) &&
                m_RetroB->memory().readValue<uint8_t>(&bot,  p.n64bot)  &&
                m_RetroA->memory().readValue<uint8_t>(&gcnbot, p.gcnbot)) {
              if (chr == 0x06) chr = 0x07;
              else if (chr == 0x07) chr = 0x06;
              m_RetroA->memory().writeValue<uint8_t>(chr,  p.gcnchar);
              m_RetroA->memory().writeValue<uint8_t>(ctrl, p.gcnctrl);
              m_RetroA->memory().writeValue<uint8_t>(diff, p.gcndiff);
              m_RetroA->memory().writeValue<uint8_t>((gcnbot & ~0x01) | (bot & 0x01), p.gcnbot);
            }
          }

          QList<unsigned> ids;
          for (const auto *mg = gcn_minigames; mg->name; mg++)
            if (mg->type == DR_MINIGAME_4P && mg->minigame_id != 0xFF)
              ids.append(mg->minigame_id);
          if (!ids.isEmpty())
          {
            m_PendingMinigameId = ids[QRandomGenerator::global()->bounded(ids.size())];
            m_PendingMinigameFrames = 60 * 5;
            for (const auto *mg = gcn_minigames; mg->name; mg++)
              if (mg->minigame_id == m_PendingMinigameId)
                printf("GCN minigame: %s (0x%02X)\n", mg->name, m_PendingMinigameId);
          }

          m_Stack->setCurrentIndex(0);
          m_SwitchButton->setText("Switch to B");
          writing = 30;
        }
      }
    }
  }, Qt::DirectConnection);

  resize(640, 510);
}

void MainWindow::switchCore()
{
  int next = (m_Stack->currentIndex() + 1) % m_Stack->count();
  m_Stack->setCurrentIndex(next);
  m_SwitchButton->setText(next == 0 ? "Switch to B" : "Switch to A");
}

MainWindow::~MainWindow()
{
  delete m_RetroA;
  delete m_RetroB;
}
