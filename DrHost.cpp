#include "DrHost.h"
#include "DrGuest.h"

#include <QRetro.h>
#include <QString>
#include <QStringList>

DrHost::DrHost(const DrHostConfig &config, QObject *parent)
  : DrRetro(parent), m_config(config)
{
  m_core     = new QRetro();
  m_ownCore  = true;
  if (!m_core->loadCore(config.core.c_str())) {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }
  if (!m_core->loadContent(config.game.c_str())) {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(config.game.c_str())));
    m_valid = false;
  }

  connect(m_core, &QRetro::frameBegin, this, [this]() {
    if (m_core->frames() <= 120)
      return;

    if (m_clearCountdown > 0 && --m_clearCountdown == 0) {
      for (unsigned i = 0; i < 4; i++) {
        writeu16(0, m_config.result_addr[i]);
        if (m_config.bonus_result_addr[i])
          writeu16(0, m_config.bonus_result_addr[i]);
      }
      if (m_config.battle_addr)
        writeu16(0, m_config.battle_addr);
    }

    if (m_writing > 0) {
      writeu8(m_resultsScene, m_config.scene_addr);
      if (--m_writing == 0) {
        m_lastScene = m_resultsScene;
        m_clearCountdown = 10 * 60;
      }
      return;
    }

    uint8_t val;
    if (readu8(&val, m_config.scene_addr) != DR_OK || val == m_lastScene)
      return;

    emit logMessage(DR_LOG_INFO, QString("N64_SCENE_ADDR: 0x%1").arg(val, 2, 16, QChar('0')));
    m_lastScene = val;

    if (val == m_config.scene_miniexplain) {
      uint8_t teamBytes[4];
      uint8_t panelColors[4];
      for (unsigned i = 0; i < 4; i++) {
        if (readu8(&teamBytes[i], m_config.team_addr[i]) != DR_OK)
          teamBytes[i] = 0xFF;
        if (readu8(&panelColors[i], m_config.panel_color_addr[i]) != DR_OK)
          panelColors[i] = 0xFF;
      }

      // Count occurrences of each distinct team value
      unsigned teamCount[4] = {};
      uint8_t  teamVals[4]  = {};
      unsigned numTeams     = 0;
      for (unsigned i = 0; i < 4; i++) {
        unsigned j = 0;
        for (; j < numTeams; j++)
          if (teamVals[j] == teamBytes[i]) { teamCount[j]++; break; }
        if (j == numTeams) { teamVals[numTeams] = teamBytes[i]; teamCount[numTeams++] = 1; }
      }

      // Determine minigame type.
      // Battle is detected via battle_addr (non-zero coin amount); otherwise by team pattern:
      //   01 02 03 04 = 4P
      //   00 00 01 01 = 2V2
      //   00 01 01 01 = 1V3
      //   00 01 02 02 = Duel (two duelers + pair of spectators)
      dr_minigame_type mgType = DR_MINIGAME_INVALID;
      if (m_config.battle_addr) {
        uint16_t battleCoins = 0;
        readu16(&battleCoins, m_config.battle_addr);
        if (battleCoins > 0)
          mgType = DR_MINIGAME_BATTLE;
      }
      if (mgType == DR_MINIGAME_INVALID) {
        if (numTeams == 4) {
          mgType = DR_MINIGAME_4P;
        } else if (numTeams == 2) {
          unsigned a = teamCount[0], b = teamCount[1];
          bool vals01 = (teamVals[0] | teamVals[1]) == 1; // exactly {0,1}
          if      (a == 2 && b == 2) mgType = DR_MINIGAME_2V2;
          else if ((a == 1 || b == 1) && vals01) mgType = DR_MINIGAME_1V3;
        } else if (numTeams == 3) {
          for (unsigned j = 0; j < 3; j++)
            if (teamCount[j] == 2) { mgType = DR_MINIGAME_DUEL; break; }
        }
      }

      if (mgType == DR_MINIGAME_INVALID) {
        emit logMessage(DR_LOG_WARN,
          QString("skipping minigame: teams = [%1, %2, %3, %4]")
            .arg(teamBytes[0]).arg(teamBytes[1]).arg(teamBytes[2]).arg(teamBytes[3]));
        return;
      }

      // Read N64 memory on the timing thread, then hand off to GUI thread
      dr_player_t players[4] = {};
      bool        playerValid[4] = {};
      for (unsigned i = 0; i < 4; i++) {
        uint8_t chr, ctrl, diff, bot;
        if (readu8(&chr,  m_config.character_addr[i])  == DR_OK &&
            readu8(&ctrl, m_config.controller_addr[i]) == DR_OK &&
            readu8(&diff, m_config.difficulty_addr[i]) == DR_OK &&
            readu8(&bot,  m_config.bot_addr[i])        == DR_OK) {
          dr_player_t &p = players[i];
          if (chr  < m_config.char_to_dr_size) p.character  = m_config.char_to_dr[chr];
          if (diff < m_config.diff_to_dr_size) p.difficulty = m_config.diff_to_dr[diff];
          p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
          p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
          p.team_color   = (panelColors[i] < m_config.panel_color_to_dr_size)
                           ? m_config.panel_color_to_dr[panelColors[i]]
                           : DR_TEAM_COLOR_INVALID;
          for (unsigned j = 0; j < numTeams; j++)
            if (teamVals[j] == teamBytes[i]) { p.team_id = j; break; }
          playerValid[i] = true;
        }
      }

      // Determine team_type for each player, then normalize team_id to a stable convention
      if (mgType == DR_MINIGAME_1V3) {
        unsigned soloTeamId = (teamCount[0] == 1) ? 0u : 1u;
        for (unsigned i = 0; i < 4; i++)
          if (playerValid[i])
            players[i].team_type = (players[i].team_id == soloTeamId)
              ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
        // Normalize: solo=0, group=1 regardless of which N64 player is solo
        for (unsigned i = 0; i < 4; i++)
          if (playerValid[i])
            players[i].team_id = (players[i].team_type == DR_TEAM_TYPE_1V3_SOLO) ? 0u : 1u;
      } else if (mgType == DR_MINIGAME_2V2) {
        for (unsigned i = 0; i < 4; i++)
          if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_2V2;
      } else if (mgType == DR_MINIGAME_4P) {
        for (unsigned i = 0; i < 4; i++)
          if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_4P;
      } else {
        for (unsigned i = 0; i < 4; i++)
          if (playerValid[i]) players[i].team_type = DR_TEAM_TYPE_SOLO;
      }

      emit logMessage(DR_LOG_INFO, QString("starting %1:").arg(dr_minigame_type_name(mgType)));
      for (unsigned i = 0; i < 4; i++) {
        if (!playerValid[i]) continue;
        emit logMessage(DR_LOG_INFO,
          QString("  %1: id %2 color %3")
            .arg(dr_character_name(players[i].character))
            .arg(players[i].team_id)
            .arg(dr_team_color_name(players[i].team_color)));
      }

      std::array<dr_player_t, 4> playersArr    = { players[0], players[1], players[2], players[3] };
      std::array<bool, 4>        playerValidArr = { playerValid[0], playerValid[1], playerValid[2], playerValid[3] };
      m_resultsScene = (mgType == DR_MINIGAME_BATTLE && m_config.scene_miniresults_battle)
                       ? m_config.scene_miniresults_battle
                       : m_config.scene_miniresults;
      QMetaObject::invokeMethod(this, [this, mgType, playersArr, playerValidArr]() {
        emit minigameRequested(mgType, playersArr, playerValidArr);
      }, Qt::QueuedConnection);
      m_writing = 30;

    }
  }, Qt::DirectConnection);
}

void DrHost::writeResults(DrGuest *guest)
{
  for (unsigned i = 0; i < 4; i++) {
    auto result = guest->minigameResult(i);

    uint8_t chr = 0;
    readu8(&chr, m_config.character_addr[i]);
    dr_character character = (chr < m_config.char_to_dr_size)
      ? m_config.char_to_dr[chr] : DR_CHARACTER_INVALID;

    emit logMessage(DR_LOG_INFO,
      QString("%1 gets %2 coins").arg(dr_character_name(character)).arg(
        (result.coins && result.bonus_coins)
          ? QString("%1+%2").arg(result.coins).arg(result.bonus_coins)
          : QString::number(result.coins ? result.coins : result.bonus_coins)));

    writeu16(static_cast<uint16_t>(result.coins), m_config.result_addr[i]);
    if (m_config.bonus_result_addr[i])
      writeu16(static_cast<uint16_t>(result.bonus_coins), m_config.bonus_result_addr[i]);
  }
  m_writing   = 30;
  m_lastScene = 0xFF;
}
