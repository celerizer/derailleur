#include "MarioTennis.h"

#include <QFile>
#include <QRetro.h>

static const dr_value_t MT_CHARACTER[4] = {
  { 0x80065214, DR_VALUE_TYPE_U32 },
  { 0x80065218, DR_VALUE_TYPE_U32 },
  { 0x8006521C, DR_VALUE_TYPE_U32 },
  { 0x80065220, DR_VALUE_TYPE_U32 }
};

// u32 bool: 1 = using alternate color
static const dr_value_t MT_COLOR[4] = {
  { 0x80065224, DR_VALUE_TYPE_U32 },
  { 0x80065228, DR_VALUE_TYPE_U32 },
  { 0x8006522C, DR_VALUE_TYPE_U32 },
  { 0x80065230, DR_VALUE_TYPE_U32 }
};

// Court slots: [0]=team0/singles1, [1]=team1/singles2, [2]=team0 partner, [3]=team1 partner
// Value = player index (0=P1 .. 3=P4)
static const dr_value_t MT_SLOT[4] = {
  { 0x80065234, DR_VALUE_TYPE_U8 },
  { 0x80065235, DR_VALUE_TYPE_U8 },
  { 0x80065236, DR_VALUE_TYPE_U8 },
  { 0x80065237, DR_VALUE_TYPE_U8 }
};

// u8: 0x00-0x03 human port, 0xFF bot
static const dr_value_t MT_CONTROL[4] = {
  { 0x80065238, DR_VALUE_TYPE_U8 },
  { 0x80065239, DR_VALUE_TYPE_U8 },
  { 0x8006523A, DR_VALUE_TYPE_U8 },
  { 0x8006523B, DR_VALUE_TYPE_U8 }
};

static const dr_value_t MT_DIFFICULTY[4] = {
  { 0x8006523C, DR_VALUE_TYPE_U8 },
  { 0x8006523D, DR_VALUE_TYPE_U8 },
  { 0x8006523E, DR_VALUE_TYPE_U8 },
  { 0x8006523F, DR_VALUE_TYPE_U8 }
};

// sets won: index 0 = team_id 0 side, index 1 = team_id 1 side
static const dr_value_t MT_SETS_WON[2] = {
  { 0x8015344F, DR_VALUE_TYPE_U8 },
  { 0x80153450, DR_VALUE_TYPE_U8 }
};
static const size_t MT_GAMES_WON_ADDR[2] = { 0x8015344D, 0x8015344E };
static const size_t MT_POINTS_ADDR[2] = { 0x8015344A, 0x8015344B };

static const dr_value_t MT_COURT = { 0x80065240, DR_VALUE_TYPE_U8 }; // u8: court (0x00-0x0F random, 0x10 bowser)
static const dr_value_t MT_SETS = { 0x80065243, DR_VALUE_TYPE_U8 };  // u8: number of sets
static const dr_value_t MT_GAMES = { 0x80065244, DR_VALUE_TYPE_U8 }; // u8: number of games
static const dr_value_t MT_GAME_TYPE = { 0x80065248, DR_VALUE_TYPE_U32 }; // u32: 00=tournament 01=piranha 03=exhibition 04=tb5 05=ringshot 06=bowser 07=tb7
static const dr_value_t MT_DOUBLES = { 0x8006524F, DR_VALUE_TYPE_U8 }; // u8: 1 = doubles, 0 = singles

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
  { 0x03, 0x00 }, // DR_CHARACTER_KOOPA_KID (Bowser)
  { 0x03, 0x00 }, // DR_CHARACTER_KOOPA_KID_R (Bowser)
  { 0x03, 0x00 }, // DR_CHARACTER_KOOPA_KID_G (Bowser)
  { 0x03, 0x01 }, // DR_CHARACTER_KOOPA_KID_B (Bowser, alternate colour)
  { 0x13, 0x01 }, // DR_CHARACTER_TOADETTE
  { 0x0C, 0x00 }, // DR_CHARACTER_BIRDO
  { 0xFF, 0x00 }, // DR_CHARACTER_DRY_BONES
};

static const dr_mp_minigame_t MT_MINIGAMES[] = {
  { "Tennis: Exhibition", DR_MINIGAME_2V2, 0x03, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tennis: Bowser Stage", DR_MINIGAME_2V2, 0x06, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tennis: Tiebreaker", DR_MINIGAME_DUEL, 0x07, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

MarioTennis::MarioTennis(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  m_retro->init(coreId(), rom());
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

  /* If every player is a CPU, force controller 1 to press A so the game can proceed */
  bool allCpu = true;
  for (unsigned i = 0; i < 4; i++)
    if (dr_team_type_participates(m_players[i].team_type) &&
        m_players[i].control_type == DR_CONTROL_TYPE_HUMAN)
    {
      allCpu = false;
      break;
    }

  if (allCpu)
  {
    m_allCpuFrames++;
    // Hold A for a few frames, then release so it reads as a press.
    if (m_allCpuFrames == 600)
    {
      log(DR_LOG_INFO, "all players CPU: forcing controller 1 A press to advance");
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
    }
    else if (m_allCpuFrames == 608)
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);
  }

  for (unsigned team = 0; team < 2; team++)
  {
    int64_t setsWon = 0;
    if (m_retro->readValue(&setsWon, MT_SETS_WON[team]) != DR_OK
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

    m_finishCountdown = 5 * 60;
    break;
  }
}

const dr_mp_minigame_t *MarioTennis::minigames() const
{
  return MT_MINIGAMES;
}

static uint8_t mtDifficulty(dr_difficulty difficulty)
{
  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0x00;
  case DR_DIFFICULTY_NORMAL:
    return 0x01;
  case DR_DIFFICULTY_HARD:
    return 0x02;
  case DR_DIFFICULTY_VERY_HARD:
    return 0x03;
  default:
    return 0x01;
  }
}

void MarioTennis::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;

  m_winners = 0;
  m_finishCountdown = 0;
  m_allCpuFrames = 0;
  loadState(state());
  bool doubles = (minigame->type == DR_MINIGAME_2V2);
  unsigned long rc = dr_rand_count();
  uint8_t court = (minigame->minigame_id == 0x06) ? 0x10 : (uint8_t)(dr_rand() % 16);
  log(DR_LOG_INFO, qPrintable(QString("MT court=%1 id=0x%2 randcount=%3")
                       .arg(court).arg(minigame->minigame_id, 2, 16, QChar('0')).arg(rc)));
  m_retro->writeValue(court, MT_COURT);
  m_retro->writeValue(0x01, MT_SETS);
  m_retro->writeValue(0x01, MT_GAMES);
  m_retro->writeValue(minigame->minigame_id, MT_GAME_TYPE);
  m_retro->writeValue(doubles ? 0x01 : 0x00, MT_DOUBLES);

  /* Record all players and write each one's character/difficulty. */
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = data.players[i];
    if (p.character < DR_CHARACTER_SIZE && MT_DR_TO_CHAR[p.character].id != 0xFF)
    {
      m_retro->writeValue(MT_DR_TO_CHAR[p.character].id, MT_CHARACTER[i]);
      m_retro->writeValue(MT_DR_TO_CHAR[p.character].color, MT_COLOR[i]);
    }
    m_retro->writeValue(mtDifficulty(p.difficulty), MT_DIFFICULTY[i]);
  }
  applyTeams();

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
    m_retro->writeValue(static_cast<uint8_t>(pi), MT_SLOT[s]);
    uint8_t ctrl = (m_players[pi].control_type == DR_CONTROL_TYPE_CPU)
                     ? 0xFF
                     : static_cast<uint8_t>(m_players[pi].control_port - DR_CONTROL_PORT_P1);
    m_retro->writeValue(ctrl, MT_CONTROL[s]);
  }
}
