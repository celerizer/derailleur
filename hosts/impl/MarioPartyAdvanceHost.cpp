#include "MarioPartyAdvanceHost.h"

#include "asm/mpa.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>

#include <cstring>

/* Board context, the pointer the board loop allocates and runs from */
static const size_t MPA_BOARD_CTX_PTR = 0x030013C0;
#define MPA_BOARD_CTX_BLOCK_SIZE 0xE70 /* heap block size, at ctx - 8 */
#define MPA_CTX_MODE 0x00              /* u8, index into the handler table */
#define MPA_CTX_SUBSTATE 0x01          /* u8, the handler's own state */
#define MPA_CTX_WAIT 0x04              /* u16[2], cleared on a mode change */

#define MPA_MODE_CHALLENGE 18
#define MPA_MODE_CHALLENGE_RESULT 19
#define MPA_MODE_SPACE 20
#define MPA_MODE_SPACE_RESULT 21
#define MPA_HANDOFF 4 /* sub-state both modes are parked in by the hand-off hooks */

/* Roulette list, three indices into the ROM mini-game table, then the slot */
static const dr_value_t MPA_ROULETTE_SLOT = { 0x02034E1F, DR_VALUE_TYPE_U8 };
#define MPA_ROULETTE_SLOTS 3

/* Overlay parameter block result: nonzero clears the challenge */
static const dr_value_t MPA_MG_RESULT = { 0x02030003, DR_VALUE_TYPE_S8 };

/* Shroom City character, 0-3 */
static const dr_value_t MPA_CHARACTER = { 0x0300440E, DR_VALUE_TYPE_U8 };

#define MPA_TITLE_SLOT_SIZE 32
#define MPA_TITLE_CHARS 12    /* 8px monospace into a 96px slot */
#define MPA_ANNOUNCE_CHARS 20 /* fits Tumble's "...<name>!" on one line */

/* Frames the board may keep running after a request before it is resumed */
#define MPA_LAUNCH_TIMEOUT_FRAMES 300

#define MPA_WARMUP_FRAMES 120

static const dr_character MPA_CHARACTERS[4] =
{
  DR_CHARACTER_MARIO,
  DR_CHARACTER_LUIGI,
  DR_CHARACTER_PEACH,
  DR_CHARACTER_YOSHI,
};

/* Types a roulette slot can roll */
static const dr_minigame_type MPA_SLOT_TYPES[] =
{
  DR_MINIGAME_1P,
  DR_MINIGAME_4P,
  DR_MINIGAME_1V3,
  DR_MINIGAME_2V2,
  DR_MINIGAME_DUEL,
  DR_MINIGAME_BATTLE,
};

/* One character in the game's text encoding, or 0 if it has no glyph */
static uint8_t mpa_encode_char(QChar character)
{
  const ushort unit = character.unicode();

  if ((unit >= 'A' && unit <= 'Z') || (unit >= 'a' && unit <= 'z') || (unit >= '0' && unit <= '9'))
    return static_cast<uint8_t>(unit);

  switch (unit)
  {
  case ' ':
    return 0x10;
  case '-':
    return '=';
  case '\'':
    return '\\';
  case '/':
    return '_';
  case '!':
    return 0xC2;
  case '?':
    return 0xC3;
  case ',':
    return 0x90;
  case '.':
    return 0x85;
  default:
    return 0;
  }
}

MarioPartyAdvanceHost::MarioPartyAdvanceHost(QObject *parent)
  : DrHostRetro(parent)
{
  const QString corePath = dr_core_path(DR_CORE_MGBA);

  setRetro(new DrRetro(this));
  m_retro->setCore(new QRetro());
  m_retro->setGamePath((dr_roms_directory() + "/Mario Party Advance (USA).gba").toStdString());

  core()->setBilinearFilter(false);
  core()->setIntegerScaling(true);

  if (!core()->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
    return;
  }

  core()->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  if (!core()->loadContent(m_retro->gamePath().c_str()))
  {
    log(DR_LOG_ERROR,
      qPrintable(QString("failed to load content: %1").arg(QString::fromStdString(m_retro->gamePath()))));
    m_valid = false;
    return;
  }

  connect(core(), &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);
}

size_t MarioPartyAdvanceHost::boardContext(void)
{
  uint32_t ctx = 0, block = 0;

  if (m_retro->readu32(&ctx, MPA_BOARD_CTX_PTR) != DR_OK || ctx < 0x02000008 || ctx >= 0x02040000)
    return 0;
  if (m_retro->readu32(&block, ctx - 8) != DR_OK || block != MPA_BOARD_CTX_BLOCK_SIZE)
    return 0;

  return ctx;
}

void MarioPartyAdvanceHost::applyHooks(void)
{
  unsigned i;

  core()->memory().writeBuffer(MPA_CAVE, MPA_CAVE_ADDR, MPA_TITLE_BLOCK - MPA_CAVE_ADDR);
  for (i = 0; MPA_HOOK_BOARD[i][0] != 0; i++)
    m_retro->writeu32(MPA_HOOK_BOARD[i][1], MPA_HOOK_BOARD[i][0]);
}

void MarioPartyAdvanceHost::setState(dr_mpa_host_state state)
{
  static const char *names[] =
  {
    "INVALID",
    "BOARD",
    "ROULETTE",
    "MINIGAME"
  };

  log(DR_LOG_INFO, qPrintable(QString("host state: %1 -> %2").arg(names[m_state]).arg(names[state])));
  m_state = state;
}

void MarioPartyAdvanceHost::stampTitle(unsigned slot, const char *name)
{
  const bool space = m_mode == MPA_MODE_SPACE;
  const int limit = space ? MPA_ANNOUNCE_CHARS : MPA_TITLE_CHARS;
  const size_t address = space ? MPA_ANNOUNCE_TITLE : MPA_TITLE_BLOCK + slot * MPA_TITLE_SLOT_SIZE;
  uint8_t buffer[MPA_TITLE_SLOT_SIZE] = {};
  QString fitted = name ? QString::fromUtf8(name) : QString("???");
  int i, length = 0;

  /* Cut long names at the last word that fits, or mid-word if that's too short */
  if (fitted.size() > limit)
  {
    const int word = fitted.lastIndexOf(QChar(' '), limit);

    fitted = fitted.left(word >= limit / 2 ? word : limit);
  }

  for (i = 0; i < fitted.size() && length < MPA_TITLE_SLOT_SIZE - 1; i++)
  {
    const uint8_t code = mpa_encode_char(fitted.at(i));

    if (code)
      buffer[length++] = code;
  }

  core()->memory().writeBuffer(buffer, address, sizeof(buffer));
}

void MarioPartyAdvanceHost::rollSlots(unsigned slot_count)
{
  dr_minigame_type types[sizeof(MPA_SLOT_TYPES) / sizeof(MPA_SLOT_TYPES[0])];
  unsigned type_count = 0, slot, i;

  m_slots = {};
  m_slotTypes = {};
  if (!m_MinigameSource)
    return;

  m_MinigameSource->rerollMinigames();

  for (i = 0; i < sizeof(MPA_SLOT_TYPES) / sizeof(MPA_SLOT_TYPES[0]); i++)
    if (m_MinigameSource->minigameCandidates(MPA_SLOT_TYPES[i])[0].guest)
      types[type_count++] = MPA_SLOT_TYPES[i];

  if (!type_count)
  {
    log(DR_LOG_ERROR, "roulette: no mini-games available for any type");
    return;
  }

  for (slot = 0; slot < slot_count && slot < MPA_ROULETTE_SLOTS; slot++)
  {
    const dr_minigame_type type = types[static_cast<unsigned>(dr_rand()) % type_count];
    unsigned same = 0;

    /* Take the next unused candidate of a type rolled more than once */
    for (i = 0; i < slot; i++)
      if (m_slotTypes[i] == type)
        same++;

    m_slotTypes[slot] = type;
    m_slots[slot] = m_MinigameSource->minigameCandidates(type)[same];
    if (!m_slots[slot].guest)
      m_slots[slot] = m_MinigameSource->minigameCandidates(type)[0];

    stampTitle(slot, m_slots[slot].minigame ? m_slots[slot].minigame->name : nullptr);
    log(DR_LOG_INFO,
      qPrintable(QString("%1 slot %2: %3 (%4)")
          .arg(m_mode == MPA_MODE_SPACE ? "space" : "roulette")
          .arg(slot)
          .arg(m_slots[slot].minigame && m_slots[slot].minigame->name ? m_slots[slot].minigame->name : "(none)")
          .arg(dr_minigame_type_name(type))));
  }
}

bool MarioPartyAdvanceHost::readPlayerSetup(DrPlayerArray &players)
{
  readPlayers(players, DR_MINIGAME_4P);
  return true;
}

void MarioPartyAdvanceHost::readPlayers(DrPlayerArray &players, dr_minigame_type type)
{
  int64_t character = 0;
  unsigned i, cpu = 0;

  m_retro->readValue(&character, MPA_CHARACTER);
  if (character < 0 || character > 3)
    character = 0;

  players = {};
  for (i = 0; i < 4; i++)
  {
    dr_player_t &player = players[i];

    if (i == 0)
      player.character = MPA_CHARACTERS[character];
    else
    {
      /* CPUs take the other three characters in order */
      if (cpu == static_cast<unsigned>(character))
        cpu++;
      player.character = MPA_CHARACTERS[cpu++];
    }

    player.control_port = i;
    player.control_type = i == 0 ? DR_CONTROL_TYPE_HUMAN : DR_CONTROL_TYPE_CPU;
    player.difficulty = DR_DIFFICULTY_NORMAL;

    switch (type)
    {
    case DR_MINIGAME_1P:
      player.team_id = i == 0 ? 0 : 1;
      player.team_type = i == 0 ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
      break;
    case DR_MINIGAME_1V3:
      player.team_id = i == 0 ? 0 : 1;
      player.team_type = i == 0 ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
      break;
    case DR_MINIGAME_2V2:
      player.team_id = i < 2 ? 0 : 1;
      player.team_type = DR_TEAM_TYPE_2V2;
      break;
    case DR_MINIGAME_DUEL:
      player.team_id = i == 0 ? 1 : i == 1 ? 0 : 2;
      player.team_type = i == 0   ? DR_TEAM_TYPE_DUEL_INITIATOR
                         : i == 1 ? DR_TEAM_TYPE_DUEL_TARGET
                                  : DR_TEAM_TYPE_DUEL_NONPARTICIPANT;
      break;
    default:
      player.team_id = i;
      player.team_type = DR_TEAM_TYPE_4P;
      break;
    }
  }
}

void MarioPartyAdvanceHost::startMinigame(void)
{
  const DrMinigameCandidate *candidate;

  if (m_slot < 0 || m_slot >= MPA_ROULETTE_SLOTS || !m_slots[m_slot].guest || !m_slots[m_slot].minigame)
  {
    log(DR_LOG_ERROR, qPrintable(QString("startMinigame: no candidate for slot %1").arg(m_slot)));
    resumeBoard(false);
    return;
  }

  candidate = &m_slots[m_slot];
  readPlayers(m_players, m_slotTypes[m_slot]);

  log(DR_LOG_INFO,
    qPrintable(QString("launching mini-game: %1 (slot %2)")
        .arg(candidate->minigame->name ? candidate->minigame->name : "(unnamed)")
        .arg(m_slot)));

  m_waitFrames = 0;
  setState(DR_MPA_HOST_STATE_MINIGAME);
  emit minigameRequested(*candidate, m_players);
}

bool MarioPartyAdvanceHost::playerOneWon(DrGuest *guest)
{
  const dr_minigame_result_t own = guest->minigameResult(0);
  unsigned i;

  if (m_slot >= 0 && m_slotTypes[m_slot] == DR_MINIGAME_BATTLE)
    return own.coins == DR_MINIGAME_RESULT_BATTLE_1ST;

  if (own.coins <= 0)
    return false;

  /* Beat or tie every participant not on P1's team */
  for (i = 1; i < 4; i++)
  {
    if (!dr_team_type_participates(m_players[i].team_type) || m_players[i].team_id == m_players[0].team_id)
      continue;
    if (guest->minigameResult(i).coins > own.coins)
      return false;
  }

  return true;
}

void MarioPartyAdvanceHost::resumeBoard(bool cleared)
{
  const size_t ctx = boardContext();

  setState(DR_MPA_HOST_STATE_BOARD);
  if (!ctx)
  {
    log(DR_LOG_ERROR, "resume: the board is no longer running");
    return;
  }

  log(DR_LOG_INFO, cleared ? "mini-game cleared" : "mini-game failed");

  m_retro->writeValue(cleared ? 1 : 0, MPA_MG_RESULT);
  m_retro->writeu8(m_mode == MPA_MODE_SPACE ? MPA_MODE_SPACE_RESULT : MPA_MODE_CHALLENGE_RESULT,
    ctx + MPA_CTX_MODE);
  m_retro->writeu8(0, ctx + MPA_CTX_SUBSTATE);
  m_retro->writeu16(0, ctx + MPA_CTX_WAIT);
  m_retro->writeu16(0, ctx + MPA_CTX_WAIT + 2);
}

void MarioPartyAdvanceHost::writeResults(DrGuest *guest)
{
  unsigned i;

  if (m_state != DR_MPA_HOST_STATE_MINIGAME)
    return;

  for (i = 0; i < 4; i++)
    log(DR_LOG_INFO, qPrintable(resultLogLine(i, m_players[i].character, guest->minigameResult(i))));

  resumeBoard(playerOneWon(guest));
}

void MarioPartyAdvanceHost::clearResults(void)
{
  if (m_state == DR_MPA_HOST_STATE_MINIGAME)
    resumeBoard(false);
}

void MarioPartyAdvanceHost::run(void)
{
  size_t ctx;
  uint8_t mode = 0, substate = 0;

  m_retro->tickFrameWrites();

  if (core()->frames() <= MPA_WARMUP_FRAMES)
    return;

  applyHooks();

  ctx = boardContext();
  if (ctx)
  {
    m_retro->readu8(&mode, ctx + MPA_CTX_MODE);
    m_retro->readu8(&substate, ctx + MPA_CTX_SUBSTATE);
  }

  switch (m_state)
  {
  case DR_MPA_HOST_STATE_INVALID:
    setState(DR_MPA_HOST_STATE_BOARD);
    break;

  case DR_MPA_HOST_STATE_BOARD:
    /* Titles go in before the roulette draws them or Tumble names the game; a
     * state loaded at the hand-off still rolls, and launches on the next frame */
    if (ctx && (mode == MPA_MODE_CHALLENGE || mode == MPA_MODE_SPACE))
    {
      m_mode = mode;
      rollSlots(mode == MPA_MODE_SPACE ? 1 : MPA_ROULETTE_SLOTS);
      setState(DR_MPA_HOST_STATE_ROULETTE);
    }
    break;

  case DR_MPA_HOST_STATE_ROULETTE:
  {
    int64_t slot = -1;

    if (!ctx || mode != m_mode)
    {
      setState(DR_MPA_HOST_STATE_BOARD);
      break;
    }
    if (substate != MPA_HANDOFF)
      break;

    if (m_mode == MPA_MODE_SPACE)
      slot = 0;
    else
      m_retro->readValue(&slot, MPA_ROULETTE_SLOT);
    m_slot = slot >= 0 && slot < MPA_ROULETTE_SLOTS ? static_cast<int>(slot) : -1;
    startMinigame();
    break;
  }

  case DR_MPA_HOST_STATE_MINIGAME:
    /* The core is paused while a guest runs; still running means it never launched */
    if (++m_waitFrames >= MPA_LAUNCH_TIMEOUT_FRAMES)
    {
      log(DR_LOG_ERROR, "mini-game never launched, resuming the board");
      resumeBoard(false);
    }
    break;

  case DR_MPA_HOST_STATE_SIZE:
    break;
  }
}
