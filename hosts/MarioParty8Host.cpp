#include "MarioParty8Host.h"

#include <asm/mp8.h>

#include <QRetroDirectories.h>

static dr_character mp8_char_to_dr(unsigned chr)
{
  switch (chr)
  {
  case 0x00:
    return DR_CHARACTER_MARIO;
  case 0x01:
    return DR_CHARACTER_LUIGI;
  case 0x02:
    return DR_CHARACTER_PEACH;
  case 0x03:
    return DR_CHARACTER_YOSHI;
  case 0x04:
    return DR_CHARACTER_WARIO;
  case 0x05:
    return DR_CHARACTER_DAISY;
  case 0x06:
    return DR_CHARACTER_WALUIGI;
  case 0x07:
    return DR_CHARACTER_TOAD;
  case 0x08:
    return DR_CHARACTER_BOO;
  case 0x09:
    return DR_CHARACTER_TOADETTE;
  case 0x0A:
    return DR_CHARACTER_BIRDO;
  case 0x0B:
    return DR_CHARACTER_DRY_BONES;
  case 0x0C:
    return DR_CHARACTER_BLOOPER;
  case 0x0D:
    return DR_CHARACTER_HAMMER_BRO;
  /* 0x0E-0x11 are the four Mii slots, which have no dr_character. */
  }

  return DR_CHARACTER_INVALID;
}

static const dr_minigame_type MP8_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P,      /* 0 */
  DR_MINIGAME_1V3,     /* 1 */
  DR_MINIGAME_2V2,     /* 2 */
  DR_MINIGAME_BATTLE,  /* 3 */
  DR_MINIGAME_INVALID, /* bowser? 1 mini */
  DR_MINIGAME_INVALID, /* 0 minis */
  DR_MINIGAME_DUEL,    /* 6 */
  DR_MINIGAME_INVALID, /* 0 minis */
  DR_MINIGAME_1P,      /* 3 minis */
  DR_MINIGAME_INVALID, /* 0 minis */
  DR_MINIGAME_INVALID, /* 0 minis */
  DR_MINIGAME_INVALID, /* 7 empty minis? unused? */
};

static const dr_scene_name_t MP8_SCENE_NAMES[] =
{
  { 0x04, "Star Carnival", false },
  { 0x05, "Board results", false },
  { 0x06, "Star Battle", false },

  { 0x0a, "Free-play", false },

  { 0x10, "DK's Treetop Temple", false },
  { 0x11, "Goomba's Booty Boardwalk", false },
  { 0x12, "King Boo's Haunted Hideaway", false },
  { 0x13, "Shy Guy's Perplex Express", false },
  { 0x14, "Koopa's Tycoon Town", false },
  { 0x15, "Bowser's Warped Orbit", false },

  { 0x16, "Mini-Game explanation", true },

  { 0x17, "Speedy Graffiti", false },
  { 0x18, "Swing Kings", false },
  { 0x19, "Water Ski Spree", false },
  { 0x1a, "Punch-a-Bunch", false },
  { 0x1d, "Mosh-Pit Playroom", false },
  { 0x1e, "Mario Matrix", false },
  { 0x1f, "??? - Hammer de Pokari", false },
  { 0x20, "Grabby Gridiron", false },
  { 0x21, "Lava or Leave 'Em", false },
  { 0x22, "Kartastrophe", false },
  { 0x23, "??? - Ribbon Game", false },
  { 0x24, "Aim of the Game", false },
  { 0x25, "Rudder Madness", false },
  { 0x26, "Gun the Runner", false },
  { 0x27, "Grabbin' Gold", false },
  { 0x28, "Power Trip", false },
  { 0x29, "Bob-ombs Away", false },
  { 0x2a, "Swervin' Skies", false },
  { 0x2b, "Picture Perfect", false },
  { 0x2c, "Snow Way Out", false },
  { 0x2d, "Thrash 'n' Crash", false },
  { 0x2e, "Chump Rope", false },
  { 0x2f, "Sick and Twisted", false },
  { 0x30, "Bumper Balloons", false },
  { 0x31, "Rowed to Victory", false },
  { 0x32, "Winner or Dinner", false },
  { 0x33, "Paint Misbehavin'", false },
  { 0x34, "Sugar Rush", false },
  { 0x35, "King of the Thrill", false },
  { 0x37, "Lean, Mean Ravine", false },
  { 0x38, "Boo-ting Gallery", false },
  { 0x39, "Crops 'n' Robbers", false },
  { 0x3a, "In the Nick of Time", false },
  { 0x3b, "Cut from the Team", false },
  { 0x3c, "Snipe for the Picking", false },
  { 0x3d, "Saucer Swarm", false },
  { 0x3e, "Glacial Meltdown", false },
  { 0x41, "Wing and a Scare", false },
  { 0x42, "Lob to Rob", false },
  { 0x44, "Cosmic Slalom", false },
  { 0x45, "Lava Lobbers", false },
  { 0x46, "Loco Motives", false },
  { 0x47, "Specter Inspector", false },
  { 0x48, "Frozen Assets", false },
  { 0x4a, "Surf's Way Up", false },
  { 0x4b, "??? - Bull Riding", false },
  { 0x4c, "Balancing Act", false },
  { 0x4d, "Ion the Prize", false },
  { 0x4e, "You're the Bob-omb", false },
  { 0x4f, "Scooter Pursuit", false },
  { 0x50, "Cardiators", false },
  { 0x53, "Table Menace", false },
  { 0x54, "Flagging Rights", false },
  { 0x55, "Trial by Tile", false },
  { 0x56, "Star Carnival Bowling", false },
  { 0x57, "Puzzle Pillars", false },
  { 0x58, "Canyon Cruisers", false },
  { 0x59, "Chomping Frenzy", false },
  { 0x5a, "Settle It in Court", false },
  { 0x5b, "Moped Mayhem", false },
  { 0x5c, "Flip the Chimp", false },
  { 0x5d, "Pour to Score", false },
  { 0x5e, "Fruit Picker", false },
  { 0x5f, "Stampede", false },
  { 0x60, "Superstar Showdown", false },
  { 0x61, "Alpine Assault", false },
  { 0x62, "Treacherous Tightrope", false },

  { -1, nullptr },
};

/**
 * Battle mini-game payouts
 *
 * Each row is one pattern the four players can finish in, written as their
 * placements in finishing order (0 = 1st), with the cut of the pot each of them
 * takes in hundredths. A pattern is matched after the placements are compressed
 * to dense ranks, so any four placements land on exactly one row. Rows 1 and 7
 * don't add up to the whole pot; whatever is left over is handed out at random.
 */
struct mp8_battle_payout
{
  uint8_t placements[4];
  uint8_t share[4];
  const char *name;
};

static const struct mp8_battle_payout MP8_BATTLE_PAYOUTS[] = {
  { { 0, 0, 0, 0 }, { 25, 25, 25, 25 }, "4-way tie" },
  { { 0, 0, 0, 1 }, { 33, 33, 33,  0 }, "3-way tie for 1st" },
  { { 0, 0, 1, 1 }, { 50, 50,  0,  0 }, "tie for 1st, tie for 3rd" },
  { { 0, 0, 1, 2 }, { 50, 50,  0,  0 }, "tie for 1st" },
  { { 0, 1, 1, 1 }, { 70, 10, 10, 10 }, "3-way tie for 2nd" },
  { { 0, 1, 1, 2 }, { 70, 15, 15,  0 }, "tie for 2nd" },
  { { 0, 1, 2, 2 }, { 70, 30,  0,  0 }, "tie for 3rd" },
  { { 0, 1, 2, 3 }, { 70, 25,  5,  0 }, "clean finish" },
};

/**
 * Board roulette hooks
 *
 * MP8 compiles its board engine into each board overlay rather than main.dol,
 * so the forced-mini-game-list hook ships once per board and only the loaded
 * board's row may be written -- see the block comment in asm/mp8.h. Scene ids
 * are what tells us which overlay is up.
 *
 * @todo MP8_HOOK_W10's board has no scene id yet, so it isn't listed.
 */
typedef struct
{
  int32_t scene;
  const unsigned int (*hooks)[3];
  const char *name;
} mp8_board_hook_t;

static const mp8_board_hook_t MP8_BOARD_HOOKS[] = {
  { 0x10, MP8_HOOK_W01, "w01" },
  { 0x11, MP8_HOOK_W02, "w02" },
  { 0x12, MP8_HOOK_W03, "w03" },
  { 0x13, MP8_HOOK_W04, "w04" },
  { 0x14, MP8_HOOK_W05, "w05" },
  { 0x15, MP8_HOOK_W06, "w06" },

  { -1, nullptr, nullptr },
};

/* Frames to let a board overlay settle before touching its hook sites: the
 * scene id turns over while the overlay is still being loaded and relocated,
 * so a patch written on the first matching frame gets loaded straight over. */
static const int MP8_HOOK_DELAY_FRAMES = 5 * 60;

static const mp8_board_hook_t *mp8_board_hook(int32_t scene)
{
  const mp8_board_hook_t *board;

  for (board = MP8_BOARD_HOOKS; board->scene != -1; board++)
    if (board->scene == scene)
      return board;

  return nullptr;
}

/* Use 4:3 mode for Mario Party 8 board */
static const dr_core_option_t MP8_CORE_OPTIONS[] = {
  { "dolphin_widescreen", "disabled" },

  { nullptr, nullptr },
};

static DrGcnHostConfig makeConfig()
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 8 (USA, Asia) (Rev 2).rvz").toStdString();

  config.options = MP8_CORE_OPTIONS;

  config.cheats.cave = MP8_CAVE;
  config.cheats.cave_addr = MP8_CAVE_ADDR;
  config.cheats.cave_size = MP8_CAVE_SIZE;
  config.cheats.cheat_board = nullptr;
  /* main.dol's half of the patches; the per-board halves go in applyGameHooks. */
  config.cheats.hooks = MP8_HOOK_BOARD;

  config.values.scene = { 0x802CD220, DR_VALUE_TYPE_S32 };

  /* Pre-game setup array: character, port, difficulty, team, bot at a 0xA stride. */
  config.values.character[0] = { 0x802282D0, DR_VALUE_TYPE_U16 };
  config.values.character[1] = { 0x802282DA, DR_VALUE_TYPE_U16 };
  config.values.character[2] = { 0x802282E4, DR_VALUE_TYPE_U16 };
  config.values.character[3] = { 0x802282EE, DR_VALUE_TYPE_U16 };
  config.values.controller[0] = { 0x802282D2, DR_VALUE_TYPE_U16 };
  config.values.controller[1] = { 0x802282DC, DR_VALUE_TYPE_U16 };
  config.values.controller[2] = { 0x802282E6, DR_VALUE_TYPE_U16 };
  config.values.controller[3] = { 0x802282F0, DR_VALUE_TYPE_U16 };
  config.values.difficulty[0] = { 0x802282D4, DR_VALUE_TYPE_U16 };
  config.values.difficulty[1] = { 0x802282DE, DR_VALUE_TYPE_U16 };
  config.values.difficulty[2] = { 0x802282E8, DR_VALUE_TYPE_U16 };
  config.values.difficulty[3] = { 0x802282F2, DR_VALUE_TYPE_U16 };
  config.values.team[0] = { 0x802282D6, DR_VALUE_TYPE_U16 };
  config.values.team[1] = { 0x802282E0, DR_VALUE_TYPE_U16 };
  config.values.team[2] = { 0x802282EA, DR_VALUE_TYPE_U16 };
  config.values.team[3] = { 0x802282F4, DR_VALUE_TYPE_U16 };
  config.values.bot[0] = { 0x802282D8, DR_VALUE_TYPE_U16 };
  config.values.bot[1] = { 0x802282E2, DR_VALUE_TYPE_U16 };
  config.values.bot[2] = { 0x802282EC, DR_VALUE_TYPE_U16 };
  config.values.bot[3] = { 0x802282F6, DR_VALUE_TYPE_U16 };
  config.values.result[0] = { 0x8022832A, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x80228442, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x8022855A, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x80228672, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[0] = { 0x80228328, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[1] = { 0x80228440, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[2] = { 0x80228558, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[3] = { 0x80228670, DR_VALUE_TYPE_U16 };
  config.values.coins[0] = { 0x8022831E, DR_VALUE_TYPE_U16 };
  config.values.coins[1] = { 0x80228436, DR_VALUE_TYPE_U16 };
  config.values.coins[2] = { 0x8022854E, DR_VALUE_TYPE_U16 };
  config.values.coins[3] = { 0x80228666, DR_VALUE_TYPE_U16 };
  config.values.stars[0] = { 0x80228330, DR_VALUE_TYPE_U16 };
  config.values.stars[1] = { 0x80228448, DR_VALUE_TYPE_U16 };
  config.values.stars[2] = { 0x80228560, DR_VALUE_TYPE_U16 };
  config.values.stars[3] = { 0x80228678, DR_VALUE_TYPE_U16 };
  config.values.mg_star[0] = { 0x80228320, DR_VALUE_TYPE_U16 };
  config.values.mg_star[1] = { 0x80228438, DR_VALUE_TYPE_U16 };
  config.values.mg_star[2] = { 0x80228550, DR_VALUE_TYPE_U16 };
  config.values.mg_star[3] = { 0x80228668, DR_VALUE_TYPE_U16 };
  config.values.battle_ante[0] = { 0x80228326, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[1] = { 0x8022843E, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[2] = { 0x80228556, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[3] = { 0x8022866E, DR_VALUE_TYPE_S16 };

  config.values.title_block = { MP8_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };

  config.values.minigame_id = { 0x802287CC, DR_VALUE_TYPE_S16 };

  config.scene_miniexplain = 0x16;

  config.scene_names = MP8_SCENE_NAMES;

  config.char_to_dr = mp8_char_to_dr;

  config.minigame_type_to_dr = MP8_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP8_MINIGAME_TYPE_TO_DR) / sizeof(*MP8_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP8_HOST_STATE;

  /* save_files: MP8 is a Wii title, so its save lives in the NAND, not a GCI. */

  return config;
}

MarioParty8Host::MarioParty8Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
  /* Use pointer controls on the board */
  applyWiiControls(DR_WII_CONTROL_POINTER);
}

const struct mp8_battle_payout *MarioParty8Host::battlePayout(const int placements[4],
  unsigned pot, int64_t payout[4])
{
  const struct mp8_battle_payout *row = nullptr;
  unsigned order[4] = { 0, 1, 2, 3 };
  int ranks[4] = { 0, 0, 0, 0 };
  unsigned paid[4] = { 0, 0, 0, 0 };
  unsigned payees = 0;
  int64_t remainder = pot;
  unsigned i, j;

  for (i = 0; i < 4; i++)
    payout[i] = 0;

  /* Finishing order, and dense ranks so "2nd, 2nd, 5th, 5th" reads as 0,0,1,1. */
  for (i = 0; i < 4; i++)
    for (j = i + 1; j < 4; j++)
      if (placements[order[j]] < placements[order[i]])
      {
        const unsigned swap = order[i];

        order[i] = order[j];
        order[j] = swap;
      }
  for (i = 1; i < 4; i++)
    ranks[i] = placements[order[i]] == placements[order[i - 1]] ? ranks[i - 1] : ranks[i - 1] + 1;

  for (i = 0; i < sizeof(MP8_BATTLE_PAYOUTS) / sizeof(*MP8_BATTLE_PAYOUTS); i++)
  {
    const struct mp8_battle_payout *candidate = &MP8_BATTLE_PAYOUTS[i];

    if (candidate->placements[0] == ranks[0] && candidate->placements[1] == ranks[1] &&
        candidate->placements[2] == ranks[2] && candidate->placements[3] == ranks[3])
    {
      row = candidate;
      break;
    }
  }

  if (!row)
    return nullptr;

  for (i = 0; i < 4; i++)
  {
    payout[order[i]] = pot * row->share[i] / 100;
    remainder -= payout[order[i]];
    if (row->share[i])
      paid[payees++] = order[i];
  }

  /* Hand out remainder */
  while (remainder > 0 && payees)
  {
    payout[paid[dr_rand() % payees]]++;
    remainder--;
  }

  return row;
}

void MarioParty8Host::writeResults(DrGuest *guest)
{
  const struct mp8_battle_payout *row = nullptr;
  dr_minigame_result_t results[4];
  const bool battle = m_MinigameType == DR_MINIGAME_BATTLE;
  const bool write_coins = (m_MinigameType != DR_MINIGAME_1P) &&
                           (m_MinigameType != DR_MINIGAME_DUEL);
  int64_t payout[4] = { 0, 0, 0, 0 };
  int placements[4] = { 0, 0, 0, 0 };
  unsigned pot = 0;
  unsigned i;

  for (i = 0; i < 4; i++)
    results[i] = guest->minigameResult(i);

  /* A battle mini-game reports where each player placed rather than what they
   * won: the coins all come out of the pot the board collected. */
  if (battle)
  {
    for (i = 0; i < 4; i++)
      placements[i] = static_cast<int>(results[i].coins);

    pot = battlePot();
    row = battlePayout(placements, pot, payout);

    log(DR_LOG_INFO,
      qPrintable(QString("battle pot %1 coins, placements %2,%3,%4,%5 (%6)")
        .arg(pot).arg(placements[0]).arg(placements[1]).arg(placements[2]).arg(placements[3])
        .arg(row ? row->name : "no payout pattern matched")));
  }

  for (i = 0; i < 4; i++)
  {
    const int64_t won = battle ? payout[i] : results[i].coins + results[i].bonus_coins;
    int64_t chr = 0, coins = 0, mg_star = 0;
    dr_character character = DR_CHARACTER_INVALID;

    if (m_config.char_to_dr && readValue(&chr, m_config.values.character[i]) == DR_OK)
      character = m_config.char_to_dr(static_cast<unsigned>(chr));

    /* Write base result fields -- for duels mostly */
    writeValue(results[i].coins, m_config.values.result[i]);
    writeValue(results[i].bonus_coins, m_config.values.bonus_result[i]);

    /* Only accept full success */
    if (m_MinigameType == DR_MINIGAME_1P)
    {
      if (results[i].coins + results[i].bonus_coins >= 10)
        writeValue(results[i].coins, 10);
    }

    /* Write directly to coins, clamped to zero */
    if (write_coins &&
        m_config.values.coins[i].address &&
        readValue(&coins, m_config.values.coins[i]) == DR_OK)
    {
      coins = coins + won < 0 ? 0 : coins + won;
      writeValue(coins, m_config.values.coins[i]);
    }

    /* Write Minigame Star */
    if (!duel && m_config.values.mg_star[i].address &&
        readValue(&mg_star, m_config.values.mg_star[i]) == DR_OK)
      writeValue(mg_star + won < 0 ? 0 : mg_star + won, m_config.values.mg_star[i]);

    if (battle)
      log(DR_LOG_INFO,
        qPrintable(QString("Player %1 (%2): placed %3, wins %4 of the pot, board total %5")
          .arg(i + 1).arg(dr_character_name(character))
          .arg(placements[i] + 1).arg(won).arg(coins)));
    else
      log(DR_LOG_INFO, qPrintable(resultLogLine(i, character, results[i])
        + QString(", board total %1").arg(coins)));
  }

  returnToBoard();
}

void MarioParty8Host::returnToBoard(void)
{
  rollMinigames();
  m_State = DR_GCN_HOST_STATE_BEFORE_BOARD;
  log(DR_LOG_INFO, "MP8 -- returning to board...");
}

void MarioParty8Host::applyGameHooks(int32_t scene)
{
  const mp8_board_hook_t *board = mp8_board_hook(scene);
  bool announce;
  unsigned i;

  if (!board)
  {
    m_HookedScene = -1;
    m_BoardFrames = 0;
    return;
  }

  /* Not on this board long enough yet (see MP8_HOOK_DELAY_FRAMES). */
  if (m_BoardFrames < MP8_HOOK_DELAY_FRAMES)
  {
    m_BoardFrames++;
    return;
  }

  announce = scene != m_HookedScene;
  m_HookedScene = scene;

  for (i = 0; board->hooks[i][0] != 0; i++)
  {
    const unsigned int *hook = board->hooks[i];
    uint32_t word = 0;

    if (readu32(&word, hook[0]) != DR_OK || word == hook[1])
      continue;

    writeu32(hook[1], hook[0]);

    if (announce)
      log(DR_LOG_INFO,
        qPrintable(QString("%1 roulette hook installed at 0x%2 (scene 0x%3)")
          .arg(board->name).arg(hook[0], 8, 16, QChar('0'))
          .arg(scene, 2, 16, QChar('0'))));
  }
}
