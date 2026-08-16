#ifndef DR_MARIO_PARTY_GCN_HOST_H
#define DR_MARIO_PARTY_GCN_HOST_H

#include "../DrHost.h"
#include <string>

typedef enum
{
  DR_GCN_HOST_STATE_INVALID = 0,
  DR_GCN_HOST_STATE_BEFORE_BOARD,
  DR_GCN_HOST_STATE_BOARD,
  DR_GCN_HOST_STATE_BEFORE_ROULETTE,
  DR_GCN_HOST_STATE_ROULETTE,
  DR_GCN_HOST_STATE_AFTER_ROULETTE,
  DR_GCN_HOST_STATE_MINIGAME,

  DR_GCN_HOST_STATE_SIZE
} dr_gcn_host_state;

struct DrGcnHostConfig
{
  std::string core;
  std::string game;

  struct
  {
    dr_value_t scene;                 // current scene id
    dr_value_t character[4];          // per-slot character id
    dr_value_t controller[4];         // per-slot controller port
    dr_value_t difficulty[4];         // per-slot cpu difficulty
    dr_value_t team[4];               // per-slot team id
    dr_value_t bot[4];                // per-slot cpu flag
    dr_value_t result[4];             // per-slot finishing place
    dr_value_t bonus_result[4];       // per-slot bonus coins
    dr_value_t panel_color[4];        // per-slot panel color
    dr_value_t coins[4];              // per-slot current coins
    dr_value_t stars[4];              // per-slot current stars
    dr_value_t mg_star[4];            // per-slot mini-game star
    dr_value_t minigame_title_color;  // color array the title trampoline copies into
    dr_value_t battle;                // total battle coins
    dr_value_t minigame_type;         // mini-game type byte
    dr_value_t minigame_id;           // chosen mini-game id (width per game)
    dr_value_t title_block;           // base of the injected glyph block
    dr_value_t title_color;           // base of the injected color table
    dr_value_t title_type_duel;       // duel-overlay type byte
    dr_value_t scene_stack;           // base of the native scene stack
    dr_value_t scene_stack_count;     // scene-stack element count
    dr_value_t turn_total;            // total turn count
    dr_value_t turn_current;          // current turn count

    /// The value that tracks whose turn it is on the board
    dr_value_t turn_owner;

    /// The value that tracks which space the current player is standing on
    dr_value_t space_index;
  } values;
};

/// Abstract base for a GameCube Mario Party host. Owns a Dolphin core and runs a
/// board/roulette state machine, mirroring MarioPartyN64Host. This is a skeleton:
/// it holds the state and the memory map but does not drive any transitions yet.
/// Concrete games (MarioParty4Host) supply a DrGcnHostConfig and game().
class MarioPartyGcnHost : public DrHost
{
  Q_OBJECT

public:
  explicit MarioPartyGcnHost(const DrGcnHostConfig &config, QObject *parent = nullptr);

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;

  void run(void);

protected:
  DrGcnHostConfig m_config;
  dr_gcn_host_state m_State = DR_GCN_HOST_STATE_INVALID;
};

#endif
