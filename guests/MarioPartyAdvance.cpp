#include "MarioPartyAdvance.h"

#include <QRetro.h>
#include <QRetroDirectories.h>

#define MPA_MINIGAME_ACTIVE_ADDR 0x0300440C
#define MPA_MINIGAME_ID_ADDR 0x0300440D
#define MPA_PLAYER_1_ADDR 0x0300440E
#define MPA_PLAYER_2_ADDR 0x0300440F


#define MPA_TYPE_1_WINDOW 1
#define MPA_TYPE_2_WINDOW 2
#define MPA_TYPE_4_WINDOW 3

enum
{
  MPA_COMPATICOM   = 0x04,
  MPA_SHROOM_SLIDE = 0x07,
  MPA_STICK_TO_IT  = 0x08,
  MPA_SHROOM_DROP  = 0x14,
  MPA_4P_PINBALL   = 0x16,
  MPA_ATTACK_FROG  = 0x1A,
  MPA_BOMB_GAME    = 0x1E,
  MPA_EGG_PANIC    = 0x22,
  MPA_BLOCK_PUNCH  = 0x23,
  MPA_DART_ATTACK  = 0x25,
  MPA_CHICKEN_RACE = 0x27,
  MPA_BOO_BYE      = 0x44,
};

static const dr_mp_minigame_t MPA_MINIGAMES[] =
{
  { "Compat-I-Com", DR_MINIGAME_INVALID, MPA_COMPATICOM,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Shroom Slide", DR_MINIGAME_INVALID, MPA_SHROOM_SLIDE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Stick to It",  DR_MINIGAME_INVALID, MPA_STICK_TO_IT,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Shroom Drop",  DR_MINIGAME_INVALID, MPA_SHROOM_DROP,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "4-P Piball",   DR_MINIGAME_INVALID, MPA_4P_PINBALL,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Attack Frog",  DR_MINIGAME_INVALID, MPA_ATTACK_FROG,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Bomb Game",    DR_MINIGAME_INVALID, MPA_BOMB_GAME,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Egg Panic",    DR_MINIGAME_INVALID, MPA_EGG_PANIC,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Block Punch",  DR_MINIGAME_INVALID, MPA_BLOCK_PUNCH,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  // 24 toad force v
  { "Dart Attack",  DR_MINIGAME_INVALID, MPA_DART_ATTACK,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Chicken Race", DR_MINIGAME_INVALID, MPA_CHICKEN_RACE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },

  { "Boo Bye", DR_MINIGAME_INVALID, MPA_BOO_BYE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

MarioPartyAdvance::MarioPartyAdvance(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);
  QRetro *c = new QRetro();
  if (!c->loadCore(dr_core_path(DR_CORE_MGBA).toStdString().c_str()))
  {
    log(DR_LOG_ERROR, "failed to load core: mgba_libretro.so");
    m_valid = false;
  }
  if (!c->loadContent(
        (dr_roms_directory() + "/Mario Party Advance (USA).gba").toStdString().c_str()))
  {
    log(DR_LOG_ERROR, "failed to load content: Mario Party Advance (USA).gba");
    m_valid = false;
  }
  m_retro->setCore(c, true);
}

void MarioPartyAdvance::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

const dr_mp_minigame_t *MarioPartyAdvance::minigames() const
{
  return MPA_MINIGAMES;
}

dr_minigame_result_t MarioPartyAdvance::minigameResult(unsigned index)
{
  return { (m_winners & (1u << index)) ? 10 : 0, 0 };
}

void MarioPartyAdvance::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;

  m_gameStarted = false;
  m_winners = 0;
  m_EndWaitFrames = 0;
  if (m_retro && m_retro->core())
  {
    QString statePath = dr_state_directory() + "/mpadvance.state.zip";
    bool ok = m_retro->core()->unserializeFromFile(statePath);
    log(DR_LOG_INFO, qPrintable(QString("unserializeFromFile(%1): %2").arg(statePath).arg(ok ? "ok" : "failed")));
  }
  if (m_retro)
    m_retro->writeu8((uint8_t)minigame->minigame_id, MPA_MINIGAME_ID_ADDR);

  /* Only the first two board players map onto Mario Party Advance's two slots. */
  for (unsigned i = 0; i < 2; i++)
  {
    size_t address = i ? MPA_PLAYER_2_ADDR : MPA_PLAYER_1_ADDR;

    switch (data.players[i].character)
    {
    case DR_CHARACTER_MARIO:
    case DR_CHARACTER_WARIO:
      m_retro->writeu8(0, address);
      break;
    case DR_CHARACTER_LUIGI:
    case DR_CHARACTER_WALUIGI:
      m_retro->writeu8(1, address);
      break;
    case DR_CHARACTER_PEACH:
    case DR_CHARACTER_DAISY:
      m_retro->writeu8(2, address);
      break;
    case DR_CHARACTER_YOSHI:
    case DR_CHARACTER_DONKEY_KONG:
      m_retro->writeu8(3, address);
      break;
    default:
      m_retro->writeu8(i, address);
    }
  }

  startMinigame();
}

void MarioPartyAdvance::run4pPinball()
{
  static const size_t MPA_4PP_PLAYER_OUT = 0x03006688;
  static const size_t MPA_4PP_PLAYER_OUT_COUNT = 0x0300668C;
  unsigned out_count = 0;

  if (m_retro->readu32(&out_count, MPA_4PP_PLAYER_OUT_COUNT) != DR_OK)
    return;

  if (out_count >= 3)
  {
    if (!m_winners)
    {
      for (unsigned i = 0; i < 4; i++)
      {
        uint8_t out = 0;
        m_retro->readu8(&out, MPA_4PP_PLAYER_OUT + i);
        if (!out)
          m_winners |= (1u << i);
      }
    }
    ++m_EndWaitFrames;
    if (m_EndWaitFrames > 248)
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, false);
    else if (m_EndWaitFrames >= 240)
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, true);
  }
}

void MarioPartyAdvance::run()
{
  if (!m_minigame || !m_minigameActive)
    return;

  uint8_t active = 0;
  if (!m_retro || m_retro->readu8(&active, MPA_MINIGAME_ACTIVE_ADDR) != DR_OK)
    return;

  if (!m_gameStarted)
  {
    if (active == 1)
      m_gameStarted = true;
  }
  else if (active == 0)
  {
    finishMinigame();
  }
  else
  {
    switch (m_minigame->minigame_id)
    {
    case MPA_4P_PINBALL:
      run4pPinball();
      break;
    }
  }
}
