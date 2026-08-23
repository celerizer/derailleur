#include "SonicShuffle.h"

#include <cstddef>

#include <QFile>
#include <QRetro.h>

/* u8 - which player (0-3) is the "1" in a 1 vs 3 mini-game */
static const size_t SS_MINIGAME_SOLO_ADDR = 0x21D804;

/* u8 - mini-game supertype: 1 = Normal, 2 = Accident, 3 = Stage Clear */
static const size_t SS_MINIGAME_SUPERTYPE_ADDR = 0x21D805;

/* u8 - mini-game to load (id; see SS_MINIGAMES). ids restart per supertype. */
static const size_t SS_MINIGAME_ID_ADDR = 0x21D806;

/* s8 - mini-game type: -1 = Other, 0 = VS 4, 1 = 1 vs 3, 2 = 2 vs 2 */
static const size_t SS_MINIGAME_TYPE_ADDR = 0x21D807;

/* u8 - per-player team in the mini-game (0 or 1) */
static const size_t SS_MINIGAME_TEAM_ADDR[4] = { 0x000FB10C, 0x000FB10D, 0x000FB10E, 0x000FB10F };

/* u8 - per-player role within the team in the mini-game (0 or 1) */
static const size_t SS_MINIGAME_ROLE_ADDR[4] = { 0x000FB114, 0x000FB115, 0x000FB116, 0x000FB117 };

/* s32 - becomes -1 once the mini-game is loaded and ready (after the state load) */
static const size_t SS_MINIGAME_READY_ADDR = 0x000FB1E8;

/* u8 - "arrangement": order the characters appear on the loading screen
 * (index into SS_ARRANGEMENTS). */
static const size_t SS_MINIGAME_ARRANGEMENT_ADDR = 0x21D8F5;

/* Loading-screen character order for each arrangement value. */
static const uint8_t SS_ARRANGEMENTS[12][4] =
{
  { 0, 1, 2, 3 }, /* 0x0 */
  { 1, 0, 2, 3 }, /* 0x1 */
  { 2, 0, 1, 3 }, /* 0x2 */
  { 3, 0, 1, 2 }, /* 0x3 */
  { 0, 1, 3, 2 }, /* 0x4 */
  { 1, 0, 3, 2 }, /* 0x5 */
  { 2, 0, 3, 1 }, /* 0x6 */
  { 3, 0, 2, 1 }, /* 0x7 */
  { 0, 2, 1, 3 }, /* 0x8 */
  { 1, 2, 0, 3 }, /* 0x9 */
  { 2, 1, 0, 3 }, /* 0xa */
  { 3, 1, 0, 2 }, /* 0xb */
};

/* Supertype, stored in each SS_MINIGAMES entry's 4th field. */
typedef enum
{
  SS_SUPERTYPE_NORMAL = 1,
  SS_SUPERTYPE_ACCIDENT = 2,
  SS_SUPERTYPE_STAGE_CLEAR = 3
} ss_supertype;

/* u8 - CPU difficulty per player (1 = Easy, 2 = Normal, 3 = Hard) */
static const size_t SS_CPU_DIFFICULTY_ADDR[4] = { 0x21D818, 0x21D819, 0x21D81A, 0x21D81B };

/* u8 - turn order per player (0-3) */
static const size_t SS_TURN_ORDER_ADDR[4] = { 0x21D81C, 0x21D81D, 0x21D81E, 0x21D81F };

/* u32 - board mini-event bit-flags (not a mini-game). Observed bytes:
 *   80 00 00 00 Slot Machine   01 00 00 00 Lake   00 01 00 00 Magic Show (three cards)
 *   00 04 00 00 Temple   00 00 10 00 Bazaar (lottery tickets)   00 00 40 00 Bazaar (bag)
 *   00 00 00 80 Medusa */
static const size_t SS_MINIEVENT_ADDR = 0x21D84C;

/* s16 - per-player mini-game result rings */
static const size_t SS_MINIGAME_BONUS_ADDR[4] = { 0x21D914, 0x21D918, 0x21D91C, 0x21D920 };
static const size_t SS_MINIGAME_EARNED_ADDR[4] = { 0x21D924, 0x21D928, 0x21D92C, 0x21D930 };

/* Per-player state, 0x200 bytes each from SS_PLAYER_BASE; pad keeps offsets (asserted below). */
#pragma pack(push, 1)
typedef struct
{
  uint8_t flags;            /* 0x00 - 0x41 or 0x89 */
  uint8_t unknown;          /* 0x01 */
  uint8_t is_bot;           /* 0x02 - bool: CPU */
  uint8_t _pad0a;
  /* 0x04 character: 0 Invalid 1 Sonic 2 Tails 3 Knuckles 4 Amy 5 Big 6 Gamma 7 Chao 8 Super Sonic */
  uint8_t character;
  uint8_t _pad0b[7];
  uint16_t rings;           /* 0x0C */
  uint8_t _pad1[6];
  uint8_t roll;             /* 0x14 - last die roll */
  uint8_t _pad2[7];
  uint8_t cards_in_hand[7]; /* 0x1C */
  uint8_t num_cards;        /* 0x23 */
  uint8_t _pad3[7];
  uint8_t precioustones;    /* 0x2B */
  uint8_t _pad4a[0x48 - 0x2C];
  int16_t rings_to_lose;    /* 0x48 - rings available to lose (?) */
  uint8_t _pad4b[0x200 - 0x4A];
} ss_player_t;
#pragma pack(pop)

static_assert(sizeof(ss_player_t) == 0x200, "ss_player_t must be 0x200 bytes");
static_assert(offsetof(ss_player_t, character) == 0x04, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, rings) == 0x0C, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, roll) == 0x14, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, cards_in_hand) == 0x1C, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, num_cards) == 0x23, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, precioustones) == 0x2B, "ss_player_t layout drifted");
static_assert(offsetof(ss_player_t, rings_to_lose) == 0x48, "ss_player_t layout drifted");

/* Base of player 0's struct in Flycast RAM. */
static const size_t SS_PLAYER_BASE = 0x21D940;

/* Address of `field` in player p's struct (p = 0..3). */
#define SS_PLAYER_ADDR(p, field) \
  (SS_PLAYER_BASE + (p) * sizeof(ss_player_t) + offsetof(ss_player_t, field))

/* dr_difficulty -> Sonic Shuffle CPU difficulty. It only has three levels, so the
 * outer two of ours collapse inward. */
static uint8_t ss_difficulty(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 1; /* Easy */
  case DR_DIFFICULTY_HARD:
  case DR_DIFFICULTY_VERY_HARD:
    return 3; /* Hard */
  case DR_DIFFICULTY_NORMAL:
  default:
    return 2; /* Normal */
  }
}

/* Arrangement value for a desired loading-screen `order`. Prefers the exact order,
 * but the 12 arrangements can't express every ordering (the 2nd/4th slots are always
 * ascending), so it falls back to any arrangement with the same (1st,3rd) pairing --
 * keeping teammates paired even when the exact role order isn't representable. */
static uint8_t ss_arrangement_value(const uint8_t order[4])
{
  unsigned v, k;

  for (v = 0; v < 12; v++)
  {
    for (k = 0; k < 4; k++)
      if (SS_ARRANGEMENTS[v][k] != order[k])
        break;
    if (k == 4)
      return (uint8_t)v;
  }
  for (v = 0; v < 12; v++)
    if ((SS_ARRANGEMENTS[v][0] == order[0] && SS_ARRANGEMENTS[v][2] == order[2]) ||
        (SS_ARRANGEMENTS[v][0] == order[2] && SS_ARRANGEMENTS[v][2] == order[0]))
      return (uint8_t)v;
  return (uint8_t)(dr_rand() % 12);
}

/* Fields: name, host type, id (SS_MINIGAME_ID_ADDR), supertype (SS_MINIGAME_SUPERTYPE_ADDR),
 * quirks. ids restart at 0 per supertype, so the supertype is what disambiguates them. */
static const dr_mp_minigame_t SS_MINIGAMES[] =
{
  { "Sonic the Thief", DR_MINIGAME_2V2, 0x00, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonicola", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stop and Go", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Over the Bridge", DR_MINIGAME_4P, 0x03, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Gun Slinger", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Live", DR_MINIGAME_4P, 0x05, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Psychic Sonic", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Tag", DR_MINIGAME_4P, 0x07, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wrong Way Climb", DR_MINIGAME_1V3, 0x08, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bungee Jump", DR_MINIGAME_1V3, 0x09, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Manic Maze", DR_MINIGAME_1V3, 0x0A, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shadow Tag", DR_MINIGAME_4P, 0x0B, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Frosty Rumble", DR_MINIGAME_4P, 0x0C, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Great Escape", DR_MINIGAME_4P, 0x0D, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Egg & the Chicken", DR_MINIGAME_4P, 0x0E, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Tank", DR_MINIGAME_4P, 0x0F, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fun Fun Sonic", DR_MINIGAME_4P, 0x10, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Jump the Snake", DR_MINIGAME_4P, 0x11, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Zero G Snap Shot", DR_MINIGAME_4P, 0x12, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Thor's Hammer", DR_MINIGAME_4P, 0x13, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Over the Rainbow", DR_MINIGAME_4P, 0x14, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Twister", DR_MINIGAME_4P, 0x15, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Number Jump", DR_MINIGAME_4P, 0x16, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonicooking", DR_MINIGAME_1V3, 0x17, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Egg in Space", DR_MINIGAME_4P, 0x18, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Gargantua", DR_MINIGAME_1V3, 0x19, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shoddy Work", DR_MINIGAME_2V2, 0x1A, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bucket-o-Rings", DR_MINIGAME_2V2, 0x1B, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Bomb Relay", DR_MINIGAME_2V2, 0x1C, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tractor Beam Tag", DR_MINIGAME_4P, 0x1D, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Eggbot's Attack!", DR_MINIGAME_1V3, 0x1E, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic DJ", DR_MINIGAME_1V3, 0x1F, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Turn order mini-games -- could possibly be used as battles but aren't very fun */
  { "Sonic Slot", DR_MINIGAME_SPECIAL, 0x20, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Darts", DR_MINIGAME_SPECIAL, 0x21, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sonic Hi Lo", DR_MINIGAME_SPECIAL, 0x22, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Accident */
  { "Thru the Tunnel", DR_MINIGAME_4P, 0x00, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ring Lasso", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Rapid Climb", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sky Bridge", DR_MINIGAME_4P, 0x03, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Croc-Attack", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ring of Fire", DR_MINIGAME_4P, 0x05, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Sonic Parasol", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Ring Tide", DR_MINIGAME_4P, 0x07, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Final Frontier", DR_MINIGAME_4P, 0x08, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Stage Clear */
  { "Void Battle", DR_MINIGAME_4P, 0x00, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stop the Train", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sky Diving", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 03 unused
  { "Earth Quake", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 05 unused
  { "Sonic Surfing", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

SonicShuffle::SonicShuffle(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);
  m_retro->init(coreId(), rom());
}

void SonicShuffle::run(void)
{
  m_retro->tickFrameWrites();

  /* A second after boot, switch to accurate per-pixel alpha sorting; set late so
   * the renderer is already up. */
  if (m_alphaSortDelay > 0 && --m_alphaSortDelay == 0)
    core()->options()->setOptionValue("reicast_alpha_sorting", "per-pixel (accurate)");

  /* Keep the core muted from boot through the mini-game load; unmuted at ready.
   * m_stateLoadCountdown is driven by the base's deferred-boot hook. */
  if (m_stateLoadCountdown > 0 || m_waitingForReady)
    if (auto *a = core()->audio())
      a->setMute(true);

  /* Setup is done but the game is still loading the mini-game; hold the loading
   * overlay up (don't start yet) until it signals ready by setting the flag to -1. */
  if (m_waitingForReady)
  {
    int32_t ready = 0;
    m_retro->reads32(&ready, SS_MINIGAME_READY_ADDR);
    if (ready == -1)
    {
      m_waitingForReady = false;
      if (auto *a = core()->audio())
        a->setMute(false);
      /* Seed the end-flash baseline now that the mini-game is actually live. */
      m_endFlashes = 0;
      m_endDelay = 0;
      m_lastEndFlag = 0;
      m_retro->readu8(&m_lastEndFlag, SS_PLAYER_ADDR(0, flags));
      startMinigame(); /* emits minigameStarted -> overlay fades, end detection on */
    }
    return;
  }

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* Once the end flashes are seen, wait 30 more frames before actually ending. */
  if (m_endDelay > 0)
  {
    if (--m_endDelay == 0)
      finishMinigame();
    return;
  }

  /* Player 0's flags byte flashes 0x89 as the mini-game ends. Normal games flash
   * twice; Accident and Stage Clear games flash only once. On the last expected
   * edge-detected change into 0x89, arm the 30-frame wait above. */
  {
    const uint8_t supertype =
      m_minigame ? static_cast<uint8_t>(m_minigame->scene_id) : SS_SUPERTYPE_NORMAL;
    const int needed = (supertype == SS_SUPERTYPE_NORMAL) ? 2 : 1;

    uint8_t flag = 0;
    m_retro->readu8(&flag, SS_PLAYER_ADDR(0, flags));
    if (flag == 0x89 && m_lastEndFlag != 0x89)
    {
      m_endFlashes++;
      log(DR_LOG_INFO, qPrintable(QString("end flash %1/%2").arg(m_endFlashes).arg(needed)));
      if (m_endFlashes >= needed)
        m_endDelay = 5;
    }
    m_lastEndFlag = flag;
  }
}

const dr_mp_minigame_t *SonicShuffle::minigames(void) const
{
  return SS_MINIGAMES;
}

/* Called by the base's deferred-boot hook once the core has booted (bootFrames). */
void SonicShuffle::doApplyGameData(const DrGameData &data)
{
  const dr_minigame_type type = m_minigame ? m_minigame->type : DR_MINIGAME_INVALID;
  unsigned i, slot;

  m_minigameFrames = 0;

  loadState(state());

  /* Inject which mini-game to load: supertype + id together select it (see
   * SS_MINIGAMES). Held for a while so the game reads our values as it comes out
   * of the parked state rather than picking its own. */
  if (m_minigame)
  {
    uint8_t supertype = static_cast<uint8_t>(m_minigame->scene_id);
    uint8_t id = static_cast<uint8_t>(m_minigame->minigame_id);
    int8_t mgtype = (type == DR_MINIGAME_4P)  ? 0
                  : (type == DR_MINIGAME_1V3) ? 1
                  : (type == DR_MINIGAME_2V2) ? 2
                  : -1; /* Other */
    m_retro->writeForFrames(SS_MINIGAME_SUPERTYPE_ADDR, &supertype, 1, 120);
    m_retro->writeForFrames(SS_MINIGAME_ID_ADDR, &id, 1, 120);
    m_retro->writeForFrames(SS_MINIGAME_TYPE_ADDR, &mgtype, 1, 120);
  }

  /* Board coins -> rings + rings to lose; board stars -> Precioustones. Also push
   * who is a CPU and how strong they are. @todo character, once we have a
   * dr_character -> Sonic Shuffle roster mapping. */
  for (i = 0; i < 4; i++)
  {
    slot = dr_player_slot(m_players[i], i);
    m_retro->writeu16(static_cast<uint16_t>(m_players[i].coins), SS_PLAYER_ADDR(slot, rings));
    //m_retro->writes16(static_cast<int16_t>(m_players[i].coins), SS_PLAYER_ADDR(slot, rings_to_lose));
    m_retro->writeu8(static_cast<uint8_t>(m_players[i].stars), SS_PLAYER_ADDR(slot, precioustones));
    m_retro->writeu8(m_players[i].control_type == DR_CONTROL_TYPE_CPU ? 1 : 0,
      SS_PLAYER_ADDR(slot, is_bot));
    m_retro->writeu8(ss_difficulty(m_players[i].difficulty), SS_CPU_DIFFICULTY_ADDR[slot]);
  }

  /* For 1v3, the "1" is the host's solo player; determined here and reused below
   * for both SS_MINIGAME_SOLO_ADDR and the loading-screen arrangement. */
  uint8_t solo = 0;

  /* Per-slot team (0/1) and role (0/1) for a 2v2, assigned locally from the host's
   * team data. Indexed by in-game slot, matching SS_MINIGAME_TEAM/ROLE_ADDR. */
  uint8_t team_of[4] = { 0, 0, 0, 0 };
  uint8_t role_of[4] = { 0, 0, 0, 0 };

  /* 2v2: set the teams ourselves from the host assignment (player 0's team is 0,
   * the other team is 1), then give each team one role 0 and one role 1. This is
   * all known locally, so nothing is read back from the game. */
  if (type == DR_MINIGAME_2V2)
  {
    const unsigned ref_team = m_players[0].team_id;
    unsigned team, count, members[4], zero;

    for (i = 0; i < 4; i++)
    {
      slot = dr_player_slot(m_players[i], i);
      team_of[slot] = (m_players[i].team_id == ref_team) ? 0 : 1;
    }

    for (team = 0; team < 2; team++)
    {
      count = 0;
      for (slot = 0; slot < 4; slot++)
        if (team_of[slot] == team)
          members[count++] = slot;
      if (count == 2)
      {
        zero = dr_rand() % 2;
        role_of[members[zero]] = 0;
        role_of[members[!zero]] = 1;
      }
    }

    for (i = 0; i < 4; i++)
    {
      m_retro->writeForFrames(SS_MINIGAME_TEAM_ADDR[i], &team_of[i], 1, 120);
      m_retro->writeForFrames(SS_MINIGAME_ROLE_ADDR[i], &role_of[i], 1, 120);
    }
  }
  else if (type == DR_MINIGAME_1V3)
  {
    for (i = 0; i < 4; i++)
      if (m_players[i].team_type == DR_TEAM_TYPE_1V3_SOLO)
      {
        solo = static_cast<uint8_t>(dr_player_slot(m_players[i], i));
        break;
      }
    m_retro->writeForFrames(SS_MINIGAME_SOLO_ADDR, &solo, 1, 120);
  }

  /* Loading-screen arrangement: 2v2 pairs teammates as (1st,3rd)/(2nd,4th)
   * ordered by team then role; 1v3 shows the solo player first; else random. */
  {
    uint8_t arrangement;

    if (type == DR_MINIGAME_2V2)
    {
      uint8_t order[4] = { 0, 0, 0, 0 };
      unsigned pos = 0, r, tm, sl;

      /* team 0 role 0, team 1 role 0, team 0 role 1, team 1 role 1 -- read from
       * the teams/roles we just assigned locally, not the game's memory. */
      for (r = 0; r < 2; r++)
        for (tm = 0; tm < 2; tm++)
          for (sl = 0; sl < 4; sl++)
            if (team_of[sl] == tm && role_of[sl] == r)
            {
              order[pos++] = (uint8_t)sl;
              break;
            }
      arrangement = (pos == 4) ? ss_arrangement_value(order) : (uint8_t)(dr_rand() % 12);
    }
    else if (type == DR_MINIGAME_1V3)
      arrangement = solo;
    else
      arrangement = (uint8_t)(dr_rand() % 12);

    m_retro->writeForFrames(SS_MINIGAME_ARRANGEMENT_ADDR, &arrangement, 1, 120);
  }

  /* Data is injected; wait for the game to load and signal ready (see run) before
   * starting — this keeps the loading overlay up until the mini-game is live. */
  m_waitingForReady = true;

  /* First boot only: arm accurate alpha sorting ~1s out. (The base nudges the
   * window to fill its container after boot, so no resize here.) */
  if (m_firstBoot)
  {
    m_firstBoot = false;
    m_alphaSortDelay = 60;
  }
}

dr_minigame_result_t SonicShuffle::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  unsigned slot;
  int16_t earned = 0, bonus = 0;

  if (index >= 4)
    return result;

  slot = dr_player_slot(m_players[index], index);
  m_retro->reads16(&earned, SS_MINIGAME_EARNED_ADDR[slot]);
  m_retro->reads16(&bonus, SS_MINIGAME_BONUS_ADDR[slot]);

  /* SS earned/bonus map onto MP coins/bonus_coins swapped */
  result.coins = bonus;
  result.bonus_coins = earned;
  return result;
}
