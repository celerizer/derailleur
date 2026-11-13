#include "MaryKateAndAshleySweet16LicensedToDrive.h"
#include "DrPaths.h"

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QRetro.h>

static const char *MKA_TEXTURE_DIR = "/User/Load/Textures/GMAE51";

/* Number of frames to mute audio for once a mini-game starts */
#define MKA_MUTE_FRAMES 120

typedef struct
{
  /* Replacement file names and bundled sources, indexed by mka_character */
  const char *files[4];
  const char *sources[4];

  /* Size the replacement is written at */
  int size;
} mka_portrait_set_t;

static const mka_portrait_set_t MKA_PORTRAIT_SETS[] =
{
  /* 64x64 portraits, written at 2x */
  {
    {
      "tex1_64x64_56e226d2ebb91ca4_14.png",
      "tex1_64x64_f6434313c0f06da2_14.png",
      "tex1_64x64_7944840bfe79c571_14.png",
      "tex1_64x64_677b8ab3f9a5b8fb_14.png"
    },
    {
      ":/assets/sweet16/portrait/mary_kate.png",
      ":/assets/sweet16/portrait/ashley.png",
      ":/assets/sweet16/portrait/claire.png",
      ":/assets/sweet16/portrait/tiffany.png"
    },
    128
  },

  /* 128x128 icons, written at 1x */
  {
    {
      "tex1_128x128_cf5ab5a5a44481af_14.png",
      "tex1_128x128_58cc99d2fed9423c_14.png",
      "tex1_128x128_4c4a3722324a2ee5_14.png",
      "tex1_128x128_e4c1804dcf2ef0b8_14.png"
    },
    {
      ":/assets/sweet16/icon/mary_kate.png",
      ":/assets/sweet16/icon/ashley.png",
      ":/assets/sweet16/icon/claire.png",
      ":/assets/sweet16/icon/tiffany.png"
    },
    128
  }
};

/* Bounding box of the image's non-transparent pixels */
static QRect mka_opaque_rect(const QImage &image)
{
  int left = image.width(), top = image.height(), right = -1, bottom = -1;
  int x, y;

  for (y = 0; y < image.height(); y++)
    for (x = 0; x < image.width(); x++)
      if (qAlpha(image.pixel(x, y)))
      {
        if (x < left) left = x;
        if (x > right) right = x;
        if (y < top) top = y;
        if (y > bottom) bottom = y;
      }
  if (right < 0)
    return image.rect();

  return QRect(left, top, right - left + 1, bottom - top + 1);
}

static const dr_value_t MKA_PLAYERS = { 0x801D7490, DR_VALUE_TYPE_U32 };
static const dr_value_t MKA_GAME_ID = { 0x801D74C8, DR_VALUE_TYPE_U32 };
static const dr_value_t MKA_GAME_MODE = { 0x801D7500, DR_VALUE_TYPE_U32 };

/* Human count and slot bitmasks committed when the game starts */
static const dr_value_t MKA_HUMAN_COUNT = { 0x80416540, DR_VALUE_TYPE_S32 };
static const dr_value_t MKA_HUMAN_MASK = { 0x804152CC, DR_VALUE_TYPE_U32 };
static const dr_value_t MKA_HUMAN_MENU_MASK = { 0x80416544, DR_VALUE_TYPE_U32 };

/* Player data, stride 0x20 */
static const dr_value_t MKA_CHARACTER[4] =
{
  { 0x803A40F8, DR_VALUE_TYPE_S32 },
  { 0x803A4118, DR_VALUE_TYPE_S32 },
  { 0x803A4138, DR_VALUE_TYPE_S32 },
  { 0x803A4158, DR_VALUE_TYPE_S32 }
};

/* Player coins, stride 0x80 */
static const dr_value_t MKA_COINS[4] =
{
  { 0x80333840, DR_VALUE_TYPE_S32 },
  { 0x803338C0, DR_VALUE_TYPE_S32 },
  { 0x80333940, DR_VALUE_TYPE_S32 },
  { 0x803339C0, DR_VALUE_TYPE_S32 }
};

/* Results object at 0x80334B5C; winners are player indices, -1 if unused */
static const dr_value_t MKA_RESULT_MODE = { 0x80334C1C, DR_VALUE_TYPE_S32 };

/* Team arrangement: 2v2 pairs slots 0-1 and 2-3, 1v3 has the solo player in slot 0 */
static const dr_value_t MKA_RESULT_TEAM[4] =
{
  { 0x80334C20, DR_VALUE_TYPE_S32 },
  { 0x80334C24, DR_VALUE_TYPE_S32 },
  { 0x80334C28, DR_VALUE_TYPE_S32 },
  { 0x80334C2C, DR_VALUE_TYPE_S32 }
};
static const dr_value_t MKA_RESULT_WINNER[4] =
{
  { 0x80334C30, DR_VALUE_TYPE_S32 },
  { 0x80334C34, DR_VALUE_TYPE_S32 },
  { 0x80334C38, DR_VALUE_TYPE_S32 },
  { 0x80334C3C, DR_VALUE_TYPE_S32 }
};

typedef enum
{
  MKA_CHARACTER_NONE = -1,
  MKA_CHARACTER_MARY_KATE = 0, // blue, the white girl with blonde hair
  MKA_CHARACTER_ASHLEY = 1, // red, the white girl with blonde hair
  MKA_CHARACTER_CLAIRE = 2, // yellow, the purple hair girl
  MKA_CHARACTER_TIFFANY = 3 // green, the ginger girl
} mka_character;

typedef enum
{
  MKA_GAME_BEACH_BLAST = 0,
  MKA_GAME_OFFROAD_CHALLENGE = 1,
  MKA_GAME_DESERT_DASH = 2,
  MKA_GAME_TAG_YOURE_IT = 3,
  MKA_GAME_KEYS_PLEASE = 4,
  MKA_GAME_PICTURE_PERFECT = 5,
  MKA_GAME_FASHION_FLYAWAY = 6,
  MKA_GAME_CRASH_COURSE = 7,
  MKA_GAME_FREEWAY_FRENZY = 8,
  MKA_GAME_OFF_RAMP_ORDEAL = 9,
  MKA_GAME_FRONT_ROW_DASH = 10,
  MKA_GAME_HIT_THE_WAVES = 11,
  MKA_GAME_PIER_PRESSURE = 12,
  MKA_GAME_MIXED_UP_PARTS = 13,
  MKA_GAME_CANYON_RUN = 14,
  MKA_GAME_TRIAL_RAPIDS = 15,
  MKA_GAME_XTREME_CLIMB = 16,
  MKA_GAME_CLIFF_CLIMB = 17,
  MKA_GAME_TROPICAL_WINDS = 18,
  MKA_GAME_SAILBOARD_SLALOM = 19,
  MKA_GAME_TRAVEL_TRIVIA = 20,
  MKA_GAME_TRAIL_TRIVIA = 21,
  MKA_GAME_LAGOON_SPRINT = 22,
  MKA_GAME_THUNDER_COVE = 23,
  MKA_GAME_ARENA_BEACH = 24,
  MKA_GAME_CUPCAKE_CHAOS = 25,
  MKA_GAME_BIRTHDAY_BLOWOUT = 26,
  MKA_GAME_MATCHING_MAYHEM = 27,
  MKA_GAME_CELL_PHONE_SIMON = 28,
  MKA_GAME_PARK_N_HIDE = 29,
  MKA_GAME_STEREO_SHOWDOWN = 30
} mka_game_id;

typedef enum
{
  MKA_MODE_FREE_FOR_ALL = 0,
  MKA_MODE_ELIMINATION = 1,
  MKA_MODE_TEAMS = 2,
  MKA_MODE_THREE_ON_ONE = 3,
  MKA_MODE_ONE_ON_ONE = 4
} mka_game_mode;

/* Minigame IDs pack the game ID with the game mode */
#define MKA_MINIGAME_ID(game, mode) (((mode) << 8) | (game))

static const dr_mp_minigame_t MKA_SWEET16_MINIGAMES[] =
{
  { "Beach Blast", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_BEACH_BLAST, MKA_MODE_FREE_FOR_ALL), MKA_GAME_BEACH_BLAST, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Offroad Challenge", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_OFFROAD_CHALLENGE, MKA_MODE_FREE_FOR_ALL), MKA_GAME_OFFROAD_CHALLENGE, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Desert Dash", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_DESERT_DASH, MKA_MODE_FREE_FOR_ALL), MKA_GAME_DESERT_DASH, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tag, You're It!", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_TAG_YOURE_IT, MKA_MODE_FREE_FOR_ALL), MKA_GAME_TAG_YOURE_IT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Keys Please!", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_KEYS_PLEASE, MKA_MODE_FREE_FOR_ALL), MKA_GAME_KEYS_PLEASE, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Picture Perfect", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_PICTURE_PERFECT, MKA_MODE_FREE_FOR_ALL), MKA_GAME_PICTURE_PERFECT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fashion Flyaway", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_FASHION_FLYAWAY, MKA_MODE_FREE_FOR_ALL), MKA_GAME_FASHION_FLYAWAY, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crash Course", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_CRASH_COURSE, MKA_MODE_FREE_FOR_ALL), MKA_GAME_CRASH_COURSE, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Freeway Frenzy", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_FREEWAY_FRENZY, MKA_MODE_FREE_FOR_ALL), MKA_GAME_FREEWAY_FRENZY, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Off-Ramp Ordeal", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_OFF_RAMP_ORDEAL, MKA_MODE_FREE_FOR_ALL), MKA_GAME_OFF_RAMP_ORDEAL, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Front Row Dash", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_FRONT_ROW_DASH, MKA_MODE_FREE_FOR_ALL), MKA_GAME_FRONT_ROW_DASH, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hit the Waves", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_HIT_THE_WAVES, MKA_MODE_FREE_FOR_ALL), MKA_GAME_HIT_THE_WAVES, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pier Pressure", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_PIER_PRESSURE, MKA_MODE_FREE_FOR_ALL), MKA_GAME_PIER_PRESSURE, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mixed Up Parts", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_MIXED_UP_PARTS, MKA_MODE_FREE_FOR_ALL), MKA_GAME_MIXED_UP_PARTS, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Canyon Run", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_CANYON_RUN, MKA_MODE_FREE_FOR_ALL), MKA_GAME_CANYON_RUN, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trial Rapids", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_TRIAL_RAPIDS, MKA_MODE_FREE_FOR_ALL), MKA_GAME_TRIAL_RAPIDS, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tropical Winds", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_TROPICAL_WINDS, MKA_MODE_FREE_FOR_ALL), MKA_GAME_TROPICAL_WINDS, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sailboard Slalom", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_SAILBOARD_SLALOM, MKA_MODE_FREE_FOR_ALL), MKA_GAME_SAILBOARD_SLALOM, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Travel Trivia", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_TRAVEL_TRIVIA, MKA_MODE_FREE_FOR_ALL), MKA_GAME_TRAVEL_TRIVIA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trail Trivia", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_TRAIL_TRIVIA, MKA_MODE_FREE_FOR_ALL), MKA_GAME_TRAIL_TRIVIA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lagoon Sprint", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_LAGOON_SPRINT, MKA_MODE_FREE_FOR_ALL), MKA_GAME_LAGOON_SPRINT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Thunder Cove", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_THUNDER_COVE, MKA_MODE_FREE_FOR_ALL), MKA_GAME_THUNDER_COVE, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Arena Beach", DR_MINIGAME_4P, MKA_MINIGAME_ID(MKA_GAME_ARENA_BEACH, MKA_MODE_FREE_FOR_ALL), MKA_GAME_ARENA_BEACH, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Picture Perfect", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_PICTURE_PERFECT, MKA_MODE_TEAMS), MKA_GAME_PICTURE_PERFECT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fashion Flyaway", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_FASHION_FLYAWAY, MKA_MODE_TEAMS), MKA_GAME_FASHION_FLYAWAY, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Canyon Run", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_CANYON_RUN, MKA_MODE_TEAMS), MKA_GAME_CANYON_RUN, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trial Rapids", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_TRIAL_RAPIDS, MKA_MODE_TEAMS), MKA_GAME_TRIAL_RAPIDS, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Travel Trivia", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_TRAVEL_TRIVIA, MKA_MODE_TEAMS), MKA_GAME_TRAVEL_TRIVIA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trail Trivia", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_TRAIL_TRIVIA, MKA_MODE_TEAMS), MKA_GAME_TRAIL_TRIVIA, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cupcake Chaos", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_CUPCAKE_CHAOS, MKA_MODE_TEAMS), MKA_GAME_CUPCAKE_CHAOS, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Birthday Blowout", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_BIRTHDAY_BLOWOUT, MKA_MODE_TEAMS), MKA_GAME_BIRTHDAY_BLOWOUT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Matching Mayhem", DR_MINIGAME_2V2, MKA_MINIGAME_ID(MKA_GAME_MATCHING_MAYHEM, MKA_MODE_TEAMS), MKA_GAME_MATCHING_MAYHEM, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Fashion Flyaway", DR_MINIGAME_1V3, MKA_MINIGAME_ID(MKA_GAME_FASHION_FLYAWAY, MKA_MODE_THREE_ON_ONE), MKA_GAME_FASHION_FLYAWAY, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Birthday Blowout", DR_MINIGAME_1V3, MKA_MINIGAME_ID(MKA_GAME_BIRTHDAY_BLOWOUT, MKA_MODE_THREE_ON_ONE), MKA_GAME_BIRTHDAY_BLOWOUT, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Park-n-Hide", DR_MINIGAME_1V3, MKA_MINIGAME_ID(MKA_GAME_PARK_N_HIDE, MKA_MODE_THREE_ON_ONE), MKA_GAME_PARK_N_HIDE, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

MaryKateAndAshleySweet16LicensedToDrive::MaryKateAndAshleySweet16LicensedToDrive(QObject *parent)
  : DrGuestGcn("gcn-sweet16", (DrPaths::instance().romsDirectory() + "/Mary-Kate and Ashley - Sweet 16 - Licensed to Drive (USA)").toStdString(), parent)
{
}

const dr_mp_minigame_t *MaryKateAndAshleySweet16LicensedToDrive::minigames(void) const
{
  return MKA_SWEET16_MINIGAMES;
}

void MaryKateAndAshleySweet16LicensedToDrive::writePortrait(unsigned character, dr_character player_character)
{
  const QString dir =
    QString::fromUtf8(core()->directories()->get(QRetroDirectories::Save)) + MKA_TEXTURE_DIR;
  const QImage face(dr_player_icon_32px(m_hostPlatform, player_character));
  unsigned set;

  if (!QDir().mkpath(dir))
  {
    log(DR_LOG_WARN, qPrintable(QString("portraits: cannot create %1").arg(dir)));
    return;
  }
  if (face.isNull())
    log(DR_LOG_WARN, qPrintable(QString("portrait %1: no icon for character %2")
      .arg(character).arg(static_cast<int>(player_character))));

  for (set = 0; set < sizeof(MKA_PORTRAIT_SETS) / sizeof(MKA_PORTRAIT_SETS[0]); set++)
  {
    const mka_portrait_set_t &entry = MKA_PORTRAIT_SETS[set];
    const QString path = dir + "/" + entry.files[character];
    QImage source(entry.sources[character]);
    QImage canvas;

    if (source.isNull())
    {
      log(DR_LOG_WARN, qPrintable(QString("portrait %1: missing source %2")
        .arg(character).arg(entry.sources[character])));
      continue;
    }

    canvas = source.convertToFormat(QImage::Format_ARGB32).scaled(
      entry.size, entry.size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    /* Face goes in the bottom-right quadrant of the visible area */
    if (!face.isNull())
    {
      const QRect area = mka_opaque_rect(canvas);
      const int quadrant = area.width() < area.height() ? area.width() / 2 : area.height() / 2;
      const QImage scaled = face.scaled(quadrant, quadrant, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      QPainter painter(&canvas);

      painter.drawImage(area.right() + 1 - quadrant + (quadrant - scaled.width()) / 2,
        area.bottom() + 1 - quadrant + (quadrant - scaled.height()) / 2, scaled);
    }

    if (!canvas.save(path, "PNG"))
      log(DR_LOG_WARN, qPrintable(QString("portrait %1: failed to write %2").arg(character).arg(path)));
  }
}

void MaryKateAndAshleySweet16LicensedToDrive::onBeforeBoot(const DrGameData &data)
{
  unsigned i;

  /* Characters follow port order */
  for (i = 0; i < 4; i++)
    writePortrait(data.players[i].control_port < 4 ? data.players[i].control_port : i,
      data.players[i].character);
}

unsigned MaryKateAndAshleySweet16LicensedToDrive::assignSlots(const DrGameData &data)
{
  unsigned humans = 0;
  unsigned i;

  /* Each player takes the in-game slot matching their controller port */
  m_humanMask = 0;
  for (i = 0; i < 4; i++)
  {
    const unsigned port = data.players[i].control_port;

    m_playerSlot[i] = port < 4 ? port : i;
    if (data.players[i].control_type == DR_CONTROL_TYPE_HUMAN)
    {
      m_humanMask |= 1u << m_playerSlot[i];
      humans++;
    }
  }

  for (i = 0; i < 4; i++)
    log(DR_LOG_INFO, qPrintable(QString("player %1: port %2 slot %3 %4")
      .arg(i).arg(data.players[i].control_port).arg(m_playerSlot[i])
      .arg(data.players[i].control_type == DR_CONTROL_TYPE_HUMAN ? "human" : "cpu")));

  return humans;
}

void MaryKateAndAshleySweet16LicensedToDrive::applyTeams(void)
{
  const dr_minigame_type type = m_minigame ? m_minigame->type : DR_MINIGAME_4P;
  int64_t teams[4] = { -1, -1, -1, -1 };
  unsigned first = 0, second = 2, solo = 0, group = 1;
  unsigned i;

  for (i = 0; i < 4; i++)
  {
    const dr_player_t &player = m_players[i];

    if (type == DR_MINIGAME_2V2)
    {
      if (player.team_id == 0 && first < 2)
        teams[first++] = m_playerSlot[i];
      else if (second < 4)
        teams[second++] = m_playerSlot[i];
    }
    else if (type == DR_MINIGAME_1V3)
    {
      if (player.team_type == DR_TEAM_TYPE_1V3_SOLO && solo < 1)
        teams[solo++] = m_playerSlot[i];
      else if (group < 4)
        teams[group++] = m_playerSlot[i];
    }
    else
      teams[i] = m_playerSlot[i];
  }

  for (i = 0; i < 4; i++)
  {
    m_retro->writeValueForFrames(teams[i], MKA_RESULT_TEAM[i], 120);
    log(DR_LOG_INFO, qPrintable(QString("team slot %1: player slot %2").arg(i).arg(teams[i])));
  }
}

void MaryKateAndAshleySweet16LicensedToDrive::doApplyGameData(const DrGameData &data)
{
  const int64_t game = data.minigame->minigame_id & 0xFF;
  const int64_t mode = data.minigame->minigame_id >> 8;

  m_retro->loadState(state());
  m_minigameFrames = 0;
  m_finishPending = false;

  m_retro->writeValueForFrames(game, MKA_GAME_ID, 120);
  m_retro->writeValueForFrames(mode, MKA_GAME_MODE, 120);
  m_humans = assignSlots(data);

  core()->input()->clearPortRoutes();
  m_setupFrames = 0;
  m_setupActive = true;
  log(DR_LOG_INFO, qPrintable(QString("setting up game %1 in mode %2 with %3 human player(s)")
    .arg(game).arg(mode).arg(m_humans)));
}

void MaryKateAndAshleySweet16LicensedToDrive::pressA(bool pressed)
{
  core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, pressed);
}

void MaryKateAndAshleySweet16LicensedToDrive::writePlayers(void)
{
  const unsigned humans = m_humans ? m_humans : 1;
  const int64_t mask = m_humanMask ? m_humanMask : 1;
  unsigned i;

  m_retro->writeValueForFrames(humans - 1, MKA_PLAYERS, 30);
  m_retro->writeValueForFrames(humans, MKA_HUMAN_COUNT, 90);
  m_retro->writeValueForFrames(mask, MKA_HUMAN_MASK, 90);
  m_retro->writeValueForFrames(mask, MKA_HUMAN_MENU_MASK, 90);

  /* Characters follow the physical port */
  for (i = 0; i < 4; i++)
    m_retro->writeValueForFrames(m_playerSlot[i], MKA_CHARACTER[m_playerSlot[i]], 30);
}

void MaryKateAndAshleySweet16LicensedToDrive::finishSetup(void)
{
  unsigned i;

  /* Clear the previous winner so the next one marks the end */
  for (i = 0; i < 4; i++)
    m_retro->writeValue(-1, MKA_RESULT_WINNER[i]);

  applyTeams();
  m_setupActive = false;
  startMinigame();
}

void MaryKateAndAshleySweet16LicensedToDrive::run(void)
{
  int64_t winner;

  if (m_retro->booting())
    return;

  if (m_setupActive)
  {
    /* A, 120 frames, players, A, 30 frames, start */
    switch (++m_setupFrames)
    {
    case 1:
    case 121:
      pressA(true);
      break;
    case 9:
    case 129:
      pressA(false);
      break;
    case 120:
      writePlayers();
      break;
    case 151:
      finishSetup();
      break;
    }
    return;
  }

  if (!m_minigameActive)
    return;

  m_minigameFrames++;
  if (m_minigameFrames <= MKA_MUTE_FRAMES)
    if (auto *audio = core()->audio())
      audio->setMute(m_minigameFrames < MKA_MUTE_FRAMES);

  if (m_finishPending || m_minigameFrames < 120 ||
      m_retro->readValue(&winner, MKA_RESULT_WINNER[0]) != DR_OK || winner < 0)
    return;

  log(DR_LOG_INFO, qPrintable(QString("mini-game finished, winner is in slot %1").arg(winner)));
  m_finishPending = true;
  finishMinigameInFrames(180);
}

dr_minigame_result_t MaryKateAndAshleySweet16LicensedToDrive::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t mode, winner, player, slot;
  int winner_position = -1, player_position = -1;
  unsigned i;

  if (index >= 4)
    return result;
  slot = m_playerSlot[index];

  if (m_retro->readValue(&mode, MKA_RESULT_MODE) != DR_OK ||
      m_retro->readValue(&winner, MKA_RESULT_WINNER[0]) != DR_OK ||
      winner < 0)
    return result;

  if (mode == MKA_MODE_TEAMS || mode == MKA_MODE_THREE_ON_ONE)
  {
    for (i = 0; i < 4; i++)
    {
      if (m_retro->readValue(&player, MKA_RESULT_TEAM[i]) != DR_OK)
        continue;
      if (player == winner)
        winner_position = i;
      if (player == slot)
        player_position = i;
    }
    if (winner_position < 0 || player_position < 0)
      return result;

    if (mode == MKA_MODE_TEAMS && winner_position / 2 == player_position / 2)
      result.coins = 10;
    else if (mode == MKA_MODE_THREE_ON_ONE && (winner_position == 0) == (player_position == 0))
      result.coins = 10;
  }
  else
  {
    for (i = 0; i < 4; i++)
    {
      if (m_retro->readValue(&player, MKA_RESULT_WINNER[i]) != DR_OK)
        continue;
      if (player == slot)
      {
        result.coins = 10;
        break;
      }
    }
  }

  return result;
}
