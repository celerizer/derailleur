#include "MarioPartyN64Host.h"
#include "DrGuest.h"

#include <QRandomGenerator>
#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>
#include <algorithm>
#include <cctype>
#include <cstring>

void MarioPartyN64Host::setSceneQueue(const mp64_overlay_t overlays[5], int overlay_count)
{
  if (!m_config.scene_stack_addr || !m_config.scene_stack_count_addr)
    return;

  for (int i = 0; i < 5; i++)
  {
    const size_t elem = m_config.scene_stack_addr + static_cast<size_t>(i) * 8;
    writes32(overlays[i].id, elem);
    writes16(overlays[i].event, elem + 4);
    writes16(overlays[i].stat, elem + 6);
  }
  writes16(static_cast<int16_t>(overlay_count), m_config.scene_stack_count_addr);
}

void MarioPartyN64Host::run(void)
{
  static const uint8_t ff = 0xff;

  tickFrameWrites();

  /* MP3 mini-game star bandaid: a second after the results, add the winnings to
   * any player whose star total didn't move on its own (see fixup_mg_star). */
  if (m_mgStarFixupCountdown > 0 && --m_mgStarFixupCountdown == 0)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      if (!m_config.mg_star_addr[i] || m_mgStarAdd[i] == 0)
        continue;
      int16_t cur = 0;
      reads16(&cur, m_config.mg_star_addr[i]);
      if (cur == m_mgStarPrev[i])
      {
        writes16(static_cast<int16_t>(cur + m_mgStarAdd[i]), m_config.mg_star_addr[i]);
        emit logMessage(DR_LOG_INFO,
          QString("mg star fixup: player %1 += %2").arg(i).arg(m_mgStarAdd[i]));
      }
    }
  }

  static const char *stateNames[] = {
    "INVALID", "BEFORE_BOARD", "BOARD", "BEFORE_ROULETTE", "ROULETTE", "AFTER_ROULETTE", "MINIGAME"
  };
  auto setState = [&](dr_host_state s) {
    emit logMessage(DR_LOG_INFO,
      QString("host state: %1 -> %2").arg(stateNames[m_State]).arg(stateNames[s]));
    m_State = s;
  };

  int16_t scene_id = 0;
  reads16(&scene_id, m_config.scene_addr);

  if ((uint8_t)scene_id != m_lastScene)
  {
    const char *scene_name = dr_scene_name(m_config.scene_names, (uint8_t)scene_id);
    if (scene_name)
      emit logMessage(DR_LOG_INFO,
        QString("scene: 0x%1 (%2)").arg((uint8_t)scene_id, 2, 16, QChar('0')).arg(scene_name));
    else
      emit logMessage(DR_LOG_WARN,
        QString("scene: 0x%1 <<< UNKNOWN SCENE ID >>>").arg((uint8_t)scene_id, 2, 16, QChar('0')));

    /* MP1: randomize the title-screen image on entry to the Intro scene (0x61). */
    if (game() == DR_GAME_MARIOPARTY1 && (uint8_t)scene_id == 0x61)
      writes8(static_cast<int8_t>(dr_rand() % 7), 0x800ED146);

    /* Item mini-games play natively with a single participant. Grant that player golf
     * mode (0 input delay, priority) on entry and clear it on exit. The participant is
     * the board slot with team id 0; its controller_addr holds the player index. */
    if (m_config.scene_item_first || m_config.scene_item_last)
    {
      auto inItem = [&](uint8_t v) {
        return v >= m_config.scene_item_first && v <= m_config.scene_item_last;
      };
      const bool wasItem = inItem(m_lastScene);
      const bool isItem = inItem((uint8_t)scene_id);
      if (isItem && !wasItem)
      {
        int authority = -1;
        for (unsigned i = 0; i < 4; i++)
        {
          uint8_t team = 0xFF;
          readu8(&team, m_config.team_addr[i]);
          if (team == 0)
          {
            uint8_t ctrl = 0;
            readu8(&ctrl, m_config.controller_addr[i]);
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
    setState(DR_HOST_STATE_BEFORE_BOARD);
    break;
  case DR_HOST_STATE_BEFORE_BOARD:
  {
    writeForFrames(m_config.minigame_type_addr, &ff, 1, 30);

    for (unsigned r = 0; r < m_config.scene_board_id_count; r++)
    {
      if ((uint8_t)scene_id == m_config.scene_board_ids[r])
      {
        m_lastBoardScene = (uint8_t)scene_id;
        m_isDuelBoard = false;
        for (unsigned d = 0; d < m_config.scene_duel_board_id_count; d++)
          if (m_lastBoardScene == m_config.scene_duel_board_ids[d])
          {
            m_isDuelBoard = true;
            break;
          }
        if (m_config.cheat_regular_board)
          m_core->cheatSet(1, !m_isDuelBoard, m_config.cheat_regular_board);
        if (m_config.cheat_duel_board)
          m_core->cheatSet(2, m_isDuelBoard, m_config.cheat_duel_board);
        setState(DR_HOST_STATE_BOARD);
        break;
      }
    }
    break;
  }
  case DR_HOST_STATE_BOARD:
  {
    if (m_itemPending)
    {
      int16_t itemId = 0;
      if (m_config.minigame_id_is_8bit)
      {
        int8_t v = 0;
        reads8(&v, m_config.minigame_id_addr);
        itemId = v;
      }
      else
        reads16(&itemId, m_config.minigame_id_addr);

      if (!m_itemSceneLeft)
      {
        writeu8(m_itemChosenId, m_config.minigame_id_addr);
        if ((uint8_t)scene_id != m_lastBoardScene)
          m_itemSceneLeft = true;
      }
      else if ((uint8_t)scene_id == m_lastBoardScene)
      {
        m_itemPending = false;
        m_itemSceneLeft = false;
        writeForFrames(m_config.minigame_type_addr, &ff, 1, 30);
      }
      break;
    }

    /* Ignore any progression if we are not on the board */
    if ((uint8_t)scene_id != m_lastBoardScene)
      return;

    if (m_isDuelBoard)
    {
      uint8_t slot0 = 0;
      readu8(&slot0, m_config.scene_duel_slot0_addr);
      if (slot0 == 2)
      {
        for (unsigned i = 0; i < m_config.minigame_type_to_dr_size; i++)
          if (m_config.minigame_type_to_dr[i] == DR_MINIGAME_DUEL) { m_MinigameType = i; break; }
        rollCandidates(DR_MINIGAME_DUEL);
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
      }
      break;
    }

    /* Check if the mini-game type value been set */
    uint8_t minigame_type = 0;
    readu8(&minigame_type, m_config.minigame_type_addr);
    if (minigame_type != 0xFF && minigame_type < m_config.minigame_type_to_dr_size)
    {
      dr_minigame_type mg_type = m_config.minigame_type_to_dr[minigame_type];

      if (mg_type == DR_MINIGAME_ITEM)
      {
        m_lastMinigameId = -1;
        writeu8(0xFF, m_config.minigame_id_addr);
        m_itemChosenId = 0x3B + (dr_rand() % 6);
        m_itemPending = true;
        m_itemSceneLeft = false;
        break;
      }
      else if (mg_type != DR_MINIGAME_INVALID)
      {
        m_MinigameType = minigame_type;
        rollCandidates(mg_type);
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
        break;
      }
    }
    writeu8(0xFF, m_config.minigame_type_addr);
    break;
  }
  case DR_HOST_STATE_BEFORE_ROULETTE:
    if ((uint8_t)scene_id != m_lastBoardScene)
    {
      setState(DR_HOST_STATE_BEFORE_BOARD);
      break;
    }
    if (!m_candidates[0].minigame)
      break;
    injectMinigameTitles(m_candidates);
    if (m_config.minigame_id_is_8bit)
    {
      int8_t v = 0;
      reads8(&v, m_config.minigame_id_addr);
      m_lastMinigameId = v;
    }
    else
      reads16(&m_lastMinigameId, m_config.minigame_id_addr);
    setState(DR_HOST_STATE_ROULETTE);
    break;

  case DR_HOST_STATE_ROULETTE:
  {
    if ((uint8_t)scene_id != m_lastBoardScene)
    {
      setState(DR_HOST_STATE_BEFORE_BOARD);
      break;
    }

    if (!m_isDuelBoard)
    {
      uint8_t minigame_type = 0;
      readu8(&minigame_type, m_config.minigame_type_addr);
      if (minigame_type != (uint8_t)m_MinigameType)
      {
        m_lastMinigameId = -1;
        writeu8(0xFF, m_config.minigame_id_addr);
        setState(DR_HOST_STATE_BOARD);
        break;
      }
    }

    int16_t id = 0;
    if (m_config.minigame_id_is_8bit)
    {
      int8_t v = 0;
      reads8(&v, m_config.minigame_id_addr);
      id = v;
    }
    else
      reads16(&id, m_config.minigame_id_addr);

    if (id == m_lastMinigameId)
      break;

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

        /* TODO remove: diagnose MP1 chosen-minigame reading in as 0. */
        {
          int16_t rawId = 0;
          if (m_config.minigame_id_is_8bit)
          {
            int8_t v = 0;
            reads8(&v, m_config.minigame_id_addr);
            rawId = v;
          }
          else
            reads16(&rawId, m_config.minigame_id_addr);
          emit logMessage(DR_LOG_ERROR,
            QString("TODO remove: minigame_id_addr @0x%1 = 0x%2 (latched m_lastMinigameId = 0x%3)")
              .arg(m_config.minigame_id_addr, 0, 16)
              .arg(static_cast<uint16_t>(rawId), 0, 16)
              .arg(static_cast<uint16_t>(m_lastMinigameId), 0, 16));
        }

        onMiniexplainDetected(mg_type, m_lastMinigameId, m_pendingPlayers);
        startMinigame(m_pendingStartIndex);

        m_lastMinigameId = -1;
        writeu8(0xFF, m_config.minigame_id_addr);
        setState(DR_HOST_STATE_MINIGAME);
      }
      break;
    }

    /* Wait for the game to queue its own scene overlays. */
    int16_t count = 0;
    reads16(&count, m_config.scene_stack_count_addr);
    if (!count)
      break;

    dr_minigame_type mg_type = (m_MinigameType < (int)m_config.minigame_type_to_dr_size)
      ? m_config.minigame_type_to_dr[m_MinigameType] : DR_MINIGAME_INVALID;
    int16_t stat = m_config.scene_stat_minigame;
    switch (mg_type)
    {
    case DR_MINIGAME_4P:
    case DR_MINIGAME_1V3:
    case DR_MINIGAME_2V2:
    case DR_MINIGAME_1P:
      m_resultsScene = m_config.scene_miniresults;
      m_resultsModifier = 0;
      stat = m_config.scene_stat_minigame;
      break;
    case DR_MINIGAME_BATTLE:
      m_resultsScene = m_config.scene_miniresults_battle;
      m_resultsModifier = 2;
      stat = m_config.scene_stat_minigame;
      break;
    case DR_MINIGAME_DUEL:
      if (m_isDuelBoard)
      {
        m_resultsScene = m_config.scene_miniresults_duel;
        m_resultsModifier = 0;
        stat = m_config.scene_stat_duel;
      }
      else
      {
        m_resultsScene = 0x75;
        m_resultsModifier = 2;
        stat = m_config.scene_stat_board;
      }
      break;
    case DR_MINIGAME_ITEM:
      m_resultsScene = m_lastBoardScene;
      m_resultsModifier = 2;
      stat = m_config.scene_stat_board;
      break;
    default:
      emit logMessage(DR_LOG_INFO,
        QString("bad mini-game type: 0x%1").arg(m_MinigameType, 2, 16, QChar('0')));
      break;
    }

    /* Read the whole queue, swap the mini-game explanation element for our results
     * screen, and keep everything else. */
    mp64_overlay_t overlays[5] = {};
    for (int i = 0; i < 5; i++)
    {
      const size_t elem = m_config.scene_stack_addr + static_cast<size_t>(i) * 8;
      reads32(&overlays[i].id, elem);
      reads16(&overlays[i].event, elem + 4);
      reads16(&overlays[i].stat, elem + 6);
    }

    const int active = (count < 5) ? count : 5;
    int explainIdx = -1;
    for (int i = 0; i < active && explainIdx < 0; i++)
      for (unsigned e = 0; e < m_config.scene_miniexplain_count; e++)
        if (overlays[i].id == static_cast<int32_t>(m_config.scene_miniexplain[e]))
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
    m_startDelay = 30; // spin before launching the guest (see top of this case)
    break;
  }

  case DR_HOST_STATE_MINIGAME:
  {
    if ((uint8_t)scene_id == m_lastBoardScene)
    {
      m_lastMinigameId = -1;
      writeu8(0xFF, m_config.minigame_id_addr);
      writeForFrames(m_config.minigame_type_addr, &ff, 1, 30);
      /* Back on the board -- roll a fresh pool for the next roulette. This happens
       * inside the lockstepped run(), so every netplay peer rerolls in sync. */
      if (m_MinigameSource)
        m_MinigameSource->rerollMinigames();
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

void MarioPartyN64Host::injectMinigameTitles(const std::array<DrMinigameCandidate, 5> &candidates)
{
  if (!m_config.title_addrs) // no title table configured -> skip injection
    return;

  std::array<std::string, 5> names;
  for (unsigned i = 0; i < 5; i++)
  {
    const char *name =
      (candidates[i].minigame && candidates[i].minigame->name) ? candidates[i].minigame->name : "";
    std::string s;
    for (size_t k = 0, len = strnlen(name, 32); k < len; k++)
    {
      char c = name[k];
      if (isalpha((unsigned char)c) || isdigit((unsigned char)c) || c == ' ')
        s += c;
      else if (c == '.') s += '\x85';
      else if (c == ',') s += '\x82';
      else if (c == '\'') s += '\x5C';
      else if (c == '-') s += '\x3D';
      else if (c == '!') s += '\xC2';
      else if (c == '?') s += '\xC3';
      else if (c == '_') s += '\x86';
      else if (c == '&') s += '\x7E';
      else if (c == '%') s += '\x7C';
      else if (c == ':') s += '\x7B';
    }
    names[i] = s;
  }
  writeMinigameNames(names);
}

bool MarioPartyN64Host::initTitleSlots()
{
  for (unsigned i = 0; i < 5; i++)
  {
    size_t addr = m_config.title_addrs[i];
    uint8_t marker = 0;

    if (readu8(&marker, addr + 1) != DR_OK)
      return false;
    if (marker != 0x0B)
      log(DR_LOG_WARN,
        qPrintable(QString("initTitleSlots[%1]: expected 0x0B at 0x%2, got 0x%3")
                     .arg(i).arg(addr + 1, 8, 16, QChar('0')).arg(marker, 2, 16, QChar('0'))));
    m_titleSlotAddrs[i] = addr;
  }
  return true;
}

void MarioPartyN64Host::writeMinigameNames(const std::array<std::string, 5> &names)
{
  if (!m_titleSlotsValid)
  {
    if (!initTitleSlots())
      return;
    m_titleSlotsValid = true;
  }

  auto w = [&](uint8_t val, size_t addr) {
    writeu8(val, addr);
  };

  for (unsigned i = 0; i < 5; i++)
  {
    size_t addr = m_titleSlotAddrs[i];
    uint8_t nameLen = (uint8_t)std::min(names[i].size(), (size_t)32);

    size_t contentBytes = m_config.title_addrs[i + 1] - addr - 2;
    w(nameLen + m_config.title_len_offset, addr);
    for (size_t j = 0; j < contentBytes; j++)
      w(j < nameLen ? (uint8_t)names[i][j] : 0, addr + 2 + j);
  }
}

void MarioPartyN64Host::rollCandidates(dr_minigame_type type)
{
  if (!m_MinigameSource)
  {
    m_candidates = {};
    return;
  }

  /* Copy out the five candidates for the type the board just chose. The pool is
   * rerolled when we return to the board after a mini-game, not here; the very
   * first query lazily fills it. Titles are injected in BEFORE_ROULETTE. */
  m_candidates = m_MinigameSource->minigameCandidates(type);
}

void MarioPartyN64Host::startMinigame(unsigned index)
{
  if (index >= 5 || !m_candidates[index].guest || !m_candidates[index].minigame)
    return;
  const dr_mp_minigame_t *mg = m_candidates[index].minigame;
  emit logMessage(DR_LOG_INFO,
    QString("chosen mini-game: %1 (0x%2)")
      .arg(mg->name ? mg->name : "(unnamed)")
      .arg(mg->minigame_id, 2, 16, QChar('0')));
  emit minigameRequested(m_candidates[index], m_pendingPlayers);
}

void MarioPartyN64Host::readPlayers(dr_minigame_type type)
{
  m_pendingPlayers = {};

  uint8_t panelColors[4];
  for (unsigned i = 0; i < 4; i++)
    if (readu8(&panelColors[i], m_config.panel_color_addr[i]) != DR_OK)
      panelColors[i] = 0xFF;

  for (unsigned i = 0; i < 4; i++)
  {
    uint8_t chr, ctrl, diff, bot, team;
    if (readu8(&chr,  m_config.character_addr[i])   != DR_OK) continue;
    if (readu8(&ctrl, m_config.controller_addr[i])  != DR_OK) continue;
    if (readu8(&diff, m_config.difficulty_addr[i])  != DR_OK) continue;
    if (readu8(&bot,  m_config.bot_addr[i])          != DR_OK) continue;
    if (readu8(&team, m_config.team_addr[i])         != DR_OK) continue;

    dr_player_t &p = m_pendingPlayers[i];
    p.character   = (chr  < m_config.char_to_dr_size)          ? m_config.char_to_dr[chr]          : DR_CHARACTER_INVALID;
    p.difficulty  = (diff < m_config.diff_to_dr_size)          ? m_config.diff_to_dr[diff]          : DR_DIFFICULTY_INVALID;
    p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
    p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
    p.team_color  = (panelColors[i] < m_config.panel_color_to_dr_size)
                      ? m_config.panel_color_to_dr[panelColors[i]] : DR_TEAM_COLOR_INVALID;
    p.team_id = team;

    if (m_config.coins_addr[i])
    {
      uint16_t coins = 0;
      readu16(&coins, m_config.coins_addr[i]);
      p.coins = coins;
    }
    if (m_config.stars_addr[i])
    {
      uint8_t stars = 0;
      readu8(&stars, m_config.stars_addr[i]);
      p.stars = stars;
    }
  }

  for (unsigned i = 0; i < 4; i++)
  {
    if (type == DR_MINIGAME_1V3)
      m_pendingPlayers[i].team_type = (m_pendingPlayers[i].team_id == 0) ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
    else if (type == DR_MINIGAME_2V2)
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_2V2;
    else if (type == DR_MINIGAME_4P)
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_4P;
    else if (type == DR_MINIGAME_DUEL)
      m_pendingPlayers[i].team_type = (m_pendingPlayers[i].team_id == 0) ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
    else
      m_pendingPlayers[i].team_type = DR_TEAM_TYPE_SOLO;
  }
}

void MarioPartyN64Host::writeBattleCoins()
{
  if (!m_config.battle_addr)
    return;

  int16_t totalCoins = 0;
  reads16(&totalCoins, m_config.battle_addr);

  // Read each player's place (0 = 1st, 1 = 2nd, ...)
  uint16_t places[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
  for (unsigned i = 0; i < 4; i++)
    readu16(&places[i], m_config.result_addr[i]);

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
    if (m_config.bonus_result_addr[i])
      writes16(bonusCoins[i], m_config.bonus_result_addr[i]);
}

void MarioPartyN64Host::setCurrentTurn(unsigned turn)
{
  if (!m_config.turn_current_addr)
  {
    emit logMessage(DR_LOG_WARN, "set turn: this game has no turn counter configured");
    return;
  }
  writeu8(static_cast<uint8_t>(turn), m_config.turn_current_addr);
  emit logMessage(DR_LOG_INFO, QString("set current turn to %1").arg(turn));
}

void MarioPartyN64Host::clearResults()
{
  for (unsigned i = 0; i < 4; i++)
  {
    writes16(0, m_config.result_addr[i]);
    if (m_config.bonus_result_addr[i])
      writes16(0, m_config.bonus_result_addr[i]);
  }
}

void MarioPartyN64Host::writeResults(DrGuest *guest)
{
  for (unsigned i = 0; i < 4; i++)
  {
    auto result = guest->minigameResult(i);

    uint8_t chr = 0;
    readu8(&chr, m_config.character_addr[i]);
    dr_character character =
      (chr < m_config.char_to_dr_size) ? m_config.char_to_dr[chr] : DR_CHARACTER_INVALID;

    emit logMessage(
      DR_LOG_INFO, QString("%1 gets %2 coins")
                     .arg(dr_character_name(character))
                     .arg((result.coins && result.bonus_coins)
                            ? QString("%1+%2").arg(result.coins).arg(result.bonus_coins)
                            : QString::number(result.coins ? result.coins : result.bonus_coins)));

    int16_t coins = static_cast<int16_t>(
      m_config.bonus_result_addr[i] ? result.coins : result.coins + result.bonus_coins);
    writes16(coins, m_config.result_addr[i]);
    if (m_config.bonus_result_addr[i])
      writes16(static_cast<int16_t>(result.bonus_coins), m_config.bonus_result_addr[i]);

    m_mgStarAdd[i] = static_cast<int16_t>(result.coins + result.bonus_coins);
  }

  /* MP3 bandaid: for a normal (non-duel, non-battle) mini-game, snapshot each
   * player's mini-game star and arm a check a second out -- MP3 sometimes doesn't
   * add the winnings to it (see fixup_mg_star / run()). */
  if (m_config.fixup_mg_star && m_resultsScene == m_config.scene_miniresults)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      m_mgStarPrev[i] = 0;
      if (m_config.mg_star_addr[i])
        reads16(&m_mgStarPrev[i], m_config.mg_star_addr[i]);
    }
    m_mgStarFixupCountdown = 60;
  }
  if (m_resultsScene == m_config.scene_miniresults_battle)
  {
    /* Read back the placements the loop just wrote (0 = 1st, 1 = 2nd, ...). A
     * four-way tie has no meaningful pot split, so rather than the battle results
     * scene, send the game to the current board scene with modifier 2. */
    uint16_t places[4];
    for (unsigned i = 0; i < 4; i++)
      readu16(&places[i], m_config.result_addr[i]);
    if (places[0] == places[1] && places[1] == places[2] && places[2] == places[3])
    {
      emit logMessage(DR_LOG_INFO, "battle: four-way tie, returning to board");
      mp64_overlay_t overlays[5] = {};
      overlays[0] = { static_cast<int32_t>(m_lastBoardScene), 2, m_config.scene_stat_board };
      setSceneQueue(overlays, 1);
    }
    else
      writeBattleCoins();
  }
  writeu8(0xFF, m_config.minigame_type_addr);
  m_writing = 30;
  m_lastScene = 0xFF;
}
