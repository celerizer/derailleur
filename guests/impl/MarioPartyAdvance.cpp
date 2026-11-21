#include "MarioPartyAdvance.h"

#include <QFile>
#include <QRetro.h>
#include <QRetroDirectories.h>

static const dr_value_t MPA_MINIGAME_ACTIVE = { 0x0300440C, DR_VALUE_TYPE_U8 };
static const dr_value_t MPA_MINIGAME_ID = { 0x0300440D, DR_VALUE_TYPE_U8 };
static const dr_value_t MPA_PLAYER_1 = { 0x0300440E, DR_VALUE_TYPE_U8 };
static const dr_value_t MPA_PLAYER_2 = { 0x0300440F, DR_VALUE_TYPE_U8 };

/* Parameter block the game hands to a mini-game overlay */
static const dr_value_t MPA_MG_LEVEL = { 0x02030001, DR_VALUE_TYPE_U8 };
static const dr_value_t MPA_MG_RESULT = { 0x02030003, DR_VALUE_TYPE_S8 };
static const dr_value_t MPA_MG_CHARACTER = { 0x0203000A, DR_VALUE_TYPE_U8 };
static const dr_value_t MPA_MG_CHALLENGE = { 0x02030013, DR_VALUE_TYPE_U8 };

#define MPA_TYPE_1_WINDOW 1
#define MPA_TYPE_2_WINDOW 2
#define MPA_TYPE_4_WINDOW 3

enum
{
  MPA_COMPATICOM   = 0x04,
  MPA_SHROOM_SLIDE = 0x07,
  MPA_STICK_TO_IT  = 0x08,
  MPA_SHROOM_DROP  = 0x14,
  MPA_4P_PINBALL   = 0x16,
  MPA_ATTACK_FROG  = 0x1A,
  MPA_BOMB_GAME    = 0x1E,
  MPA_EGG_PANIC    = 0x22,
  MPA_BLOCK_PUNCH  = 0x23,
  MPA_DART_ATTACK  = 0x25,
  MPA_CHICKEN_RACE = 0x27,

  MPA_BOO_BYE      = 0x44,
  MPA_GRABBIT      = 0x45,
  MPA_CHOMP_WALKER = 0x46,
  MPA_CLOUD_CLIMB  = 0x47,
  MPA_BARREL_PERIL = 0x48,
  MPA_BIG_POPPER   = 0x49,
  MPA_FOREST_JUMP  = 0x4A,
  MPA_SWITCH_WAY   = 0x4B,
  MPA_AMPLIFRIED   = 0x4C,
  MPA_FLINGSHOT    = 0x4D,
  MPA_SPOOKY_SPIKE = 0x4E,
  MPA_BOB_OOOM     = 0x4F,
  MPA_REEL_CHEEP   = 0x50,
  MPA_SHELL_STACK  = 0x51,
  MPA_BILL_BOUNCE  = 0x52,
  MPA_BUNNY_BELT   = 0x53,
  MPA_PEST_ASIDE   = 0x54,
  MPA_MELON_FOLLY  = 0x55,
  MPA_SORT_STACK   = 0x56,
  MPA_ON_THE_SPOT  = 0x57,
  MPA_KOOPA_KRUNCH = 0x59,
  MPA_FLOOR_IT     = 0x5A,
  MPA_DREADMILL    = 0x5B,
  MPA_STOMPBOT_XL  = 0x5C,
  MPA_GO_GO_POGO   = 0x5D,
  MPA_SLED_SLIDE   = 0x5E,
  MPA_FLIPPIN_OUT  = 0x5F,
  MPA_SEE_MONKEY   = 0x60,
  MPA_OUTTA_MY_WAY = 0x61,
  MPA_BROOM_ZOOM   = 0x62,
};

static const dr_mp_minigame_t MPA_MINIGAMES[] =
{
  { "Compat-I-Com", DR_MINIGAME_INVALID, MPA_COMPATICOM,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shroom Slide", DR_MINIGAME_INVALID, MPA_SHROOM_SLIDE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stick to It",  DR_MINIGAME_INVALID, MPA_STICK_TO_IT,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shroom Drop",  DR_MINIGAME_INVALID, MPA_SHROOM_DROP,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "4-P Piball",   DR_MINIGAME_INVALID, MPA_4P_PINBALL,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Attack Frog",  DR_MINIGAME_INVALID, MPA_ATTACK_FROG,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bomb Game",    DR_MINIGAME_INVALID, MPA_BOMB_GAME,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Egg Panic",    DR_MINIGAME_INVALID, MPA_EGG_PANIC,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Block Punch",  DR_MINIGAME_INVALID, MPA_BLOCK_PUNCH,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  // 24 toad force v
  { "Dart Attack",  DR_MINIGAME_INVALID, MPA_DART_ATTACK,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Chicken Race", DR_MINIGAME_INVALID, MPA_CHICKEN_RACE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Boo Bye",      DR_MINIGAME_1P, MPA_BOO_BYE,      MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Grabbit",      DR_MINIGAME_1P, MPA_GRABBIT,      MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Chomp Walker", DR_MINIGAME_1P, MPA_CHOMP_WALKER, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cloud Climb",  DR_MINIGAME_1P, MPA_CLOUD_CLIMB,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Barrel Peril", DR_MINIGAME_1P, MPA_BARREL_PERIL, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Big Popper",   DR_MINIGAME_1P, MPA_BIG_POPPER,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Forest Jump",  DR_MINIGAME_1P, MPA_FOREST_JUMP,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Switch Way?",  DR_MINIGAME_1P, MPA_SWITCH_WAY,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Amplifried",   DR_MINIGAME_1P, MPA_AMPLIFRIED,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Flingshot",    DR_MINIGAME_1P, MPA_FLINGSHOT,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Spooky Spike", DR_MINIGAME_1P, MPA_SPOOKY_SPIKE, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bob-OOOM!",    DR_MINIGAME_1P, MPA_BOB_OOOM,     MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Reel Cheep",   DR_MINIGAME_1P, MPA_REEL_CHEEP,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Shell Stack",  DR_MINIGAME_1P, MPA_SHELL_STACK,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bill Bounce",  DR_MINIGAME_1P, MPA_BILL_BOUNCE,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bunny Belt",   DR_MINIGAME_1P, MPA_BUNNY_BELT,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pest Aside",   DR_MINIGAME_1P, MPA_PEST_ASIDE,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Melon Folly",  DR_MINIGAME_1P, MPA_MELON_FOLLY,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sort Stack",   DR_MINIGAME_1P, MPA_SORT_STACK,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "On the Spot",  DR_MINIGAME_1P, MPA_ON_THE_SPOT,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Koopa Krunch", DR_MINIGAME_1P, MPA_KOOPA_KRUNCH, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Floor It!",    DR_MINIGAME_1P, MPA_FLOOR_IT,     MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dreadmill",    DR_MINIGAME_1P, MPA_DREADMILL,    MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stompbot XL",  DR_MINIGAME_1P, MPA_STOMPBOT_XL,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Go-go Pogo",   DR_MINIGAME_1P, MPA_GO_GO_POGO,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sled Slide",   DR_MINIGAME_1P, MPA_SLED_SLIDE,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Flippin' Out", DR_MINIGAME_1P, MPA_FLIPPIN_OUT,  MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "See Monkey?",  DR_MINIGAME_1P, MPA_SEE_MONKEY,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Outta My Way", DR_MINIGAME_1P, MPA_OUTTA_MY_WAY, MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Broom Zoom",   DR_MINIGAME_1P, MPA_BROOM_ZOOM,   MPA_TYPE_1_WINDOW, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/* Mario Party Advance's own character id (0-3) for a board character */
static unsigned mpa_character(dr_character character, unsigned fallback)
{
  switch (character)
  {
  case DR_CHARACTER_MARIO:
  case DR_CHARACTER_WARIO:
    return 0;
  case DR_CHARACTER_LUIGI:
  case DR_CHARACTER_WALUIGI:
    return 1;
  case DR_CHARACTER_PEACH:
  case DR_CHARACTER_DAISY:
    return 2;
  case DR_CHARACTER_YOSHI:
  case DR_CHARACTER_DONKEY_KONG:
    return 3;
  default:
    return fallback;
  }
}

MarioPartyAdvance::MarioPartyAdvance(QObject *parent)
  : DrGuestRetro(parent)
{
  setRetro(new DrRetro(this));
  m_retro->init(coreId(), rom());
  core()->setBilinearFilter(false);
  core()->setIntegerScaling(true);
}

const dr_mp_minigame_t *MarioPartyAdvance::minigames() const
{
  return MPA_MINIGAMES;
}

dr_minigame_result_t MarioPartyAdvance::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t outcome = 0;

  if (m_minigame && m_minigame->type == DR_MINIGAME_1P)
  {
    if (static_cast<int>(index) == m_soloPlayer &&
        m_retro->readValue(&outcome, MPA_MG_RESULT) == DR_OK && outcome > 0)
      result.coins = 10;
  }
  else if (m_winners & (1u << index))
    result.coins = 10;

  return result;
}

void MarioPartyAdvance::doApplyGameData(const DrGameData &data)
{
  const dr_mp_minigame_t *minigame = data.minigame;
  unsigned i;

  m_gameStarted = false;
  m_winners = 0;
  m_EndWaitFrames = 0;
  m_soloPlayer = -1;
  m_retro->loadState(state());
  m_retro->writeValue(minigame->minigame_id, MPA_MINIGAME_ID);
  core()->input()->clearPortRoutes();

  /* Only the first two board players map onto Mario Party Advance's two slots. */
  for (i = 0; i < 2; i++)
    m_retro->writeValue(mpa_character(data.players[i].character, i),
                        i ? MPA_PLAYER_2 : MPA_PLAYER_1);

  if (minigame->type == DR_MINIGAME_1P)
  {
    const dr_player_t *solo;

    for (i = 0; i < 4 && m_soloPlayer < 0; i++)
      if (data.players[i].team_type == DR_TEAM_TYPE_SOLO)
        m_soloPlayer = static_cast<int>(i);
    if (m_soloPlayer < 0)
      m_soloPlayer = 0;
    solo = &data.players[m_soloPlayer];

    /* Goal mode at the harder target tier instead of chasing a high score */
    m_retro->writeValue(1, MPA_MG_CHALLENGE);
    m_retro->writeValue(2, MPA_MG_LEVEL);
    m_retro->writeValue(0, MPA_MG_RESULT);
    m_retro->writeValue(mpa_character(solo->character, 0), MPA_MG_CHARACTER);

    if (solo->control_port < DR_CONTROL_PORT_SIZE)
      core()->input()->setPortRoute(0, static_cast<int>(solo->control_port));
  }

  startMinigame();
}

void MarioPartyAdvance::run4pPinball()
{
  static const size_t MPA_4PP_PLAYER_OUT = 0x03006688;
  static const dr_value_t MPA_4PP_PLAYER_OUT_COUNT = { 0x0300668C, DR_VALUE_TYPE_U32 };
  int64_t out_count = 0;

  if (m_retro->readValue(&out_count, MPA_4PP_PLAYER_OUT_COUNT) != DR_OK)
    return;

  if (out_count >= 3)
  {
    if (!m_winners)
    {
      for (unsigned i = 0; i < 4; i++)
      {
        uint8_t out = 0;
        m_retro->readu8(&out, MPA_4PP_PLAYER_OUT + i);
        if (!out)
          m_winners |= (1u << i);
      }
    }
    ++m_EndWaitFrames;
    if (m_EndWaitFrames > 248)
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, false);
    else if (m_EndWaitFrames >= 240)
      core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_START, true);
  }
}

void MarioPartyAdvance::run()
{
  if (!m_minigame || !m_minigameActive)
    return;

  int64_t active = 0;
  if (!m_retro || m_retro->readValue(&active, MPA_MINIGAME_ACTIVE) != DR_OK)
    return;

  if (!m_gameStarted)
  {
    if (active == 1)
      m_gameStarted = true;
  }
  else if (active == 0)
  {
    finishMinigame();
  }
  else
  {
    switch (m_minigame->minigame_id)
    {
    case MPA_4P_PINBALL:
      run4pPinball();
      break;
    }
  }
}
