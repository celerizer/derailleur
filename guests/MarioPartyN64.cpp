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
  if (!core->loadCore(m_config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(m_config.core.c_str())));
    m_valid = false;
  }
  /* Content is loaded lazily on the first launch (see DrGuest::applyGameData);
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

  int16_t val;
  if (m_retro->reads16(&val, m_config.scene_addr) == DR_OK && val != m_lastScene)
  {
    log(DR_LOG_INFO,
      qPrintable(QString("MP_SCENE_ADDR: 0x%1").arg((uint16_t)val, 4, 16, QChar('0'))));
    m_lastScene = val;
    if (m_minigameActive && m_minigameFrames >= 60 &&
        val != m_config.scene_miniexplain && val != m_minigame->scene_id)
      finishMinigame();
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
  /* Load the state first, then write the players on top of it. */
  core()->unserializeFromFile(m_config.state.c_str());
  m_lastScene = -1;
  m_minigameFrames = 0;
  int16_t id = static_cast<int16_t>(data.minigame->minigame_id);
  m_retro->writeForFrames(m_config.minigame_addr, &id, sizeof(id), 120);

  for (unsigned i = 0; i < 4; i++)
  {
    const dr_player_t &p = data.players[i];
    m_retro->writeu8(m_config.character_ids[p.character], m_config.character_addr[i]);
    m_retro->writeu8(static_cast<uint8_t>(p.control_port - 1), m_config.controller_addr[i]);

    uint8_t bot = 0;
    if (m_retro->readu8(&bot, m_config.bot_addr[i]) == DR_OK)
      m_retro->writeu8((bot & ~0x01) | (p.control_type == DR_CONTROL_TYPE_CPU ? 1 : 0),
        m_config.bot_addr[i]);

    m_retro->writeu8(mpN64Difficulty(p.difficulty), m_config.difficulty_addr[i]);
    m_retro->writeu8(static_cast<uint8_t>(p.team_id), m_config.team_addr[i]);
  }

  startMinigame();
}

dr_minigame_result_t MarioPartyN64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (index < 4)
  {
    int16_t coins, bonus;

    if (m_retro->reads16(&coins, m_config.result_addr[index]) == DR_OK)
      result.coins = coins;
    if (m_retro->reads16(&bonus, m_config.bonus_result_addr[index]) == DR_OK)
      result.bonus_coins = bonus;
  }

  return result;
}

