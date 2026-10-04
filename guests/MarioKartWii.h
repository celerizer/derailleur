#ifndef DR_GUEST_MARIO_KART_WII_H
#define DR_GUEST_MARIO_KART_WII_H

#include "DolphinGuest.h"
#include <string>

class MarioKartWii : public DolphinGuest
{
  Q_OBJECT

public:
  MarioKartWii(QRetro *sharedCore, QObject *parent = nullptr);

  const char *name() const override { return "Mario Kart Retro Rewind"; }
  dr_guest id() const override { return DR_GUEST_MARIOKARTWII; }

  std::string corePath() const override { return m_corePath; }
  std::string discPath() const override { return m_discPath; }
  std::string statePath() const override { return m_statePath; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  /// Writes menu player `slot`'s character, costume, type, and vehicle `group`
  /// (a small/medium/large triple) sized to the character.
  void writePlayer(unsigned slot, dr_character character, bool cpu, unsigned group);
  /// Binds race players 1-4 to Classic Controllers 1-4.
  void bindClassicControllers(void);
  /// Turns off saving so the race doesn't stop on a save file error.
  void disableSaving(void);
  /// Patches Retro Rewind so CPUs wear their character's costume like humans.
  void patchCpuCostumes(void);
  /// Address of race player `player`'s RaceManagerPlayer, or 0 outside a race.
  size_t racePlayerAddr(unsigned player);

  DrRetro *m_retro = nullptr;
  std::string m_corePath;
  std::string m_discPath;
  std::string m_statePath;
  int m_minigameFrames = 0;
  /// Slot of the first player to finish, or -1 while the race is on.
  int m_winner = -1;
};

#endif
