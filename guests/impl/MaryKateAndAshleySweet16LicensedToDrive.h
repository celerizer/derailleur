#ifndef DR_GUEST_MARY_KATE_AND_ASHLEY_SWEET_16_LICENSED_TO_DRIVE_H
#define DR_GUEST_MARY_KATE_AND_ASHLEY_SWEET_16_LICENSED_TO_DRIVE_H

#include "../DrGuestGcn.h"

class MaryKateAndAshleySweet16LicensedToDrive : public DrGuestGcn
{
  Q_OBJECT

public:
  MaryKateAndAshleySweet16LicensedToDrive(QObject *parent = nullptr);

  const char *name(void) const override { return "Mary-Kate and Ashley: Sweet 16 - Licensed to Drive"; }
  dr_guest id(void) const override { return DR_GUEST_MARYKATEANDASHLEYSWEET16LICENSEDTODRIVE; }
  const char *state(void) const override { return "sweet16"; }

  const dr_mp_minigame_t *minigames(void) const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run(void) override;
  void onBeforeBoot(const DrGameData &data) override;
  void doApplyGameData(const DrGameData &data) override;

private:
  /// Writes `character`'s portrait texture with `player_character`'s face in the
  /// bottom-right quadrant.
  void writePortrait(unsigned character, dr_character player_character);

  /// Places each player in the in-game slot matching their controller port and
  /// records which slots are human. Returns the number of humans.
  unsigned assignSlots(const DrGameData &data);

  /// Writes the 2v2 or 1v3 team arrangement into the results object.
  void applyTeams(void);

  /// Holds or releases A on the first pad.
  void pressA(bool pressed);

  /// Writes the human count and each slot's character.
  void writePlayers(void);

  /// Routes pads to their slots, writes teams and starts the mini-game.
  void finishSetup(void);

  /* In-game slot of each player */
  unsigned m_playerSlot[4] = { 0, 1, 2, 3 };
  /* Bitmask of in-game slots controlled by humans */
  unsigned m_humanMask = 1;
  unsigned m_humans = 1;
  int m_setupFrames = 0;
  bool m_setupActive = false;
  int m_minigameFrames = 0;
  bool m_finishPending = false;
};

#endif
