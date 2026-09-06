#include "MarioPartyGcn.h"

MarioPartyGcn::MarioPartyGcn(const MpGcnConfig &config, QRetro *sharedCore, QObject *parent)
  : DolphinGuest(parent)
  , m_config(config)
{
  m_retro = new DrRetro(sharedCore, this);
}

void MarioPartyGcn::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

void MarioPartyGcn::run()
{
  m_retro->tickFrameWrites();

  if (m_minigameActive)
    m_minigameFrames++;

  int64_t val;
  if (m_retro->readValue(&val, m_config.scene) == DR_OK && val != m_lastScene)
  {
    log(DR_LOG_INFO, qPrintable(QString("%1 scene: 0x%2").arg(name()).arg(val, 4, 16, QChar('0'))));
    m_lastScene = val;
    if (!m_minigameActive && val == m_config.scene_miniexplain)
      startMinigame();
    if (m_minigameActive && m_minigameFrames >= 60 &&
        val != m_config.scene_miniexplain && val != m_minigame->scene_id &&
        val != -1)
    {
      log(DR_LOG_INFO, qPrintable(QString("finishing minigame on scene 0x%1").arg(val, 4, 16, QChar('0'))));
      finishMinigame();
    }
  }
}

const dr_mp_minigame_t *MarioPartyGcn::minigames() const
{
  return m_config.minigames;
}

void MarioPartyGcn::doApplyGameData(const DrGameData &data)
{
  m_lastScene = -1;
  m_minigameFrames = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    m_slotOf[i] = static_cast<int>(i);
  }
  m_retro->writeValueForFrames(data.minigame->minigame_id, m_config.minigame, 120);
  applyPlayers();
}

dr_minigame_result_t MarioPartyGcn::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4)
  {
    unsigned slot = static_cast<unsigned>(m_slotOf[index]);
    int64_t coins;
    if (m_retro->readValue(&coins, m_config.result[slot]) == DR_OK)
      result.coins = coins;
    if (m_retro->readValue(&coins, m_config.bonus_result[slot]) == DR_OK)
      result.bonus_coins = coins;
  }
  return result;
}

/* doApplyGameData records all four players at once, then calls this so the duel
 * slot remap can see every player's team type together. */
void MarioPartyGcn::applyPlayers()
{
  unsigned characters[4] = { 0, 0, 0, 0 };

  for (unsigned i = 0; i < 4; i++)
    m_slotOf[i] = static_cast<int>(i);

  /* Mario Party 4 duel mini-games from story mode always use first two player indices */
  if (id() == DR_GUEST_MARIOPARTY4 && m_minigame && m_minigame->type == DR_MINIGAME_DUEL)
  {
    int slot = 0;
    for (unsigned i = 0; i < 4; i++)
      if (dr_team_type_participates(m_players[i].team_type))
        m_slotOf[i] = slot++;
    for (unsigned i = 0; i < 4; i++)
      if (!dr_team_type_participates(m_players[i].team_type))
        m_slotOf[i] = slot++;
  }

  /* Anyone the game doesn't have takes a free slot rather than doubling up on
   * whoever their stand-in points at. */
  dr_resolve_characters(m_config.char_from_dr, m_players, m_config.roster_size, characters);

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = m_players[i];
    unsigned slot = static_cast<unsigned>(m_slotOf[i]);

    m_retro->writeValue(characters[i], m_config.character[slot]);

    if (p.control_port != DR_CONTROL_PORT_INVALID && p.control_port < DR_CONTROL_PORT_SIZE)
      m_retro->writeValue(p.control_port - 1, m_config.controller[slot]);

    int64_t current = 0;
    int64_t is_bot = (p.control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
    if (m_retro->readValue(&current, m_config.bot[slot]) == DR_OK)
      m_retro->writeValue((current & ~0x01) | is_bot, m_config.bot[slot]);

    int64_t mp_difficulty;
    switch (p.difficulty)
    {
    case DR_DIFFICULTY_VERY_EASY:
    case DR_DIFFICULTY_EASY:
      mp_difficulty = 0x00;
      break;
    case DR_DIFFICULTY_NORMAL:
      mp_difficulty = 0x01;
      break;
    case DR_DIFFICULTY_HARD:
      mp_difficulty = 0x02;
      break;
    case DR_DIFFICULTY_VERY_HARD:
      mp_difficulty = 0x03;
      break;
    default:
      mp_difficulty = 0x01;
      break;
    }
    m_retro->writeValue(mp_difficulty, m_config.difficulty[slot]);

    m_retro->writeValue(p.team_id, m_config.team[slot]);

    /* Carry the board totals over, so a mini-game that shows coins/stars shows
     * the same numbers the host does. */
    if (m_config.coins[slot].address)
      m_retro->writeValue(p.coins, m_config.coins[slot]);
    if (m_config.stars[slot].address)
      m_retro->writeValue(p.stars, m_config.stars[slot]);
  }
}
