#include "DrHost.h"
#include "DrGuest.h"

#include <QRandomGenerator>
#include <QRetro.h>
#include <QString>
#include <algorithm>
#include <cctype>
#include <cstring>

void DrHost::run(void)
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
    m_lastScene = (uint8_t)scene_id;
  }

  switch (m_State)
  {
  case DR_HOST_STATE_INVALID:
    if (m_core->frames() <= 120)
      return;
    else
      setState(DR_HOST_STATE_BEFORE_BOARD);
    break;
  case DR_HOST_STATE_BEFORE_BOARD:
  {
    writeu32(0, m_config.scene_trampoline_addr);

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
        m_candidates = {};
        emit candidatesNeeded(DR_MINIGAME_DUEL);
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
        writeu32(0, m_config.scene_trampoline_addr);
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
        m_candidates = {};
        emit candidatesNeeded(mg_type);
        setState(DR_HOST_STATE_BEFORE_ROULETTE);
        break;
      }
    }
    writeu8(0xFF, m_config.minigame_type_addr);
    writeu32(0, m_config.scene_trampoline_addr);
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

    dr_minigame_type mg_type = (m_MinigameType < (int)m_config.minigame_type_to_dr_size)
      ? m_config.minigame_type_to_dr[m_MinigameType]
      : DR_MINIGAME_INVALID;
    switch (mg_type)
    {
    case DR_MINIGAME_4P:
    case DR_MINIGAME_1V3:
    case DR_MINIGAME_2V2:
    case DR_MINIGAME_1P:
      m_resultsScene = m_config.scene_miniresults;
      m_resultsModifier = 0;
      break;
    case DR_MINIGAME_BATTLE:
      m_resultsScene = m_config.scene_miniresults_battle;
      m_resultsModifier = 2;
      break;
    case DR_MINIGAME_DUEL:
    {
      if (m_isDuelBoard)
      {
        m_resultsScene = 0x5a;
        m_resultsModifier = 0;
      }
      else
      {
        m_resultsScene = 0x75;
        m_resultsModifier = 2;
      }
      break;
    }
    case DR_MINIGAME_ITEM:
      m_resultsScene = m_lastBoardScene;
      m_resultsModifier = 2;
      break;
    default:
      emit logMessage(DR_LOG_INFO,
        QString("bad mini-game type: 0x%1").arg(m_MinigameType, 2, 16, QChar('0')));
      break;
    }

    emit logMessage(DR_LOG_INFO,
      QString("next_scene: 0x%1 modifier 0x%2")
        .arg(m_resultsScene, 2, 16, QChar('0'))
        .arg(m_resultsModifier, 2, 16, QChar('0')));
    writeu32(((uint32_t)m_resultsScene << 16) | (uint16_t)m_resultsModifier, m_config.scene_trampoline_addr);
    m_afterRouletteSceneLeft = false;
    setState(DR_HOST_STATE_AFTER_ROULETTE);
    break;
  }

  case DR_HOST_STATE_AFTER_ROULETTE:
  {
    if (!m_afterRouletteSceneLeft)
    {
      if ((uint8_t)scene_id != m_lastBoardScene)
        m_afterRouletteSceneLeft = true;
      else
        break;
    }
    if ((uint8_t)scene_id != m_resultsScene)
      break;

    dr_minigame_type mg_type = (m_MinigameType < (int)m_config.minigame_type_to_dr_size)
      ? m_config.minigame_type_to_dr[m_MinigameType] : DR_MINIGAME_INVALID;
    readPlayers(mg_type);
    onMiniexplainDetected(mg_type, m_lastMinigameId, m_pendingPlayers);
    startMinigame(m_pendingStartIndex);

    emit logMessage(DR_LOG_INFO,
      QString("board_scene: 0x%1 modifier 1").arg(m_lastBoardScene, 2, 16, QChar('0')));
    if (m_config.turn_total_addr)
    {
      uint8_t totalTurns = 0, currentTurns = 0;
      readu8(&totalTurns,   m_config.turn_total_addr);
      readu8(&currentTurns, m_config.turn_current_addr);
      if (currentTurns > totalTurns)
      {
        /* Game over - force the board's results scene next instead of a new turn */
        emit logMessage(DR_LOG_INFO,
          QString("entering board results (turn %1/%2)").arg(currentTurns).arg(totalTurns));
        writeu32(m_config.scene_board_results
                   ? (((uint32_t)m_config.scene_board_results << 16) | 1) : 0,
          m_config.scene_trampoline_addr);
        setState(DR_HOST_STATE_INVALID);
        break;
      }
      if (totalTurns - currentTurns == 4)
      {
        /* Last 5 turns - force that event scene once with modifier 1 */
        if (!m_lastFiveTriggered)
        {
          m_lastFiveTriggered = true;
          emit logMessage(DR_LOG_INFO,
            QString("entering last 5 turns (turn %1/%2)").arg(currentTurns).arg(totalTurns));
          writeu32(m_config.scene_last_five_turns
                     ? (((uint32_t)m_config.scene_last_five_turns << 16) | 1) : 0,
            m_config.scene_trampoline_addr);
          setState(DR_HOST_STATE_INVALID);
          break;
        }
      }
      else
      {
        /* Off the trigger turn - re-arm so a later game (or wrap) fires again. */
        m_lastFiveTriggered = false;
      }
    }
    writeu32(((uint32_t)m_lastBoardScene << 16) | 1, m_config.scene_trampoline_addr);
    m_lastMinigameId = -1;
    writeu8(0xFF, m_config.minigame_id_addr);
    setState(DR_HOST_STATE_MINIGAME);
    break;
  }

  case DR_HOST_STATE_MINIGAME:
  {
    if ((uint8_t)scene_id == m_lastBoardScene)
    {
      m_lastMinigameId = -1;
      writeu8(0xFF, m_config.minigame_id_addr);
      writeForFrames(m_config.minigame_type_addr, &ff, 1, 30);
      setState(DR_HOST_STATE_BOARD);
    }
    break;
  }
  case DR_HOST_STATE_SIZE:
    break;
  }
}

DrHost::DrHost(const DrHostConfig &config, QObject *parent)
  : DrRetro(parent)
  , m_config(config)
{
  qRegisterMetaType<DrMinigameCandidate>();
  qRegisterMetaType<DrPlayerArray>();
  qRegisterMetaType<dr_minigame_type>();

  m_core = new QRetro();
  m_ownCore = true;
  if (!m_core->loadCore(config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }
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

void DrHost::injectMinigameTitles(const std::array<DrMinigameCandidate, 5> &candidates)
{
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

bool DrHost::initTitleSlots()
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

void DrHost::writeMinigameNames(const std::array<std::string, 5> &names)
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

void DrHost::setCandidates(std::array<DrMinigameCandidate, 5> candidates)
{
  m_candidates = candidates;
  injectMinigameTitles(candidates);
}

void DrHost::startMinigame(unsigned index)
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

void DrHost::readPlayers(dr_minigame_type type)
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

void DrHost::writeBattleCoins()
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

void DrHost::setCurrentTurn(unsigned turn)
{
  if (!m_config.turn_current_addr)
  {
    emit logMessage(DR_LOG_WARN, "set turn: this game has no turn counter configured");
    return;
  }
  writeu8(static_cast<uint8_t>(turn), m_config.turn_current_addr);
  emit logMessage(DR_LOG_INFO, QString("set current turn to %1").arg(turn));
}

void DrHost::clearResults()
{
  for (unsigned i = 0; i < 4; i++)
  {
    writes16(0, m_config.result_addr[i]);
    if (m_config.bonus_result_addr[i])
      writes16(0, m_config.bonus_result_addr[i]);
  }
}

void DrHost::writeResults(DrGuest *guest)
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
      writeu32(((uint32_t)m_lastBoardScene << 16) | 2, m_config.scene_trampoline_addr);
    }
    else
      writeBattleCoins();
  }
  writeu8(0xFF, m_config.minigame_type_addr);
  m_writing = 30;
  m_lastScene = 0xFF;
}
