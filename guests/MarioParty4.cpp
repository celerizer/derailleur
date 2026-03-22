#include "MarioParty4.h"

#define MP4_CORE "/media/keith/devtools/libretro/cores/dolphin_libretro.so"
#define MP4_GAME "/media/keith/devtools/libretro/roms/Mario Party 4 (USA) (Rev 1).rvz"

#define MP4_SCENE_ADDR        0x801d3ce3
#define MP4_MINIGAME_ADDR     0x8018FD2d
#define MP4_SCENE_MINIEXPLAIN 0x03
#define MP4_SCENE_MINIRESULTS 0x54

static const size_t MP4_CHARACTER_ADDR[4] = { 0x8018fc11, 0x8018fc1b, 0x8018fc25, 0x8018fc2f };
static const size_t MP4_CONTROLLER_ADDR[4] = { 0x8018fc13, 0x8018fc1d, 0x8018fc27, 0x8018fc31 };
static const size_t MP4_DIFFICULTY_ADDR[4] = { 0x8018fc15, 0x8018fc1f, 0x8018fc29, 0x8018fc33 };
static const size_t MP4_BOT_ADDR[4]        = { 0x8018fc19, 0x8018fc23, 0x8018fc2d, 0x8018fc37 };
static const size_t MP4_RESULT_ADDR[4]     = { 0x8018fc61, 0x8018fc91, 0x8018fcc1, 0x8018fcf1 };

static const uint8_t MP4_CHARACTER_ID[DR_CHARACTER_SIZE] = {
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0x00,
  [DR_CHARACTER_LUIGI]       = 0x01,
  [DR_CHARACTER_PEACH]       = 0x02,
  [DR_CHARACTER_YOSHI]       = 0x03,
  [DR_CHARACTER_WARIO]       = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
  [DR_CHARACTER_DAISY]       = 0x07,
  [DR_CHARACTER_WALUIGI]     = 0x06,
};

static const uint8_t MP4_DIFFICULTY_ID[DR_DIFFICULTY_SIZE] = {
  [DR_DIFFICULTY_INVALID]   = 0xFF,
  [DR_DIFFICULTY_VERY_EASY] = 0x00,
  [DR_DIFFICULTY_EASY]      = 0x01,
  [DR_DIFFICULTY_NORMAL]    = 0x02,
  [DR_DIFFICULTY_HARD]      = 0x03,
  [DR_DIFFICULTY_VERY_HARD] = 0x04,
};

dr_minigame_result_t MarioParty4::minigameResult(unsigned index) const
{
  dr_minigame_result_t result = {};

  if (index < 4)
    memory().readValue<uint8_t>((uint8_t*)&result.coins, MP4_RESULT_ADDR[index]);

  return result;
}

dr_error MarioParty4::setPlayerCharacter(unsigned index, dr_character character)
{
  if (index >= 4 || character >= DR_CHARACTER_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  memory().writeValue<uint8_t>(MP4_CHARACTER_ID[character], MP4_CHARACTER_ADDR[index]);

  return DR_OK;
}

dr_error MarioParty4::setPlayerControlPort(unsigned index, dr_control_port control_port)
{
  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;
  
  memory().writeValue<uint8_t>(static_cast<uint8_t>(control_port), MP4_CONTROLLER_ADDR[index]);
  
  return DR_OK;
}

dr_error MarioParty4::setPlayerControlType(unsigned index, dr_control_type control_type)
{
  uint8_t current;

  if (index >= 4)
    return DR_ERR_INVALID_PARAMETER;
  else if (memory().readValue<uint8_t>(&current, MP4_BOT_ADDR[index]))
  {
    uint8_t isBot = (control_type == DR_CONTROL_TYPE_CPU) ? 1 : 0;
    memory().writeValue<uint8_t>((current & ~0x01) | isBot, MP4_BOT_ADDR[index]);
  }

  return DR_OK;
}

dr_error MarioParty4::setPlayerDifficulty(unsigned index, dr_difficulty difficulty)
{
  if (index >= 4 || difficulty >= DR_DIFFICULTY_SIZE)
    return DR_ERR_INVALID_PARAMETER;

  memory().writeValue<uint8_t>(MP4_DIFFICULTY_ID[difficulty], MP4_DIFFICULTY_ADDR[index]);

  return DR_OK;
}

dr_error MarioParty4::setPlayerTeam(unsigned index, dr_team team)
{
  (void)index;
  (void)team;
  return DR_ERR_INVALID_PARAMETER;
}
