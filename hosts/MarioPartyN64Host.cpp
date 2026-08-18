#include "MarioPartyN64Host.h"
#include "../DrGuest.h"

#include <QRandomGenerator>
#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>
#include <cctype>
#include <cstring>

typedef enum
{
  MP64_TEXT_BLACK = 0,
  MP64_TEXT_BLUE = 1,
  MP64_TEXT_RED = 2,
  MP64_TEXT_MAGENTA = 3,
  MP64_TEXT_GREEN = 4,
  MP64_TEXT_CYAN = 5,
  MP64_TEXT_YELLOW = 6,
  MP64_TEXT_WHITE = 7,

  MP64_TEXT_SIZE
} mp64_text_color;

void MarioPartyN64Host::setSceneQueue(const dr_mp64_overlay_t overlays[5], int overlay_count)
{
  if (!m_config.values.scene_stack.address || !m_config.values.scene_stack_count.address)
    return;

  for (int i = 0; i < 5; i++)
  {
    const size_t elem = m_config.values.scene_stack.address + static_cast<size_t>(i) * 8;
    writes32(overlays[i].id, elem);
    writes16(overlays[i].event, elem + 4);
    writes16(overlays[i].stat, elem + 6);
  }
  writes16(static_cast<int16_t>(overlay_count), m_config.values.scene_stack_count.address);
}

void MarioPartyN64Host::resetRuntimeState()
{
  /* Release any netplay golf mode we were holding for a 1P/item mini-game. */
  if (m_hostGolfMode)
    emit golfModeRequested(-1, 30);

  m_writing = 0;
  m_lastMinigameId = -1;
  m_lastBoardScene = 0;
  m_resultsScene = 0;
  m_resultsModifier = 0;
  m_MinigameType = -1;
  m_isDuelBoard = false;
  m_lastDuelType = 0xFF;
  m_sceneChangeGrace = 0;
  m_guardTurn = 0;
  m_guardSpace = 0;
  m_pendingStartIndex = 0;
  m_startDelay = 0;
  m_itemPending = false;
  m_itemSceneLeft = false;
  m_itemChosenId = 0;
  m_hostGolfMode = false;
  m_candidates = {};
  m_pendingPlayers = {};

  m_State = DR_HOST_STATE_INVALID;
}

void MarioPartyN64Host::run(void)
{
  static const uint8_t ff = 0xff;

  tickFrameWrites();

  static const char *stateNames[] = {
    "INVALID", "BEFORE_BOARD", "BOARD", "BEFORE_ROULETTE", "ROULETTE", "AFTER_ROULETTE", "MINIGAME"
  };
  auto setState = [&](dr_host_state s) {
    emit logMessage(DR_LOG_INFO,
      QString("host state: %1 -> %2").arg(stateNames[m_State]).arg(stateNames[s]));
    m_State = s;
  };

  int16_t scene_id = 0;
  reads16(&scene_id, m_config.values.scene.address);

  if ((uint8_t)scene_id != m_lastScene)
  {
    const char *scene_name = dr_scene_name(m_config.scene_names, (uint8_t)scene_id);
    if (scene_name)
      emit logMessage(DR_LOG_INFO,
        QString("scene: 0x%1 (%2)").arg((uint8_t)scene_id, 2, 16, QChar('0')).arg(scene_name));
    else
      emit logMessage(DR_LOG_WARN,
        QString("scene: 0x%1 <<< UNKNOWN SCENE ID >>>").arg((uint8_t)scene_id, 2, 16, QChar('0')));

    /* Returning to the main menu abandons the current game: wipe every bit of runtime
     * state and drop back to INVALID so the next board is armed from scratch. */
    if (m_config.scenes.main_menu && (uint8_t)scene_id == m_config.scenes.main_menu)
    {
      emit logMessage(DR_LOG_INFO, "returned to main menu: resetting host state");
      resetRuntimeState();
    }

    /* Board events (e.g. Bowser) zero the mini-game type as they come and go. Ignore
     * the type byte for a bit after any scene change so the transient isn't read as a
     * roulette -- a real roulette opens without changing scene, so nothing is missed. */
    m_sceneChangeGrace = 30;

    /* MP1: randomize the title-screen image on entry to the Intro scene (0x61). */
    if (game() == DR_GAME_MARIOPARTY1 && (uint8_t)scene_id == 0x61)
      writes8(static_cast<int8_t>(dr_rand() % 7), 0x800ED146);

    /* Item mini-games play natively with a single participant. Grant that player golf
     * mode (0 input delay, priority) on entry and clear it on exit. The participant is
     * the board slot with team id 0; its controller_addr holds the player index. */
    if (m_config.scenes.single_player_ids[0] != -1)
    {
      auto inItem = [&](uint8_t v) {
        for (unsigned i = 0;
             i < sizeof(m_config.scenes.single_player_ids) / sizeof(*m_config.scenes.single_player_ids)
               && m_config.scenes.single_player_ids[i] != -1;
             i++)
          if (m_config.scenes.single_player_ids[i] == v)
            return true;
        return false;
      };
      const bool wasItem = inItem(m_lastScene);
      const bool isItem = inItem((uint8_t)scene_id);
      if (isItem && !wasItem)
      {
        int authority = -1;
        for (unsigned i = 0; i < 4; i++)
        {
          uint8_t team = 0xFF;
          readu8(&team, m_config.values.team[i].address);
          if (team == 0)
          {
            uint8_t ctrl = 0;
            readu8(&ctrl, m_config.values.controller[i].address);
            authority = ctrl;
            break;
          }
        }
        emit logMessage(DR_LOG_INFO,
          QString("item mini-game: golf mode -> player %1").arg(authority));
        emit golfModeRequested(authority, 30);
      }
      else if (wasItem && !isItem)
      {
        emit logMessage(DR_LOG_INFO, "item mini-game over: golf mode off");
        emit golfModeRequested(-1, 30);
      }
    }

    m_lastScene = (uint8_t)scene_id;
  }

  switch (m_State)
  {
  case DR_HOST_STATE_INVALID:
    if (m_core->frames() <= 120)
      return;
    /* First roll + title stamp for the whole block; refreshed on every return to the
     * board after a mini-game (see DR_HOST_STATE_MINIGAME). */
    rollAndStampTitles();
    setState(DR_HOST_STATE_BEFORE_BOARD);
    break;
  case DR_HOST_STATE_BEFORE_BOARD:
  {
    writeForFrames(mgTypeAddr(), &ff, 1, 30);

    bool matched = false;
    bool isDuel = false;

    for (unsigned r = 0; m_config.scenes.boards[r] != -1; r++)
      if ((uint8_t)scene_id == m_config.scenes.boards[r])
    {
      matched = true;
      break;
    }

    for (unsigned d = 0; !matched && m_config.scenes.boards_duel[d] != -1; d++)
      if ((uint8_t)scene_id == m_config.scenes.boards_duel[d])
    {
      matched = true;
      isDuel = true;
      break;
    }

    if (matched)
    {
      m_lastBoardScene = (uint8_t)scene_id;
      m_isDuelBoard = isDuel;

      /* The initial roll happened before we knew this was a duel board, so the duel
       * candidates weren't stamped into row 0; reroll now that m_isDuelBoard is set. */
      if (m_isDuelBoard)
      {
        m_lastDuelType = 0xFF;
        rollAndStampTitles();
      }

      /* Hold the active type byte at 0xFF for a bit so a value left over from loading
       * doesn't read as an in-progress roulette the instant we enter the board. */
      writeForFrames(mgTypeAddr(), &ff, 1, 30);

      /* Stamp the code cave into RAM, then enable the board hook (slot 2) that jumps
       * into it -- a single code that both stamps the roulette titles and forces the
       * chosen id onto the slot index. A game with a separate duel overlay (MP3) has
       * its own hook (slot 3); the two are toggled so only the one matching the
       * current board is live. */
      auto logCheat = [&](int slot, const char *name, bool enabled) {
        emit logMessage(DR_LOG_INFO,
          QString("cheat: %1 (slot %2) %3 on board 0x%4")
            .arg(name).arg(slot).arg(enabled ? "ENABLED" : "disabled")
            .arg((uint8_t)scene_id, 2, 16, QChar('0')));
      };
      if (m_config.cheats.cave && m_config.cheats.cave_addr)
      {
        /* Write the blob as aligned 32-bit words: each four bytes are a big-endian
         * MIPS word, and writeu32 uses the class's wordflipped endianness to land
         * them in RAM correctly. */
        for (unsigned off = 0; off + 4 <= m_config.cheats.cave_size; off += 4)
        {
          const uint8_t *b = m_config.cheats.cave + off;
          const uint32_t word = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
                              | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
          writeu32(word, m_config.cheats.cave_addr + off);
        }
        emit logMessage(DR_LOG_INFO,
          QString("cave: stamped %1 bytes at 0x%2 on board 0x%3")
            .arg(m_config.cheats.cave_size)
            .arg((quint32)m_config.cheats.cave_addr, 8, 16, QChar('0'))
            .arg((uint8_t)scene_id, 2, 16, QChar('0')));
      }
      if (m_config.cheats.cheat_board)
      {
        m_core->cheatSet(2, !m_isDuelBoard, m_config.cheats.cheat_board);
        logCheat(2, "board hook", !m_isDuelBoard);
      }
      if (m_config.cheats.cheat_duel)
      {
        m_core->cheatSet(3, m_isDuelBoard, m_config.cheats.cheat_duel);
        logCheat(3, "duel hook", m_isDuelBoard);
      }
      setState(DR_HOST_STATE_BOARD);
    }
    break;
  }
  case DR_HOST_STATE_BOARD:
  {
    if (m_itemPending)
    {
      if (!m_itemSceneLeft)
      {
        writeu8(m_itemChosenId, m_config.values.minigame_id.address);
        if ((uint8_t)scene_id != m_lastBoardScene)
          m_itemSceneLeft = true;
      }
      else if ((uint8_t)scene_id == m_lastBoardScene)
      {
        m_itemPending = false;
        m_itemSceneLeft = false;
        writeForFrames(mgTypeAddr(), &ff, 1, 30);
      }
      break;
    }

    /* Ignore any progression if we are not on the board */
    if ((uint8_t)scene_id != m_lastBoardScene)
      return;

    /* Grace period after a scene change (e.g. returning from a Bowser event): hold the
     * type byte at 0xFF and don't read it as a roulette until things settle. */
    if (m_sceneChangeGrace > 0)
    {
      m_sceneChangeGrace--;
      writeu8(0xFF, mgTypeAddr());
      if (m_isDuelBoard)
        m_lastDuelType = 0xFF;
      break;
    }

    if (m_isDuelBoard)
    {
      /* A duel board only ever runs the duel type, so unlike the regular board (which
       * watches several types) we gate the roulette purely on the duel type byte
       * transitioning to 0. */
      uint8_t minigame_type = m_lastDuelType;
      readu8(&minigame_type, m_config.values.title_type_duel.address);
      if (minigame_type != m_lastDuelType)
        emit logMessage(DR_LOG_INFO,
          QString("duel mg type: 0x%1").arg(minigame_type, 2, 16, QChar('0')));
      const bool toZero = (minigame_type == 0 && m_lastDuelType != 0);
      m_lastDuelType = minigame_type;
      if (toZero)
      {
        for (unsigned i = 0; i < m_config.minigame_type_to_dr_size; i++)
          if (m_config.minigame_type_to_dr[i] == DR_MINIGAME_DUEL) { m_MinigameType = i; break; }
        rollCandidates(DR_MINIGAME_DUEL);
        captureBoardGuard();
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
      }
      break;
    }

    /* Check if the mini-game type value been set */
    uint8_t minigame_type = 0;
    readu8(&minigame_type, m_config.values.minigame_type.address);
    if (minigame_type != 0xFF)
      emit logMessage(DR_LOG_INFO,
        QString("board mg type: 0x%1").arg(minigame_type, 2, 16, QChar('0')));
    if (minigame_type != 0xFF && minigame_type < m_config.minigame_type_to_dr_size)
    {
      dr_minigame_type mg_type = m_config.minigame_type_to_dr[minigame_type];

      if (mg_type == DR_MINIGAME_ITEM)
      {
        m_lastMinigameId = -1;
        writeu8(0xFF, m_config.values.minigame_id.address);
        m_itemChosenId = 0x3B + (dr_rand() % 6);
        m_itemPending = true;
        m_itemSceneLeft = false;
        break;
      }
      else if (mg_type != DR_MINIGAME_INVALID)
      {
        m_MinigameType = minigame_type;
        rollCandidates(mg_type);
        captureBoardGuard();
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
        break;
      }
    }
    writeu8(0xFF, mgTypeAddr());
    break;
  }
  case DR_HOST_STATE_BEFORE_ROULETTE:
    if ((uint8_t)scene_id != m_lastBoardScene)
    {
      setState(DR_HOST_STATE_BEFORE_BOARD);
      break;
    }

    /* Write -1 to current mini-game to monitor for change */
    writeValue(-1, m_config.values.minigame_id);

    /* Proceed... */
    setState(DR_HOST_STATE_ROULETTE);
    break;

  case DR_HOST_STATE_ROULETTE:
  {
    if ((uint8_t)scene_id != m_lastBoardScene)
    {
      setState(DR_HOST_STATE_BEFORE_BOARD);
      break;
    }
    if (boardGuardTripped())
    {
      emit logMessage(DR_LOG_WARN,
        "roulette failsafe: turn/space changed mid-roulette; returning to BOARD");
      /* Clear the (stale/false) type byte so BOARD doesn't immediately re-fire. */
      writeForFrames(mgTypeAddr(), &ff, 1, 30);
      if (m_isDuelBoard)
        m_lastDuelType = 0xFF;
      setState(DR_HOST_STATE_BOARD);
      break;
    }

    /**
     * Check for changes to mini-game type while rolling.
     * Useful if we somehow opened a new roulette before another finished.
     */
    if (!m_isDuelBoard)
    {
      uint8_t minigame_type = 0;

      readu8(&minigame_type, m_config.values.minigame_type.address);
      if (minigame_type != (uint8_t)m_MinigameType)
      {
        emit logMessage(DR_LOG_ERROR,
          QString("Mini-game type switched from %1 to %2 somehow! Fixing...")
            .arg(dr_minigame_type_name((dr_minigame_type)m_MinigameType))
            .arg(dr_minigame_type_name((dr_minigame_type)minigame_type)));
        m_MinigameType = minigame_type;
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
      }
    }

    /* Wait for a change to chosen mini-game */
    int64_t idValue = 0;
    readValue(&idValue, m_config.values.minigame_id);
    int16_t id = static_cast<int16_t>(idValue);
    if (id == m_lastMinigameId)
      break;

    /* A mini-game was chosen. Proceed... */
    emit logMessage(DR_LOG_INFO,
      QString("roulette chose id %1 (duel=%2)").arg(id).arg(m_isDuelBoard ? 1 : 0));
    m_lastMinigameId = id;
    m_startDelay = 0;
    setState(DR_HOST_STATE_AFTER_ROULETTE);
    break;
  }

  case DR_HOST_STATE_AFTER_ROULETTE:
  {
    /* Once the queue has been overwritten, spin a while so the game settles onto the
     * results scene before we launch the guest mini-game. */
    if (m_startDelay > 0)
    {
      if (--m_startDelay == 0)
      {
        dr_minigame_type mg_type = (m_MinigameType < (int)m_config.minigame_type_to_dr_size)
          ? m_config.minigame_type_to_dr[m_MinigameType] : DR_MINIGAME_INVALID;
        readPlayers(mg_type);

        /* A 1P mini-game has a single participant (the board slot with team id 0);
         * give that player netplay golf mode so they play without waiting on the idle
         * peers. Cleared when the mini-game returns to the board. */
        if (mg_type == DR_MINIGAME_1P)
        {
          int authority = -1;
          for (unsigned i = 0; i < 4; i++)
            if (m_pendingPlayers[i].team_id == 0)
            {
              authority = static_cast<int>(m_pendingPlayers[i].control_port) - DR_CONTROL_PORT_P1;
              break;
            }
          emit golfModeRequested(authority, 30);
          m_hostGolfMode = true;
        }

        /* I can't remember why but these are offset differently */
        if (game() != DR_GAME_MARIOPARTY1)
          m_lastMinigameId -= 1;

        /* Start the mini-game! */
        startMinigame(m_lastMinigameId);

        m_lastMinigameId = -1;
        writeu8(0xFF, m_config.values.minigame_id.address);
        setState(DR_HOST_STATE_MINIGAME);
      }
      break;
    }

    /* Wait for the game to queue its own scene overlays. */
    int16_t count = 0;
    reads16(&count, m_config.values.scene_stack_count.address);
    if (!count)
      break;

    dr_minigame_type mg_type = (m_MinigameType < (int)m_config.minigame_type_to_dr_size)
      ? m_config.minigame_type_to_dr[m_MinigameType] : DR_MINIGAME_INVALID;
    int16_t stat = m_config.stat.minigame;
    switch (mg_type)
    {
    case DR_MINIGAME_4P:
    case DR_MINIGAME_1V3:
    case DR_MINIGAME_2V2:
    case DR_MINIGAME_1P:
      m_resultsScene = m_config.scenes.minigame_results;
      m_resultsModifier = 0;
      stat = m_config.stat.minigame;
      m_startDelay = 45;
      break;
    case DR_MINIGAME_BATTLE:
      m_resultsScene = m_config.scenes.minigame_results_battle;
      m_resultsModifier = 2;
      stat = m_config.stat.minigame;
      m_startDelay = 15;
      break;
    case DR_MINIGAME_DUEL:
      if (m_isDuelBoard)
      {
        m_resultsScene = m_config.scenes.minigame_results_duel;
        m_resultsModifier = 0;
        stat = m_config.stat.duel;
      }
      else
      {
        m_resultsScene = 0x75;
        m_resultsModifier = 2;
        stat = m_config.stat.board;
      }
      m_startDelay = 15;
      break;
    case DR_MINIGAME_ITEM:
      m_resultsScene = m_lastBoardScene;
      m_resultsModifier = 2;
      stat = m_config.stat.board;
      m_startDelay = 15;
      break;
    default:
      emit logMessage(DR_LOG_INFO,
        QString("bad mini-game type: 0x%1").arg(m_MinigameType, 2, 16, QChar('0')));
      break;
    }

    /* Read the whole queue, swap the mini-game explanation element for our results
     * screen, and keep everything else. */
    dr_mp64_overlay_t overlays[5] = {};
    for (int i = 0; i < 5; i++)
    {
      const size_t elem = m_config.values.scene_stack.address + static_cast<size_t>(i) * 8;
      reads32(&overlays[i].id, elem);
      reads16(&overlays[i].event, elem + 4);
      reads16(&overlays[i].stat, elem + 6);
    }

    const int active = (count < 5) ? count : 5;
    int explainIdx = -1;
    for (int i = 0; i < active && explainIdx < 0; i++)
      for (unsigned e = 0; m_config.scenes.minigame_explain[e] != -1; e++)
        if (overlays[i].id == static_cast<int32_t>(m_config.scenes.minigame_explain[e]))
        {
          explainIdx = i;
          break;
        }

    if (explainIdx < 0)
    {
      emit logMessage(DR_LOG_ERROR,
        "AFTER_ROULETTE: no mini-game explanation in the scene queue; aborting");
      setState(DR_HOST_STATE_INVALID);
      break;
    }

    overlays[explainIdx] = { static_cast<int32_t>(m_resultsScene), m_resultsModifier, stat };
    setSceneQueue(overlays, count);
    break;
  }

  case DR_HOST_STATE_MINIGAME:
  {
    /* Wait for the game to return to the board */
    if ((uint8_t)scene_id == m_lastBoardScene)
    {
      /* Invalidate our watchpoints */
      m_lastMinigameId = -1;
      writeu8(0xFF, m_config.values.minigame_id.address);
      writeForFrames(mgTypeAddr(), &ff, 1, 30);

      /* Clear any golf mode we granted for a 1P mini-game. */
      if (m_hostGolfMode)
      {
        emit golfModeRequested(-1, 30);
        m_hostGolfMode = false;
      }

      /* Roll our next set of mini-games */
      rollAndStampTitles();

      /* Proceed... */
      setState(DR_HOST_STATE_BOARD);
    }
    break;
  }

  case DR_HOST_STATE_SIZE:
    break;
  }
}

MarioPartyN64Host::MarioPartyN64Host(const DrHostConfig &config, QObject *parent)
  : DrHost(parent)
  , m_config(config)
{
  m_core = new QRetro();
  m_ownCore = true;
  m_gamePath = config.game; // so gamePath() yields the ROM (used for the netplay save name)
  if (!m_core->loadCore(config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }

  /* Read/write the save from the derailleur save dir (default cwd/save). A netplay
   * client redirects dr_save_directory() to its per-session netplay dir before
   * building us, so we load the host's save. Set before loadContent (SRAM read). */
  m_core->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  if (!m_core->loadContent(config.game.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(config.game.c_str())));
    m_valid = false;
  }

  /** @todo For right now, every host is N64 */
  m_endianness = DR_ENDIANNESS_WORDFLIPPED;
  applyN64Remaps();

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);
}

void MarioPartyN64Host::stampTitleRow(
  unsigned row, const std::array<DrMinigameCandidate, 5> &candidates)
{
  if (!m_config.values.title_block.address || row >= 8) // block is [8][5][32]
    return;

  for (unsigned slot = 0; slot < 5; slot++)
  {
    const char *name =
      (candidates[slot].minigame && candidates[slot].minigame->name)
        ? candidates[slot].minigame->name : "";

    uint8_t glyphs[32] = {};
    unsigned n = 0;
    for (size_t k = 0, len = strnlen(name, 64); k < len && n < 31; k++)
    {
      char c = name[k];
      if (isalpha((unsigned char)c) || isdigit((unsigned char)c) || c == ' ')
        glyphs[n++] = (uint8_t)c;
      else if (c == '.') glyphs[n++] = 0x85;
      else if (c == ',') glyphs[n++] = 0x82;
      else if (c == '\'') glyphs[n++] = 0x5C;
      else if (c == '-') glyphs[n++] = 0x3D;
      else if (c == '!') glyphs[n++] = 0xC2;
      else if (c == '?') glyphs[n++] = 0xC3;
      else if (c == '_') glyphs[n++] = 0x86;
      else if (c == '&') glyphs[n++] = 0x7E;
      else if (c == '%') glyphs[n++] = 0x7C;
      else if (c == ':') glyphs[n++] = 0x7B;
    }
    /* glyphs[n..31] stay zero: NUL terminator plus padding. */

    const size_t base = m_config.values.title_block.address + ((size_t)row * 5 + slot) * 32;
    for (unsigned j = 0; j < 32; j++)
      writeu8(glyphs[j], base + j);
  }
}

void MarioPartyN64Host::rollAndStampTitles(void)
{
  if (!m_MinigameSource)
    return;

  /* One shared reroll keeps every netplay peer's pool identical (see DrMinigameSource). */
  m_MinigameSource->rerollMinigames();

  /* Stamp each mini-game type's five names into its title row and its five colors into
   * its 8-byte color row, so the trampoline reads whichever row the rolled type selects. */
  for (unsigned t = 0; t < m_config.minigame_type_to_dr_size; t++)
  {
    const dr_minigame_type dr = m_config.minigame_type_to_dr[t];
    if (dr == DR_MINIGAME_INVALID)
      continue;
    const std::array<DrMinigameCandidate, 5> &cands = m_MinigameSource->minigameCandidates(dr);
    stampTitleRow(t, cands);
    stampTitleColors(t, cands);
  }

  /* A duel board always uses row 0 for the duel candidates, regardless of the duel
   * mini-game's type byte; the duel trampoline reads row 0 unconditionally. */
  if (m_isDuelBoard)
  {
    const std::array<DrMinigameCandidate, 5> &cands =
      m_MinigameSource->minigameCandidates(DR_MINIGAME_DUEL);
    stampTitleRow(0, cands);
    stampTitleColors(0, cands);
  }
}

void MarioPartyN64Host::stampTitleColors(
  unsigned row, const std::array<DrMinigameCandidate, 5> &candidates)
{
  if (!m_config.values.title_color.address || row >= 8) // color table is 8 rows of 8 bytes
    return;

  /* 8 color bytes for this type row (see mp64_text_color): slots 0-4 follow the
   * candidates' flags (yellow = lucky, red = unlucky, else white), 5-7 are padding.
   * The trampoline copies the rolled type's row into the game's color array. */
  for (unsigned i = 0; i < 8; i++)
  {
    const dr_mp_minigame_t *mg = (i < 5) ? candidates[i].minigame : nullptr;
    uint8_t color = MP64_TEXT_WHITE;

    if (mg && mg->flags.flags.lucky)
      color = MP64_TEXT_YELLOW;
    else if (mg && mg->flags.flags.unlucky)
      color = MP64_TEXT_RED;
    writeu8(color, m_config.values.title_color.address + (size_t)row * 8 + i);
  }
}

void MarioPartyN64Host::rollCandidates(dr_minigame_type type)
{
  /* The pool is already rolled (rollAndStampTitles); just copy out the chosen type's
   * five so startMinigame launches the right guest. */
  if (!m_MinigameSource)
  {
    m_candidates = {};
    return;
  }

  m_candidates = m_MinigameSource->minigameCandidates(type);
}

int64_t MarioPartyN64Host::readBoardGuard(const dr_value_t &value)
{
  if (!value.address)
    return 0;
  int64_t out = 0;
  readValue(&out, value);
  return out;
}

void MarioPartyN64Host::captureBoardGuard()
{
  m_guardTurn = readBoardGuard(m_config.values.turn_owner);
  m_guardSpace = readBoardGuard(m_config.values.space_index);
}

bool MarioPartyN64Host::boardGuardTripped()
{
  if (!m_config.values.turn_owner.address && !m_config.values.space_index.address)
    return false;
  return readBoardGuard(m_config.values.turn_owner) != m_guardTurn
    || readBoardGuard(m_config.values.space_index) != m_guardSpace;
}

void MarioPartyN64Host::startMinigame(unsigned index)
{
  if (index >= 5 || !m_candidates[index].guest || !m_candidates[index].minigame)
  {
    emit logMessage(DR_LOG_ERROR,
      QString("startMinigame: no valid candidate at index %1 (guest=%2 mg=%3)")
        .arg(static_cast<int>(index))
        .arg(index < 5 && m_candidates[index].guest ? 1 : 0)
        .arg(index < 5 && m_candidates[index].minigame ? 1 : 0));
    return;
  }
  const dr_mp_minigame_t *mg = m_candidates[index].minigame;
  emit logMessage(DR_LOG_INFO,
    QString("chosen mini-game: %1 - %2 (0x%3)")
      .arg(index)
      .arg(mg->name ? mg->name : "(unnamed)")
      .arg(mg->minigame_id, 2, 16, QChar('0')));
  emit minigameRequested(m_candidates[index], m_pendingPlayers);
}

void MarioPartyN64Host::readPlayers(dr_minigame_type type)
{
  m_pendingPlayers = {};

  uint8_t panelColors[4];
  for (unsigned i = 0; i < 4; i++)
    if (readu8(&panelColors[i], m_config.values.panel_color[i].address) != DR_OK)
      panelColors[i] = 0xFF;

  for (unsigned i = 0; i < 4; i++)
  {
    uint8_t chr, ctrl, diff, bot, team;
    if (readu8(&chr,  m_config.values.character[i].address)   != DR_OK) continue;
    if (readu8(&ctrl, m_config.values.controller[i].address)  != DR_OK) continue;
    if (readu8(&diff, m_config.values.difficulty[i].address)  != DR_OK) continue;
    if (readu8(&bot,  m_config.values.bot[i].address)          != DR_OK) continue;
    if (readu8(&team, m_config.values.team[i].address)         != DR_OK) continue;

    dr_player_t &p = m_pendingPlayers[i];
    p.character   = (chr  < m_config.char_to_dr_size)          ? m_config.char_to_dr[chr]          : DR_CHARACTER_INVALID;
    p.difficulty  = (diff < m_config.diff_to_dr_size)          ? m_config.diff_to_dr[diff]          : DR_DIFFICULTY_INVALID;
    p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
    p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
    /* The panel-color byte maps 1:1 onto dr_team_color (0 invalid, 1 blue .. 4 green). */
    p.team_color  = (panelColors[i] < DR_TEAM_COLOR_SIZE)
                      ? static_cast<dr_team_color>(panelColors[i]) : DR_TEAM_COLOR_INVALID;
    p.team_id = team;

    if (m_config.values.coins[i].address)
    {
      uint16_t coins = 0;
      readu16(&coins, m_config.values.coins[i].address);
      p.coins = coins;
    }
    if (m_config.values.stars[i].address)
    {
      uint8_t stars = 0;
      readu8(&stars, m_config.values.stars[i].address);
      p.stars = stars;
    }
  }

  for (unsigned i = 0; i < 4; i++)
  {
    if (type == DR_MINIGAME_1V3)
    {
      switch (m_pendingPlayers[i].team_id)
      {
      case 0:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_1V3_SOLO;
        break;
      case 1:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_1V3_GROUP;
        break;
      default:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_INVALID;
      }
    }
    else if (type == DR_MINIGAME_2V2)
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_2V2;
    else if (type == DR_MINIGAME_4P)
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_4P;
    else if (type == DR_MINIGAME_DUEL)
    {
      switch (m_pendingPlayers[i].team_id)
      {
      case 0:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_DUEL_TARGET;
        break;
      case 1:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_DUEL_INITIATOR;
        break;
      default:
        m_pendingPlayers[i].team_type = DR_TEAM_TYPE_DUEL_NONPARTICIPANT;
        break;
      }
    }
    else
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_SOLO;
  }

  /* TODO remove: diagnose in-game duels being odd. Dump what team_addr/controller_addr
   * actually hold, to compare against the debug menu (which sets both duelists team_id 0). */
  if (type == DR_MINIGAME_DUEL)
    for (unsigned i = 0; i < 4; i++)
    {
      const dr_player_t &p = m_pendingPlayers[i];
      emit logMessage(DR_LOG_ERROR,
        QString("TODO duel: p%1 team_id=%2 team_type=%3 port=%4 chr=%5 %6")
          .arg(i).arg(p.team_id).arg((int)p.team_type)
          .arg((int)p.control_port - DR_CONTROL_PORT_P1)
          .arg(dr_character_name(p.character))
          .arg(p.control_type == DR_CONTROL_TYPE_CPU ? "CPU" : "human"));
    }
}

void MarioPartyN64Host::writeBattleCoins()
{
  if (!m_config.values.battle.address)
    return;

  int16_t totalCoins = 0;
  reads16(&totalCoins, m_config.values.battle.address);

  // Read each player's place (0 = 1st, 1 = 2nd, ...)
  uint16_t places[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
  for (unsigned i = 0; i < 4; i++)
    readu16(&places[i], m_config.values.result[i].address);

  // Count players at each place; find 1st and 2nd place values
  unsigned count[4] = {};
  uint16_t firstPlace = 0xFFFF;
  uint16_t secondPlace = 0xFFFF;
  for (unsigned i = 0; i < 4; i++)
  {
    if (places[i] >= 4)
      continue;
    count[places[i]]++;
    if (places[i] < firstPlace)
      firstPlace = places[i];
  }
  if (firstPlace == 0xFFFF)
    return;
  int numFirst = count[firstPlace];
  if (numFirst == 1)
  {
    for (uint16_t p = 0; p < 4; p++)
      if (p != firstPlace && count[p] > 0)
      {
        secondPlace = p;
        break;
      }
  }

  // Compute truncated shares
  int16_t bonusCoins[4] = {};
  int16_t totalAssigned = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    if (places[i] == firstPlace)
    {
      bonusCoins[i] =
        (numFirst == 1) ? (int16_t)(totalCoins * 70 / 100) : (int16_t)(totalCoins / numFirst);
    }
    else if (places[i] == secondPlace)
    {
      bonusCoins[i] = (int16_t)(totalCoins * 30 / (100 * count[secondPlace]));
    }
    totalAssigned += bonusCoins[i];
  }

  // One random qualifying player receives the remainder
  if (int16_t remainder = totalCoins - totalAssigned)
  {
    unsigned qualifying[4], numQ = 0;
    for (unsigned i = 0; i < 4; i++)
      if (places[i] == firstPlace || places[i] == secondPlace)
        qualifying[numQ++] = i;
    if (numQ > 0)
      bonusCoins[qualifying[dr_rand() % numQ]] += remainder;
  }

  for (unsigned i = 0; i < 4; i++)
    if (m_config.values.bonus_result[i].address)
      writes16(bonusCoins[i], m_config.values.bonus_result[i].address);
}

void MarioPartyN64Host::setCurrentTurn(unsigned turn)
{
  if (!m_config.values.turn_current.address)
  {
    emit logMessage(DR_LOG_WARN, "set turn: this game has no turn counter configured");
    return;
  }
  writeu8(static_cast<uint8_t>(turn), m_config.values.turn_current.address);
  emit logMessage(DR_LOG_INFO, QString("set current turn to %1").arg(turn));
}

void MarioPartyN64Host::clearResults()
{
  for (unsigned i = 0; i < 4; i++)
  {
    writes16(0, m_config.values.result[i].address);
    if (m_config.values.bonus_result[i].address)
      writes16(0, m_config.values.bonus_result[i].address);
  }
}

void MarioPartyN64Host::writeResults(DrGuest *guest)
{
  for (unsigned i = 0; i < 4; i++)
  {
    auto result = guest->minigameResult(i);

    uint8_t chr = 0;
    readu8(&chr, m_config.values.character[i].address);
    dr_character character =
      (chr < m_config.char_to_dr_size) ? m_config.char_to_dr[chr] : DR_CHARACTER_INVALID;

    emit logMessage(
      DR_LOG_INFO, QString("%1 gets %2 coins")
                     .arg(dr_character_name(character))
                     .arg((result.coins && result.bonus_coins)
                            ? QString("%1+%2").arg(result.coins).arg(result.bonus_coins)
                            : QString::number(result.coins ? result.coins : result.bonus_coins)));

    int16_t coins = static_cast<int16_t>(
      m_config.values.bonus_result[i].address ? result.coins : result.coins + result.bonus_coins);
    writes16(coins, m_config.values.result[i].address);
    if (m_config.values.bonus_result[i].address)
      writes16(static_cast<int16_t>(result.bonus_coins), m_config.values.bonus_result[i].address);
  }

  if (m_resultsScene == m_config.scenes.minigame_results_battle)
  {
    /* Read back the placements the loop just wrote (0 = 1st, 1 = 2nd, ...). A
     * four-way tie has no meaningful pot split, so rather than the battle results
     * scene, send the game to the current board scene with modifier 2. */
    uint16_t places[4];
    for (unsigned i = 0; i < 4; i++)
      readu16(&places[i], m_config.values.result[i].address);
    if (places[0] == places[1] && places[1] == places[2] && places[2] == places[3])
    {
      emit logMessage(DR_LOG_INFO, "battle: four-way tie, returning to board");
      dr_mp64_overlay_t overlays[5] = {};
      overlays[0] = { static_cast<int32_t>(m_lastBoardScene), 2, m_config.stat.board };
      setSceneQueue(overlays, 1);
    }
    else
      writeBattleCoins();
  }
  writeu8(0xFF, mgTypeAddr());
  m_writing = 30;
  m_lastScene = 0xFF;
}
