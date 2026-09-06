#include "MarioPartyGcnHost.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QString>

#include <asm/mp4.h>

#include <cstddef>

typedef enum
{
  MPGC_TEXT_COLOR_BLACK = 0,
  MPGC_TEXT_COLOR_BLACK_2 = 1,
  MPGC_TEXT_COLOR_BLUE = 2,
  MPGC_TEXT_COLOR_RED = 3,
  MPGC_TEXT_COLOR_MAGENTA = 4,
  MPGC_TEXT_COLOR_GREEN = 5,
  MPGC_TEXT_COLOR_CYAN = 6,
  MPGC_TEXT_COLOR_YELLOW = 7,
  MPGC_TEXT_COLOR_WHITE = 8,
  MPGC_TEXT_COLOR_DARK_GRAY = 9,
  MPGC_TEXT_COLOR_LIGHT_GRAY = 10
} mpgc_text_color;

/* The battle roulette picks a mini-game from pictures, so they get redrawn as the
 * names of the candidates on offer (see DrGcnHostConfig::battle_icons). */
#define MPGC_BATTLE_ICON_SCALE 2

/* Draws `title` centered into an icon-sized image and writes it to `path`. The
 * text wraps, and the font shrinks until every line fits, so nothing is cut off. */
static bool mpgcWriteBattleIcon(
  const QString &path, const QString &title, int width, int height, bool whiteBackground)
{
  const int scale = MPGC_BATTLE_ICON_SCALE;
  const int outline = 2 * scale;
  const int flags = Qt::AlignCenter | Qt::TextWordWrap;
  QImage image(width * scale, height * scale, QImage::Format_ARGB32);
  QPainter painter;
  QRect rect;
  QFont font;
  int pixels;

  image.fill(whiteBackground ? Qt::white : Qt::transparent);

  if (!painter.begin(&image))
    return false;
  painter.setRenderHint(QPainter::TextAntialiasing, true);

  /* The margin leaves room for the outline pass on every side */
  rect = image.rect().adjusted(6 * scale, 4 * scale, -6 * scale, -4 * scale);

  font = painter.font();
  font.setBold(true);

  for (pixels = (height / 5) * scale; pixels > 6 * scale; pixels--)
  {
    QRect bounds;

    font.setPixelSize(pixels);
    painter.setFont(font);
    bounds = painter.boundingRect(rect, flags, title);
    if (bounds.height() <= rect.height() && bounds.width() <= rect.width())
      break;
  }

  /* Outline first, so the name reads against whatever the icon sat on. Every
   * offset in the square is drawn, otherwise the thicker ring leaves gaps. */
  painter.setPen(whiteBackground ? Qt::white : Qt::black);
  for (int dy = -outline; dy <= outline; dy += scale)
    for (int dx = -outline; dx <= outline; dx += scale)
      if (dx || dy)
        painter.drawText(rect.translated(dx, dy), flags, title);

  painter.setPen(whiteBackground ? Qt::black : Qt::white);
  painter.drawText(rect, flags, title);
  painter.end();

  return image.save(path);
}

MarioPartyGcnHost::MarioPartyGcnHost(const DrGcnHostConfig &config, QObject *parent)
  : DrHost(parent)
  , m_config(config)
{
  m_core = new QRetro();
  m_ownCore = true;

  /* Pretend to not support gyro/accel so we can use the sticks */
  m_core->setEnvironmentCallbackSupported(RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE, false);

  m_gamePath = config.game;
  if (!m_core->loadCore(config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }

  m_core->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  /* Forced core options go in before the content does, while the core is still
   * reading its settings. */
  for (const dr_core_option_t *option = config.options; option && option->key; option++)
  {
    m_core->options()->setOptionValue(option->key, option->value);
    log(DR_LOG_INFO,
      qPrintable(QString("core option: %1 = %2").arg(option->key).arg(option->value)));
  }

  if (!m_core->loadContent(config.game.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(config.game.c_str())));
    m_valid = false;
  }

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);
}

QStringList MarioPartyGcnHost::saveFilePatterns(void) const
{
  if (m_config.save_files.empty())
    return DrHost::saveFilePatterns();

  QStringList patterns;
  for (const std::string &file : m_config.save_files)
    patterns.append(QString::fromStdString(file));
  return patterns;
}

void MarioPartyGcnHost::applyHooks(const unsigned int (*hooks)[3])
{
  for (int i = 0; hooks && hooks[i][0] != 0; i++)
  {
    const unsigned int *hook = hooks[i];

    switch (hook[2])
    {
    case 1:
      writeu8(hook[1], hook[0]);
      break;
    case 2:
      writeu16(hook[1], hook[0]);
      break;
    case 4:
      writeu32(hook[1], hook[0]);
      break;
    }
  }
}

void MarioPartyGcnHost::stampCave(void)
{
  if (!m_config.cheats.cave || !m_config.cheats.cave_addr)
    return;

  for (unsigned i = 0; i <= m_config.cheats.cave_size; i++)
    writeu8(m_config.cheats.cave[i], m_config.cheats.cave_addr + i);
}

void MarioPartyGcnHost::rollMinigames(void)
{
  /* One shared reroll keeps every netplay peer's pool identical; the specific
   * type's candidates are cached/stamped later once the roulette reveals it. */
  if (!m_MinigameSource)
    return;

  m_MinigameSource->rerollMinigames();

  /* Games whose battle roulette shows pictures name them here. */
  if (m_config.battle_icons.dir && m_config.battle_icons.files)
    stampBattleIcons();
}

void MarioPartyGcnHost::stampBattleIcons(void)
{
  const std::array<DrMinigameCandidate, 5> &candidates =
    m_MinigameSource->minigameCandidates(DR_MINIGAME_BATTLE);
  const QString dir = dr_save_directory() + m_config.battle_icons.dir;

  if (!QDir().mkpath(dir))
  {
    log(DR_LOG_WARN, qPrintable(QString("battle icons: cannot create %1").arg(dir)));
    return;
  }

  /* One icon per picture the roulette shows, filled from the type's candidates. */
  for (unsigned i = 0; m_config.battle_icons.files[i] && i < candidates.size(); i++)
  {
    const dr_mp_minigame_t *minigame = candidates[i].minigame;
    const QString title = (minigame && minigame->name)
      ? QString::fromUtf8(minigame->name) : QString();
    const QString path = dir + "/" + m_config.battle_icons.files[i];

    if (mpgcWriteBattleIcon(path, title, m_config.battle_icons.width,
          m_config.battle_icons.height, m_config.battle_icons.white_background))
      log(DR_LOG_INFO, qPrintable(QString("battle icon %1: %2").arg(i).arg(title)));
    else
      log(DR_LOG_WARN,
        qPrintable(QString("battle icon %1: failed to write %2").arg(i).arg(path)));
  }
}

/* Mini-game names the roulette can show: the cave reserves this many 32-byte
 * slots after the title block's base. */
static const unsigned MPGC_TITLE_SLOTS = 4;

/* MP6 and MP7 divert both roulette list builders into one cave routine, which
 * stamps which of the two ran. Serve that list and nothing else, so a mic
 * roulette offers only mic mini-games and the normal one never does. */
dr_mic_mode MarioPartyGcnHost::micMode(void)
{
  int8_t mic = 0;

  if (!m_config.mic_lists || !m_config.host_state_addr)
    return DR_MIC_ANY;

  reads8(&mic, m_config.host_state_addr + offsetof(dr_host_state_t, mic));

  return mic ? DR_MIC_ONLY : DR_MIC_OFF;
}

void MarioPartyGcnHost::stampTitles(dr_minigame_type type)
{
  if (!m_MinigameSource)
    return;

  /* Cache the chosen type's five so startMinigame() can resolve the choice. This
   * only reads the already-rolled pool, so it stays deterministic across peers. */
  m_Candidates = m_MinigameSource->minigameCandidates(type, m_MicMode);

  /* Stamp the candidate names into the title block (4 slots x 32 bytes) -- the
   * roulette only ever shows four, which is what startMinigame's id mask reads
   * back. A game without a block still caches its candidates above. */
  if (!m_config.values.title_block.address)
    return;

  for (unsigned slot = 0; slot < MPGC_TITLE_SLOTS; slot++)
  {
    const char *name =
      (m_Candidates[slot].minigame && m_Candidates[slot].minigame->name)
        ? m_Candidates[slot].minigame->name : "";
    const size_t addr = m_config.values.title_block.address + slot * 32;

    unsigned j = 0;

    writeu8(0x0B, addr + j++);

    /* A board that doesn't split its lists can land on a mic mini-game, so say so
     * -- the player needs the microphone plugged in. Boards that do split have
     * every entry the same either way, so there is nothing to single out. */
    if (m_MicMode == DR_MIC_ANY && m_Candidates[slot].minigame &&
        m_Candidates[slot].minigame->flags.flags.mic)
    {
      writeu8(0x1e, addr + j++);
      writeu8(MPGC_TEXT_COLOR_MAGENTA, addr + j++);
    }
    else if (m_Candidates[slot].minigame && m_Candidates[slot].minigame->flags.flags.lucky)
    {
      writeu8(0x1e, addr + j++);
      writeu8(MPGC_TEXT_COLOR_YELLOW, addr + j++);
    }
    else if (m_Candidates[slot].minigame && m_Candidates[slot].minigame->flags.flags.unlucky)
    {
      writeu8(0x1e, addr + j++);
      writeu8(MPGC_TEXT_COLOR_RED, addr + j++);
    }

    for (const char *p = name; *p && j < 31; p++)
    {
      unsigned char chara = static_cast<unsigned char>(*p);

      if (chara == ' ')
        chara = 0x10; /* thin space */
      else if (chara == '.') chara = 0x85;
      else if (chara == ',') chara = 0x82;
      else if (chara == '\'') chara = 0x5C;
      else if (chara == '-') chara = 0x3D;
      else if (chara == '!') chara = 0xC2;
      else if (chara == '?') chara = 0xC3;
      else if (chara == '_') chara = 0x86;
      else if (chara == '&') chara = 0x7E;
      else if (chara == '%') chara = 0x7C;
      else if (chara == ':') chara = 0x7B;

      writeu8(chara, addr + j++);
    }
    writeu8(0x00, addr + j); /* null-terminate */
  }
}

void MarioPartyGcnHost::run(void)
{
  tickFrameWrites();

  if (m_core->frames() % 120 == 0)
  {
    stampCave();
    if (m_MinigameType != DR_MINIGAME_INVALID)
      stampTitles(m_MinigameType);
  }

  applyHooks(m_config.cheats.hooks);

  int64_t scene_value = 0;
  readValue(&scene_value, m_config.values.scene);
  const int32_t current_scene = static_cast<int32_t>(scene_value);
  const int32_t previous_scene = m_PreviousScene;
  m_PreviousScene = current_scene;

  applyGameHooks(current_scene);

  if (previous_scene != current_scene)
  {
    static const int8_t clear = -1;
    if (m_config.host_state_addr)
      writeForFrames(m_config.host_state_addr + offsetof(dr_host_state_t, minigame_type),
        &clear, 1, 120);

    /* -1 is the gap between overlays, not a scene: don't name or warn about it. */
    if (current_scene != -1)
    {
      const char *scene_name = dr_scene_name(m_config.scene_names, current_scene);

      if (scene_name)
        log(DR_LOG_INFO,
          qPrintable(QString("scene: 0x%1 (%2)")
            .arg(current_scene, 4, 16, QChar('0')).arg(scene_name)));
      else
        log(DR_LOG_WARN,
          qPrintable(QString("scene: 0x%1 <<< UNKNOWN SCENE ID >>>")
            .arg(current_scene, 4, 16, QChar('0'))));
    }
  }

  static const char *stateNames[] = {
    "INVALID", "BEFORE_BOARD", "BOARD", "BEFORE_ROULETTE", "ROULETTE", "AFTER_ROULETTE", "MINIGAME"
  };
  auto setState = [&](dr_gcn_host_state s) {
    log(DR_LOG_INFO,
      qPrintable(QString("host state: %1 -> %2").arg(stateNames[m_State]).arg(stateNames[s])));
    m_State = s;
  };

  switch (m_State)
  {
  case DR_GCN_HOST_STATE_INVALID:
    if (m_core->frames() <= 120)
      return;
    /* Roll an initial pool; the type-specific set is stamped once a roulette opens. */
    rollMinigames();
    setState(DR_GCN_HOST_STATE_BEFORE_BOARD);
    break;

  case DR_GCN_HOST_STATE_BEFORE_BOARD:
    writeValue(-1, m_config.values.minigame_id);
    if (m_config.host_state_addr)
      writes8(-1, m_config.host_state_addr + offsetof(dr_host_state_t, minigame_type));
    setState(DR_GCN_HOST_STATE_BOARD);
    break;

  case DR_GCN_HOST_STATE_BOARD:
  {
    int8_t minigame_type = -1;

    if (m_config.host_state_addr)
      reads8(&minigame_type, m_config.host_state_addr + offsetof(dr_host_state_t, minigame_type));

    /* Has the board signalled a mini-game roulette has opened? */
    if (minigame_type >= 0 && (unsigned)minigame_type < m_config.minigame_type_to_dr_size)
    {
      m_MinigameType = m_config.minigame_type_to_dr[minigame_type];

      /* Latched with the type: the cave stamps both as the list is built, and the
       * roulette re-stamps from here on without asking again. */
      m_MicMode = micMode();

      log(DR_LOG_INFO,
        qPrintable(QString("roulette type %1 (%2)%3")
          .arg(minigame_type).arg(dr_minigame_type_name(m_MinigameType))
          .arg(m_MicMode == DR_MIC_ONLY ? " mic"
             : m_MicMode == DR_MIC_OFF  ? " no-mic" : "")));

      /* Stamp the detected type's candidates so the roulette shows the right names. */
      stampTitles(m_MinigameType);
      setState(DR_GCN_HOST_STATE_BEFORE_ROULETTE);
    }

    break;
  }

  case DR_GCN_HOST_STATE_BEFORE_ROULETTE:
    writeValue(-1, m_config.values.minigame_id);
    setState(DR_GCN_HOST_STATE_ROULETTE);
    break;

  case DR_GCN_HOST_STATE_ROULETTE:
  {
    int8_t minigame_type = -1;
    int64_t minigame_id = -1;

    /* Watch for minigame_type changes; shouldn't happen but fix and log */
    if (m_config.host_state_addr)
      reads8(&minigame_type, m_config.host_state_addr + offsetof(dr_host_state_t, minigame_type));
    if (minigame_type >= 0 && (unsigned)minigame_type < m_config.minigame_type_to_dr_size
        && m_config.minigame_type_to_dr[minigame_type] != m_MinigameType)
    {
      const dr_minigame_type type = m_config.minigame_type_to_dr[minigame_type];

      log(DR_LOG_ERROR,
        qPrintable(QString("roulette type changed mid-roulette: %1 -> %2 (%3)")
          .arg(dr_minigame_type_name(m_MinigameType))
          .arg(dr_minigame_type_name(type)).arg(minigame_type)));

      m_MinigameType = type;
      stampTitles(type);
    }

    readValue(&minigame_id, m_config.values.minigame_id);
    if (minigame_id > 0)
      setState(DR_GCN_HOST_STATE_AFTER_ROULETTE);

    break;
  }

  case DR_GCN_HOST_STATE_AFTER_ROULETTE:

    /**
     * MP8 doesn't do an overlay transition here so just count frames after
     * roulette decision
     */
    if (game() == DR_GAME_MARIOPARTY8)
    {
      if (m_AfterRouletteTimer >= 60) /// @todo magic number
      {
        startMinigame();
        setState(DR_GCN_HOST_STATE_MINIGAME);
        m_AfterRouletteTimer = 0;
      }
      else
        m_AfterRouletteTimer++;
    }
    else if (current_scene != previous_scene)
    {
      startMinigame();
      setState(DR_GCN_HOST_STATE_MINIGAME);
    }
    break;

  case DR_GCN_HOST_STATE_MINIGAME:
    /* Mario Party 8 has its own handler to leave here */
    if (current_scene != previous_scene && previous_scene == m_config.scene_miniresults)
    {
      rollMinigames();
      setState(DR_GCN_HOST_STATE_BEFORE_BOARD);
    }
    break;

  case DR_GCN_HOST_STATE_SIZE:
    break;
  }
}

/* The board stores difficulty as 0-3; keep the two directions next to each other. */
static dr_difficulty gcn_difficulty_to_dr(int64_t diff)
{
  switch (diff)
  {
  case 0x00: return DR_DIFFICULTY_EASY;
  case 0x01: return DR_DIFFICULTY_NORMAL;
  case 0x02: return DR_DIFFICULTY_HARD;
  case 0x03: return DR_DIFFICULTY_VERY_HARD;
  }

  return DR_DIFFICULTY_NORMAL;
}

static int gcn_difficulty_from_dr(dr_difficulty d)
{
  switch (d)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:      return 0x00;
  case DR_DIFFICULTY_NORMAL:    return 0x01;
  case DR_DIFFICULTY_HARD:      return 0x02;
  case DR_DIFFICULTY_VERY_HARD: return 0x03;
  default:                      return -1;
  }
}

/* Scan the native->dr mapping backwards to find the native id for `c`, or -1. The id
 * space is tiny, so a linear probe beats keeping a second table per game. */
static int gcn_char_from_dr(const DrGcnHostConfig &config, dr_character c)
{
  if (config.char_to_dr && c != DR_CHARACTER_INVALID)
    for (unsigned n = 0; n < 0x100; n++)
      if (config.char_to_dr(n) == c)
        return static_cast<int>(n);

  return -1;
}

void MarioPartyGcnHost::readPlayers(DrPlayerArray &players)
{
  players = {};

  for (unsigned i = 0; i < 4; i++)
  {
    int64_t chr = 0, ctrl = 0, diff = 0, bot = 0, team = 0;
    readValue(&chr, m_config.values.character[i]);
    readValue(&ctrl, m_config.values.controller[i]);
    readValue(&diff, m_config.values.difficulty[i]);
    readValue(&bot, m_config.values.bot[i]);
    readValue(&team, m_config.values.team[i]);

    dr_player_t &p = players[i];

    p.character = m_config.char_to_dr
      ? m_config.char_to_dr(static_cast<unsigned>(chr)) : DR_CHARACTER_INVALID;

    p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
    p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
    p.difficulty = gcn_difficulty_to_dr(diff);
    p.team_id = static_cast<unsigned>(team);

    if (m_config.values.coins[i].address)
    {
      int64_t coins = 0;
      readValue(&coins, m_config.values.coins[i]);
      p.coins = static_cast<signed>(coins);
    }
    if (m_config.values.stars[i].address)
    {
      int64_t stars = 0;
      readValue(&stars, m_config.values.stars[i]);
      p.stars = static_cast<signed>(stars);
    }

    switch (m_MinigameType)
    {
    case DR_MINIGAME_2V2:
      p.team_type = DR_TEAM_TYPE_2V2;
      break;
    case DR_MINIGAME_1V3:
      p.team_type = (p.team_id == 0) ? DR_TEAM_TYPE_1V3_SOLO
                  : (p.team_id == 1) ? DR_TEAM_TYPE_1V3_GROUP
                                     : DR_TEAM_TYPE_INVALID;
      break;
    case DR_MINIGAME_DUEL:
      p.team_type = (p.team_id == 0) ? DR_TEAM_TYPE_DUEL_TARGET
                  : (p.team_id == 1) ? DR_TEAM_TYPE_DUEL_INITIATOR
                                     : DR_TEAM_TYPE_DUEL_NONPARTICIPANT;
      break;
    default:
      p.team_type = DR_TEAM_TYPE_4P;
      break;
    }
  }
}

void MarioPartyGcnHost::startMinigame(void)
{
  int64_t id = -1;
  int64_t chosen;

  readValue(&id, m_config.values.minigame_id);
  chosen = id;

  id--;

  /* For MP4, since we use mini-game IDs 0x11-0x15 and 0x25,0x26 */
  id &= 0x03;

  log(DR_LOG_INFO,
    qPrintable(QString("roulette chose mini-game id 0x%1 -> candidate slot %2")
      .arg(chosen, 2, 16, QChar('0')).arg(id)));

  if (id < 0 || id >= static_cast<int64_t>(m_Candidates.size()) || !m_Candidates[id].guest)
  {
    log(DR_LOG_ERROR,
      qPrintable(QString("startMinigame: no candidate for id 0x%1").arg(id, 2, 16, QChar('0'))));
    return;
  }

  DrPlayerArray players;
  readPlayers(players);

  log(DR_LOG_INFO,
    qPrintable(QString("launching mini-game: %1 (index %2)")
      .arg(m_Candidates[id].minigame->name ? m_Candidates[id].minigame->name : "(unnamed)")
      .arg(id)));

  emit minigameRequested(m_Candidates[id], players);
}

bool MarioPartyGcnHost::readPlayerSetup(DrPlayerArray &players)
{
  if (!m_config.values.character[0].address)
    return false;

  for (unsigned i = 0; i < 4; i++)
  {
    int64_t chr = 0, ctrl = 0, diff = 0, bot = 0, team = 0;

    readValue(&chr, m_config.values.character[i]);
    readValue(&ctrl, m_config.values.controller[i]);
    readValue(&diff, m_config.values.difficulty[i]);
    readValue(&bot, m_config.values.bot[i]);
    readValue(&team, m_config.values.team[i]);

    dr_player_t &p = players[i];
    p.character = m_config.char_to_dr
      ? m_config.char_to_dr(static_cast<unsigned>(chr)) : DR_CHARACTER_INVALID;
    p.control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + ctrl);
    p.control_type = (bot & 0x01) ? DR_CONTROL_TYPE_CPU : DR_CONTROL_TYPE_HUMAN;
    p.difficulty = gcn_difficulty_to_dr(diff);
    p.team_id = static_cast<unsigned>(team);
  }

  return true;
}

bool MarioPartyGcnHost::writePlayerSetup(const DrPlayerArray &players)
{
  if (!m_config.values.character[0].address)
    return false;

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = players[i];
    const int chr = gcn_char_from_dr(m_config, p.character);
    const int diff = gcn_difficulty_from_dr(p.difficulty);
    int64_t bot = 0;

    if (chr >= 0)
      writeValue(chr, m_config.values.character[i]);
    else
      log(DR_LOG_WARN,
        qPrintable(QString("write players: %1 has no id in this game, leaving slot %2 alone")
          .arg(dr_character_name(p.character)).arg(i + 1)));

    if (diff >= 0)
      writeValue(diff, m_config.values.difficulty[i]);

    writeValue(p.control_port - DR_CONTROL_PORT_P1, m_config.values.controller[i]);
    writeValue(p.team_id, m_config.values.team[i]);

    /* Only bit 0 is the cpu flag; keep whatever else the game stores alongside it. */
    readValue(&bot, m_config.values.bot[i]);
    bot = (p.control_type == DR_CONTROL_TYPE_CPU) ? (bot | 0x01) : (bot & ~0x01);
    writeValue(bot, m_config.values.bot[i]);
  }

  log(DR_LOG_INFO, "write players: stamped the debug player setup");

  return true;
}

void MarioPartyGcnHost::writeResults(DrGuest *guest)
{
  for (unsigned i = 0; i < 4; i++)
  {
    const dr_minigame_result_t result = adjustResult(i, guest->minigameResult(i));
    int64_t chr = 0;
    dr_character character = DR_CHARACTER_INVALID;

    writeValue(result.coins, m_config.values.result[i]);
    writeValue(result.bonus_coins, m_config.values.bonus_result[i]);

    if (m_config.char_to_dr && readValue(&chr, m_config.values.character[i]) == DR_OK)
      character = m_config.char_to_dr(static_cast<unsigned>(chr));

    log(DR_LOG_INFO, qPrintable(resultLogLine(i, character, result)));
  }
}

void MarioPartyGcnHost::clearResults(void)
{
  for (unsigned i = 0; i < 4; i++)
  {
    writeValue(0, m_config.values.result[i]);
    writeValue(0, m_config.values.bonus_result[i]);
  }
}
