#include "MarioPartyE.h"

#include <QFile>
#include <QRetro.h>

static const dr_mp_minigame_t MPE_MINIGAMES[] = {
  { "Cast Away Mario!", DR_MINIGAME_INVALID, 1, 0xFF, DR_NO_QUIRKS }, // 1p
  { "Mario's Mallet", DR_MINIGAME_INVALID, 2, 0xFF, DR_NO_QUIRKS }, // 1p
  { "Daisy's Rodeo!", DR_MINIGAME_INVALID, 3, 0xFF, DR_NO_QUIRKS }, // 1p
  { "Lakitu's Luck", DR_MINIGAME_INVALID, 4, 0xFF, DR_NO_QUIRKS }, // board
  { "Spinister Bowser", DR_MINIGAME_INVALID, 5, 0xFF, DR_NO_QUIRKS }, // board
  { "Fast Feed Yoshi!", DR_MINIGAME_INVALID, 6, 0xFF, DR_NO_QUIRKS }, // 1p
  { "Bolt from Boo", DR_MINIGAME_INVALID, 7, 0xFF, DR_NO_QUIRKS }, // 2p
  { "Balloon Burst!", DR_MINIGAME_INVALID, 8, 0xFF, DR_NO_QUIRKS }, // 2p
  { "Time Bomb Ticks!", DR_MINIGAME_INVALID, 9, 0xFF, DR_NO_QUIRKS }, // 2p
  { "Waluigi's Reign", DR_MINIGAME_INVALID, 10, 0xFF, DR_NO_QUIRKS }, // 2p
  { "Wario's Bluff", DR_MINIGAME_INVALID, 11, 0xFF, DR_NO_QUIRKS }, // 2p

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

MarioPartyE::MarioPartyE(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);
  m_gamePath = (dr_roms_directory() + "/e-Reader (USA).gba").toStdString();
  QRetro *c = new QRetro();
  if (!c->loadCore(dr_core_path(DR_CORE_MGBA).toUtf8().constData()))
  {
    log(DR_LOG_ERROR, "failed to load core: mgba_libretro.so");
    m_valid = false;
  }
  /* Content is loaded lazily on the first launch (see DrGuest::applyGameData). */
  if (!QFile::exists(QString::fromStdString(m_gamePath)))
  {
    log(DR_LOG_ERROR, "rom not found: e-Reader (USA).gba");
    m_valid = false;
  }
  m_retro->setCore(c, true);
}

void MarioPartyE::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioPartyE::runTimeBombTicks()
{
  static const size_t TBT_STATE = 0x02001A57; // u8, on 05 mini is finished
  [[maybe_unused]] static const size_t TBT_TICK_TYPE = 0x02001A59; // u8, 0=none, 1=up, 2=down
  [[maybe_unused]] static const size_t TBT_TIME = 0x02001A5A; // u16
  static const size_t TBT_PLAYER = 0x020100A5; // bool, 0=p1, 1=p2
  static const size_t TBT_P1_WON = 0x020100A6; // bool

  uint8_t state = 0;
  if (m_retro->readu8(&state, TBT_STATE) != DR_OK)
    return;

  if (state == 0x05)
  {
    uint8_t p1Won = 0;
    m_retro->readu8(&p1Won, TBT_P1_WON);
    m_winners |= p1Won ? (1u << 0) : (1u << 1);
    finishMinigame();
    return;
  }

  if (auto *c = core())
  {
    uint8_t player = 0;
    m_retro->readu8(&player, TBT_PLAYER);
    auto &p1 = c->input()->joypads()[0];
    auto &p2 = c->input()->joypads()[1];
    for (unsigned i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
      p1.setForcedButton(i, player ? p2.digitalButton(i) : false);
  }
}

void MarioPartyE::run()
{
  if (!m_minigame || !m_minigameActive)
    return;

  switch (m_minigame->minigame_id)
  {
  case 9:
    runTimeBombTicks();
    break;
  }
}

const dr_mp_minigame_t *MarioPartyE::minigames() const
{
  return MPE_MINIGAMES;
}

void MarioPartyE::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;

  m_winners = 0;
  QString stateFile = dr_state_directory()
    + QString("/mpe/mpe-%1.state.zip").arg(minigame->minigame_id, 2, 10, QChar('0'));
  core()->unserializeFromFile(stateFile);
  log(DR_LOG_INFO, qPrintable(QString("loading %1 from %2").arg(minigame->name).arg(stateFile)));
  startMinigame();
}

dr_minigame_result_t MarioPartyE::minigameResult(unsigned index)
{
  return { (m_winners & (1u << index)) ? 10 : 0, 0 };
}
