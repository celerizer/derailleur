#ifndef DR_GUEST_POKEMON_STADIUM_H
#define DR_GUEST_POKEMON_STADIUM_H

#include "../DrGuest.h"

class PokemonStadium : public DrGuest
{
  Q_OBJECT

public:
  PokemonStadium(QObject *parent = nullptr);
  const char *name() const override { return "Pokemon Stadium"; }
  dr_guest id() const override { return DR_GUEST_POKEMONSTADIUM; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Pokemon Stadium (USA) (Rev 2).z64"; }
  const char *state() const override { return "pokemonstadium"; }

  /// Content/boot are deferred to the first minigame launch, so this guest opts
  /// out of the startup warmup; the base boots it on demand (see applyGameData).
  bool usesWarmup() const override { return false; }
  unsigned bootFrames() const override { return 16; }

  dr_minigame_result_t minigameResult(unsigned index) override;
  const dr_mp_minigame_t *minigames() const override;

private:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  void onBeforeBoot(const DrGameData &data) override;

  /// Bitmask of the board players who won the round (wins counter went up).
  unsigned computeWinners(void);

  /// Writes the assigned character's icon into this slot's hires textures.
  void writePlayerIcon(unsigned slot, dr_character character);

  int m_slotToIndex[4] = { 0, 1, 2, 3 }; // in-game slot -> board player index
  int m_resultsWatchDelay = 0; // frames to wait after launch before watching for results
  int m_resultsFrames = 0;      // frames the round result has been on screen
  bool m_finishArmed = false;   // finish already scheduled for this round
  int m_aPressDelay = 0;     // frames after load before forcing a P1 A press (0 = idle)
  int m_aReleaseDelay = 0;   // frames to hold the forced A before releasing it
  int m_muteFrames = 0;      // frames to keep the core muted after a launch
};

#endif
