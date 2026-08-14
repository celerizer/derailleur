#include "MarioGolf.h"

#include <QRetro.h>
#include <QString>

// u32 per-player selected/menu character.
static const size_t MG_MENU_CHARACTER_ADDR[4] = { 0x8012F480, 0x8012F484, 0x8012F488, 0x8012F48C };

// u8 per-player (true, in-game) character (raw per-player addresses).
static const size_t MG_CHARACTER_ADDR[4] = { 0x801B71ED, 0x801B72A5, 0x801B735D, 0x801B7415 };

// u8 per-player color (raw per-player addresses, 0xB8 stride).
static const size_t MG_COLOR_ADDR[4] = { 0x801B724B, 0x801B7303, 0x801B73BB, 0x801B7473 };

// u8 bool per-player: is this golfer a bot? (0xB8-strided from P1).
static const size_t MG_BOT_ADDR[4] = { 0x801B71EE, 0x801B72A6, 0x801B735E, 0x801B7416 };

// u32 course id (see mario_golf_course) and u32 current hole.
static const size_t MG_COURSE_ADDR = 0x801B6094;
static const size_t MG_HOLE_ADDR = 0x801B6098;

// u32 game state. The intro/loading screen covers everything up to MG_STATE_FINISHED,
// which is when the actual mini-golf gameplay becomes visible.
static const size_t MG_STATE_ADDR = 0x801B6088;
typedef enum
{
  MG_STATE_INTRO = 3,
  MG_STATE_INTRO2 = 4,
  MG_STATE_PLAYING = 6,
  MG_STATE_FINISHED = 7,
} mg_state;

// u32 whose turn it is (0-3). Drives netplay golf mode so the active golfer has 0
// input latency (priority) while everyone else waits.
static const size_t MG_TURN_ADDR = 0x800FBE74;

// u8 per-player controller port (0-3), 0xB8-strided. The block defaults to 0 (all
// golfers read controller 0), so we write each golfer its own port for local + netplay.
static const size_t MG_PLAYER_PORT_ADDR[4] = { 0x801B71F7, 0x801B72AF, 0x801B7367, 0x801B741F };

// Per-player hole state, 0xB8-strided (P1 0x801B71F0, P3 0x801B7360, P4 0x801B7418).
static const size_t MG_SHOTS_ADDR[4] = { 0x801B71F0, 0x801B72A8, 0x801B7360, 0x801B7418 }; // u32 strokes
static const size_t MG_SUNK_ADDR[4]  = { 0x801B71F6, 0x801B72AE, 0x801B7366, 0x801B741E }; // u8 ball sunk

// u8 par for the current hole.
static const size_t MG_PAR_ADDR = 0x800BAA04;

typedef enum
{
  MG_CHARACTER_PLUM = 0,
  MG_CHARACTER_CHARLIE = 1,
  MG_CHARACTER_PEACH = 5,
  MG_CHARACTER_BABY_MARIO = 3,
  MG_CHARACTER_LUIGI = 4,
  MG_CHARACTER_YOSHI = 10,
  MG_CHARACTER_SONNY = 6,
  MG_CHARACTER_WARIO = 7,
  MG_CHARACTER_HARRY = 8,
  MG_CHARACTER_MARIO = 9,
  MG_CHARACTER_MAPLE = 2,
  MG_CHARACTER_DONKEY_KONG = 11,
  MG_CHARACTER_BOWSER = 12,
  MG_CHARACTER_METAL_MARIO = 13,
  MG_CHARACTER_KID = 14,
  MG_CHARACTER_SHERRY = 15,
  MG_CHARACTER_AZALEA = 16,
  MG_CHARACTER_JOE = 17
} mg_character;

typedef struct
{
  mg_character character;
  uint8_t color;
} mg_char_pick;

static mg_char_pick mgCharFor(dr_character character)
{
  switch (character)
  {
  case DR_CHARACTER_MARIO:       return { MG_CHARACTER_MARIO, 0 };
  case DR_CHARACTER_LUIGI:       return { MG_CHARACTER_LUIGI, 0 };
  case DR_CHARACTER_PEACH:       return { MG_CHARACTER_PEACH, 0 };
  case DR_CHARACTER_YOSHI:       return { MG_CHARACTER_YOSHI, 0 };
  case DR_CHARACTER_WARIO:       return { MG_CHARACTER_WARIO, 0 };
  case DR_CHARACTER_DONKEY_KONG: return { MG_CHARACTER_DONKEY_KONG, 0 };
  case DR_CHARACTER_DAISY:       return { MG_CHARACTER_AZALEA, 0 };
  case DR_CHARACTER_WALUIGI:     return { MG_CHARACTER_HARRY, 0 };
  default:                       return { MG_CHARACTER_MARIO, 0 };
  }
}

typedef enum
{
  MG_COURSE_MINIGOLF_LUIGIS_GARDEN = 6,
  MG_COURSE_MINIGOLF_PEACHS_CASTLE = 9,
} mg_course;

MarioGolf::MarioGolf(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  m_retro->init(coreId(), rom());

  /* One minigame per hole: minigame_id = course, scene_id = hole (0-17). */
  static const struct { signed id; const char *name; } courses[MG_COURSE_COUNT] = {
    { MG_COURSE_MINIGOLF_LUIGIS_GARDEN, "Luigi's Garden" },
    { MG_COURSE_MINIGOLF_PEACHS_CASTLE, "Peach's Castle" },
  };

  int n = 0;
  for (int c = 0; c < MG_COURSE_COUNT; c++)
    for (int hole = 0; hole < MG_HOLE_COUNT; hole++, n++)
    {
      const QByteArray label =
        QString("Golf: %1 %2").arg(courses[c].name).arg(hole + 1).toUtf8();
      qstrncpy(m_minigameNames[n], label.constData(), sizeof(m_minigameNames[n]));
      m_minigames[n] = { m_minigameNames[n], DR_MINIGAME_4P, courses[c].id, hole, DR_NO_QUIRKS };
    }
  m_minigames[n] = { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS };
}

const dr_mp_minigame_t *MarioGolf::minigames() const
{
  return m_minigames;
}

void MarioGolf::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_lastTurn = -1;
  m_winners = 0;
  m_finishPending = false;
  for (unsigned i = 0; i < 4; i++)
  {
    m_slotToIndex[i] = -1;
    m_prevShots[i] = 0;
    m_prevSunk[i] = false;
    m_holeStrokes[i] = -1;
  }

  loadState(state());

  /* Each minigame encodes its course (minigame_id) and hole (scene_id). The hole address
   * is 0-based one lower than the hole number, so the first hole writes -1. */
  if (data.minigame)
  {
    m_retro->writeu32(static_cast<uint32_t>(data.minigame->minigame_id), MG_COURSE_ADDR);
    m_retro->writes32(data.minigame->scene_id - 1, MG_HOLE_ADDR);
  }

  /* Each board player's golfer + color go into the in-game slot matching their
   * controller port. m_slotToIndex maps back for scoring. */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(data.players[i], i);
    if (slot >= 4)
      continue;
    m_slotToIndex[slot] = static_cast<int>(i);

    const mg_char_pick pick = mgCharFor(data.players[i].character);
    m_retro->writeu32(pick.character, MG_MENU_CHARACTER_ADDR[slot]);
    m_retro->writeu8(pick.character, MG_CHARACTER_ADDR[slot]);
    m_retro->writeu8(pick.color, MG_COLOR_ADDR[slot]);

    /* Assign the golfer in this slot to its own controller port. The block defaults to
     * 0 (all golfers -> controller 0); hold our value through init with writeForFrames. */
    const uint8_t port = static_cast<uint8_t>(slot);
    m_retro->writeForFrames(MG_PLAYER_PORT_ADDR[slot], &port, 1, 300);

    /* Mark CPU-controlled golfers as bots. */
    const uint8_t bot = (data.players[i].control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
    m_retro->writeForFrames(MG_BOT_ADDR[slot], &bot, 1, 300);
  }

  /* Don't reveal yet. Wait 60 frames for the loaded state to settle, force an A press
   * on P1 to advance past the intro, then reveal (startMinigame) once the game state
   * reaches MG_STATE_FINISHED -- see run(). */
  m_startupDelay = 120;
  m_aReleaseDelay = 0;
}

void MarioGolf::run()
{
  m_retro->tickFrameWrites();

  /* Release the forced P1 A press a few frames after it was applied. */
  if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);

  /* Hidden startup: run behind the loading screen until the game is fully in the
   * mini-golf state, then reveal via startMinigame(). */
  if (!m_minigameActive)
  {
    if (m_startupDelay > 0)
    {
      if (--m_startupDelay == 0)
      {
        /* Force an A press on P1 to advance past the intro. */
        core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
        m_aReleaseDelay = 8;
      }
      return;
    }

    uint32_t gameState = 0;
    if (m_retro->readu32(&gameState, MG_STATE_ADDR) == DR_OK &&
        gameState == MG_STATE_INTRO)
      startMinigame();
    return;
  }

  m_minigameFrames++;

  /* Give whoever's turn it is 0 input latency (priority) via netplay golf mode; the
   * value is deterministic game state, so every peer toggles it on the same frame.
   * @todo if per-player slot remapping is added, translate turn (in-game slot) to the
   * board/peer index before passing it. */
  uint32_t turn = 0;
  if (m_retro->readu32(&turn, MG_TURN_ADDR) == DR_OK && turn < 4 &&
      static_cast<int>(turn) != m_lastTurn)
  {
    m_lastTurn = static_cast<int>(turn);
    /* The turn value is an in-game slot; golf mode wants the board/peer index. */
    const int authority = m_slotToIndex[turn];
    if (authority >= 0)
      setGolfMode(authority);
  }

  if (m_finishPending)
    return;

  /* Read the hole state: strokes taken and whether each active player has sunk it. */
  uint8_t par = 0;
  if (m_retro->readu8(&par, MG_PAR_ADDR) != DR_OK)
    return;

  if (static_cast<int>(par) != m_prevPar)
  {
    m_prevPar = static_cast<int>(par);
    log(DR_LOG_INFO, qPrintable(QString("Mario Golf: par is now %1").arg(par)));
  }

  uint32_t shots[4] = {};
  bool sunk[4] = {};
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0)
      continue;
    m_retro->readu32(&shots[slot], MG_SHOTS_ADDR[slot]);
    uint8_t s = 0;
    m_retro->readu8(&s, MG_SUNK_ADDR[slot]);
    sunk[slot] = (s != 0);

    if (shots[slot] > m_prevShots[slot])
      log(DR_LOG_INFO, qPrintable(QString("Mario Golf: player %1 stroke %2")
        .arg(m_slotToIndex[slot]).arg(shots[slot])));
    if (sunk[slot] && !m_prevSunk[slot])
    {
      log(DR_LOG_INFO, qPrintable(QString("Mario Golf: player %1 sunk it in %2 strokes")
        .arg(m_slotToIndex[slot]).arg(shots[slot])));
      /* Latch the hole-out stroke count. The live sunk flag is transient (and the game
       * clears it when it settles/advances the hole), so the decision below must run off
       * this latch, not the instantaneous read. */
      if (m_holeStrokes[slot] < 0)
        m_holeStrokes[slot] = static_cast<int>(shots[slot]);
    }

    /* Mercy rule: if their par-stroke didn't sink, max them out (9) so the game gives up
     * on the hole and moves on instead of letting them keep putting. */
    if (!sunk[slot] && shots[slot] == par)
    {
      m_retro->writeu32(9, MG_SHOTS_ADDR[slot]);
      shots[slot] = 9;
      log(DR_LOG_INFO, qPrintable(QString("Mario Golf: player %1 mercy-ruled (missed par)")
        .arg(m_slotToIndex[slot])));
    }

    m_prevShots[slot] = shots[slot];
    m_prevSunk[slot] = sunk[slot];
  }

  /* A sink only counts if it is par or better; find the lowest such latched hole-out. */
  bool anyValidSink = false;
  uint32_t best = 0;
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0 || m_holeStrokes[slot] < 0 ||
        static_cast<uint32_t>(m_holeStrokes[slot]) > par)
      continue;
    if (!anyValidSink || static_cast<uint32_t>(m_holeStrokes[slot]) < best)
    {
      best = static_cast<uint32_t>(m_holeStrokes[slot]);
      anyValidSink = true;
    }
  }

  /* A slot is still "unresolved" if it could still change the outcome. Once no slot is
   * unresolved, the hole is decided. A slot that has holed out (latched) is resolved. */
  bool finish = true;
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0 || m_holeStrokes[slot] >= 0)
      continue; // inactive, or already holed out -> resolved

    /* Not yet holed out: can it still matter? shots[] counts strokes already taken, so a
     * player can still reach a target of N only while shots < N (their next stroke would
     * be shots+1). With a par-or-better winner up, the target is `best`; otherwise it's
     * par. shots == target means they've already used those strokes without sinking. */
    const uint32_t threshold = anyValidSink ? best : par;
    if (shots[slot] < threshold)
    {
      finish = false;
      break;
    }
  }

  if (finish)
  {
    /* Winners are the par-or-better hole-outs tied at `best` (none if it washed out). */
    m_winners = 0;
    if (anyValidSink)
      for (unsigned slot = 0; slot < 4; slot++)
        if (m_slotToIndex[slot] >= 0 && m_holeStrokes[slot] >= 0 &&
            static_cast<uint32_t>(m_holeStrokes[slot]) == best)
          m_winners |= (1u << m_slotToIndex[slot]);
  }

  if (finish)
  {
    log(DR_LOG_INFO, qPrintable(QString("Mario Golf: hole decided (par %1, anyValidSink %2, "
      "best %3, winners 0x%4) -- finishing")
      .arg(par).arg(anyValidSink ? 1 : 0).arg(best).arg(m_winners, 0, 16)));
    m_finishPending = true;
    setGolfMode(-1); // done -- don't leave golf priority set on the board
    finishMinigameInFrames(240);
  }
}

dr_minigame_result_t MarioGolf::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4 && (m_winners & (1u << index)))
    result.coins = 10;
  return result;
}
