#ifndef DR_HOST_MARIO_PARTY_ADVANCE_H
#define DR_HOST_MARIO_PARTY_ADVANCE_H

#include "../DrHostRetro.h"

#include <array>

typedef enum
{
  DR_MPA_HOST_STATE_INVALID = 0,
  DR_MPA_HOST_STATE_BOARD,
  DR_MPA_HOST_STATE_ROULETTE,
  DR_MPA_HOST_STATE_MINIGAME,

  DR_MPA_HOST_STATE_SIZE
} dr_mpa_host_state;

/* Shroom City with one human on P1. The Mushroom Challenge roulette's three
 * slots and the mini-game space's announced game can be any mini-game type;
 * either is cleared when P1 wins. */
class MarioPartyAdvanceHost : public DrHostRetro
{
  Q_OBJECT

public:
  explicit MarioPartyAdvanceHost(QObject *parent = nullptr);

  dr_game game(void) const override { return DR_GAME_MARIOPARTYADVANCE; }

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;

  bool readPlayerSetup(DrPlayerArray &players) override;

private:
  void run(void);

  void applyHooks(void);

  /* Base of the board context, or 0 when the Shroom City board isn't running. */
  size_t boardContext(void);

  /* Roll a type and candidate for each of `slot_count` slots and stamp their titles:
   * three for the roulette, one for a mini-game space. */
  void rollSlots(unsigned slot_count);
  void stampTitle(unsigned slot, const char *name);

  void startMinigame(void);
  void readPlayers(DrPlayerArray &players, dr_minigame_type type);

  /* Whether board player 0 won, judged from the guest's results. */
  bool playerOneWon(DrGuest *guest);

  /* Hand the result to the board and send it on to settle the challenge. */
  void resumeBoard(bool cleared);

  void setState(dr_mpa_host_state state);

  dr_mpa_host_state m_state = DR_MPA_HOST_STATE_INVALID;

  /* The board mode that rolled: the roulette or a mini-game space. */
  uint8_t m_mode = 0;

  /* What each roulette slot holds, kept until the result is written. */
  std::array<DrMinigameCandidate, 3> m_slots = {};
  std::array<dr_minigame_type, 3> m_slotTypes = {};
  int m_slot = -1;

  /* The players the running mini-game was launched with. */
  DrPlayerArray m_players = {};

  /* Frames run since a mini-game was requested, to recover from a launch that
   * never happened. */
  unsigned m_waitFrames = 0;
};

#endif
