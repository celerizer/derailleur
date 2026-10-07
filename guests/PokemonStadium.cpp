#include "PokemonStadium.h"

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QRetro.h>

/* Kids Club state lives in relocatable fragments, so addresses are fragment base
 * + offset. Offsets are for Rev 2 (pret/pokestadium names are US 1.0, which is
 * 0x10 lower in fragment 2). Bases are fixed within our savestate. */

/* RAM base of fragment 2 (Kids Club shared state) in the savestate */
static const size_t PS1_FRAG2_BASE = 0x80114D40;
/* RAM base of fragment 39 (Kids Club select menu) in the savestate */
static const size_t PS1_FRAG39_BASE = 0x80125AE0;

/* s16[4]: per slot, 0 = human, 1 = CPU (D_8780FA30) */
static const size_t PS1_IS_CPU = 0xFA40;
/* s16: shared CPU difficulty (D_8780FA38); default 3, so at least 0-3 */
static const size_t PS1_CPU_DIFFICULTY = 0xFA48;
/* s16: wins needed to end the set (D_8780FA3A) */
static const size_t PS1_WINS_NEEDED = 0xFA4A;
/* s16[4]: wins per slot, bumped for the round winner (D_8780FA40) */
static const size_t PS1_WINS = 0xFA50;

/* s16: highlighted minigame in the select menu, 0-8 (D_8250A288.unk_06) */
static const size_t PS1_MENU_CURSOR = 0xA28E;

/* s32: results flow (D_8780FC40); 1 = round result shown, 3 = "play again" dialog */
static const size_t PS1_RESULTS_STATE = 0xFC50;
static const int PS1_RESULTS_SHOWN = 1;
static const int PS1_RESULTS_DIALOG = 3;
/* Frames the result stays up before the dialog can be opened (D_8780FE44 = 0xB4) */
static const int PS1_RESULTS_HOLD_FRAMES = 0xB4;

/* minigame_id is the select menu cursor; the comment is the game's own id */
typedef enum
{
  PS1_MINIGAME_MAGIKARPS_SPLASH = 0, /* 1 */
  PS1_MINIGAME_CLEFAIRY_SAYS,        /* 2 */
  PS1_MINIGAME_RUN_RATTATA_RUN,      /* 3 */
  PS1_MINIGAME_SNORE_WAR,            /* 5 */
  PS1_MINIGAME_THUNDERING_DYNAMO,    /* 6 */
  PS1_MINIGAME_SUSHI_GO_ROUND,       /* 9 */
  PS1_MINIGAME_EKANS_HOOP_HURL,      /* 13 */
  PS1_MINIGAME_ROCK_HARDEN,          /* 15 */
  PS1_MINIGAME_DIG_DIG_DIG           /* 16 */
} pokemonstadium_minigame_id;

static const dr_mp_minigame_t PS1_MINIGAMES[] = {
  { "Magikarp's Splash", DR_MINIGAME_4P, PS1_MINIGAME_MAGIKARPS_SPLASH, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Clefairy Says", DR_MINIGAME_4P, PS1_MINIGAME_CLEFAIRY_SAYS, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Run, Rattata, Run", DR_MINIGAME_4P, PS1_MINIGAME_RUN_RATTATA_RUN, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Snore War", DR_MINIGAME_4P, PS1_MINIGAME_SNORE_WAR, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Thundering Dynamo", DR_MINIGAME_4P, PS1_MINIGAME_THUNDERING_DYNAMO, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sushi-Go-Round", DR_MINIGAME_4P, PS1_MINIGAME_SUSHI_GO_ROUND, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ekans' Hoop Hurl", DR_MINIGAME_4P, PS1_MINIGAME_EKANS_HOOP_HURL, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rock Harden", DR_MINIGAME_4P, PS1_MINIGAME_ROCK_HARDEN, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dig! Dig! Dig!", DR_MINIGAME_4P, PS1_MINIGAME_DIG_DIG_DIG, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

static const char *PS1_HIRES_DIR =
  "system/Mupen64plus/hires_texture/POKEMON STADIUM/GLideNHQ";

/* 36x26 player indicators per slot: human, then CPU */
static const char *PS1_PLAYER_ICON_FILES[4][2] = {
  { "POKEMON STADIUM#D9289E4B#0#3_all.png", "POKEMON STADIUM#6CAC178E#0#3_all.png" },
  { "POKEMON STADIUM#51A0AF94#0#3_all.png", "POKEMON STADIUM#6E9ADFF5#0#3_all.png" },
  { "POKEMON STADIUM#7C8193BA#0#3_all.png", "POKEMON STADIUM#285F4C99#0#3_all.png" },
  { "POKEMON STADIUM#D925D975#0#3_all.png", "POKEMON STADIUM#8FA278DE#0#3_all.png" },
};

/* Fits src_path (aspect kept) centered on a transparent w x h canvas */
static QImage ps1_fit_icon(const QString &src_path, int w, int h)
{
  QImage art(src_path);
  QImage scaled;
  QImage icon(w, h, QImage::Format_ARGB32);

  if (art.isNull())
    return QImage();
  scaled = art.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
  icon.fill(Qt::transparent);
  QPainter p(&icon);
  p.drawImage((w - scaled.width()) / 2, (h - scaled.height()) / 2, scaled);
  return icon;
}

static dr_value_t ps1_value(size_t base, size_t offset)
{
  dr_value_t value = { base + offset, DR_VALUE_TYPE_S16 };
  return value;
}

static int16_t ps1_difficulty(dr_difficulty d)
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

PokemonStadium::PokemonStadium(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  m_retro->init(coreId(), rom());

  /* Let L = L because of Dig! Dig! Dig! (R1 is already R) */
  for (unsigned port = 0; port < 4; port++)
  {
    core()->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_SELECT);
    core()->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_SELECT);
  }
}

void PokemonStadium::run()
{
  int64_t results = 0;

  m_retro->tickFrameWrites();

  /* Keep the core muted for a while after a launch (boot/transition blip) */
  if (m_muteFrames > 0)
  {
    if (auto *a = core()->audio())
      a->setMute(true);
    if (--m_muteFrames == 0)
      if (auto *a = core()->audio())
        a->setMute(false);
  }

  if (!m_minigame || !m_minigameActive)
    return;

  /* Forced P1 A press to confirm the highlighted minigame */
  if (m_aPressDelay > 0 && --m_aPressDelay == 0)
  {
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
    m_aReleaseDelay = 8;
  }
  else if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);

  /* Finish as the "play again" dialog opens, or once it could (P1 may be a bot
   * that never presses A); wait first to let the writes settle */
  if (m_resultsWatchDelay > 0)
  {
    m_resultsWatchDelay--;
    return;
  }
  if (m_finishArmed)
    return;
  if (m_retro->readValue(&results, { PS1_FRAG2_BASE + PS1_RESULTS_STATE, DR_VALUE_TYPE_S32 }) != DR_OK)
    return;
  if (results == PS1_RESULTS_SHOWN)
    m_resultsFrames++;
  if (results >= PS1_RESULTS_DIALOG || m_resultsFrames >= PS1_RESULTS_HOLD_FRAMES)
  {
    m_finishArmed = true;
    finishMinigameInFrames(120); /* linger 2 seconds on the result */
  }
}

const dr_mp_minigame_t *PokemonStadium::minigames() const
{
  return PS1_MINIGAMES;
}

/* Pre-boot (every launch): reset per-game state and lay down icon textures */
void PokemonStadium::onBeforeBoot(const DrGameData &data)
{
  unsigned i;

  m_muteFrames = 60;
  for (i = 0; i < 4; i++)
    m_slotToIndex[i] = i;
  for (i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(m_players[i], i);

    m_slotToIndex[slot] = i;
    writePlayerIcon(slot, data.players[i].character);
  }
}

/* Post-boot: load the menu state, write players and the minigame, and start it */
void PokemonStadium::doApplyGameData(const DrGameData &data)
{
  int16_t difficulty = 0;
  int16_t cursor = static_cast<int16_t>(m_minigame ? m_minigame->minigame_id : 0);
  unsigned i;

  (void)data; /* players already recorded; m_minigame set by base */

  loadState(state());

  if (PS1_FRAG39_BASE)
    m_retro->writeValueForFrames(cursor, ps1_value(PS1_FRAG39_BASE, PS1_MENU_CURSOR), 120);
  else
    log(DR_LOG_WARN, "menu fragment base not set; minigame left as saved");

  /* Player flags are per in-game slot (controller port) */
  for (i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(m_players[i], i);
    const int16_t cpu = (m_players[i].control_type == DR_CONTROL_TYPE_HUMAN) ? 0 : 1;

    m_retro->writeValueForFrames(cpu, ps1_value(PS1_FRAG2_BASE, PS1_IS_CPU + slot * 2), 120);
    m_retro->writeValueForFrames(0, ps1_value(PS1_FRAG2_BASE, PS1_WINS + slot * 2), 120);
    if (m_players[i].control_type == DR_CONTROL_TYPE_CPU)
      difficulty = qMax(difficulty, ps1_difficulty(m_players[i].difficulty));
  }
  m_retro->writeValueForFrames(difficulty, ps1_value(PS1_FRAG2_BASE, PS1_CPU_DIFFICULTY), 120);
  m_retro->writeValueForFrames(1, ps1_value(PS1_FRAG2_BASE, PS1_WINS_NEEDED), 120);

  startMinigame();
  m_resultsWatchDelay = 120; // don't watch for results until settled
  m_resultsFrames = 0;
  m_finishArmed = false;
  m_aPressDelay = 120;     // confirm the highlighted minigame with a P1 A press
}

unsigned PokemonStadium::computeWinners(void)
{
  unsigned winners = 0;
  unsigned slot;

  /* Wins are per in-game slot; map back to board indices */
  for (slot = 0; slot < 4; slot++)
  {
    int64_t wins = 0;

    if (m_retro->readValue(&wins, ps1_value(PS1_FRAG2_BASE, PS1_WINS + slot * 2)) == DR_OK &&
        wins > 0)
      winners |= (1u << m_slotToIndex[slot]);
  }
  return winners;
}

dr_minigame_result_t PokemonStadium::minigameResult(unsigned index)
{
  const unsigned winners = computeWinners();
  return { (winners & (1u << index)) ? 10 : 0, 0 };
}

void PokemonStadium::writePlayerIcon(unsigned slot, dr_character character)
{
  const QString dest_dir = QString::fromUtf8(PS1_HIRES_DIR);
  QImage icon;
  unsigned i;

  if (slot >= 4)
    return;

  /* 2x of the 36x26 indicator; human and CPU variants get the same face */
  icon = ps1_fit_icon(dr_player_icon_32px(m_hostPlatform, character), 72, 52);
  if (icon.isNull())
  {
    log(DR_LOG_WARN, qPrintable(QString("no 32px player icon for character %1")
      .arg(static_cast<int>(character))));
    return;
  }

  QDir().mkpath(dest_dir);
  for (i = 0; i < 2; i++)
  {
    const QString dest = dest_dir + "/" + QString::fromUtf8(PS1_PLAYER_ICON_FILES[slot][i]);

    if (!icon.save(dest, "PNG"))
      log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(dest)));
  }
}
