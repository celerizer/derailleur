#include "MarioTennis.h"

#include <QRetro.h>

static const size_t MT_CHARACTER_ADDR[4] = {
  0x80065214,
  0x80065218,
  0x8006521C,
  0x80065220,
};

// u32 bool: 1 = using alternate color
static const size_t MT_COLOR_ADDR[4] = {
  0x80065224,
  0x80065228,
  0x8006522C,
  0x80065230,
};

// Court slots: [0]=team0/singles1, [1]=team1/singles2, [2]=team0 partner, [3]=team1 partner
// Value = player index (0=P1 .. 3=P4)
static const size_t MT_SLOT_ADDR[4] = {
  0x80065234,
  0x80065235,
  0x80065236,
  0x80065237,
};

// u8: 0x00-0x03 human port, 0xFF bot
static const size_t MT_CONTROL_ADDR[4] = {
  0x80065238,
  0x80065239,
  0x8006523A,
  0x8006523B,
};

static const size_t MT_DIFFICULTY_ADDR[4] = {
  0x8006523C,
  0x8006523D,
  0x8006523E,
  0x8006523F,
};

// sets won: index 0 = team_id 0 side, index 1 = team_id 1 side
static const size_t MT_SETS_WON_ADDR[2] = { 0x8015344F, 0x80153450 };

static const size_t MT_COURT_ADDR = 0x80065240; // u8: court (0x00-0x0F random, 0x10 bowser)
static const size_t MT_SETS_ADDR = 0x80065243;  // u8: number of sets
static const size_t MT_GAMES_ADDR = 0x80065244; // u8: number of games
static const size_t MT_GAME_TYPE_ADDR =
  0x80065248; // u32: 00=tournament 01=piranha 03=exhibition 04=tb5 05=ringshot 06=bowser 07=tb7
static const size_t MT_DOUBLES_ADDR = 0x8006524F; // u8: 1 = doubles, 0 = singles

static const dr_character MT_CHAR_TO_DR[] = {
  DR_CHARACTER_YOSHI, // 0x00
  DR_CHARACTER_PEACH, // 0x01
  DR_CHARACTER_MARIO, // 0x02
  DR_CHARACTER_INVALID, // 0x03 Bowser
  DR_CHARACTER_BOO, // 0x04
  DR_CHARACTER_DONKEY_KONG, // 0x05
  DR_CHARACTER_INVALID, // 0x06 Baby Mario
  DR_CHARACTER_TOAD, // 0x07
  DR_CHARACTER_WALUIGI, // 0x08
  DR_CHARACTER_WARIO, // 0x09
  DR_CHARACTER_LUIGI, // 0x0A
  DR_CHARACTER_DAISY, // 0x0B
  DR_CHARACTER_BIRDO, // 0x0C
  DR_CHARACTER_INVALID, // 0x0D Shy Guy
  DR_CHARACTER_INVALID, // 0x0E DK Jr.
  DR_CHARACTER_INVALID, // 0x0F Paratroopa
  DR_CHARACTER_INVALID, // 0x10 Alex
  DR_CHARACTER_INVALID, // 0x11 Harry
  DR_CHARACTER_INVALID, // 0x12 Kate
  DR_CHARACTER_INVALID, // 0x13 Nina
  DR_CHARACTER_INVALID, // 0x14 used for an empty character
};

struct mt_character_t
{
  uint32_t id;
  uint32_t color;
};

static const mt_character_t MT_DR_TO_CHAR[DR_CHARACTER_SIZE] = {
  { 0x14, 0x00 }, // DR_CHARACTER_INVALID
  { 0x02, 0x00 }, // DR_CHARACTER_MARIO
  { 0x0A, 0x00 }, // DR_CHARACTER_LUIGI
  { 0x01, 0x00 }, // DR_CHARACTER_PEACH
  { 0x00, 0x00 }, // DR_CHARACTER_YOSHI
  { 0x09, 0x00 }, // DR_CHARACTER_WARIO
  { 0x05, 0x00 }, // DR_CHARACTER_DONKEY_KONG
  { 0x08, 0x00 }, // DR_CHARACTER_WALUIGI
  { 0x0B, 0x00 }, // DR_CHARACTER_DAISY
  { 0x07, 0x00 }, // DR_CHARACTER_TOAD
  { 0x04, 0x00 }, // DR_CHARACTER_BOO
  { 0x0F, 0x01 }, // DR_CHARACTER_KOOPA_KID (Paratroopa, Green)
  { 0x13, 0x01 }, // DR_CHARACTER_TOADETTE
  { 0x0C, 0x00 }, // DR_CHARACTER_BIRDO
  { 0xFF, 0x00 }, // DR_CHARACTER_DRY_BONES
};

static const dr_mp_minigame_t MT_MINIGAMES[] = {
  { "Tennis: Exhibition", DR_MINIGAME_2V2, 0x03, 0xFF, DR_NO_QUIRKS },
  { "Tennis: Bowser Stage", DR_MINIGAME_2V2, 0x06, 0xFF, DR_NO_QUIRKS },
  { "Tennis: Tiebreaker", DR_MINIGAME_DUEL, 0x07, 0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

MarioTennis::MarioTennis(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  QString gamePath = dr_roms_directory() + "/Mario Tennis (USA).z64";
  QRetro *c = new QRetro();
  if (!c->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!c->loadContent(gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(c, true);
  m_retro->applyN64Remaps();
}

void MarioTennis::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioTennis::run()
{
  if (!m_minigame || !m_minigameActive)
    return;

  if (m_finishCountdown > 0)
  {
    if (--m_finishCountdown == 0)
      finishMinigame();
    return;
  }

  for (unsigned team = 0; team < 2; team++)
  {
    uint8_t setsWon = 0;
    if (m_retro->readu8(&setsWon, MT_SETS_WON_ADDR[team]) != DR_OK
      || setsWon < 1)
      continue;

    QString winners;
    for (unsigned i = 0; i < 4; i++)
    {
      if (m_players[i].team_id == team)
      {
        m_winners |= (1u << i);
        if (!winners.isEmpty())
          winners += ", ";
        winners += dr_character_name(m_players[i].character);
      }
    }
    log(DR_LOG_INFO, qPrintable(QString("team %1 wins: %2").arg(team).arg(winners)));

    m_finishCountdown = 3 * 60;
    break;
  }
}

const dr_mp_minigame_t *MarioTennis::minigames() const
{
  return MT_MINIGAMES;
}

void MarioTennis::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  m_winners = 0;
  m_finishCountdown = 0;
  core()->unserializeFromFile(dr_state_directory() + "/mariotennis.state.zip");
  bool doubles = (minigame->type == DR_MINIGAME_2V2);
  uint8_t court = (minigame->minigame_id == 0x06) ? 0x10 : (uint8_t)(rand() % 16);
  m_retro->writeu8(court, MT_COURT_ADDR);
  m_retro->writeu8(0x01, MT_SETS_ADDR);
  m_retro->writeu8(0x01, MT_GAMES_ADDR);
  m_retro->writeu32(minigame->minigame_id, MT_GAME_TYPE_ADDR);
  m_retro->writeu8(doubles ? 0x01 : 0x00, MT_DOUBLES_ADDR);

  log(DR_LOG_INFO,
    qPrintable(
      QString("starting %1: court 0x%2").arg(minigame->name).arg(court, 2, 16, QChar('0'))));
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    log(DR_LOG_INFO, qPrintable(QString("  P%1: %2 | port %3 | team %4")
                         .arg(i + 1)
                         .arg(dr_character_name(p.character))
                         .arg(static_cast<int>(p.control_port - DR_CONTROL_PORT_P1) + 1)
                         .arg(p.team_id)));
  }

  startMinigame();
}

dr_minigame_result_t MarioTennis::minigameResult(unsigned index)
{
  return { (m_winners & (1u << index)) ? 10 : 0, 0 };
}

dr_error MarioTennis::doSetPlayerCharacter(unsigned index, dr_character character)
{
  if (character >= DR_CHARACTER_SIZE || MT_DR_TO_CHAR[character].id == 0xFF)
    return DR_ERR_INVALID_PARAMETER;
  m_players[index].character = character;
  m_retro->writeu32(MT_DR_TO_CHAR[character].id, MT_CHARACTER_ADDR[index]);
  m_retro->writeu32(MT_DR_TO_CHAR[character].color, MT_COLOR_ADDR[index]);
  return DR_OK;
}

dr_error MarioTennis::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  m_players[index].control_port = control_port;
  return DR_OK;
}

dr_error MarioTennis::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  m_players[index].control_type = control_type;
  return DR_OK;
}

dr_error MarioTennis::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  uint8_t val;
  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    val = 0x00;
    break;
  case DR_DIFFICULTY_NORMAL:
    val = 0x01;
    break;
  case DR_DIFFICULTY_HARD:
    val = 0x02;
    break;
  case DR_DIFFICULTY_VERY_HARD:
    val = 0x03;
    break;
  default:
    val = 0x01;
    break;
  }
  return m_retro->writeu8(val, MT_DIFFICULTY_ADDR[index]);
}

void MarioTennis::applyTeams()
{
  unsigned team[2][2] = {};
  unsigned count[2] = {};
  for (unsigned i = 0; i < 4; i++)
  {
    unsigned tid = m_players[i].team_id;
    if (tid < 2 && count[tid] < 2)
      team[tid][count[tid]++] = i;
  }

  // slots: [0]=team0/singles1, [1]=team1/singles2,
  //        [2]=team0 partner,   [3]=team1 partner
  unsigned slotCount = 0;
  unsigned slotPlayer[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

  if (count[0] == 2 && count[1] == 2)
  {
    slotPlayer[0] = team[0][0];
    slotPlayer[1] = team[1][0];
    slotPlayer[2] = team[0][1];
    slotPlayer[3] = team[1][1];
    slotCount = 4;
  }
  else if (count[0] >= 1 && count[1] >= 1)
  {
    slotPlayer[0] = team[0][0];
    slotPlayer[1] = team[1][0];
    slotCount = 2;
  }

  for (unsigned s = 0; s < slotCount; s++)
  {
    unsigned pi = slotPlayer[s];
    m_retro->writeu8(static_cast<uint8_t>(pi), MT_SLOT_ADDR[s]);
    uint8_t ctrl = (m_players[pi].control_type == DR_CONTROL_TYPE_CPU)
                     ? 0xFF
                     : static_cast<uint8_t>(m_players[pi].control_port - DR_CONTROL_PORT_P1);
    m_retro->writeu8(ctrl, MT_CONTROL_ADDR[s]);
  }
}

dr_error MarioTennis::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  m_players[index].team_color = color;
  m_players[index].team_type = type;
  m_players[index].team_id = team_id;
  applyTeams();
  return DR_OK;
}
