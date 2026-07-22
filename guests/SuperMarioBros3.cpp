#include "SuperMarioBros3.h"

#include <QFile>

/// u8 The current Vs. mode map
static const size_t SMB3_VS_MAP_ADDR = 0x7F24;

/// u8 bool -- 0 if Mario won, 1 if Luigi won. Set once the game has fully faded to black
static const size_t SMB3_VS_WINNER_ADDR = 0x073E;

/// u8 bool -- Set to 1 when the mini-game is finished
static const size_t SMB3_VS_GAME_OVER_ADDR = 0x007D;

/// u8 [4] -- PPU Mario colors. Typically 00 16 36 12
static const size_t SMB3_VS_MARIO_PPU_ADDR = 0x80003010;

/// u8 [4] -- PPU Luigi colors. Typically 00 1A 36 0C
static const size_t SMB3_VS_LUIGI_PPU_ADDR = 0x80003014;

typedef struct
{
  uint8_t invisible;
  uint8_t hat;
  uint8_t skin;
  uint8_t overalls;
} smb3_player_colors_t;

typedef enum
{
  SMB3_VS_MAP_SPINY = 0,  /* Spiny only */
  SMB3_VS_MAP_FIGHTER_FLY = 1,  /* Fighter Fly only */
  SMB3_VS_MAP_SPINY_FIGHTER_FLY = 2,  /* Spiny and Fighter Fly */
  SMB3_VS_MAP_STATIC_COINS = 3,  /* Static coins */
  SMB3_VS_MAP_SPINY_SIDESTEPPER = 4,  /* Spiny and Sidestepper */
  SMB3_VS_MAP_FIGHTER_FLY_SIDESTEPPER = 5,  /* Fighter Fly and Sidestepper */
  SMB3_VS_MAP_SIDESTEPPER = 6,  /* Sidestepper only */
  SMB3_VS_MAP_COIN_FOUNTAIN = 7,  /* Coin Fountain (Fountain battlefield) */
  SMB3_VS_MAP_SPINY_ALT = 8,  /* Spiny only (alt) */
  SMB3_VS_MAP_FIGHTER_FLY_ALT = 9,  /* Fighter Fly only (alt) */
  SMB3_VS_MAP_SIDESTEPPER_ALT = 10, /* Sidestepper only (alt) */
  SMB3_VS_MAP_LADDERS = 11  /* Ladder and [?] blocks (Ladders battlefield) */
} smb3_vs_map;

static const dr_mp_minigame_t SMB3_MINIGAMES[] = {
  { "SMB3: Vs. Spinies", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY, 0x00, DR_NO_QUIRKS },
  { "SMB3: Vs. Fighter Flies", DR_MINIGAME_DUEL, SMB3_VS_MAP_FIGHTER_FLY, 0x00, DR_NO_QUIRKS },
  { "SMB3: Vs. Spinies & Flies", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY_FIGHTER_FLY, 0x00, DR_NO_QUIRKS },
  { "SMB3: Get the Coins", DR_MINIGAME_DUEL, SMB3_VS_MAP_STATIC_COINS, 0x00, DR_NO_QUIRKS },
  { "SMB3: Vs. Spinies & Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY_SIDESTEPPER, 0x00, DR_NO_QUIRKS },
  { "SMB3: Vs. Flies & Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_FIGHTER_FLY_SIDESTEPPER, 0x00, DR_NO_QUIRKS },
  { "SMB3: Vs. Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_SIDESTEPPER, 0x00, DR_NO_QUIRKS },
  { "SMB3: Avoid the Flames", DR_MINIGAME_DUEL, SMB3_VS_MAP_COIN_FOUNTAIN, 0x00, DR_NO_QUIRKS },
  { "SMB3: Hidden Blocks", DR_MINIGAME_DUEL, SMB3_VS_MAP_LADDERS, 0x00, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

SuperMarioBros3::SuperMarioBros3(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetro(this);

  QString corePath = dr_core_path(DR_CORE_FCEUMM);
  QString gamePath = dr_roms_directory() + "/Super Mario Bros. 3 (USA) (Rev 1).nes";
  m_gamePath = gamePath.toStdString();

  QRetro *core = new QRetro();
  core->setSavingEnabled(false);
  if (!core->loadCore(corePath.toUtf8().constData()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(corePath)));
    m_valid = false;
  }
  if (!QFile::exists(gamePath))
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found: %1").arg(gamePath)));
    m_valid = false;
  }
  m_retro->setCore(core, true);
}

void SuperMarioBros3::startCore()
{
  if (auto *c = core())
    connect(c, &QRetro::frameBegin, this, [this]() { run(); }, Qt::DirectConnection);
  m_retro->startCore();
}

const dr_mp_minigame_t *SuperMarioBros3::minigames() const
{
  return SMB3_MINIGAMES;
}

/* Vs. sprite recolor per character. Mario and Luigi use their normal SMB3
 * palettes; Wario and Waluigi get custom ones. { invisible, hat, skin, overalls } */
static smb3_player_colors_t smb3ColorsFor(dr_character c)
{
  switch (c)
  {
  case DR_CHARACTER_LUIGI: /* 00 1A 36 0C */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_MEDIUM_GREEN, NES_COLOR_PALE_RED, NES_COLOR_DARK_CYAN };
  case DR_CHARACTER_WARIO: /* 00 28 37 04 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_LIGHT_YELLOW, NES_COLOR_PALE_ORANGE, NES_COLOR_DARK_MAGENTA };
  case DR_CHARACTER_WALUIGI: /* 00 14 37 0F */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_MEDIUM_MAGENTA, NES_COLOR_PALE_ORANGE, NES_COLOR_BLACK };
  case DR_CHARACTER_YOSHI: /* 00 1B 1A 20 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_MEDIUM_SPRING, NES_COLOR_MEDIUM_GREEN, NES_COLOR_WHITE };
  case DR_CHARACTER_DONKEY_KONG: /* 00 07 37 07 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_DARK_ORANGE, NES_COLOR_PALE_ORANGE, NES_COLOR_DARK_ORANGE };
  case DR_CHARACTER_PEACH: /* 00 25 37 15 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_LIGHT_ROSE, NES_COLOR_PALE_ORANGE, NES_COLOR_MEDIUM_ROSE };
  case DR_CHARACTER_DAISY: /* 00 27 37 17 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_LIGHT_ORANGE, NES_COLOR_PALE_ORANGE, NES_COLOR_MEDIUM_ORANGE };
  case DR_CHARACTER_BOO: /* 00 20 30 00 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_WHITE, NES_COLOR_PALE_GRAY, NES_COLOR_DARK_GRAY };
  case DR_CHARACTER_TOAD: /* 00 26 30 16 */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_LIGHT_RED, NES_COLOR_PALE_GRAY, NES_COLOR_MEDIUM_RED };
  case DR_CHARACTER_KOOPA_KID: /* 00 0A 27 0B */
    return { NES_COLOR_DARK_GRAY, NES_COLOR_DARK_GREEN, NES_COLOR_LIGHT_ORANGE, NES_COLOR_DARK_SPRING };
  case DR_CHARACTER_MARIO: /* 00 16 36 12 */
  default:
    return { NES_COLOR_DARK_GRAY, NES_COLOR_MEDIUM_RED, NES_COLOR_PALE_RED, NES_COLOR_MEDIUM_BLUE };
  }
}

void SuperMarioBros3::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  for (unsigned i = 0; i < 4; i++)
    m_players[i] = data.players[i];

  core()->unserializeFromFile(dr_state_directory() + "/supermariobros3.state.zip");

  /* Select the Vs. map for this mini-game (the entry's id); hold it as the match
   * spins up. */
  uint8_t map = static_cast<uint8_t>(data.minigame->minigame_id);
  m_retro->writeForFrames(SMB3_VS_MAP_ADDR, &map, sizeof(map), 30);

  /* Recolor the two Vs. duelists to match their characters. The participating
   * players (team_type != invalid) fill the Mario and Luigi sprite slots; hold the
   * write for 30 frames so it sticks as the match spins up. */
  const size_t ppuAddr[2] = { SMB3_VS_MARIO_PPU_ADDR, SMB3_VS_LUIGI_PPU_ADDR };
  m_slotToPlayer[0] = m_slotToPlayer[1] = -1;
  core()->input()->clearPortRoutes();
  unsigned slot = 0;
  for (unsigned i = 0; i < 4 && slot < 2; i++)
  {
    if (m_players[i].team_type == DR_TEAM_TYPE_INVALID)
      continue;
    smb3_player_colors_t colors = smb3ColorsFor(m_players[i].character);
    m_retro->writeForFrames(ppuAddr[slot], &colors, sizeof(colors), 30);
    m_slotToPlayer[slot] = static_cast<int>(i);

    /* Route this NES slot (0 = Mario, 1 = Luigi) to read the player's physical
     * controller port, so whichever board players are duelling drive the match. */
    if (m_players[i].control_port >= DR_CONTROL_PORT_P1 &&
        m_players[i].control_port <= DR_CONTROL_PORT_P4)
      core()->input()->setPortRoute(slot,
        static_cast<int>(m_players[i].control_port - DR_CONTROL_PORT_P1));

    slot++;
  }

  m_retro->core()->audio()->setMute(true);

  startMinigame();
}

void SuperMarioBros3::run()
{
  m_retro->tickFrameWrites();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* Mute for first 30 frames of execution to ignore a beep */
  if (m_minigameFrames > 30)
    m_retro->core()->audio()->setMute(false);

  /* The game-over flag is set once the match ends. Guard with a few frames so a
   * stale flag from the loaded state can't finish us instantly. */
  uint8_t over = 0;
  if (m_minigameFrames >= 30 &&
      m_retro->readu8(&over, SMB3_VS_GAME_OVER_ADDR) == DR_OK && over)
    finishMinigame();
}

dr_minigame_result_t SuperMarioBros3::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  uint8_t winner = 0;

  /* The winner byte is the sprite slot (0 = Mario, 1 = Luigi); map it back to the
   * board player that filled that slot. */
  if (m_retro->readu8(&winner, SMB3_VS_WINNER_ADDR) == DR_OK &&
      winner < 2 && m_slotToPlayer[winner] == static_cast<int>(index))
    result.coins = 10;

  return result;
}
