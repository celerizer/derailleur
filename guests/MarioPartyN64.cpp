#include "MarioPartyN64.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include "partystuffer.h"

/* Frames the mini-game id is held for when booting from a savestate. */
static const unsigned MPN64_MINIGAME_HOLD_FRAMES = 120;

/* Frames the player writes are held for when booting without a savestate: 10s at
 * 60 Hz. Nothing waits for the boot to reach the mini-game -- the writes simply
 * keep being re-stamped across the whole of it. */
static const unsigned MPN64_BOOT_HOLD_FRAMES = 600;

/* Frames to wait after a reset before stamping the players. retro_reset schedules
 * its NMI 50M cycles out -- about half a second -- so this clears the reboot with
 * room to spare rather than writing across it. */
static const int MPN64_RESET_APPLY_DELAY_FRAMES = 60;

/* Frames after the first boot's apply before it is snapshotted: 1s at 60 Hz, by
 * which point the boot patches have walked the game all the way in. Every launch
 * after that loads the snapshot instead of booting again. */
static const int MPN64_BOOT_STATE_CAPTURE_FRAMES = 60;

MarioPartyN64::~MarioPartyN64()
{
  delete m_retro;
  m_retro = nullptr;
}

MarioPartyN64::MarioPartyN64(const MpN64Config &config, QObject *parent)
  : DrGuest(parent)
  , m_config(config)
{
  /* The snapshot is good for this session only, so clear any left behind by the
   * last one. It has to happen here rather than on the way out: the app closes
   * with _Exit/TerminateProcess (see MainWindow::closeEvent), so no destructor
   * runs, and a crash would strand the file too. */
  if (!m_config.temp_state.empty())
    QFile::remove(QString::fromStdString(m_config.temp_state));

  m_retro = new DrRetroN64(this);
  QRetro *core = new QRetro();
  core->setSavingEnabled(false);
  dr_apply_global_core_options(core, DR_CORE_MUPEN64PLUSNEXT);
  /* The core is dlopen'd lazily on the first launch (see corePath()).
   * Content is loaded lazily on the first launch (see DrGuest::applyGameData);
   * just verify the ROM exists here so an absent one drops the guest at startup. */
  if (!QFile::exists(QString::fromStdString(m_config.game)))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(m_config.game.c_str())));
    m_valid = false;
  }
  m_retro->setCore(core, true);
  m_retro->applyN64Remaps();
}

void MarioPartyN64::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioPartyN64::applyBootPatches(void)
{
  unsigned i;

  for (i = 0; i < sizeof(m_config.boot_patches) / sizeof(*m_config.boot_patches); i++)
  {
    const MpN64BootPatch &patch = m_config.boot_patches[i];
    int64_t val = 0;

    if (!patch.when.address)
      break;
    if (m_retro->readValue(&val, patch.when) == DR_OK && val == patch.equals)
      m_retro->writeValue(patch.value, patch.dest);
  }
}

void MarioPartyN64::run()
{
  /* Every frame until the mini-game starts, so the boot gets walked all the way
   * in. The conditions are on the boot overlay, so this stops doing anything of
   * its own accord once the game moves past it.
   *
   * Dropping the m_minigameActive half was tried once, so that a patch on an
   * overlay that only loads after the mini-game could land. It does not work:
   * boot patches are gated on an exact instruction word, and past the boot that
   * word is only unique against the *static* overlay images -- at run time a
   * gate address can fall in an overlay's bss or heap and match by chance. An
   * overlay that loads after the mini-game needs its own mechanism, not this
   * one. (The freezes that coincided with that experiment were unrelated: they
   * were injected models whose deformation clusters the name table no longer
   * matched.) */
  if (!m_minigameActive && bootsWithoutState())
    applyBootPatches();

  /* The players a reset deferred, once its reboot has been and gone. */
  if (m_applyCountdown > 0 && --m_applyCountdown == 0)
    applyPlayers(m_pendingPlayers, false);

  if (m_bootStateCountdown > 0 && --m_bootStateCountdown == 0)
    captureBootState();

  m_retro->tickFrameWrites();

  if (m_minigameActive)
  {
    m_minigameFrames++;
    for (unsigned k = 0; k < 4; k++)
    {
      int64_t v = 0;

      if (m_config.result[k].address && m_retro->readValue(&v, m_config.result[k]) == DR_OK && v)
        m_resultSeen[k] = v;
    }
  }

  int64_t val;
  if (m_retro->readValue(&val, m_config.scene) == DR_OK && val != m_lastScene)
  {
    const int16_t last = m_lastScene;

    log(DR_LOG_INFO,
      qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg((uint16_t)val, 4, 16, QChar('0'))));
    m_lastScene = val;
    if (!m_minigameActive &&
        (val == m_config.scene_miniexplain[0] || val == m_config.scene_miniexplain[1] ||
          (m_minigame && m_minigame->type == DR_MINIGAME_DUEL && val == m_minigame->scene_id)))
    {
      seedRng();
      for (unsigned k = 0; k < 4; k++)
        m_resultSeen[k] = 0;

      /* A duel starts on its own overlay rather than on an explanation screen,
       * so by here the mini-game is already up and the boot-time writes have
       * nothing left to hold down. Stop them: they re-stamp for 600 frames,
       * which reaches well into the mini-game, and GwSystem is reused once an
       * overlay owns it. */
      if (m_minigame && m_minigame->type == DR_MINIGAME_DUEL)
        m_retro->clearFrameWrites();

      startMinigame();
    }

    /* MP1 puts the characters back the way it likes them somewhere in the
     * explanation, so the hidden character only sticks if he goes in on the
     * hand-off into the mini-game itself. */
    if (m_hiddenSlots && m_minigameActive && m_minigame && val == m_minigame->scene_id &&
        (last == m_config.scene_miniexplain[0] || last == m_config.scene_miniexplain[1]))
      writeHiddenCharacters();
    if (m_minigameActive && m_minigameFrames >= 60 &&
        val != m_config.scene_miniexplain[0] && val != m_config.scene_miniexplain[1] &&
        val != m_minigame->scene_id)
      finishMinigame();
  }
}

void MarioPartyN64::seedRng()
{
  if (!m_config.rng.address)
    return;

  /* dr_rand is shared and lockstepped, so netplay peers seed identically. */
  const uint32_t seed = static_cast<uint32_t>(dr_rand());

  m_retro->writeValue(seed, m_config.rng);
  log(DR_LOG_INFO, qPrintable(QString("RNG seed: 0x%1").arg(seed, 8, 16, QChar('0'))));
}

bool MarioPartyN64::hiddenCharacterPlayable(const dr_mp_minigame_t *minigame) const
{
  unsigned i;

  if (!minigame || m_config.hidden.character == DR_CHARACTER_INVALID)
    return false;

  for (i = 0; i < sizeof(m_config.hidden.minigame_ids) / sizeof(m_config.hidden.minigame_ids[0]);
       i++)
  {
    if (m_config.hidden.minigame_ids[i] == -1)
      break;
    else if (m_config.hidden.minigame_ids[i] == minigame->minigame_id)
      return true;
  }

  return false;
}

void MarioPartyN64::writeHiddenCharacters(void)
{
  unsigned i;

  for (i = 0; i < 4; i++)
  {
    if (!(m_hiddenSlots & (1 << i)))
      continue;
    m_retro->writeValue(m_config.hidden.native_id, m_config.character[i]);
    log(DR_LOG_INFO, qPrintable(QString("hidden character: forced P%1 to 0x%2 entering %3")
      .arg(i + 1).arg(m_config.hidden.native_id, 2, 16, QChar('0'))
      .arg(m_minigame ? m_minigame->name : "mini-game")));
  }
}

const dr_mp_minigame_t *MarioPartyN64::minigames() const
{
  return m_config.minigames;
}

static uint8_t mpN64Difficulty(dr_difficulty difficulty)
{
  switch (difficulty)
  {
  case DR_DIFFICULTY_VERY_EASY:
  case DR_DIFFICULTY_EASY:
    return 0x00;
  case DR_DIFFICULTY_NORMAL:
    return 0x01;
  case DR_DIFFICULTY_HARD:
    return 0x02;
  case DR_DIFFICULTY_VERY_HARD:
    return 0x03;
  default:
    return 0x01;
  }
}

static QString unpackCharacter(QTemporaryDir &dir, const QString &name)
{
  const QString root = ":/assets/characters/" + name;
  const QString out = dir.filePath(name);
  QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);

  if (!dir.isValid())
    return QString();

  while (it.hasNext())
  {
    const QString from = it.next();
    const QString to = out + from.mid(root.size());

    if (!QDir().mkpath(QFileInfo(to).absolutePath()) || !QFile::copy(from, to))
      return QString();
  }

  return QFile::exists(out + "/character.txt") ? out : QString();
}

void MarioPartyN64::buildCharacterRom(const DrGameData &data)
{
  static const struct
  {
    dr_character character;
    const char *folder;
  } injectable[] = { { DR_CHARACTER_WALUIGI, "waluigi" }, { DR_CHARACTER_DAISY, "daisy" } };

  const QString target = QString::fromStdString(m_config.temp_rom);
  QString source = QString::fromStdString(m_config.game);
  unsigned characters[4] = { 0, 0, 0, 0 };
  QTemporaryDir packages;
  unsigned i, j;

  m_romBuilt = true;

  /* The same resolution doApplyGameData writes out, so each character is built
   * into the slot the game is going to put that player in. */
  dr_resolve_characters(m_config.char_from_dr, data.players, m_config.roster_size, characters);

  for (i = 0; i < sizeof(injectable) / sizeof(*injectable); i++)
  {
    bool played = false;
    unsigned slot = 0;
    QString folder;

    for (j = 0; j < 4; j++)
    {
      if (data.players[j].character != injectable[i].character)
        continue;
      played = true;
      slot = characters[j];
      break;
    }

    if (!played)
      continue;

    folder = unpackCharacter(packages, injectable[i].folder);

    if (folder.isEmpty())
    {
      log(DR_LOG_WARN,
        qPrintable(QString("could not unpack the %1 package").arg(injectable[i].folder)));
      continue;
    }
    else if (partystuffer_inject_character(source.toUtf8().constData(), target.toUtf8().constData(),
               folder.toUtf8().constData(), slot) != PARTYSTUFFER_OK)
    {
      log(DR_LOG_ERROR, qPrintable(QString("could not inject %1: %2")
                            .arg(injectable[i].folder)
                            .arg(QString::fromUtf8(partystuffer_last_error()))));
      continue;
    }

    log(DR_LOG_INFO,
      qPrintable(
        QString("injected %1 into character slot %2").arg(injectable[i].folder).arg(slot)));

    /* A second injection edits the ROM the first one produced. */
    source = target;
    m_romPath = m_config.temp_rom;
  }
}

void MarioPartyN64::onBeforeBoot(const DrGameData &data)
{
  /* Runs on every launch, but the ROM is only built ahead of the first boot:
   * gamePath() is read right after this returns, and a relaunch keeps whatever
   * was built then. */
  if (m_romBuilt || m_config.temp_rom.empty() || !bootsWithoutState())
    return;

  buildCharacterRom(data);
}

void MarioPartyN64::captureBootState(void)
{
  const QString path = QString::fromStdString(m_config.temp_state);

  if (path.isEmpty())
    return;
  if (core()->serializeToFile(path, true))
  {
    m_bootStateSaved = true;
    log(DR_LOG_INFO, qPrintable(QString("snapshotted boot to %1").arg(path)));
  }
  else
    log(DR_LOG_WARN, qPrintable(QString("could not snapshot boot to %1").arg(path)));
}

void MarioPartyN64::doApplyGameData(const DrGameData &data)
{
  m_lastScene = -1;
  m_minigameFrames = 0;

  if (bootsWithoutState())
  {
    const QString temp = QString::fromStdString(m_config.temp_state);

    /* A relaunch loads the snapshot this session's first boot took, which lands
     * back at the mini-game with none of the boot to sit through. Gated on the
     * flag rather than on the file existing: one left behind by an earlier run is
     * not ours to trust. */
    if (m_bootStateSaved && core()->unserializeFromFile(temp))
    {
      applyPlayers(data, true);
      return;
    }

    if (m_booted)
    {
      /* No snapshot to fall back on. retro_reset is a soft reset: it only
       * schedules an NMI about half a second out and leaves RDRAM standing, so
       * the game re-enters its boot long after this returns. Stamping now would
       * leave the players fighting that initialisation for the rest of their
       * hold, so wait the reboot out. */
      core()->reset();
      m_pendingPlayers = data;
      m_applyCountdown = MPN64_RESET_APPLY_DELAY_FRAMES;
      return;
    }

    /* First launch: run() walks the cold boot into the mini-game on its own.
     * Snapshot it once it gets there so nothing after this has to boot again. */
    m_booted = true;
    m_bootStateCountdown = MPN64_BOOT_STATE_CAPTURE_FRAMES;
    applyPlayers(data, false);
    return;
  }

  /* Load the state first, then write the players on top of it. */
  core()->unserializeFromFile(m_config.state.c_str());
  applyPlayers(data, true);
}

void MarioPartyN64::applyPlayers(const DrGameData &data, bool settled)
{
  unsigned characters[4] = { 0, 0, 0, 0 };

  /* A boot keeps initialising RAM long after the first frame, so the writes below
   * are re-stamped until it has settled rather than applied once. A state load is
   * already settled and only needs the single write. */
  const unsigned hold = MPN64_BOOT_HOLD_FRAMES;
  auto write = [&](int64_t val, const dr_value_t &value) {
    m_retro->writeValueForFrames(val, value, hold);
  };

  const bool hidden = hiddenCharacterPlayable(data.minigame);
  const bool free_for_all = data.minigame->type == DR_MINIGAME_4P;

  /* Duels are reachable only from the debug menu's BOARD branch, which is
   * patched to take the overlay straight out of this field instead of the menu
   * row -- so a duel sends its scene id where every other mini-game sends its
   * id. Both land in main RAM, which the debug overlay's load does not disturb. */
  const bool duel = data.minigame->type == DR_MINIGAME_DUEL;

  m_retro->writeValueForFrames(duel ? data.minigame->scene_id : data.minigame->minigame_id,
    m_config.minigame, hold ? hold : MPN64_MINIGAME_HOLD_FRAMES);
  /* GAME for the rest, BOARD for duels. Main RAM, so one write sticks. */
  if (m_config.debug_mode.address)
    m_retro->writeValueForFrames(duel ? 1 : 0, m_config.debug_mode,
      hold ? hold : MPN64_MINIGAME_HOLD_FRAMES);
  m_hiddenSlots = 0;

  for (unsigned i = 0; i < 4; i++)
    m_slotOf[i] = static_cast<int>(i);
  remapSlots();

  /* Anyone the game doesn't have takes a free slot rather than doubling up on
   * whoever their stand-in points at. */
  dr_resolve_characters(m_config.char_from_dr, data.players, m_config.roster_size, characters);

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = data.players[i];
    const unsigned slot = static_cast<unsigned>(m_slotOf[i]);
    uint8_t chr = static_cast<uint8_t>(characters[i]);

    /* This mini-game can play the hidden character, so send him in as himself
     * rather than as the stand-in the rest of the game gives him. */
    if (hidden && p.character == m_config.hidden.character)
    {
      chr = m_config.hidden.native_id;
      m_hiddenSlots |= 1 << slot;
      log(DR_LOG_INFO, qPrintable(QString("hidden character: P%1 %2 as 0x%3 in %4")
        .arg(i + 1).arg(dr_character_name(p.character))
        .arg(chr, 2, 16, QChar('0')).arg(data.minigame->name)));
    }

    /* The player moves into their slot with their own controller port, so a
     * remap changes who sits where without changing who holds the pad. */
    write(chr, m_config.character[slot]);
    write(p.control_port - 1, m_config.controller[slot]);

    /* An injected model names its deformation clusters after the character it
     * came from, but the engine looks them up by name using a table keyed on the
     * slot the model was injected into -- so it asks for Peach's names and gets
     * Daisy's. Point the table at the names the mesh actually carries instead of
     * rewriting the mesh, which also covers every LOD of that character at once.
     * Writes the "cNNN" head only; the family letter and "_0_DEF" tail stay.
     *
     * Not for duels. Those run in board context and load one of the ten board
     * costumes, 0xD4-0xDD, which the package leaves as the host character's and
     * which therefore still carry the host's names. Redirecting the table would
     * send the lookup after names those meshes do not have, and the duel comes
     * up black. Duels never touch the three replaced models, so leaving the
     * table alone costs nothing beyond the character wearing the host's
     * costume -- which is what the board would show anyway. */
    if (m_config.cluster_name_n && !duel && bootsWithoutState())
    {
      const uint32_t prefix = p.character == DR_CHARACTER_WALUIGI
                                ? m_config.cluster_prefix_waluigi
                                : p.character == DR_CHARACTER_DAISY
                                    ? m_config.cluster_prefix_daisy
                                    : 0;

      if (prefix)
      {
        const int off = m_config.cluster_name_stride * static_cast<int>(chr);

        write(prefix, { m_config.cluster_name_n + off, DR_VALUE_TYPE_U32 });
        write(prefix, { m_config.cluster_name_l + off, DR_VALUE_TYPE_U32 });
        log(DR_LOG_INFO, qPrintable(QString("cluster names: %1 in slot %2 -> %3")
          .arg(dr_character_name(p.character)).arg(chr)
          .arg(QString::fromLatin1(QByteArray::fromHex(QByteArray::number(prefix, 16))))));
      }
    }

    int64_t bot = 0;
    if (m_retro->readValue(&bot, m_config.bot[slot]) == DR_OK)
      write((bot & ~0x01) | (p.control_type == DR_CONTROL_TYPE_CPU ? 1 : 0), m_config.bot[slot]);

    write(mpN64Difficulty(p.difficulty), m_config.difficulty[slot]);

    /* Some 4P mini-games in MP3 want the raw index, so use it here */
    write(free_for_all ? slot : p.team_id, m_config.team[slot]);

    /* Carry coins and stars over */
    if (m_config.coins[slot].address)
      write(p.coins, m_config.coins[slot]);
    if (m_config.stars[slot].address)
      write(p.stars, m_config.stars[slot]);
  }

  /* The pot the board collected, for the battle results to pay back out. */
  if (m_config.battle_pot.address)
    write(data.battle_pot, m_config.battle_pot);
}

dr_minigame_result_t MarioPartyN64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index < 4)
  {
    const unsigned slot = static_cast<unsigned>(m_slotOf[index]);
    int64_t coins, bonus;

    if (m_retro->readValue(&coins, m_config.result[slot]) == DR_OK)
      result.coins = coins;
    if (!result.coins && m_resultSeen[slot])
      result.coins = static_cast<int>(m_resultSeen[slot]);
    if (m_retro->readValue(&bonus, m_config.bonus_result[slot]) == DR_OK)
      result.bonus_coins = bonus;
  }

  return result;
}

