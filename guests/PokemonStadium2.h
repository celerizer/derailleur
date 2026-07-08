#ifndef DR_GUEST_POKEMON_STADIUM_2_H
#define DR_GUEST_POKEMON_STADIUM_2_H

#include "../DrGuest.h"

class PokemonStadium2 : public DrGuest
{
  Q_OBJECT

public:
  PokemonStadium2(QObject *parent = nullptr);
  const char *name() const override { return "Pokemon Stadium 2"; }
  dr_guest id() const override { return DR_GUEST_POKEMONSTADIUM2; }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }
  QWidget *createWidget(QWidget *parent) override;

  /// Content/boot are deferred to the first minigame launch (see
  /// doApplyGameData), so this guest opts out of the startup warmup entirely.
  bool usesWarmup() const override { return false; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

  /// Writes the assigned character's icon into this player's hires textures.
  void writePlayerIcon(unsigned index, dr_character character);

  /// Reads the current minigame's per-player scores and returns a bitmask of the
  /// winning player(s) (most points, except Topsy-Turvy = reaches 5, Pichu's
  /// Power Plant = reaches 80, Rampage Rollout = first to count down to 0).
  unsigned computeWinners();

  /// Per-frame watch for Rampage Rollout: records the order players count their
  /// score down to 0 (recording starts once everyone is at the initial 9).
  void trackRampage();

  DrRetro *m_retro = nullptr;
  QWidget *m_container = nullptr; // window-container QWidget, for post-boot resize
  QString m_gamePath;
  dr_player_t m_players[4] = {};
  int m_slotToIndex[4] = { 0, 1, 2, 3 }; // in-game slot -> board player index
  bool m_started = false;      // content loaded + core booted (first launch)
  bool m_pendingResize = false; // nudge the window's size once the deferred core boots
  int m_stateLoadCountdown = 0; // frames until the state loads (0 = idle); a fresh
                                // boot needs a few frames of init first
  int m_tempoWatchDelay = 0;    // frames to wait after launch before watching tempo
  int m_aPressDelay = 0;        // frames after load before forcing a P1 A press (0 = idle)
  int m_aReleaseDelay = 0;      // frames to hold the forced A before releasing it
  int m_muteFrames = 0;         // frames to keep the core muted after a launch

  // Rampage Rollout finish tracking (see trackRampage).
  bool m_rampageRecording = false;
  int m_rampageCount = 0;              // players that have reached 0 so far
  int m_rampageOrder[4] = { -1, -1, -1, -1 }; // player indices in finish order
};

#endif
