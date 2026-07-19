#ifndef DR_GUEST_CORE_DOLPHIN_H
#define DR_GUEST_CORE_DOLPHIN_H

#include "../DrGuest.h"
#include "DolphinGuest.h"
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class CoreDolphin : public DrGuest
{
  Q_OBJECT

public:
  CoreDolphin(const QString &subdir, QObject *parent = nullptr);
  ~CoreDolphin();

  void addGame(DolphinGuest *game);
  void finalizeGames();

  const char *name() const override { return m_name.constData(); }
  dr_guest id() const override { return DR_GUEST_DOLPHIN; }

  /* Deferred like the other guests, but the disc/state swap in doApplyGameData
   * needs the core fully booted, so warm up for bootFrames() after the lazy
   * loadContent, and run doApplyGameData on the GUI thread (it does show(),
   * waitFrames() and processEvents()). finalizeGames writes the m3u the base loads. */
  bool usesWarmup() const override { return false; }
  std::string gamePath() const override { return m_m3uPath.toStdString(); }
  unsigned bootFrames() const override { return 5 * 60; }
  bool applyOnGuiThread() const override { return true; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }
  const dr_mp_minigame_t *minigames() const override { return m_flatList; }
  QList<DrMinigameGroup> minigameGroups() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void doApplyGameData(const DrGameData &data) override;

  void run() override;

private:
  void rebuildFlatList();

  DrRetro *m_retro = nullptr;
  QList<DolphinGuest *> m_games;
  QStringList m_discPaths;
  QList<QPair<DolphinGuest *, const dr_mp_minigame_t *>> m_entries;
  DolphinGuest *m_delegate = nullptr;
  dr_mp_minigame_t *m_flatList = nullptr;
  QString m_m3uPath;
  QByteArray m_name;
  int m_discIndex = -1;
};

#endif
