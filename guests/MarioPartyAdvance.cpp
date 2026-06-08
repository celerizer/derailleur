#include "MarioPartyAdvance.h"

#include <QRetro.h>
#include <QRetroDirectories.h>

#define MPA_MINIGAME_ACTIVE_ADDR 0x0300440C
#define MPA_MINIGAME_ID_ADDR 0x0300440D
#define MPA_PLAYER_1_ADDR 0x0300440E
#define MPA_PLAYER_2_ADDR 0x0300440F

static const dr_mp_minigame_t MPA_MINIGAMES[] =
{
  { "Compat-I-Com", DR_MINIGAME_INVALID, 0x04, -1, DR_NO_QUIRKS },
  { "Shroom Slide", DR_MINIGAME_INVALID, 0x07, -1, DR_NO_QUIRKS },
  { "Stick to It",  DR_MINIGAME_INVALID, 0x08, -1, DR_NO_QUIRKS },
  { "Shroom Drop",  DR_MINIGAME_INVALID, 0x14, -1, DR_NO_QUIRKS },
  { "4-P Piball",   DR_MINIGAME_INVALID, 0x16, -1, DR_NO_QUIRKS },
  { "Attack Frog",  DR_MINIGAME_INVALID, 0x1A, -1, DR_NO_QUIRKS },
  { "Bomb Game",    DR_MINIGAME_INVALID, 0x1E, -1, DR_NO_QUIRKS },
  { "Egg Panic",    DR_MINIGAME_INVALID, 0x22, -1, DR_NO_QUIRKS },
  { "Block Punch",  DR_MINIGAME_INVALID, 0x23, -1, DR_NO_QUIRKS },
  // toad force v
  { "Dart Attack",  DR_MINIGAME_INVALID, 0x25, -1, DR_NO_QUIRKS },
  { "Chicken Race", DR_MINIGAME_INVALID, 0x27, -1, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

MarioPartyAdvance::MarioPartyAdvance(QObject *parent)
  : DrGuest(parent)
{
  m_core = new QRetro();
  m_ownCore = true;
  if (!m_core->loadCore((dr_cores_directory() + "/mgba_libretro.so").toStdString().c_str()))
  {
    log(DR_LOG_ERROR, "failed to load core: mgba_libretro.so");
    m_valid = false;
  }
  if (!m_core->loadContent(
        (dr_roms_directory() + "/Mario Party Advance (USA).gba").toStdString().c_str()))
  {
    log(DR_LOG_ERROR, "failed to load content: Mario Party Advance (USA).gba");
    m_valid = false;
  }
}

const dr_mp_minigame_t *MarioPartyAdvance::minigames() const
{
  return MPA_MINIGAMES;
}

dr_minigame_result_t MarioPartyAdvance::minigameResult(unsigned index)
{
  (void)index;
  return {};
}

void MarioPartyAdvance::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  (void)minigame;
}

dr_error MarioPartyAdvance::doSetPlayerCharacter(unsigned index, dr_character character)
{
  if (index < 2)
  {
    size_t address = index ? MPA_PLAYER_2_ADDR : MPA_PLAYER_1_ADDR;
    switch (character)
    {
    case DR_CHARACTER_MARIO:
      writeu8(0, address);
      break;
    case DR_CHARACTER_LUIGI:
      writeu8(1, address);
      break;
    case DR_CHARACTER_PEACH:
      writeu8(2, address);
      break;
    case DR_CHARACTER_YOSHI:
      writeu8(3, address);
      break;
    default:
      writeu8(index, address);
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
