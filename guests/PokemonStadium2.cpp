#include "PokemonStadium2.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QRetro.h>

/* The texture files for P1/P2/P3/P4 indicators (small single icon). */
static const char *PS2_PLAYER_ICON_FILES[4][2] = {
  { "POKEMON STADIUM 2#82326CD4#0#3_all.png", "POKEMON STADIUM 2#88B5798C#0#3_all.png" },
  { "POKEMON STADIUM 2#7F470E07#0#3_all.png", "POKEMON STADIUM 2#C48F7E6B#0#3_all.png" },
  { "POKEMON STADIUM 2#8718526E#0#3_all.png", "POKEMON STADIUM 2#B952CF3C#0#3_all.png" },
  { "POKEMON STADIUM 2#9A7DCDA1#0#3_all.png", "POKEMON STADIUM 2#D8D0DD2F#0#3_all.png" },
};

/* Higher-quality per-player icon: a 48x40 image split into two 48x20 halves
 * (top and bottom), each backed by two hires-texture files. */
static const char *PS2_PLAYER_ICON_TOP_FILES[4][2] = {
  { "POKEMON STADIUM 2#6A2D8B66#0#3_all.png", "POKEMON STADIUM 2#A4154616#0#3_all.png" },
  { "POKEMON STADIUM 2#86F80F4C#0#3_all.png", "POKEMON STADIUM 2#B0B858C6#0#3_all.png" },
  { "POKEMON STADIUM 2#817BC8BF#0#3_all.png", "POKEMON STADIUM 2#E59A0E80#0#3_all.png" },
  { "POKEMON STADIUM 2#B6F2E60D#0#3_all.png", "POKEMON STADIUM 2#14D3E5C1#0#3_all.png" },
};

static const char *PS2_PLAYER_ICON_BOTTOM_FILES[4][2] = {
  { "POKEMON STADIUM 2#E524F371#0#3_all.png", "POKEMON STADIUM 2#CD286451#0#3_all.png" },
  { "POKEMON STADIUM 2#F09FA221#0#3_all.png", "POKEMON STADIUM 2#E16DC0E3#0#3_all.png" },
  { "POKEMON STADIUM 2#A6554D74#0#3_all.png", "POKEMON STADIUM 2#2023D9BC#0#3_all.png" },
  { "POKEMON STADIUM 2#D6F285FC#0#3_all.png", "POKEMON STADIUM 2#9DB0DF6B#0#3_all.png" },
};

/* Loads srcPath scaled to fit w x h (aspect preserved), centered on a transparent
 * w x h canvas. Returns a null image if the source is missing. */
static QImage ps2FitIcon(const QString &srcPath, int w, int h)
{
  QImage art(srcPath);
  if (art.isNull())
    return QImage();
  const QImage scaled = art.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
  QImage icon(w, h, QImage::Format_ARGB32);
  icon.fill(Qt::transparent);
  QPainter p(&icon);
  p.drawImage((w - scaled.width()) / 2, (h - scaled.height()) / 2, scaled);
  return icon;
}

static const char *PS2_HIRES_DIR =
  "system/Mupen64plus/hires_texture/POKEMON STADIUM 2/GLideNHQ";

/* A freshly booted core can't be unserialized on its first frame; let it run its
 * init for a bit first, then load the state. */
static const int PS2_BOOT_WARMUP_FRAMES = 16;

// u8: single shared bot difficulty (0/1/2 = easy/normal/hard); use the hardest
static const size_t PS2_BOT_DIFFICULTY_ADDR = 0x8012B361;

// u8 bitmask: bit i set = player i is human (else CPU)
static const size_t PS2_IS_HUMAN_ADDR = 0x8012B362;

// s8 in the minigame-select menu holding the highlighted minigame
static const size_t PS2_MINIGAME_ID_ADDR = 0x8012B363;

/* In-game player slot for a board player, from their controller port. Mario Party
 * assigns ports non-linearly, so we place each player into the slot matching their
 * port (falling back to the board index if the port is out of range). */
static unsigned ps2Slot(const dr_player_t &p, unsigned fallback)
{
  unsigned slot = static_cast<unsigned>(p.control_port - DR_CONTROL_PORT_P1);
  return slot < 4 ? slot : fallback;
}

static uint8_t ps2Difficulty(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0;
  case DR_DIFFICULTY_NORMAL:
    return 1;
  case DR_DIFFICULTY_HARD:
  case DR_DIFFICULTY_VERY_HARD:
    return 2;
  default:
    return 1;
  }
}

// u16 music tempo; it becomes the results-music tempo once a minigame ends.
static const size_t PS2_MUSIC_TEMPO_ADDR = 0x8011F304;
static const uint16_t PS2_RESULTS_TEMPO = 0x1EF0;

typedef enum
{
  PS2_MINIGAME_NONE = -1,
  PS2_MINIGAME_GUTSY_GOLBAT = 0x00,
  PS2_MINIGAME_TOPSY_TURVY,
  PS2_MINIGAME_CLEAR_CUT_CHALLENGE,
  PS2_MINIGAME_FURRETS_FROLIC,
  PS2_MINIGAME_BARRIER_BALL,
  PS2_MINIGAME_PICHUS_POWER_PLANT,
  PS2_MINIGAME_RAMPAGE_ROLLOUT,
  PS2_MINIGAME_STREAMING_STAMPEDE,
  PS2_MINIGAME_TUMBLING_TOGEPI,
  PS2_MINIGAME_DELIBIRDS_DELIVERY,
  PS2_MINIGAME_EGG_EMERGENCY,
  PS2_MINIGAME_EAGER_EEVEE
} pokemonstadium2_minigame_id;

static const dr_mp_minigame_t PS2_MINIGAMES[] = {
  { "Gutsy Golbat", DR_MINIGAME_4P, PS2_MINIGAME_GUTSY_GOLBAT, 0xFF, DR_NO_QUIRKS },
  { "Topsy-Turvy", DR_MINIGAME_4P, PS2_MINIGAME_TOPSY_TURVY, 0xFF, DR_NO_QUIRKS },
  { "Clear Cut Challenge", DR_MINIGAME_4P, PS2_MINIGAME_CLEAR_CUT_CHALLENGE, 0xFF, DR_NO_QUIRKS },
  { "Furret's Frolic", DR_MINIGAME_4P, PS2_MINIGAME_FURRETS_FROLIC, 0xFF, DR_NO_QUIRKS },
  { "Barrier Ball", DR_MINIGAME_4P, PS2_MINIGAME_BARRIER_BALL, 0xFF, DR_NO_QUIRKS },
  { "Pichu's Power Plant", DR_MINIGAME_4P, PS2_MINIGAME_PICHUS_POWER_PLANT, 0xFF, DR_NO_QUIRKS },
  { "Rampage Rollout", DR_MINIGAME_4P, PS2_MINIGAME_RAMPAGE_ROLLOUT, 0xFF, DR_NO_QUIRKS },
  { "Streaming Stampede", DR_MINIGAME_4P, PS2_MINIGAME_STREAMING_STAMPEDE, 0xFF, DR_NO_QUIRKS },
  { "Tumbling Togepi", DR_MINIGAME_4P, PS2_MINIGAME_TUMBLING_TOGEPI, 0xFF, DR_NO_QUIRKS },
  { "Delibird's Delivery", DR_MINIGAME_4P, PS2_MINIGAME_DELIBIRDS_DELIVERY, 0xFF, DR_NO_QUIRKS },
  { "Egg Emergency", DR_MINIGAME_4P, PS2_MINIGAME_EGG_EMERGENCY, 0xFF, DR_NO_QUIRKS },
  { "Eager Eevee", DR_MINIGAME_4P, PS2_MINIGAME_EAGER_EEVEE, 0xFF, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

/* u32 score addresses for each player */
static const size_t PS2_SCORES[][4] = {
  { 0x80176FBC, 0x80176FE8, 0x80177014, 0x80177040 }, // Gutsy Golbat
  { 0x80176884, 0x8017821C, 0x80179BB4, 0x8017B54C }, // Topsy-Turvy
  { 0x80174360, 0x801743F8, 0x80174490, 0x80174528 }, // Clear Cut Challenge
  { 0x801E94C0, 0x801E94D8, 0x801E94F0, 0x801E9508 }, // Furret's Frolic
  { 0x8016EA94, 0x8016EAE0, 0x8016EB2C, 0x8016EB78 }, // Barrier Ball
  { 0x8016D64C, 0x8016D6A4, 0x8016D6FC, 0x8016D754 }, // Pichu's Power Plant
  { 0x8017DFB0, 0x8017E00C, 0x8017E068, 0x8017E0C4 }, // Rampage Rollout
  { 0x80181228, 0x8018122C, 0x80181230, 0x80181234 }, // Streaming Stampede
  { 0x8018B0EC, 0x8018B198, 0x8018B244, 0x8018B2F0 }, // Tumbling Togepi
  { 0x801AC168, 0x801AC16C, 0x801AC170, 0x801AC174 }, // Delibird's Delivery
  { 0x80191ADC, 0x80191C00, 0x80191D24, 0x80191E48 }, // Egg Emergency
  { 0x8017EB54, 0x8017EB58, 0x8017EB5C, 0x8017EB60 }, // Eager Eevee
};

PokemonStadium2::PokemonStadium2(QObject *parent)
  : DrGuest(parent)
  , m_gamePath(dr_roms_directory() + "/Pokemon Stadium 2 (USA).z64")
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

  /* Just make sure the ROM exists -- we aren't actually loading it yet */
  if (!QFile::exists(m_gamePath))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(m_gamePath)));
    m_valid = false;
  }

  m_retro->setCore(core, true);
  m_retro->applyN64Remaps();

  /* Let L = L because of Chansey's mini-game */
  for (unsigned port = 0; port < 4; port++)
  {
    core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_SELECT);
    core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_SELECT);
  }
}

QWidget *PokemonStadium2::createWidget(QWidget *parent)
{
  /* Capture the window container so we can resize the core to it once the
   * deferred core boots (see run()). */
  m_container = DrGuest::createWidget(parent);
  return m_container;
}

void PokemonStadium2::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void PokemonStadium2::run()
{
  m_retro->tickFrameWrites();

  /* Keep the core muted for the first 30 frames after a launch (suppresses the
   * boot/transition blip), then restore full volume. */
  if (m_muteFrames > 0)
  {
    if (auto *a = core()->audio())
      a->setVolume(0);
    if (--m_muteFrames == 0)
      if (auto *a = core()->audio())
        a->setVolume(100);
  }

  /* Once the boot warmup elapses, load the state, force the minigame, and signal
   * the minigame is ready. */
  if (m_stateLoadCountdown > 0 && --m_stateLoadCountdown == 0)
  {
    core()->unserializeFromFile(dr_state_directory() + "/pokemonstadium2.state.zip");
    int8_t id = static_cast<int8_t>(m_minigame ? m_minigame->minigame_id : -1);
    m_retro->writeForFrames(PS2_MINIGAME_ID_ADDR, &id, 1, 120);

    /* Player setup: a human bitmask (bit i = player i human) and a single shared
     * bot difficulty set to the hardest requested. */
    uint8_t human = 0, difficulty = 0;
    for (unsigned i = 0; i < 4; i++)
    {
      /* The human bitmask is indexed by in-game slot (controller port). */
      if (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN)
        human |= (1u << ps2Slot(m_players[i], i));
      difficulty = qMax(difficulty, ps2Difficulty(m_players[i].difficulty));
    }
    m_retro->writeForFrames(PS2_IS_HUMAN_ADDR, &human, 1, 120);
    m_retro->writeForFrames(PS2_BOT_DIFFICULTY_ADDR, &difficulty, 1, 120);

    startMinigame();
    m_tempoWatchDelay = 120; // don't watch for the results tempo until settled
    m_aPressDelay = 120;      // nudge past the post-load prompt with a P1 A press

    /* This core booted late (deferred load) and QRetro sizes its window to the
     * game's native resolution on boot, so resize it to the container now — the
     * same fix the stack's currentChanged handler applies to warmed guests. */
    if (m_pendingResize)
    {
      m_pendingResize = false;
      if (m_container)
        core()->resize(m_container->width(), m_container->height());
    }
  }

  if (!m_minigame || !m_minigameActive)
    return;

  /* Force a P1 A press 60 frames after the minigame loads to clear the prompt,
   * holding it briefly so it registers as a discrete press. */
  if (m_aPressDelay > 0 && --m_aPressDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
    m_aReleaseDelay = 8;
  }
  else if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);
  }

  /* Rampage Rollout special-case */
  if (m_minigame->minigame_id == PS2_MINIGAME_RAMPAGE_ROLLOUT)
    trackRampage();

  /* Watch for the current music to be the results music. Wait a while because the savestate has it */
  if (m_tempoWatchDelay > 0)
  {
    m_tempoWatchDelay--;
    return;
  }
  uint16_t tempo = 0;
  if (m_retro->readu16(&tempo, PS2_MUSIC_TEMPO_ADDR) == DR_OK && tempo == PS2_RESULTS_TEMPO)
    finishMinigame();
}

void PokemonStadium2::trackRampage()
{
  uint32_t score[4] = {};
  for (unsigned i = 0; i < 4; i++)
    m_retro->readu32(&score[i], PS2_SCORES[PS2_MINIGAME_RAMPAGE_ROLLOUT][i]);

  /* Wait for everyone's lap counter to be 9 */
  if (!m_rampageRecording)
  {
    bool allNine = true;
    for (unsigned i = 0; i < 4; i++)
      if (score[i] != 9)
        allNine = false;
    if (allNine)
    {
      m_rampageRecording = true;
      m_rampageCount = 0;
      for (unsigned i = 0; i < 4; i++)
        m_rampageOrder[i] = -1;
    }
    return;
  }

  /* Append each player to the finish order the frame they first hit 0. */
  for (unsigned i = 0; i < 4; i++)
  {
    if (score[i] != 0)
      continue;
    bool recorded = false;
    for (int j = 0; j < m_rampageCount; j++)
      if (m_rampageOrder[j] == static_cast<int>(i))
        recorded = true;
    if (!recorded)
    {
      m_rampageOrder[m_rampageCount++] = static_cast<int>(i);
      log(DR_LOG_INFO,
        qPrintable(QString("rampage: player %1 finished (place %2)").arg(i + 1).arg(m_rampageCount)));
    }
  }
}

const dr_mp_minigame_t *PokemonStadium2::minigames() const
{
  return PS2_MINIGAMES;
}

void PokemonStadium2::doApplyGameData(const DrGameData &data)
{
  m_stateLoadCountdown = 0;
  m_rampageRecording = false;
  m_rampageCount = 0;
  m_muteFrames = 60;

  /* Record every player and lay down their icon textures now, before the boot,
   * so GLideN64 reads them. The player values are applied to RAM later, in run()
   * (after the state loads). Players are ordered in-game by controller port (Mario
   * Party assigns ports non-linearly), so the icon for a player goes to their slot. */
  for (unsigned i = 0; i < 4; i++)
  {
    m_players[i] = data.players[i];
    m_slotToIndex[i] = i;
  }
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = ps2Slot(m_players[i], i);
    m_slotToIndex[slot] = i;
    writePlayerIcon(slot, data.players[i].character);
  }

  if (!m_started)
  {
    m_started = true;
    core()->loadContent(m_gamePath.toUtf8().constData());
    startCore(); // connects frameBegin -> run() and boots
    m_stateLoadCountdown = PS2_BOOT_WARMUP_FRAMES;
    m_pendingResize = true; // resize the window once it's actually up (see run)
  }
  else
  {
    m_stateLoadCountdown = 1; // already booted; load on the next frame
  }
}

unsigned PokemonStadium2::computeWinners()
{
  if (!m_minigame)
    return 0;
  const int id = m_minigame->minigame_id;
  if (id < 0 || id >= static_cast<int>(sizeof(PS2_SCORES) / sizeof(*PS2_SCORES)))
    return 0;
  const size_t *addrs = PS2_SCORES[id];

  uint32_t score[4] = {};
  for (unsigned i = 0; i < 4; i++)
    if (addrs[i])
      m_retro->readu32(&score[i], addrs[i]);

  unsigned winners = 0;
  switch (id)
  {
  case PS2_MINIGAME_RAMPAGE_ROLLOUT: // first to count down to 0 wins (see trackRampage)
    if (m_rampageCount > 0 && m_rampageOrder[0] >= 0)
      winners = (1u << m_rampageOrder[0]);
    break;
  case PS2_MINIGAME_TOPSY_TURVY: // first to 5 wins
    for (unsigned i = 0; i < 4; i++)
      if (score[i] == 5)
        winners |= (1u << i);
    break;
  case PS2_MINIGAME_PICHUS_POWER_PLANT: // first to 80 wins
    for (unsigned i = 0; i < 4; i++)
      if (score[i] == 80)
        winners |= (1u << i);
    break;
  case PS2_MINIGAME_TUMBLING_TOGEPI: // value is placement; 1 == 1st place
    for (unsigned i = 0; i < 4; i++)
      if (score[i] == 1)
        winners |= (1u << i);
    break;
  default: // most points wins (ties share the win)
  {
    uint32_t best = 0;
    for (unsigned i = 0; i < 4; i++)
      if (score[i] > best)
        best = score[i];
    if (best > 0)
      for (unsigned i = 0; i < 4; i++)
        if (score[i] == best)
          winners |= (1u << i);
    break;
  }
  }

  /* Scores/winners above are indexed by in-game slot; map back to board indices so
   * minigameResult() awards the right players. */
  unsigned boardWinners = 0;
  for (unsigned slot = 0; slot < 4; slot++)
    if (winners & (1u << slot))
      boardWinners |= (1u << m_slotToIndex[slot]);
  return boardWinners;
}

dr_minigame_result_t PokemonStadium2::minigameResult(unsigned index)
{
  const unsigned winners = computeWinners();
  return { (winners & (1u << index)) ? 10 : 0, 0 };
}

void PokemonStadium2::writePlayerIcon(unsigned index, dr_character character)
{
  if (index >= 4)
    return;

  /* Use the GlideN64 hi-res texture pack feature to implant character faces over
   * the player indicators. */
  const int c = static_cast<int>(character);
  const QString destDir = QString::fromUtf8(PS2_HIRES_DIR);
  QDir().mkpath(destDir);

  auto saveTo = [&](const QImage &img, const char *f0, const char *f1) {
    if (img.isNull())
      return;
    for (const char *f : { f0, f1 })
    {
      const QString dest = destDir + "/" + QString::fromUtf8(f);
      if (!img.save(dest, "PNG"))
        log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(dest)));
    }
  };

  /* Small single icon: the 32px art fitted to 2x of the original 36x28 texture. */
  const QImage icon = ps2FitIcon(QString(":/assets/player-32px/%1.png").arg(c), 72, 56);
  if (icon.isNull())
    log(DR_LOG_WARN, qPrintable(QString("no 32px player icon for character %1").arg(c)));
  saveTo(icon, PS2_PLAYER_ICON_FILES[index][0], PS2_PLAYER_ICON_FILES[index][1]);

  /* Higher-quality icon: the 50px art fitted to 2x of 48x40, chopped into a top
   * and a bottom 96x40 half. */
  const QImage hq = ps2FitIcon(QString(":/assets/player-50px/%1.png").arg(c), 96, 80);
  if (hq.isNull())
    log(DR_LOG_WARN, qPrintable(QString("no 50px player icon for character %1").arg(c)));
  else
  {
    saveTo(hq.copy(0, 0, 96, 40), PS2_PLAYER_ICON_TOP_FILES[index][0],
      PS2_PLAYER_ICON_TOP_FILES[index][1]);
    saveTo(hq.copy(0, 40, 96, 40), PS2_PLAYER_ICON_BOTTOM_FILES[index][0],
      PS2_PLAYER_ICON_BOTTOM_FILES[index][1]);
  }
}
