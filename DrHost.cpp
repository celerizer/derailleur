#include "DrHost.h"
#include "DrGuest.h"

#include <QRetro.h>
#include <QString>
#include <QStringList>

DrHost::DrHost(const DrHostConfig &config, QObject *parent)
  : DrRetro(parent)
  , m_config(config)
{
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

  connect(
    m_core, &QRetro::frameBegin, this,
    [this]() {
      if (m_core->frames() <= 120)
        return;

      if (m_writing > 0)
      {
        writeu8(m_resultsScene, m_config.scene_addr);
        if (--m_writing == 0)
          m_lastScene = m_resultsScene;
        return;
      }

      uint8_t prevMinigameType = m_lastMinigameType;
      if (m_config.minigame_type_addr)
      {
        uint8_t raw = 0;
        if (readu8(&raw, m_config.minigame_type_addr) == DR_OK)
          m_lastMinigameType = raw;
      }

      if (m_config.minigame_id_addr)
      {
        int16_t id = 0;
        dr_error err = m_config.minigame_id_is_8bit
          ? ([&]{ int8_t v = 0; dr_error e = reads8(&v, m_config.minigame_id_addr); id = v; return e; }())
          : reads16(&id, m_config.minigame_id_addr);
        if (err == DR_OK && id != m_lastMinigameId)
        {
          emit logMessage(DR_LOG_INFO,
            QString("N64_MINIGAME_ID: 0x%1").arg((uint16_t)id, 2, 16, QChar('0')));
          m_lastMinigameId = id;
        }
      }

      uint8_t val;
      if (readu8(&val, m_config.scene_addr) != DR_OK || val == m_lastScene)
        return;

      emit logMessage(DR_LOG_INFO, QString("N64_SCENE_ADDR: 0x%1").arg(val, 2, 16, QChar('0')));
      m_lastScene = val;

      for (unsigned r = 0; r < m_config.scene_board_range_count; r++)
      {
        if (val >= m_config.scene_board_ranges[r].min && val <= m_config.scene_board_ranges[r].max)
        {
          m_lastBoardScene = val;
          break;
        }
      }

      bool isMiniexplain = false;
      for (unsigned i = 0; i < m_config.scene_miniexplain_count; i++)
        if (val == m_config.scene_miniexplain[i]) { isMiniexplain = true; break; }
      if (isMiniexplain)
      {
        if (m_config.minigame_id_addr && m_config.minigame_blacklist_count)
        {
          int16_t mgId = 0;
          if (m_config.minigame_id_is_8bit)
          { int8_t v = 0; reads8(&v, m_config.minigame_id_addr); mgId = v; }
          else
            reads16(&mgId, m_config.minigame_id_addr);
          for (unsigned i = 0; i < m_config.minigame_blacklist_count; i++)
          {
            if ((uint8_t)mgId == m_config.minigame_blacklist[i])
            {
              emit logMessage(DR_LOG_WARN,
                QString("skipping blacklisted minigame 0x%1").arg(mgId, 2, 16, QChar('0')));
              return;
            }
          }
        }

        dr_minigame_type mgType = DR_MINIGAME_INVALID;
        if (m_config.minigame_type_addr && prevMinigameType < m_config.minigame_type_to_dr_size)
          mgType = m_config.minigame_type_to_dr[prevMinigameType];

        if (m_config.scene_duel_board_range.max &&
            m_lastBoardScene >= m_config.scene_duel_board_range.min &&
            m_lastBoardScene <= m_config.scene_duel_board_range.max)
          mgType = DR_MINIGAME_DUEL;

        emit logMessage(DR_LOG_INFO, QString("N64_MINIGAME_TYPE: 0x%1 (%2)")
                                       .arg(prevMinigameType, 2, 16, QChar('0'))
                                       .arg(dr_minigame_type_name(mgType)));

        if (mgType == DR_MINIGAME_INVALID || mgType == DR_MINIGAME_ITEM)
        {
          emit logMessage(DR_LOG_WARN, "skipping minigame: unsupported type");
          return;
        }

        uint8_t panelColors[4];
        for (unsigned i = 0; i < 4; i++)
        {
          if (readu8(&panelColors[i], m_config.panel_color_addr[i]) != DR_OK)
            panelColors[i] = 0xFF;
        }

        dr_player_t players[4] = {};
        bool playerValid[4] = {};
        for (unsigned i = 0; i < 4; i++)
        {
          uint8_t chr, ctrl, diff, bot, team;
          if (readu8(&chr, m_config.character_addr[i]) == DR_OK &&
              readu8(&ctrl, m_config.controller_addr[i]) == DR_OK &&
              readu8(&diff, m_config.difficulty_addr[i]) == DR_OK &&
              readu8(&bot, m_config.bot_addr[i]) == DR_OK &&
              readu8(&team, m_config.team_addr[i]) == DR_OK)
          {
            dr_player_t &p = players[i];
            if (chr < m_config.char_to_dr_size)
              p.character = m_config.char_to_dr[chr];
            if (diff < m_config.diff_to_dr_size)
              p.difficulty = m_config.diff_to_dr[diff];
            p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
            p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
            p.team_color = (panelColors[i] < m_config.panel_color_to_dr_size)
                             ? m_config.panel_color_to_dr[panelColors[i]]
                             : DR_TEAM_COLOR_INVALID;
            p.team_id = team;
            playerValid[i] = true;
          }
        }

        if (mgType == DR_MINIGAME_1V3)
        {
          for (unsigned i = 0; i < 4; i++)
            if (playerValid[i])
              players[i].team_type =
                (players[i].team_id == 0) ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
        }
        else if (mgType == DR_MINIGAME_2V2)
        {
          for (unsigned i = 0; i < 4; i++)
            if (playerValid[i])
              players[i].team_type = DR_TEAM_TYPE_2V2;
        }
        else if (mgType == DR_MINIGAME_4P)
        {
          for (unsigned i = 0; i < 4; i++)
            if (playerValid[i])
              players[i].team_type = DR_TEAM_TYPE_4P;
        }
        else
        {
          for (unsigned i = 0; i < 4; i++)
            if (playerValid[i])
              players[i].team_type = DR_TEAM_TYPE_SOLO;
        }

        emit logMessage(DR_LOG_INFO, QString("starting %1:").arg(dr_minigame_type_name(mgType)));
        for (unsigned i = 0; i < 4; i++)
        {
          if (!playerValid[i])
            continue;
          emit logMessage(DR_LOG_INFO, QString("  %1: id %2 color %3")
                                         .arg(dr_character_name(players[i].character))
                                         .arg(players[i].team_id)
                                         .arg(dr_team_color_name(players[i].team_color)));
        }

        std::array<dr_player_t, 4> playersArr = { players[0], players[1], players[2], players[3] };
        std::array<bool, 4> playerValidArr = { playerValid[0], playerValid[1], playerValid[2],
          playerValid[3] };
        if (mgType == DR_MINIGAME_BATTLE && m_config.scene_miniresults_battle)
          m_resultsScene = m_config.scene_miniresults_battle;
        else if (mgType == DR_MINIGAME_DUEL && m_lastBoardScene)
          m_resultsScene = m_lastBoardScene;
        else
          m_resultsScene = m_config.scene_miniresults;
        for (unsigned i = 0; i < 4; i++)
        {
          writeu16(0, m_config.result_addr[i]);
          if (m_config.bonus_result_addr[i])
            writeu16(0, m_config.bonus_result_addr[i]);
        }

        QMetaObject::invokeMethod(
          this,
          [this, mgType, playersArr, playerValidArr]() {
            emit minigameRequested(mgType, playersArr, playerValidArr);
          },
          Qt::QueuedConnection);
        m_writing = 30;
      }
    },
    Qt::DirectConnection);
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
      bonusCoins[qualifying[rand() % numQ]] += remainder;
  }

  for (unsigned i = 0; i < 4; i++)
    if (m_config.bonus_result_addr[i])
      writes16(bonusCoins[i], m_config.bonus_result_addr[i]);
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

    writes16(static_cast<int16_t>(result.coins), m_config.result_addr[i]);
    if (m_config.bonus_result_addr[i])
      writes16(static_cast<int16_t>(result.bonus_coins), m_config.bonus_result_addr[i]);
  }
  if (m_resultsScene == m_config.scene_miniresults_battle)
    writeBattleCoins();
  m_writing = 30;
  m_lastScene = 0xFF;
}
