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

static const dr_mp_minigame_t MPA_MINIGAMES[] =
{
  { "Compat-I-Com", DR_MINIGAME_INVALID, 0x04, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Shroom Slide", DR_MINIGAME_INVALID, 0x07, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Stick to It",  DR_MINIGAME_INVALID, 0x08, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Shroom Drop",  DR_MINIGAME_INVALID, 0x14, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "4-P Piball",   DR_MINIGAME_INVALID, 0x16, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Attack Frog",  DR_MINIGAME_INVALID, 0x1A, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Bomb Game",    DR_MINIGAME_INVALID, 0x1E, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Egg Panic",    DR_MINIGAME_INVALID, 0x22, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Block Punch",  DR_MINIGAME_INVALID, 0x23, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  // toad force v
  { "Dart Attack",  DR_MINIGAME_INVALID, 0x25, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },
  { "Chicken Race", DR_MINIGAME_INVALID, 0x27, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },

  { "Boo Bye", DR_MINIGAME_INVALID, 0x44, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

MarioPartyAdvance::MarioPartyAdvance(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);
  QRetro *c = new QRetro();
  if (!c->loadCore((dr_cores_directory() + "/mgba_libretro.so").toStdString().c_str()))
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
  return { (m_winners & (1u << index)) ? 10u : 0u, 0 };
}

void MarioPartyAdvance::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_gameStarted = false;
  m_winners = 0;
  if (m_retro && m_retro->core())
  {
    QString statePath = dr_state_directory() + "/mpadvance.state.zip";
    bool ok = m_retro->core()->unserializeFromFile(statePath);
    log(DR_LOG_INFO, qPrintable(QString("unserializeFromFile(%1): %2").arg(statePath).arg(ok ? "ok" : "failed")));
  }
  if (m_retro)
    m_retro->writeu8((uint8_t)minigame->minigame_id, MPA_MINIGAME_ID_ADDR);
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
    unsigned i;

    for (i = 0; i < 4; i++)
    {
      uint8_t out = 0;

      m_retro->readu8(&out, MPA_4PP_PLAYER_OUT + i);
      if (!out)
        m_winners |= (1u << i);
    }
    finishMinigameInFrames(240);
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
    case 0x16:
      run4pPinball();
      break;
    }
  }
}

dr_error MarioPartyAdvance::doSetPlayerCharacter(unsigned index, dr_character character)
{
  if (index < 2)
  {
    size_t address = index ? MPA_PLAYER_2_ADDR : MPA_PLAYER_1_ADDR;

    switch (character)
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
      m_retro->writeu8(index, address);
    }

    return DR_OK;
  }

  return DR_OK;
}

dr_error MarioPartyAdvance::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  (void)index; (void)control_port;
  return DR_OK;
}

dr_error MarioPartyAdvance::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  (void)index; (void)control_type;
  return DR_OK;
}

dr_error MarioPartyAdvance::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  (void)index; (void)difficulty;
  return DR_OK;
}

dr_error MarioPartyAdvance::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  (void)index; (void)color; (void)type; (void)team_id;
  return DR_OK;
}
