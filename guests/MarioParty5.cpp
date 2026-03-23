#include "MarioParty5.h"

static const uint8_t k_characterIds[DR_CHARACTER_SIZE] =
{
  [DR_CHARACTER_INVALID]     = 0xFF,
  [DR_CHARACTER_MARIO]       = 0xFF, // TODO
  [DR_CHARACTER_LUIGI]       = 0xFF, // TODO
  [DR_CHARACTER_PEACH]       = 0xFF, // TODO
  [DR_CHARACTER_YOSHI]       = 0xFF, // TODO
  [DR_CHARACTER_WARIO]       = 0xFF, // TODO
  [DR_CHARACTER_DONKEY_KONG] = 0xFF, // TODO
  [DR_CHARACTER_WALUIGI]     = 0xFF, // TODO
  [DR_CHARACTER_DAISY]       = 0xFF, // TODO
};

static const dr_mp_minigame_t k_minigames[] =
{
  // TODO: add minigames
  {nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF},
};

static const MpGcnConfig k_config =
{
  .core  = "", // TODO
  .game  = "", // TODO
  .state = "", // TODO

  .scene_miniexplain = 0x00, // TODO
  .scene_miniresults = 0x00, // TODO

  .scene_addr    = 0x00000000, // TODO
  .minigame_addr = 0x00000000, // TODO

  .character_addr  = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .controller_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .difficulty_addr = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .team_addr       = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .bot_addr        = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO
  .result_addr     = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // TODO

  .character_ids = k_characterIds,
  .minigames     = k_minigames,
};

MarioParty5::MarioParty5(QWindow *parent) : MarioPartyGcn(k_config, parent)
{
}
