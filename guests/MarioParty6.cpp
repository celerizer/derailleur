#include "MarioParty6.h"

#include <cstring>

/* Mario Party 6's playable roster, in native id order. */
typedef enum
{
  MP6_CHARACTER_MARIO = 0x0,
  MP6_CHARACTER_LUIGI = 0x1,
  MP6_CHARACTER_PEACH = 0x2,
  MP6_CHARACTER_YOSHI = 0x3,
  MP6_CHARACTER_WARIO = 0x4,
  MP6_CHARACTER_DAISY = 0x5,
  MP6_CHARACTER_WALUIGI = 0x6,
  MP6_CHARACTER_TOAD = 0x7,
  MP6_CHARACTER_BOO = 0x8,
  MP6_CHARACTER_TOADETTE = 0x9,
  MP6_CHARACTER_KOOPA_KID = 0xA,
  MP6_CHARACTER_KOOPA_KID_R = 0xB,
  MP6_CHARACTER_KOOPA_KID_G = 0xC,
  MP6_CHARACTER_KOOPA_KID_B = 0xD
} mp6_character;

static dr_character_id_t mp6_char_from_dr(dr_character character)
{
  switch (character)
  {
  /* Supported characters */
  case DR_CHARACTER_MARIO:
    return { MP6_CHARACTER_MARIO, true };
  case DR_CHARACTER_LUIGI:
    return { MP6_CHARACTER_LUIGI, true };
  case DR_CHARACTER_PEACH:
    return { MP6_CHARACTER_PEACH, true };
  case DR_CHARACTER_YOSHI:
    return { MP6_CHARACTER_YOSHI, true };
  case DR_CHARACTER_WARIO:
    return { MP6_CHARACTER_WARIO, true };
  case DR_CHARACTER_DAISY:
    return { MP6_CHARACTER_DAISY, true };
  case DR_CHARACTER_WALUIGI:
    return { MP6_CHARACTER_WALUIGI, true };
  case DR_CHARACTER_TOAD:
    return { MP6_CHARACTER_TOAD, true };
  case DR_CHARACTER_BOO:
    return { MP6_CHARACTER_BOO, true };
  case DR_CHARACTER_KOOPA_KID:
    return { MP6_CHARACTER_KOOPA_KID, true };
  case DR_CHARACTER_KOOPA_KID_R:
    return { MP6_CHARACTER_KOOPA_KID_R, true };
  case DR_CHARACTER_KOOPA_KID_G:
    return { MP6_CHARACTER_KOOPA_KID_G, true };
  case DR_CHARACTER_KOOPA_KID_B:
    return { MP6_CHARACTER_KOOPA_KID_B, true };
  case DR_CHARACTER_TOADETTE:
    return { MP6_CHARACTER_TOADETTE, true };

  /* Character replacements */
  case DR_CHARACTER_DONKEY_KONG:
    return { MP6_CHARACTER_BOO, false };
  case DR_CHARACTER_BIRDO:
    return { MP6_CHARACTER_YOSHI, false };
  case DR_CHARACTER_DRY_BONES:
    return { MP6_CHARACTER_BOO, false };
  case DR_CHARACTER_BLOOPER:
    return { MP6_CHARACTER_PEACH, false };
  case DR_CHARACTER_HAMMER_BRO:
    return { MP6_CHARACTER_MARIO, false };
  default:
    return { MP6_CHARACTER_MARIO, false };
  }
}

static const dr_mp_minigame_t MP6_MINIGAMES[] = {
  // 4-Player
  { "Smashdance", DR_MINIGAME_4P, 0x00, 0x06, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Odd Card Out", DR_MINIGAME_4P, 0x01, 0x07, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Freeze Frame", DR_MINIGAME_4P, 0x02, 0x08, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "What Goes Up...", DR_MINIGAME_4P, 0x03, 0x09, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Granite Getaway", DR_MINIGAME_4P, 0x04, 0x0A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Circuit Maximus", DR_MINIGAME_4P, 0x05, 0x0B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Catch You Letter", DR_MINIGAME_4P, 0x06, 0x0C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Snow Whirled", DR_MINIGAME_4P, 0x07, 0x0D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Daft Rafts", DR_MINIGAME_4P, 0x08, 0x0E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tricky Tires", DR_MINIGAME_4P, 0x09, 0x0F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Treasure Trawlers", DR_MINIGAME_4P, 0x0A, 0x10, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Memory Lane", DR_MINIGAME_4P, 0x0B, 0x11, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mowtown", DR_MINIGAME_4P, 0x0C, 0x12, DR_QUIRK_SAFE_TEXTURE_CACHE, DR_NO_FLAGS },
  { "Cannonball Fun", DR_MINIGAME_4P, 0x0D, 0x13, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Note to Self", DR_MINIGAME_4P, 0x0E, 0x14, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Same Is Lame", DR_MINIGAME_4P, 0x0F, 0x15, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lift Leapers", DR_MINIGAME_4P, 0x11, 0x17, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Blooper Scooper", DR_MINIGAME_4P, 0x12, 0x18, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trap Ease Artist", DR_MINIGAME_4P, 0x13, 0x19, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pokey Punch-out", DR_MINIGAME_4P, 0x14, 0x1A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Money Belt", DR_MINIGAME_4P, 0x15, 0x1B, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Sunday Drivers", DR_MINIGAME_4P, 0x2F, 0x35, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Throw Me a Bone", DR_MINIGAME_4P, 0x31, 0x37, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  // 1-vs-3
  { "Cash Flow", DR_MINIGAME_1V3, 0x16, 0x1C, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Sink or Swim", DR_MINIGAME_1V3, 0x18, 0x1E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Snow Brawl", DR_MINIGAME_1V3, 0x19, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ball Dozers", DR_MINIGAME_1V3, 0x1A, 0x20, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Surge and Destroy", DR_MINIGAME_1V3, 0x1B, 0x21, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pop Star", DR_MINIGAME_1V3, 0x1C, 0x22, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stage Fright", DR_MINIGAME_1V3, 0x1D, 0x23, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Conveyor Bolt", DR_MINIGAME_1V3, 0x1E, 0x24, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Crate and Peril", DR_MINIGAME_1V3, 0x1F, 0x25, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Ray of Fright", DR_MINIGAME_1V3, 0x20, 0x26, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dust 'til Dawn", DR_MINIGAME_1V3, 0x21, 0x27, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  // 2-vs-2
  { "Garden Grab", DR_MINIGAME_2V2, 0x22, 0x28, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pixel Perfect", DR_MINIGAME_2V2, 0x23, 0x29, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Slot Trot", DR_MINIGAME_2V2, 0x24, 0x2A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Gondola Glide", DR_MINIGAME_2V2, 0x25, 0x2B, DR_NO_QUIRKS, DR_NO_FLAGS }, // sets av info to be widescreen at the end??
  { "Light Breeze", DR_MINIGAME_2V2, 0x26, 0x2C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Body Builder", DR_MINIGAME_2V2, 0x27, 0x2D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mole-it!", DR_MINIGAME_2V2, 0x28, 0x2E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cashapult", DR_MINIGAME_2V2, 0x29, 0x2F, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Jump the Gun", DR_MINIGAME_2V2, 0x2A, 0x30, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rocky Road", DR_MINIGAME_2V2, 0x2B, 0x31, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Clean Team", DR_MINIGAME_2V2, 0x2C, 0x32, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Burnstile", DR_MINIGAME_2V2, 0x43, 0x49, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  // Battle
  { "Hyper Sniper", DR_MINIGAME_BATTLE, 0x2D, 0x33, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Insectiride", DR_MINIGAME_BATTLE, 0x2E, 0x34, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stamp By Me", DR_MINIGAME_BATTLE, 0x30, 0x36, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Wrasslin' Rapids", DR_MINIGAME_BATTLE, 0x3F, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Strawberry Shortfuse", DR_MINIGAME_BATTLE, 0x4F, 0x55, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Control Shtick", DR_MINIGAME_BATTLE, 0x50, 0x56, DR_NO_QUIRKS, DR_NO_FLAGS },

  // Duel
  { "Light Up My Night", DR_MINIGAME_DUEL, 0x10, 0x16, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cog Jog", DR_MINIGAME_DUEL, 0x17, 0x1D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Black Hole Boogie", DR_MINIGAME_DUEL, 0x32, 0x38, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Full Tilt", DR_MINIGAME_DUEL, 0x33, 0x39, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Sumo of Doom-o", DR_MINIGAME_DUEL, 0x34, 0x3A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "O-Zone", DR_MINIGAME_DUEL, 0x35, 0x3B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pitifall", DR_MINIGAME_DUEL, 0x36, 0x3C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mass Meteor", DR_MINIGAME_DUEL, 0x37, 0x3D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lunar-tics", DR_MINIGAME_DUEL, 0x38, 0x3E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "T Minus Five", DR_MINIGAME_DUEL, 0x39, 0x3F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Asteroad Rage", DR_MINIGAME_DUEL, 0x3A, 0x40, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Boo'd Off the Stage", DR_MINIGAME_DUEL, 0x3B, 0x41, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Boonanza!", DR_MINIGAME_DUEL, 0x3C, 0x42, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trick or Tree", DR_MINIGAME_DUEL, 0x3D, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Something's Amist", DR_MINIGAME_DUEL, 0x3E, 0x44, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  // Mic minigames
  { "Verbal Assault", DR_MINIGAME_1V3, 0x40, 0x46, DR_NO_QUIRKS, DR_FLAG_MIC },
  { "Shoot Yer Mouth Off", DR_MINIGAME_1V3, 0x41, 0x4A, DR_NO_QUIRKS, DR_FLAG_MIC },
  { "Talkie Walkie", DR_MINIGAME_1V3, 0x42, 0x4B, DR_NO_QUIRKS, DR_FLAG_MIC },
  { "Word Herd", DR_MINIGAME_1V3, 0x44, 0x47, DR_NO_QUIRKS, DR_FLAG_MIC },
  { "Fruit Talktail", DR_MINIGAME_1V3, 0x45, 0x48, DR_NO_QUIRKS, DR_FLAG_MIC },
  
  /* Bowser */
  { "Pit Boss", DR_MINIGAME_BOWSER, 0x46, 0x4C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dizzy Rotisserie", DR_MINIGAME_BOWSER, 0x47, 0x4D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dark 'n Crispy", DR_MINIGAME_BOWSER, 0x48, 0x4E, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  /* DK */
  { "Tally Me Banana", DR_MINIGAME_DK, 0x49, 0x4F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Banana Shake", DR_MINIGAME_DK, 0x4A, 0x50, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pier Factor", DR_MINIGAME_DK, 0x4B, 0x51, DR_NO_QUIRKS, DR_NO_FLAGS },
  
  /* Rare */
  { "Seer Terror", DR_MINIGAME_SPECIAL, 0x4C, 0x52, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Block Star", DR_MINIGAME_SPECIAL, 0x4D, 0x53, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Lab Brats", DR_MINIGAME_SPECIAL, 0x4E, 0x54, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dunk Bros.", DR_MINIGAME_SPECIAL, 0x51, 0x57, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

static MpGcnConfig buildConfig()
{
  MpGcnConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 6 (USA)").toStdString();
  config.state = (dr_state_directory() + "/mp6.state.zip").toStdString();

  config.scene_miniexplain = 0x04;
  config.scene_miniresults = 0x71;

  config.scene = { 0x802C0254, DR_VALUE_TYPE_S32 };
  config.minigame = { 0x80265BA8, DR_VALUE_TYPE_S16 };

  const size_t character_addr[4]    = { 0x80265728, 0x80265732, 0x8026573c, 0x80265746 };
  const size_t controller_addr[4]   = { 0x8026572a, 0x80265734, 0x8026573e, 0x80265748 };
  const size_t difficulty_addr[4]   = { 0x8026572c, 0x80265736, 0x80265740, 0x8026574a };
  const size_t team_addr[4]         = { 0x8026572e, 0x80265738, 0x80265742, 0x8026574c };
  const size_t bot_addr[4]          = { 0x80265730, 0x8026573a, 0x80265744, 0x8026574e };
  const size_t bonus_result_addr[4] = { 0x80265776, 0x8026587e, 0x80265986, 0x80265a8e };
  const size_t result_addr[4]       = { 0x80265778, 0x80265880, 0x80265988, 0x80265a90 };

  for (unsigned i = 0; i < 4; i++)
  {
    config.character[i] = { character_addr[i], DR_VALUE_TYPE_U16 };
    config.controller[i] = { controller_addr[i], DR_VALUE_TYPE_U16 };
    config.difficulty[i] = { difficulty_addr[i], DR_VALUE_TYPE_U16 };
    config.team[i] = { team_addr[i], DR_VALUE_TYPE_U16 };
    config.bot[i] = { bot_addr[i], DR_VALUE_TYPE_U16 };
    config.bonus_result[i] = { bonus_result_addr[i], DR_VALUE_TYPE_U16 };
    config.result[i] = { result_addr[i], DR_VALUE_TYPE_U16 };
  }

  config.char_from_dr = mp6_char_from_dr;
  config.roster_size = 13;
  config.minigames = MP6_MINIGAMES;

  return config;
}

/* Whether the game listens to the microphone: 0 enabled, 1 disabled, 2 use the
 * controller instead. Held at 2 so a mic mini-game is playable without one. */
static const dr_value_t MP6_MIC_OPTION = { 0x802BF978, DR_VALUE_TYPE_S32 };

#define MP6_MIC_CONTROLLER 2

/* Board status; bit 6 is the time of day. The rest of the byte belongs to the
 * game, so it is read back and only that bit is forced. */
static const dr_value_t MP6_BOARD_STATUS = { 0x80265B80, DR_VALUE_TYPE_U8 };

/* GwMgTime, the same choice the mini-game itself reads. */
static const dr_value_t MP6_MG_TIME = { 0x802C0278, DR_VALUE_TYPE_S16 };

#define MP6_STATUS_NIGHT 0x40
#define MP6_TIME_DAY 0
#define MP6_TIME_NIGHT 1
#define MP6_DAYNIGHT_FRAMES 120

void MarioParty6::doApplyGameData(const DrGameData &data)
{
  MarioPartyGcn::doApplyGameData(data);

  m_retro->writeValue(MP6_MIC_CONTROLLER, MP6_MIC_OPTION);

  /* dr_rand is shared and lockstepped, so every netplay peer plays the same one.
   * The setup writes it back over the next couple of seconds, so it is held
   * rather than written once. */
  m_night = (dr_rand() % 2) != 0;
  m_dayNightFrames = MP6_DAYNIGHT_FRAMES;
  log(DR_LOG_INFO, qPrintable(QString("time of day: %1").arg(m_night ? "night" : "day")));
}

void MarioParty6::run()
{
  MarioPartyGcn::run();

  if (m_dayNightFrames > 0)
  {
    int64_t status = 0;

    m_dayNightFrames--;

    if (m_retro->readValue(&status, MP6_BOARD_STATUS) == DR_OK)
      m_retro->writeValue(
        m_night ? (status | MP6_STATUS_NIGHT) : (status & ~MP6_STATUS_NIGHT),
        MP6_BOARD_STATUS);

    m_retro->writeValue(m_night ? MP6_TIME_NIGHT : MP6_TIME_DAY, MP6_MG_TIME);
  }
}

MarioParty6::MarioParty6(QRetro *sharedCore, QObject *parent)
  : MarioPartyGcn(buildConfig(), sharedCore, parent)
{
}
