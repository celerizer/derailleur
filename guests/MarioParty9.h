#ifndef DR_GUEST_MARIO_PARTY_9_H
#define DR_GUEST_MARIO_PARTY_9_H

#include "DolphinGuest.h"
#include <string>

/// Which Wii Remote control layout a mini-game expects. Stored (as a plain int)
/// in each MP9 entry's dr_mp_minigame_t::scene_id slot, which MP9 doesn't use as
/// a board scene. 0 = invalid; fill the rest in per mini-game.
enum Mp9Control
{
  MP9_CONTROL_INVALID = 0,
  MP9_CONTROL_UPRIGHT,         ///< Wii Remote held vertically; stick drives d-pad
  MP9_CONTROL_SIDEWAYS,        ///< Held horizontally (NES-style); stick drives d-pad
  MP9_CONTROL_SIDEWAYS_MOTION, ///< Held horizontally with motion; no analog-to-digital
  MP9_CONTROL_POINTER,         ///< IR pointer aiming
};

/// Packs two controls for a 1v3 mini-game whose sides differ: the solo player
/// uses `solo`, the trio uses `team`. A bare MP9_CONTROL_* value (high byte 0)
/// gives everyone that layout.
#define MP9_CONTROL_SPLIT(solo, team) ((solo) | ((team) << 8))

class MarioParty9 : public DolphinGuest
{
  Q_OBJECT

public:
  MarioParty9(QRetro *sharedCore, QObject *parent = nullptr);

  const char *name() const override { return "Mario Party 9"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY9; }

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
  /// Applies control layouts for a mini-game to all players. `control` is a packed
  /// value (see MP9_CONTROL_SPLIT) so the 1v3 solo and trio can differ.
  void applyRemap(int control);

  DrRetro *m_retro = nullptr;
  std::string m_corePath;
  std::string m_discPath;
  std::string m_statePath;

  dr_player_t m_players[4] = {};
  int m_minigameFrames = 0;
  bool m_finishScheduled = false;
  uint32_t m_partyPointsStart = 0;
};

#endif
