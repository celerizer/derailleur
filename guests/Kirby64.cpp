#include "Kirby64.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QRetro.h>

// u32: single shared CPU difficulty (0/1/2/3 = easy/normal/hard/very hard);
// use the highest selected CPU difficulty.
static const size_t K64_CPU_DIFFICULTY_ADDR = 0x8018ED10;

// u32 per-player character
// 0 = Kirby, 1 = King Dedede, 2 = Waddle Dee, 3 = Ado
static const size_t K64_CHARACTER_ADDRS[4] = {
  0x8018ED18,
  0x8018ED1C,
  0x8018ED20,
  0x8018ED24,
};

typedef enum
{
  K64_CHARACTER_KIRBY = 0,
  K64_CHARACTER_DEDEDE = 1,
  K64_CHARACTER_WADDLEDEE = 2,
  K64_CHARACTER_ADO = 3,
} k64_character;

// u32 per-player costume/color
static const size_t K64_COLOR_ADDRS[4] = {
  0x8018ED28,
  0x8018ED2C,
  0x8018ED30,
  0x8018ED34,
};

/*
 * Character color indices:
 *
 * Kirby:
 * 0 = Pink
 * 1 = Yellow
 * 2 = Green
 * 3 = Blue
 *
 * King Dedede:
 * 0 = Red
 * 1 = Pink
 * 2 = Orange
 * 3 = Blue
 *
 * Waddle Dee:
 * 0 = Orange
 * 1 = Pink
 * 2 = Green
 * 3 = Blue
 *
 * Ado:
 * 0 = Green
 * 1 = Pink
 * 2 = Yellow
 * 3 = Blue
 *
 * Mario Party character mappings:
 *
 * Mario    = Pink Kirby
 * Luigi    = Green Kirby
 * Peach    = Pink Ado
 * Daisy    = Yellow Ado
 * Wario    = Yellow Kirby
 * Waluigi  = Blue Kirby
 * Donkey   = Yellow Dedede
 * Yoshi    = Green Waddle Dee
 */

// u8 per-player: 0 = human, 1 = CPU
static const size_t K64_IS_BOT_ADDRS[4] = {
  0x8018ED38,
  0x8018ED39,
  0x8018ED3A,
  0x8018ED3B,
};

// u32 currently selected minigame
static const size_t K64_MINIGAME_ID_ADDR = 0x800D71BC;

typedef enum
{
  K64_MINIGAME_100_YARD_HOP = 0x1d,
  K64_MINIGAME_BUMPER_CROP_BUMP = 0x1f,
  K64_MINIGAME_CHECKERBOARD_CHASE = 0x1e,
} k64_minigame_id;

// u32 wins for each player
static const size_t K64_WINS_ADDRS[4] = {
  0x800D71C0,
  0x800D71C4,
  0x800D71C8,
  0x800D71CC,
};

static uint32_t k64Difficulty(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0;
  case DR_DIFFICULTY_NORMAL:
    return 1;
  case DR_DIFFICULTY_HARD:
    return 2;
  case DR_DIFFICULTY_VERY_HARD:
    return 3;
  default:
    return 1;
  }
}

/* In-game player slot for a board player, from their controller port. Mario Party
 * assigns ports non-linearly, so we place each player into the slot matching their
 * port (falling back to the board index if the port is out of range). */
static unsigned k64Slot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

static const dr_mp_minigame_t K64_MINIGAMES[] = {
  { "100-Yard Hop", DR_MINIGAME_4P, K64_MINIGAME_100_YARD_HOP, 0xFF, DR_NO_QUIRKS },
  { "Bumper Crop Bump", DR_MINIGAME_4P, K64_MINIGAME_BUMPER_CROP_BUMP, 0xFF, DR_NO_QUIRKS },
  { "Checkerboard Chase", DR_MINIGAME_4P, K64_MINIGAME_CHECKERBOARD_CHASE, 0xFF, DR_NO_QUIRKS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

static const char *K64_HIRES_DIR =
  "system/Mupen64plus/hires_texture/Kirby64/GLideNHQ";

/* Player icons are injected at this integer multiple of each texture's native
 * size. GLideN64 requires an exact integer upscale, and 4x keeps these tiny HUD
 * elements legible. */
static const int K64_HIRES_SCALE = 4;

/* A freshly booted core can't be unserialized on its first frame; let it run its
 * init for a bit first, then load the state. */
static const int K64_BOOT_WARMUP_FRAMES = 16;

/* 288x18 status bar which has every player's face icon */
static const char *K64_HOP_FILE = "Kirby64#BDA2812C#2#0#02D5828F_ciByRGBA.png";

/* 32x18 infobox that follows the players -- put their heads to the left */
static const char *K64_BUMPER_FILES[4] = {
  "Kirby64#6B87909B#2#0#0E6D972E_ciByRGBA.png",
  "Kirby64#CC5A87F4#2#0#0E6D972E_ciByRGBA.png",
  "Kirby64#CD5FF294#2#0#0E6D972E_ciByRGBA.png",
  "Kirby64#2D775291#2#0#0E6D972E_ciByRGBA.png"
};

static const char *K64_CHASE_NORMAL_FILES[4] = {
  "Kirby64#04F57859#3#0_all.png",
  "Kirby64#B005E461#3#0_all.png",
  "Kirby64#8EA6034C#3#0_all.png",
  "Kirby64#C5D837E7#3#0_all.png"
};

static const char *K64_CHASE_MIRROR_FILES[4] = {
  "Kirby64#4164B671#3#0_all.png",
  "Kirby64#EC107223#3#0_all.png",
  "Kirby64#1FB8AFC2#3#0_all.png",
  "Kirby64#F046A365#3#0_all.png"
};

Kirby64::Kirby64(QObject *parent)
  : DrGuest(parent)
  , m_gamePath(dr_roms_directory() + "/Kirby 64 - The Crystal Shards (USA).z64")
{
  m_retro = new DrRetroN64(this);
  QRetro *core = new QRetro();
  core->setSavingEnabled(false);

  QString corePath = dr_core_path(DR_CORE_MUPEN64PLUSNEXT);
  if (!core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }

  if (!QFile::exists(m_gamePath))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(m_gamePath)));
    m_valid = false;
  }

  m_retro->setCore(core, true);
  m_retro->applyN64Remaps();

  /* Drive the d-pad from each player's analog stick, since the minigames read the
   * digital pad. */
  for (unsigned port = 0; port < 4; port++)
    core->input()->joypads()[port].setAnalogStickToDigitalPad(true);
}

QWidget *Kirby64::createWidget(QWidget *parent)
{
  m_container = DrGuest::createWidget(parent);
  return m_container;
}

void Kirby64::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void Kirby64::run()
{
  m_retro->tickFrameWrites();

  /* Keep the core silent while the loading overlay is up. The core boots on the
   * first launch after mainwindow's mute ran (audio() was still null then), so
   * mute here each frame until the minigame starts; mainwindow unmutes on
   * minigameStarted when the overlay fades. */
  if (!m_minigameActive)
    if (auto *a = core()->audio())
      a->setMute(true);

  /* Once the boot warmup elapses, load the state, force game variables, and signal
   * the minigame is ready. */
  if (m_stateLoadCountdown > 0 && --m_stateLoadCountdown == 0)
  {
    core()->unserializeFromFile(dr_state_directory() + "/kirby64.state.zip");

    int32_t id = static_cast<int32_t>(m_minigame ? m_minigame->minigame_id : -1);
    m_retro->writeForFrames(K64_MINIGAME_ID_ADDR, &id, sizeof(id), 120);

    uint32_t difficulty = 0;
    for (unsigned i = 0; i < 4; i++)
    {
      difficulty = qMax(difficulty, k64Difficulty(m_players[i].difficulty));

      /* Write each player into the in-game slot matching their controller port. */
      const unsigned slot = k64Slot(m_players[i], i);

      uint32_t character = 0;
      uint32_t color = 0;

      switch (m_players[i].character)
      {
      case DR_CHARACTER_MARIO:
        character = K64_CHARACTER_KIRBY;
        color = 0;
        break;
      case DR_CHARACTER_LUIGI:
        character = K64_CHARACTER_KIRBY;
        color = 2;
        break;
      case DR_CHARACTER_PEACH:
        character = K64_CHARACTER_ADO;
        color = 1;
        break;
      case DR_CHARACTER_YOSHI:
        character = K64_CHARACTER_WADDLEDEE;
        color = 2;
        break;
      case DR_CHARACTER_WARIO:
        character = K64_CHARACTER_KIRBY;
        color = 1;
        break;
      case DR_CHARACTER_DONKEY_KONG:
        character = K64_CHARACTER_DEDEDE;
        color = 2;
        break;
      case DR_CHARACTER_WALUIGI:
        character = K64_CHARACTER_KIRBY;
        color = 3;
        break;
      case DR_CHARACTER_DAISY:
        character = K64_CHARACTER_ADO;
        color = 2;
        break;
      default:
        character = K64_CHARACTER_WADDLEDEE;
        color = 0;
        break;
      }

      m_retro->writeForFrames(K64_CHARACTER_ADDRS[slot], &character, sizeof(character), 120);
      m_retro->writeForFrames(K64_COLOR_ADDRS[slot], &color, sizeof(color), 120);

      uint8_t bot = (m_players[i].control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
      m_retro->writeForFrames(K64_IS_BOT_ADDRS[slot], &bot, sizeof(bot), 120);
    }

    m_retro->writeForFrames(K64_CPU_DIFFICULTY_ADDR, &difficulty, sizeof(difficulty), 120);

    /* Press A shortly after loading to advance past the ready prompt. The overlay
     * stays up (we don't startMinigame yet) until the press completes. */
    m_aPressDelay = 16;

    if (m_pendingResize)
    {
      m_pendingResize = false;
      if (m_container)
        core()->resize(m_container->width(), m_container->height());
    }
  }

  /* Forced P1 A press: hold briefly so it registers as a press, then release and
   * start the minigame (which drops the loading overlay). */
  if (m_aPressDelay > 0 && --m_aPressDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
    m_aReleaseDelay = 40;
  }
  else if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);
    startMinigame();
  }

  if (!m_minigame || !m_minigameActive)
    return;

  m_minigameFrames++;

  /*
   * Kirby 64 keeps a running win counter rather than a live score table.
   * Once a player's counter changes from the value at the start of the
   * minigame, they are considered the winner.
   */
  if (m_winnerIndex < 0)
  {
    uint32_t wins[4] = {};
    for (unsigned i = 0; i < 4; i++)
      m_retro->readu32(&wins[i], K64_WINS_ADDRS[i]);

    /* Wins are indexed by in-game slot; map back to the board player index. */
    for (unsigned slot = 0; slot < 4; slot++)
    {
      if (wins[slot] == 1)
      {
        m_winnerIndex = m_slotToIndex[slot];
        break;
      }
    }

    if (m_winnerIndex >= 0)
      m_finishCountdown = 120;
  }

  if (m_finishCountdown > 0)
  {
    if (--m_finishCountdown == 0)
      finishMinigame();
  }
}

const dr_mp_minigame_t *Kirby64::minigames() const
{
  return K64_MINIGAMES;
}

void Kirby64::doApplyGameData(const DrGameData &data)
{
  m_stateLoadCountdown = 0;
  m_minigameFrames = 0;
  m_winnerIndex = -1;
  m_finishCountdown = -1;
  m_aPressDelay = 0;
  m_aReleaseDelay = 0;

  /* Cache players data and process all dynamic custom texture injections */
  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = data.players[i];
    m_slotToIndex[i] = i;
  }
  /* Order players in-game by controller port (Mario Party ports vary). */
  for (unsigned i = 0; i < 4; i++)
    m_slotToIndex[k64Slot(m_players[i], i)] = i;
  writePlayerIcons(data);

  if (!m_started)
  {
    m_started = true;
    core()->loadContent(m_gamePath.toUtf8().constData());
    startCore();
    m_stateLoadCountdown = K64_BOOT_WARMUP_FRAMES;
    m_pendingResize = true;
  }
  else
  {
    m_stateLoadCountdown = 1;
  }
}

dr_minigame_result_t Kirby64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if ((int)index == m_winnerIndex)
    result.coins = 10;

  return result;
}

void Kirby64::writePlayerIcons(const DrGameData &data)
{
  const QString destDir = QString::fromUtf8(K64_HIRES_DIR);
  QDir().mkpath(destDir);

  auto saveImage = [&](const QImage &img, const QString &filename) {
    if (img.isNull())
      return;
    const QString fullPath = destDir + "/" + filename;
    if (!img.save(fullPath, "PNG"))
    {
      log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(fullPath)));
      return;
    }
    /* GLideN64 only accepts a replacement whose w/h are the same integer multiple
     * of the original texture's w/h. Log the size we wrote so it can be checked
     * against a GLideN64 texture dump. */
    log(DR_LOG_INFO,
      qPrintable(QString("wrote hires %1 (%2x%3)").arg(filename).arg(img.width()).arg(img.height())));
  };

  // Pre-load all 4 source 32px player character sprites, indexed by in-game slot
  // (controller port) so each HUD indicator matches the player it controls.
  QImage srcImages[4];
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = k64Slot(data.players[i], i);
    int characterId = static_cast<int>(data.players[i].character);
    srcImages[slot] = QImage(QString(":/assets/player-32px/%1.png").arg(characterId));
    if (srcImages[slot].isNull())
      log(DR_LOG_WARN, qPrintable(QString("no 32px player icon for character %1").arg(characterId)));
  }

  const int S = K64_HIRES_SCALE;

  // --- 1. 100-Yard Hop ---
  // This core renders the hop bar as a single 288x18 texture; inject at S x
  // (1152x72). Base art is a clean integer upscale, so use nearest-neighbour.
  const int hopW = 288 * S;
  const int hopH = 18 * S;
  QImage hopCanvas(":/assets/ui/hop_base.png");
  if (hopCanvas.isNull())
  {
    log(DR_LOG_WARN, "Failed to load 100-Yard Hop base asset, falling back to transparent canvas.");
    hopCanvas = QImage(hopW, hopH, QImage::Format_ARGB32);
    hopCanvas.fill(Qt::transparent);
  }
  else
  {
    hopCanvas = hopCanvas.scaled(hopW, hopH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
  }

  QPainter hopPainter(&hopCanvas);
  // Head left-edge x positions, relative to the original 288px-wide asset.
  const int hopHeadX[4] = { 0, 70, 140, 210 };
  for (unsigned i = 0; i < 4; i++)
  {
    if (srcImages[i].isNull())
      continue;

    // Scale the head to the full bar height, preserving its aspect ratio.
    int headW = qRound(srcImages[i].width() * (double(hopH) / srcImages[i].height()));
    QImage scaledHop = srcImages[i].scaled(headW, hopH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    hopPainter.drawImage(hopHeadX[i] * S, 0, scaledHop);
  }
  hopPainter.end();

  saveImage(hopCanvas, QString::fromUtf8(K64_HOP_FILE));


  // --- 2. Bumper Crop Bump ---
  // Native 32x18 per player; inject at S x (128x72).
  const int bumperW = 32 * S;
  const int bumperH = 18 * S;
  for (unsigned i = 0; i < 4; i++)
  {
    QImage bumperCanvas(":/assets/ui/bumper_base.png");
    if (bumperCanvas.isNull())
    {
      log(DR_LOG_WARN, "Failed to load Bumper base asset, using transparent fallback.");
      bumperCanvas = QImage(bumperW, bumperH, QImage::Format_ARGB32);
      bumperCanvas.fill(Qt::transparent);
    }
    else
    {
      bumperCanvas = bumperCanvas.scaled(bumperW, bumperH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    if (!srcImages[i].isNull())
    {
      QPainter bumperPainter(&bumperCanvas);
      // 8x8 (native) head superimposed at the top-left of the base asset.
      QImage scaledBumper =
        srcImages[i].scaled(8 * S, 8 * S, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      bumperPainter.drawImage(0, 0, scaledBumper);
      bumperPainter.end();
    }

    saveImage(bumperCanvas, QString::fromUtf8(K64_BUMPER_FILES[i]));
  }


  // --- 3. Checkerboard Chase ---
  const int chaseNormalW = 16 * S;
  const int chaseNormalH = 12 * S;
  const int chaseMirrorW = 32 * S;
  const int chaseMirrorH = 20 * S;
  for (unsigned i = 0; i < 4; i++)
  {
    if (srcImages[i].isNull())
      continue;

    // Normal: preserve the head's aspect ratio, then squish ~12% thinner so the
    // in-game horizontal stretch renders it at the right proportions. Centered.
    QImage normalIcon =
      srcImages[i].scaled(chaseNormalW, chaseNormalH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    normalIcon = normalIcon.scaled(qRound(normalIcon.width() * 0.88), normalIcon.height(),
      Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage normalChase(chaseNormalW, chaseNormalH, QImage::Format_ARGB32);
    normalChase.fill(Qt::transparent);
    {
      QPainter p(&normalChase);
      p.drawImage((chaseNormalW - normalIcon.width()) / 2, (chaseNormalH - normalIcon.height()) / 2,
        normalIcon);
    }
    saveImage(normalChase, QString::fromUtf8(K64_CHASE_NORMAL_FILES[i]));

    // Mirror: preserve the head's aspect ratio, centered, then flip both axes.
    QImage mirrorIcon =
      srcImages[i].scaled(chaseMirrorW, chaseMirrorH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage mirrorChase(chaseMirrorW, chaseMirrorH, QImage::Format_ARGB32);
    mirrorChase.fill(Qt::transparent);
    {
      QPainter p(&mirrorChase);
      p.drawImage((chaseMirrorW - mirrorIcon.width()) / 2, (chaseMirrorH - mirrorIcon.height()) / 2,
        mirrorIcon);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    mirrorChase = mirrorChase.flipped(Qt::Vertical);
#else
    mirrorChase = mirrorChase.mirrored(false, true);
#endif
    saveImage(mirrorChase, QString::fromUtf8(K64_CHASE_MIRROR_FILES[i]));
  }
}
