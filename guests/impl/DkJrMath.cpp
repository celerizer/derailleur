#include "DkJrMath.h"

/* RAM found by disassembly + verification in fceumm:
 *   $03F4-$03FB  window digits (1P, 2P); compared to placard at $051E by $D526
 *   $E5/$E6      set (nonzero) when 1P/2P matched the placard this round
 *   $05C7/$05C8  apples (round wins). $E11D increments it; at 5 the game resets
 *                both to 0 the same frame and returns to the title. */
static const dr_value_t DKJM_APPLES[2] = {
  { 0x05C7, DR_VALUE_TYPE_U8 }, { 0x05C8, DR_VALUE_TYPE_U8 }
};
static const dr_value_t DKJM_MATCHED[2] = {
  { 0x00E5, DR_VALUE_TYPE_U8 }, { 0x00E6, DR_VALUE_TYPE_U8 }
};

/// u8 -- Game mode: 0 = Calculate A, 1 = Calculate B
static const dr_value_t DKJM_MODE = { 0x00EF, DR_VALUE_TYPE_U8 };

/// Apples needed to win; a match on the last apple ends the game
static const int64_t DKJM_APPLES_NEEDED = 5;

/// Frames to keep re-applying the preloaded apples after state load
static const int DKJM_PRELOAD_FRAMES = 120;

/* Apple sprites in the OAM shadow. The game only draws an apple when it awards
 * one, so the preloaded ones are drawn here: y 0x30, tile 0xAE, attr 3, 8px
 * apart starting at x 0x38 (1P, OAM 0x250) and x 0xC0 (2P, OAM 0x264). */
static const size_t DKJM_APPLE_OAM[2] = { 0x0250, 0x0264 };
static const uint8_t DKJM_APPLE_X[2] = { 0x38, 0xC0 };

/// 8-byte PRNG state. Picks the placard target, vine numbers and operator order.
static const size_t DKJM_RNG_ADDR = 0x0018;
static const size_t DKJM_RNG_SIZE = 8;

/// Frames to let the winner jump before ending the mini-game
static const int DKJM_END_DELAY = 150;

/* Sprite palettes in PPU memory. The game only uploads them at boot, so direct
 * PPU writes stick; they are still rewritten every frame to be safe.
 *   index 1 = overalls (default white 0x30)  -- darker character color
 *   index 2 = fur (1P 0x06, 2P 0x25)          -- character color
 *   index 3 = skin (0x36)                     -- kept
 * 1P Junior uses sprite palette 1, 2P (pink) Junior uses sprite palette 0. */
static const size_t DKJM_PAL_ADDR[2] = { 0x80003015, 0x80003011 };

static const dr_mp_minigame_t DKJM_MINIGAMES[] = {
  { "DK Jr. Math: Calculate A", DR_MINIGAME_DUEL, 0, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DK Jr. Math: Calculate B", DR_MINIGAME_DUEL, 1, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/// Same hue one PPU palette row darker; the darkest row falls back to black.
static uint8_t dkjm_darker(uint8_t c)
{
  return c >= 0x10 ? static_cast<uint8_t>(c - 0x10) : static_cast<uint8_t>(DrRetroNES::NES_COLOR_BLACK);
}

/// Main color for a board character (the SMB3 table's hat color)
static uint8_t dkjm_color(dr_character c)
{
  switch (c)
  {
  case DR_CHARACTER_LUIGI: return DrRetroNES::NES_COLOR_MEDIUM_GREEN;
  case DR_CHARACTER_WARIO: return DrRetroNES::NES_COLOR_LIGHT_YELLOW;
  case DR_CHARACTER_WALUIGI: return DrRetroNES::NES_COLOR_MEDIUM_MAGENTA;
  case DR_CHARACTER_YOSHI: return DrRetroNES::NES_COLOR_MEDIUM_SPRING;
  case DR_CHARACTER_DONKEY_KONG: return DrRetroNES::NES_COLOR_DARK_ORANGE;
  case DR_CHARACTER_PEACH: return DrRetroNES::NES_COLOR_LIGHT_ROSE;
  case DR_CHARACTER_DAISY: return DrRetroNES::NES_COLOR_LIGHT_ORANGE;
  case DR_CHARACTER_BOO: return DrRetroNES::NES_COLOR_LIGHT_GRAY;
  case DR_CHARACTER_TOAD: return DrRetroNES::NES_COLOR_MEDIUM_RED;
  case DR_CHARACTER_KOOPA_KID: return DrRetroNES::NES_COLOR_DARK_GREEN;
  case DR_CHARACTER_KOOPA_KID_R: return DrRetroNES::NES_COLOR_MEDIUM_RED;
  case DR_CHARACTER_KOOPA_KID_G: return DrRetroNES::NES_COLOR_MEDIUM_GREEN;
  case DR_CHARACTER_KOOPA_KID_B: return DrRetroNES::NES_COLOR_MEDIUM_BLUE;
  case DR_CHARACTER_TOADETTE: return DrRetroNES::NES_COLOR_LIGHT_MAGENTA;
  case DR_CHARACTER_BIRDO: return DrRetroNES::NES_COLOR_MEDIUM_RED;
  case DR_CHARACTER_DRY_BONES: return DrRetroNES::NES_COLOR_LIGHT_GRAY;
  case DR_CHARACTER_BLOOPER: return DrRetroNES::NES_COLOR_DARK_BLUE;
  case DR_CHARACTER_HAMMER_BRO: return DrRetroNES::NES_COLOR_MEDIUM_GREEN;
  case DR_CHARACTER_MARIO:
  default: return DrRetroNES::NES_COLOR_MEDIUM_RED;
  }
}

DkJrMath::DkJrMath(QObject *parent)
  : DrGuestNES(parent)
{
  m_retro->init(coreId(), rom());
  core()->setBilinearFilter(false);
  core()->setIntegerScaling(true);
}

const dr_mp_minigame_t *DkJrMath::minigames() const
{
  return DKJM_MINIGAMES;
}

void DkJrMath::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_endFrames = 0;
  m_winnerSide = -1;

  m_retro->loadState(state());

  /* The state is 2 frames after starting Calculate A; the mode is still read
   * from RAM, so poke it to pick Calculate B */
  m_retro->writeValue(data.minigame->minigame_id == 1 ? 1 : 0, DKJM_MODE);

  /* Randomize targets/vines from the shared dr_rand stream so netplay agrees.
   * Force one bit so the state is never all zero. */
  for (size_t i = 0; i < DKJM_RNG_SIZE; i++)
  {
    uint8_t v = static_cast<uint8_t>(dr_rand() & 0xFF);
    if (i == 0)
      v |= 0x01;
    m_retro->writeu8(v, DKJM_RNG_ADDR + i);
  }

  /* Single deciding round: both sides start one apple short of the match */
  for (int side = 0; side < 2; side++)
    m_retro->writeValue(DKJM_APPLES_NEEDED - 1, DKJM_APPLES[side]);

  m_slotToPlayer[0] = m_slotToPlayer[1] = -1;
  core()->input()->clearPortRoutes();
  unsigned slot = 0;
  for (unsigned i = 0; i < 4 && slot < 2; i++)
  {
    if (!dr_team_type_participates(m_players[i].team_type))
      continue;
    m_slotToPlayer[slot] = static_cast<int>(i);
    m_fur[slot] = dkjm_color(m_players[i].character);

    /* Route this NES slot (0 = 1P, 1 = 2P) to the player's physical controller port */
    if (m_players[i].control_port < DR_CONTROL_PORT_SIZE)
      core()->input()->setPortRoute(slot,
        static_cast<int>(m_players[i].control_port));

    slot++;
  }

  startMinigame();
}

void DkJrMath::run()
{
  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  for (unsigned slot = 0; slot < 2; slot++)
  {
    if (m_slotToPlayer[slot] < 0)
      continue;
    m_retro->writeu8(dkjm_darker(m_fur[slot]), DKJM_PAL_ADDR[slot]);
    m_retro->writeu8(m_fur[slot], DKJM_PAL_ADDR[slot] + 1);
  }

  if (m_endFrames == 0)
  {
    for (int side = 0; side < 2; side++)
    {
      if (m_minigameFrames < DKJM_PRELOAD_FRAMES)
        m_retro->writeValue(DKJM_APPLES_NEEDED - 1, DKJM_APPLES[side]);

      /* Draw the preloaded apples */
      for (unsigned n = 0; n + 1 < DKJM_APPLES_NEEDED; n++)
      {
        const uint8_t sprite[4] = { 0x30, 0xAE, 0x03,
          static_cast<uint8_t>(DKJM_APPLE_X[side] + 8 * n) };
        for (size_t b = 0; b < 4; b++)
          m_retro->writeu8(sprite[b], DKJM_APPLE_OAM[side] + 4 * n + b);
      }
    }
  }

  if (m_endFrames > 0)
  {
    if (++m_endFrames >= DKJM_END_DELAY)
      finishMinigame();
    return;
  }

  if (m_minigameFrames < 30)
    return;

  /* The game zeroes the apples the frame the 5th is awarded, so catch the
   * winning match instead: a side with 4 apples has just matched the placard. */
  for (int side = 0; side < 2; side++)
  {
    int64_t apples = 0, matched = 0;
    if (m_retro->readValue(&apples, DKJM_APPLES[side]) == DR_OK &&
        m_retro->readValue(&matched, DKJM_MATCHED[side]) == DR_OK &&
        apples >= DKJM_APPLES_NEEDED - 1 && matched)
    {
      m_winnerSide = side;
      m_endFrames = 1;
      return;
    }
  }
}

dr_minigame_result_t DkJrMath::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (m_winnerSide >= 0 && m_slotToPlayer[m_winnerSide] == static_cast<int>(index))
    result.coins = 10;

  return result;
}

dr_sync_validator_t DkJrMath::syncValidator(void)
{
  dr_sync_validator_t sync = { 0, 0 };
  uint32_t rng = 0;

  for (size_t i = 0; i < 4; i++)
  {
    uint8_t v = 0;
    if (m_retro->readu8(&v, DKJM_RNG_ADDR + i) != DR_OK)
      return sync;
    rng = (rng << 8) | v;
  }
  sync.rng = rng;
  sync.timer = static_cast<uint32_t>(m_minigameFrames);
  return sync;
}
