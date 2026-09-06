#include "MarioPartyN64.h"

#include <QFile>

MarioPartyN64::~MarioPartyN64()
{
  delete m_retro;
  m_retro = nullptr;
}

MarioPartyN64::MarioPartyN64(const MpN64Config &config, QObject *parent)
  : DrGuest(parent)
  , m_config(config)
{
  m_retro = new DrRetroN64(this);
  QRetro *core = new QRetro();
  core->setSavingEnabled(false);
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

void MarioPartyN64::run()
{
  m_retro->tickFrameWrites();

  if (m_minigameActive)
    m_minigameFrames++;

  int64_t val;
  if (m_retro->readValue(&val, m_config.scene) == DR_OK && val != m_lastScene)
  {
    const int16_t last = m_lastScene;

    log(DR_LOG_INFO,
      qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg((uint16_t)val, 4, 16, QChar('0'))));
    m_lastScene = val;
    if (!m_minigameActive &&
        (val == m_config.scene_miniexplain[0] || val == m_config.scene_miniexplain[1]))
    {
      seedRng();
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

void MarioPartyN64::doApplyGameData(const DrGameData &data)
{
  unsigned characters[4] = { 0, 0, 0, 0 };

  /* Load the state first, then write the players on top of it. */
  core()->unserializeFromFile(m_config.state.c_str());
  m_lastScene = -1;
  m_minigameFrames = 0;
  const bool hidden = hiddenCharacterPlayable(data.minigame);
  m_retro->writeValueForFrames(data.minigame->minigame_id, m_config.minigame, 120);
  m_hiddenSlots = 0;

  /* Anyone the game doesn't have takes a free slot rather than doubling up on
   * whoever their stand-in points at. */
  dr_resolve_characters(m_config.char_from_dr, data.players, m_config.roster_size, characters);

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = data.players[i];
    uint8_t chr = static_cast<uint8_t>(characters[i]);

    /* This mini-game can play the hidden character, so send him in as himself
     * rather than as the stand-in the rest of the game gives him. */
    if (hidden && p.character == m_config.hidden.character)
    {
      chr = m_config.hidden.native_id;
      m_hiddenSlots |= 1 << i;
      log(DR_LOG_INFO, qPrintable(QString("hidden character: P%1 %2 as 0x%3 in %4")
        .arg(i + 1).arg(dr_character_name(p.character))
        .arg(chr, 2, 16, QChar('0')).arg(data.minigame->name)));
    }

    m_retro->writeValue(chr, m_config.character[i]);
    m_retro->writeValue(p.control_port - 1, m_config.controller[i]);

    int64_t bot = 0;
    if (m_retro->readValue(&bot, m_config.bot[i]) == DR_OK)
      m_retro->writeValue((bot & ~0x01) | (p.control_type == DR_CONTROL_TYPE_CPU ? 1 : 0),
        m_config.bot[i]);

    m_retro->writeValue(mpN64Difficulty(p.difficulty), m_config.difficulty[i]);
    m_retro->writeValue(p.team_id, m_config.team[i]);

    /* Carry the board totals over, so a mini-game that shows coins/stars shows
     * the same numbers the host does. */
    if (m_config.coins[i].address)
      m_retro->writeValue(p.coins, m_config.coins[i]);
    if (m_config.stars[i].address)
      m_retro->writeValue(p.stars, m_config.stars[i]);
  }

  /* The pot the board collected, for the battle results to pay back out. */
  if (m_config.battle_pot.address)
    m_retro->writeValue(data.battle_pot, m_config.battle_pot);
}

dr_minigame_result_t MarioPartyN64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index < 4)
  {
    int64_t coins, bonus;

    if (m_retro->readValue(&coins, m_config.result[index]) == DR_OK)
      result.coins = coins;
    if (m_retro->readValue(&bonus, m_config.bonus_result[index]) == DR_OK)
      result.bonus_coins = bonus;
  }

  return result;
}

