#include "SonicShuffle.h"

#include <cstddef>

#include <QRetro.h>

/* u8 - mini-game supertype: 1 = Normal, 2 = Accident, 3 = Stage Clear */
static const size_t SS_MINIGAME_SUPERTYPE_ADDR = 0x21D805;

/* u8 - mini-game to load (id; see SS_MINIGAMES). ids restart per supertype. */
static const size_t SS_MINIGAME_ID_ADDR = 0x21D806;

/* s8 - mini-game type: -1 = Other, 0 = VS 4, 1 = 1 vs 3, 2 = 2 vs 2 */
static const size_t SS_MINIGAME_TYPE_ADDR = 0x21D807;

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

/* s16 - per-player mini-game result rings (contiguous, outside the player structs) */
static const size_t SS_MINIGAME_BONUS_ADDR[4] = { 0x21D91A, 0x21D91C, 0x21D91E, 0x21D920 };
static const size_t SS_MINIGAME_EARNED_ADDR[4] = { 0x21D924, 0x21D926, 0x21D928, 0x21D92A };

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

/* In-game slot from a player's controller port. @todo confirm SS orders by port */
static unsigned ss_slot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

/* Fields: name, host type, id (SS_MINIGAME_ID_ADDR), supertype (SS_MINIGAME_SUPERTYPE_ADDR),
 * quirks. ids restart at 0 per supertype, so the supertype is what disambiguates them. */
static const dr_mp_minigame_t SS_MINIGAMES[] =
{
  { "Sonic the Thief", DR_MINIGAME_2V2, 0x00, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonicola", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Stop and Go", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Over the Bridge", DR_MINIGAME_4P, 0x03, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Gun Slinger", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Live", DR_MINIGAME_4P, 0x05, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Psychic Sonic", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Tag", DR_MINIGAME_4P, 0x07, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Wrong Way Climb", DR_MINIGAME_1V3, 0x08, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Bungee Jump", DR_MINIGAME_1V3, 0x09, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Manic Maze", DR_MINIGAME_1V3, 0x0A, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Shadow Tag", DR_MINIGAME_4P, 0x0B, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Frosty Rumble", DR_MINIGAME_4P, 0x0C, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Great Escape", DR_MINIGAME_4P, 0x0D, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Egg & the Chicken", DR_MINIGAME_4P, 0x0E, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Tank", DR_MINIGAME_4P, 0x0F, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Fun Fun Sonic", DR_MINIGAME_4P, 0x10, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Jump the Snake", DR_MINIGAME_4P, 0x11, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Zero G Snap Shot", DR_MINIGAME_4P, 0x12, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Thor's Hammer", DR_MINIGAME_4P, 0x13, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Over the Rainbow", DR_MINIGAME_4P, 0x14, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Twister", DR_MINIGAME_4P, 0x15, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Number Jump", DR_MINIGAME_4P, 0x16, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonicooking", DR_MINIGAME_1V3, 0x17, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Egg in Space", DR_MINIGAME_4P, 0x18, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Gargantua", DR_MINIGAME_1V3, 0x19, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Shoddy Work", DR_MINIGAME_2V2, 0x1A, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Bucket-o-Rings", DR_MINIGAME_2V2, 0x1B, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Bomb Relay", DR_MINIGAME_2V2, 0x1C, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Tractor Beam Tag", DR_MINIGAME_4P, 0x1D, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Eggbot's Attack!", DR_MINIGAME_1V3, 0x1E, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic DJ", DR_MINIGAME_1V3, 0x1F, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },

  /* Turn order mini-games -- could possibly be used as battles but aren't very fun */
  { "Sonic Slot", DR_MINIGAME_SPECIAL, 0x20, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Darts", DR_MINIGAME_SPECIAL, 0x21, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },
  { "Sonic Hi Lo", DR_MINIGAME_SPECIAL, 0x22, SS_SUPERTYPE_NORMAL, DR_NO_QUIRKS },

  /* Accident */
  { "Thru the Tunnel", DR_MINIGAME_4P, 0x00, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Ring Lasso", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Rapid Climb", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Sky Bridge", DR_MINIGAME_4P, 0x03, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Croc-Attack", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Ring of Fire", DR_MINIGAME_4P, 0x05, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Sonic Parasol", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Ring Tide", DR_MINIGAME_4P, 0x07, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },
  { "Final Frontier", DR_MINIGAME_4P, 0x08, SS_SUPERTYPE_ACCIDENT, DR_NO_QUIRKS },

  /* Stage Clear */
  { "Void Battle", DR_MINIGAME_4P, 0x00, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS },
  { "Stop the Train", DR_MINIGAME_4P, 0x01, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS },
  { "Sky Diving", DR_MINIGAME_4P, 0x02, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS },
  // 03 unused
  { "Earth Quake", DR_MINIGAME_4P, 0x04, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS },
  // 05 unused
  { "Sonic Surfing", DR_MINIGAME_4P, 0x06, SS_SUPERTYPE_STAGE_CLEAR, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

SonicShuffle::SonicShuffle(QObject *parent)
  : DrGuest(parent)
{
  const QString corePath = dr_core_path(DR_CORE_FLYCAST);
  const QString gamePath = dr_roms_directory() + "/Sonic Shuffle (USA).chd";
  QRetro *c = new QRetro();

  m_retro = new DrRetro(this);
  c->setSavingEnabled(false);
  if (!c->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!c->loadContent(gamePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(c, true);
}

void SonicShuffle::startCore(void)
{
  QRetro *c = core();
  if (c)
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void SonicShuffle::run(void)
{
  m_retro->tickFrameWrites();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* @todo detect mini-game end and call finishMinigame() */
}

const dr_mp_minigame_t *SonicShuffle::minigames(void) const
{
  return SS_MINIGAMES;
}

void SonicShuffle::doApplyGameData(const DrGameData &data)
{
  unsigned i, slot;

  m_minigameFrames = 0;
  for (i = 0; i < 4; i++)
    m_players[i] = data.players[i];

  /* @todo load parked savestate, write supertype/id/type + player setup */
  core()->unserializeFromFile(dr_state_directory() + "/sonicshuffle.state.zip");

  /* Board coins -> rings + rings to lose; board stars -> Precioustones. */
  for (i = 0; i < 4; i++)
  {
    slot = ss_slot(m_players[i], i);
    m_retro->writeu16(static_cast<uint16_t>(m_players[i].coins), SS_PLAYER_ADDR(slot, rings));
    m_retro->writes16(static_cast<int16_t>(m_players[i].coins), SS_PLAYER_ADDR(slot, rings_to_lose));
    m_retro->writeu8(static_cast<uint8_t>(m_players[i].stars), SS_PLAYER_ADDR(slot, precioustones));
  }

  startMinigame();
}

dr_minigame_result_t SonicShuffle::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  unsigned slot;
  int16_t earned = 0, bonus = 0;

  if (index >= 4)
    return result;

  slot = ss_slot(m_players[index], index);
  m_retro->reads16(&earned, SS_MINIGAME_EARNED_ADDR[slot]);
  m_retro->reads16(&bonus, SS_MINIGAME_BONUS_ADDR[slot]);

  /* SS earned/bonus map onto MP coins/bonus_coins swapped */
  result.coins = bonus;
  result.bonus_coins = earned;
  return result;
}
