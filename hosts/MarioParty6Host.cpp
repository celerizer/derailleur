#include "MarioParty6Host.h"

#include <asm/mp6.h>

#include <QRetroDirectories.h>

static dr_character mp6_char_to_dr(unsigned chr)
{
  switch (chr)
  {
  case 0x00:
    return DR_CHARACTER_MARIO;
  case 0x01:
    return DR_CHARACTER_LUIGI;
  case 0x02:
    return DR_CHARACTER_PEACH;
  case 0x03:
    return DR_CHARACTER_YOSHI;
  case 0x04:
    return DR_CHARACTER_WARIO;
  case 0x05:
    return DR_CHARACTER_DAISY;
  case 0x06:
    return DR_CHARACTER_WALUIGI;
  case 0x07:
    return DR_CHARACTER_TOAD;
  case 0x08:
    return DR_CHARACTER_BOO;
  case 0x09:
    return DR_CHARACTER_TOADETTE;
  case 0x0A:
    return DR_CHARACTER_KOOPA_KID;
  case 0x0B:
    return DR_CHARACTER_KOOPA_KID_R;
  case 0x0C:
    return DR_CHARACTER_KOOPA_KID_G;
  case 0x0D:
    return DR_CHARACTER_KOOPA_KID_B;
  }

  return DR_CHARACTER_INVALID;
}

static const dr_minigame_type MP6_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, /* 0 */
  DR_MINIGAME_1V3, /* 1 */
  DR_MINIGAME_2V2, /* 2 */
  DR_MINIGAME_BATTLE, /* 3 */
  DR_MINIGAME_BOWSER, /* MG_TYPE_KOOPA */
  DR_MINIGAME_INVALID, /* MG_TYPE_LAST */
  DR_MINIGAME_DUEL, /* 6 */
  DR_MINIGAME_DK, /* MG_TYPE_DONKEY */
  DR_MINIGAME_INVALID, /* MG_TYPE_678 */
};

static const dr_scene_name_t MP6_SCENE_NAMES[] =
{
  // { 0x00, "" },
  // { 0x01, "" },
  // { 0x02, "" },
  // { 0x03, "" },
  { 0x04, "Mini-Game explanation", true },
  // { 0x05, "" },

  { 0x06, "Smashdance", false },
  { 0x07, "Odd Card Out", false },
  { 0x08, "Freeze Frame", false },
  { 0x09, "What Goes Up...", false },
  { 0x0a, "Granite Getaway", false },
  { 0x0b, "Circuit Maximus", false },
  { 0x0c, "Catch You Letter", false },
  { 0x0d, "Snow Whirled", false },
  { 0x0e, "Daft Rafts", false },
  { 0x0f, "Tricky Tires", false },
  { 0x10, "Treasure Trawlers", false },
  { 0x11, "Memory Lane", false },
  { 0x12, "Mowtown", false },
  { 0x13, "Cannonball Fun", false },
  { 0x14, "Note to Self", false },
  { 0x15, "Same Is Lame", false },
  { 0x16, "Light Up My Night", false },
  { 0x17, "Lift Leapers", false },
  { 0x18, "Blooper Scooper", false },
  { 0x19, "Trap Ease Artist", false },
  { 0x1a, "Pokey Punch-out", false },
  { 0x1b, "Money Belt", false },
  { 0x1c, "Cash Flow", false },
  { 0x1d, "Cog Jog", false },
  { 0x1e, "Sink or Swim", false },
  { 0x1f, "Snow Brawl", false },
  { 0x20, "Ball Dozers", false },
  { 0x21, "Surge and Destroy", false },
  { 0x22, "Pop Star", false },
  { 0x23, "Stage Fright", false },
  { 0x24, "Conveyor Bolt", false },
  { 0x25, "Crate and Peril", false },
  { 0x26, "Ray of Fright", false },
  { 0x27, "Dust 'til Dawn", false },
  { 0x28, "Garden Grab", false },
  { 0x29, "Pixel Perfect", false },
  { 0x2a, "Slot Trot", false },
  { 0x2b, "Gondola Glide", false },
  { 0x2c, "Light Breeze", false },
  { 0x2d, "Body Builder", false },
  { 0x2e, "Mole-it!", false },
  { 0x2f, "Cashapult", false },
  { 0x30, "Jump the Gun", false },
  { 0x31, "Rocky Road", false },
  { 0x32, "Clean Team", false },
  { 0x33, "Hyper Sniper", false },
  { 0x34, "Insectiride", false },
  { 0x35, "Sunday Drivers", false },
  { 0x36, "Stamp By Me", false },
  { 0x37, "Throw Me a Bone", false },
  { 0x38, "Black Hole Boogie", false },
  { 0x39, "Full Tilt", false },
  { 0x3a, "Sumo of Doom-o", false },
  { 0x3b, "O-Zone", false },
  { 0x3c, "Pitifall", false },
  { 0x3d, "Mass Meteor", false },
  { 0x3e, "Lunar-tics", false },
  { 0x3f, "T Minus Five", false },
  { 0x40, "Asteroad Rage", false },
  { 0x41, "Boo'd Off the Stage", false },
  { 0x42, "Boonanza!", false },
  { 0x43, "Trick or Tree", false },
  { 0x44, "Something's Amist", false },
  { 0x45, "Wrasslin' Rapids", false },
  { 0x46, "Verbal Assault", false },
  { 0x47, "Word Herd", false },
  { 0x48, "Fruit Talktail", false },
  { 0x49, "Burnstile", false },
  { 0x4a, "Shoot Yer Mouth Off", false },
  { 0x4b, "Talkie Walkie", false },
  { 0x4c, "Pit Boss", false },
  { 0x4d, "Dizzy Rotisserie", false },
  { 0x4e, "Dark 'n Crispy", false },
  { 0x4f, "Tally Me Banana", false },
  { 0x50, "Banana Shake", false },
  { 0x51, "Pier Factor", false },
  { 0x52, "Seer Terror", false },
  { 0x53, "Block Star", false },
  { 0x54, "Lab Brats", false },
  { 0x55, "Strawberry Shortfuse", false },
  { 0x56, "Control Shtick", false },
  { 0x57, "Dunk Bros.", false },

  // { 0x58, "" },
  // { 0x59, "" },
  // { 0x5a, "" },
  // { 0x5b, "" },
  // { 0x5c, "" },
  // { 0x5d, "" },
  // { 0x5e, "" },
  // { 0x5f, "" },
  // { 0x60, "" },
  // { 0x61, "" },
  // { 0x62, "" },
  // { 0x63, "" },
  // { 0x64, "" },
  // { 0x65, "" },
  // { 0x66, "" },
  // { 0x67, "" },
  // { 0x68, "" },
  // { 0x69, "" },
  // { 0x6a, "" },
  // { 0x6b, "" },
  // { 0x6c, "" },
  // { 0x6d, "" },
  // { 0x6e, "" },
  // { 0x6f, "" },
  // { 0x70, "" },
  { 0x71, "Mini-Game results", true },

  { -1, nullptr },
};

static DrGcnHostConfig makeConfig(const std::string &game)
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 6 (USA).rvz").toStdString();
  if (!game.empty())
    config.game = game;

  config.cheats.cave = MP6_CAVE;
  config.cheats.cave_addr = MP6_CAVE_ADDR;
  config.cheats.cave_size = MP6_CAVE_SIZE;
  config.cheats.cheat_board = nullptr;
  config.cheats.hooks = MP6_HOOK_BOARD;

  config.values.scene = { 0x802C0254, DR_VALUE_TYPE_S32 };
  config.values.character[0] = { 0x80265728, DR_VALUE_TYPE_S16 };
  config.values.character[1] = { 0x80265732, DR_VALUE_TYPE_S16 };
  config.values.character[2] = { 0x8026573c, DR_VALUE_TYPE_S16 };
  config.values.character[3] = { 0x80265746, DR_VALUE_TYPE_S16 };
  config.values.controller[0] = { 0x8026572a, DR_VALUE_TYPE_S16 };
  config.values.controller[1] = { 0x80265734, DR_VALUE_TYPE_S16 };
  config.values.controller[2] = { 0x8026573e, DR_VALUE_TYPE_S16 };
  config.values.controller[3] = { 0x80265748, DR_VALUE_TYPE_S16 };
  config.values.difficulty[0] = { 0x8026572c, DR_VALUE_TYPE_S16 };
  config.values.difficulty[1] = { 0x80265736, DR_VALUE_TYPE_S16 };
  config.values.difficulty[2] = { 0x80265740, DR_VALUE_TYPE_S16 };
  config.values.difficulty[3] = { 0x8026574a, DR_VALUE_TYPE_S16 };
  config.values.team[0] = { 0x8026572e, DR_VALUE_TYPE_S16 };
  config.values.team[1] = { 0x80265738, DR_VALUE_TYPE_S16 };
  config.values.team[2] = { 0x80265742, DR_VALUE_TYPE_S16 };
  config.values.team[3] = { 0x8026574c, DR_VALUE_TYPE_S16 };
  config.values.bot[0] = { 0x80265730, DR_VALUE_TYPE_S16 };
  config.values.bot[1] = { 0x8026573a, DR_VALUE_TYPE_S16 };
  config.values.bot[2] = { 0x80265744, DR_VALUE_TYPE_S16 };
  config.values.bot[3] = { 0x8026574e, DR_VALUE_TYPE_S16 };
  config.values.result[0] = { 0x80265778, DR_VALUE_TYPE_S16 };
  config.values.result[1] = { 0x80265880, DR_VALUE_TYPE_S16 };
  config.values.result[2] = { 0x80265988, DR_VALUE_TYPE_S16 };
  config.values.result[3] = { 0x80265a90, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[0] = { 0x80265776, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x8026587e, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x80265986, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x80265a8e, DR_VALUE_TYPE_S16 };
  config.values.coins[0] = { 0x8026576c, DR_VALUE_TYPE_S16 };
  config.values.coins[1] = { 0x80265874, DR_VALUE_TYPE_S16 };
  config.values.coins[2] = { 0x8026597c, DR_VALUE_TYPE_S16 };
  config.values.coins[3] = { 0x80265a84, DR_VALUE_TYPE_S16 };
  config.values.stars[0] = { 0x80265780, DR_VALUE_TYPE_S16 };
  config.values.stars[1] = { 0x80265888, DR_VALUE_TYPE_S16 };
  config.values.stars[2] = { 0x80265990, DR_VALUE_TYPE_S16 };
  config.values.stars[3] = { 0x80265a98, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[0] = { 0x80265774, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[1] = { 0x8026587c, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[2] = { 0x80265984, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[3] = { 0x80265a8c, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP6_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.minigame_id = { 0x80265BA8, DR_VALUE_TYPE_S16 };

  config.scene_miniexplain = 0x04;
  config.scene_miniresults = 0x71;

  config.scene_names = MP6_SCENE_NAMES;

  config.char_to_dr = mp6_char_to_dr;

  config.minigame_type_to_dr = MP6_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP6_MINIGAME_TYPE_TO_DR) / sizeof(*MP6_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP6_HOST_STATE;

  /* save_files: MP6's memory-card file name still needs filling in */

  return config;
}

MarioParty6Host::MarioParty6Host(QObject *parent, const std::string &game)
  : MarioPartyGcnHost(makeConfig(game), parent)
{
}
