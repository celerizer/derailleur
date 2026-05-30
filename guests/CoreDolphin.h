#ifndef DR_GUEST_CORE_DOLPHIN_H
#define DR_GUEST_CORE_DOLPHIN_H

#include "../DrGuest.h"
#include "MarioPartyGcn.h"
#include <QList>
#include <QPair>
#include <QString>

class CoreDolphin : public DrGuest
{
  Q_OBJECT

public:
  CoreDolphin(QObject *parent = nullptr);
  ~CoreDolphin();

  void addGame(MarioPartyGcn *game);
  void finalizeGames();

  const char *name() const override { return "Dolphin"; }
  const dr_mp_minigame_t *minigames() const override { return m_flatList; }
  QList<DrMinigameGroup> minigameGroups() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void doSetMinigame(const dr_mp_minigame_t *minigame) override;
  dr_error doSetPlayerCharacter(unsigned index, dr_character character) override;
  dr_error doSetPlayerControlPort(unsigned index, dr_control_port control_port) override;
  dr_error doSetPlayerControlType(unsigned index, dr_control_type control_type) override;
  dr_error doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty) override;
  dr_error doSetPlayerTeam(
    unsigned index, dr_team_color color, dr_team_type type, unsigned team_id) override;

private:
  void rebuildFlatList();

  QList<MarioPartyGcn *> m_games;
  QList<QPair<MarioPartyGcn *, const dr_mp_minigame_t *>> m_entries;
  MarioPartyGcn *m_delegate = nullptr;
  dr_mp_minigame_t *m_flatList = nullptr;
  QString m_m3uPath;
  int m_discIndex = -1;
};

#endif
