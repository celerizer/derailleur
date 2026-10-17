#include "WariosWoods.h"

/// u8 -- Round wins for 1P (left field). Found by RAM diffing in fceumm.
static const dr_value_t WW_VS_WINS_1P = { 0x6E81, DR_VALUE_TYPE_U8 };

/// u8 -- Round wins for 2P (right field). The match is first to 3.
static const dr_value_t WW_VS_WINS_2P = { 0x6E82, DR_VALUE_TYPE_U8 };

static const int WW_VS_WINS_NEEDED = 3;

/// Frames to keep re-applying the preloaded win counters after state load
static const int WW_VS_PRELOAD_FRAMES = 120;

/// u8 -- RNG high byte. With 0x7B6D it advances every frame and seeds the
/// starting boards, which are built during the fade-in after the save state.
static const dr_value_t WW_RNG_HI = { 0x7B6C, DR_VALUE_TYPE_U8 };

/// u8 -- RNG low byte
static const dr_value_t WW_RNG_LO = { 0x7B6D, DR_VALUE_TYPE_U8 };

/* Toad's sprite palettes. The game builds its palette at 0x6227 (target) and
 * fades it through 0x6203 (working copy) before uploading to the PPU, so both
 * copies are overwritten every frame. Each address is palette index 1:
 *   index 1 = cap base (+ body outline)  -- default red, set to darker spot color
 *   index 2 = cap spots (+ vest)         -- default white, set to character color
 *   index 3 = skin                       -- default 0x27, kept
 * 1P uses sprite palette 0, 2P uses sprite palette 3. */
static const size_t WW_TOAD_PAL_ADDR[2][2] = {
  { 0x6238, 0x6214 }, /* 1P: default 16 30 27 */
  { 0x6244, 0x6220 }, /* 2P: default 1A 30 27 */
};

/// Same hue one PPU palette row darker; the darkest row falls back to black.
static uint8_t ww_darker(uint8_t c)
{
  return c >= 0x10 ? static_cast<uint8_t>(c - 0x10) : static_cast<uint8_t>(DrRetroNES::NES_COLOR_BLACK);
}
static const uint8_t WW_TOAD_SKIN = DrRetroNES::NES_COLOR_LIGHT_ORANGE;

/// Spot color for a board character (the SMB3 table's hat color)
static uint8_t ww_spot_color(dr_character c)
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

/// Frames to let the final "WIN!" banner show before ending the mini-game
static const int WW_VS_END_DELAY = 150;

/// u8 -- 1P level slider index (0 = 1-1 ... 10 = 3-3 ... 20 = 5-5)
static const dr_value_t WW_VS_LEVEL_1P = { 0x7B5D, DR_VALUE_TYPE_U8 };

/// u8 -- 2P level slider index
static const dr_value_t WW_VS_LEVEL_2P = { 0x7B5E, DR_VALUE_TYPE_U8 };

/// u8 -- "Monster select" set used by both fields. The menu offers 0, 2-7.
static const dr_value_t WW_VS_MONSTERS = { 0x7B5A, DR_VALUE_TYPE_U8 };

/* All mini-games load one savestate taken a couple frames after both players
 * confirmed on the VS setup screen; the boards are generated during the
 * fade-in, so the level and monster set are poked right after load.
 * minigame_id = (monster set << 5) | level slider index. */
#define WW_ID(monsters, level) (((monsters) << 5) | (level))

static const dr_mp_minigame_t WW_MINIGAMES[] = {
  { "Wario's Woods: VS Lv. 1-1", DR_MINIGAME_DUEL, WW_ID(0, 0), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Lv. 3-3", DR_MINIGAME_DUEL, WW_ID(0, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Lv. 5-5", DR_MINIGAME_DUEL, WW_ID(0, 20), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 2", DR_MINIGAME_DUEL, WW_ID(2, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 3", DR_MINIGAME_DUEL, WW_ID(3, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 4", DR_MINIGAME_DUEL, WW_ID(4, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 5", DR_MINIGAME_DUEL, WW_ID(5, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 6", DR_MINIGAME_DUEL, WW_ID(6, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wario's Woods: VS Monster Set 7", DR_MINIGAME_DUEL, WW_ID(7, 10), 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

WariosWoods::WariosWoods(QObject *parent)
  : DrGuestNES(parent)
{
  m_retro->init(coreId(), rom());
  core()->setBilinearFilter(false);
  core()->setIntegerScaling(true);
}

const dr_mp_minigame_t *WariosWoods::minigames() const
{
  return WW_MINIGAMES;
}

void WariosWoods::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_endFrames = 0;

  m_retro->loadState(state());

  const int id = data.minigame->minigame_id;
  m_retro->writeValue(id & 0x1F, WW_VS_LEVEL_1P);
  m_retro->writeValue(id & 0x1F, WW_VS_LEVEL_2P);
  m_retro->writeValue((id >> 5) & 0x07, WW_VS_MONSTERS);

  /* Randomize the boards from the shared dr_rand stream so netplay agrees */
  m_retro->writeValue(dr_rand() & 0xFF, WW_RNG_HI);
  m_retro->writeValue(dr_rand() & 0xFF, WW_RNG_LO);

  /* Single deciding round: both sides start one win short of the match */
  m_retro->writeValue(WW_VS_WINS_NEEDED - 1, WW_VS_WINS_1P);
  m_retro->writeValue(WW_VS_WINS_NEEDED - 1, WW_VS_WINS_2P);

  m_slotToPlayer[0] = m_slotToPlayer[1] = -1;
  core()->input()->clearPortRoutes();
  unsigned slot = 0;
  for (unsigned i = 0; i < 4 && slot < 2; i++)
  {
    if (!dr_team_type_participates(m_players[i].team_type))
      continue;
    m_slotToPlayer[slot] = static_cast<int>(i);
    m_spots[slot] = ww_spot_color(m_players[i].character);

    /* Route this NES slot (0 = 1P, 1 = 2P) to the player's physical controller port */
    if (m_players[i].control_port < DR_CONTROL_PORT_SIZE)
      core()->input()->setPortRoute(slot,
        static_cast<int>(m_players[i].control_port));

    slot++;
  }

  startMinigame();
}

void WariosWoods::run()
{
  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* The game zeroes the win counters ~24 frames into the intro; keep them
   * preloaded until the round is underway */
  if (m_minigameFrames < WW_VS_PRELOAD_FRAMES)
  {
    m_retro->writeValue(WW_VS_WINS_NEEDED - 1, WW_VS_WINS_1P);
    m_retro->writeValue(WW_VS_WINS_NEEDED - 1, WW_VS_WINS_2P);
  }

  /* Keep both Toads recolored; the game reloads its palette between rounds */
  for (unsigned slot = 0; slot < 2; slot++)
  {
    if (m_slotToPlayer[slot] < 0)
      continue;
    for (size_t addr : WW_TOAD_PAL_ADDR[slot])
    {
      m_retro->writeu8(ww_darker(m_spots[slot]), addr);
      m_retro->writeu8(m_spots[slot], addr + 1);
      m_retro->writeu8(WW_TOAD_SKIN, addr + 2);
    }
  }
  if (m_minigameFrames < 30)
    return;

  if (m_endFrames > 0)
  {
    if (++m_endFrames >= WW_VS_END_DELAY)
      finishMinigame();
    return;
  }

  int64_t wins_1p = 0, wins_2p = 0;
  if (m_retro->readValue(&wins_1p, WW_VS_WINS_1P) == DR_OK &&
      m_retro->readValue(&wins_2p, WW_VS_WINS_2P) == DR_OK &&
      (wins_1p >= WW_VS_WINS_NEEDED || wins_2p >= WW_VS_WINS_NEEDED))
    m_endFrames = 1;
}

dr_minigame_result_t WariosWoods::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t wins_1p = 0, wins_2p = 0;

  if (m_retro->readValue(&wins_1p, WW_VS_WINS_1P) != DR_OK ||
      m_retro->readValue(&wins_2p, WW_VS_WINS_2P) != DR_OK)
    return result;

  int winner = -1;
  if (wins_1p >= WW_VS_WINS_NEEDED && wins_1p > wins_2p)
    winner = 0;
  else if (wins_2p >= WW_VS_WINS_NEEDED && wins_2p > wins_1p)
    winner = 1;

  if (winner >= 0 && m_slotToPlayer[winner] == static_cast<int>(index))
    result.coins = 10;

  return result;
}

dr_sync_validator_t WariosWoods::syncValidator(void)
{
  dr_sync_validator_t sync = { 0, 0 };
  int64_t hi = 0, lo = 0;

  if (m_retro->readValue(&hi, WW_RNG_HI) == DR_OK &&
      m_retro->readValue(&lo, WW_RNG_LO) == DR_OK)
    sync.rng = static_cast<uint32_t>((hi << 8) | lo);
  sync.timer = static_cast<uint32_t>(m_minigameFrames);
  return sync;
}
