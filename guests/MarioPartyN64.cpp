#include "MarioPartyN64.h"

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
  if (!core->loadContent(m_config.game.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(m_config.game.c_str())));
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

void MarioPartyN64::doSetMinigame(const dr_mp_minigame_t *minigame)
{
  core()->unserializeFromFile(m_config.state.c_str());
  m_lastScene = -1;
  m_minigameFrames = 0;
  int16_t id = static_cast<int16_t>(minigame->minigame_id);
  m_retro->writeForFrames(m_config.minigame_addr, &id, sizeof(id), 120);
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

dr_error MarioPartyN64::doSetPlayerCharacter(unsigned index, dr_character character)
{
  return m_retro->writeu8(m_config.character_ids[character], m_config.character_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerControlPort(unsigned index, dr_control_port control_port)
{
  return m_retro->writeu8(static_cast<uint8_t>(control_port - 1), m_config.controller_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint8_t current, is_bot;

  is_bot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;

  if (m_retro->readu8(&current, m_config.bot_addr[index]) != DR_OK)
    return DR_ERR_MEMORY_ACCESS_CORE;
  else
    return m_retro->writeu8((current & ~0x01) | is_bot, m_config.bot_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  uint8_t mp_difficulty;

  switch (difficulty)
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
  }

  return m_retro->writeu8(mp_difficulty, m_config.difficulty_addr[index]);
}

dr_error MarioPartyN64::doSetPlayerTeam(
  unsigned index, dr_team_color color, dr_team_type type, unsigned team_id)
{
  (void)color;
  (void)type;
  return m_retro->writeu8(static_cast<uint8_t>(team_id), m_config.team_addr[index]);
}
