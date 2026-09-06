#include "MarioParty7Host.h"

#include <asm/mp7.h>

#include <QRetroDirectories.h>

static dr_character mp7_char_to_dr(unsigned chr)
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
    return DR_CHARACTER_BIRDO;
  case 0x0B:
    return DR_CHARACTER_DRY_BONES;
  case 0x0C:
    return DR_CHARACTER_KOOPA_KID_R;
  case 0x0D:
    return DR_CHARACTER_KOOPA_KID_G;
  case 0x0E:
    return DR_CHARACTER_KOOPA_KID_B;
  }

  return DR_CHARACTER_INVALID;
}

static const dr_minigame_type MP7_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, /* 0 */
  DR_MINIGAME_1V3, /* 1 */
  DR_MINIGAME_2V2, /* 2 */
  DR_MINIGAME_BATTLE, /* 3 */
  DR_MINIGAME_BOWSER, /* KOOPA */
  DR_MINIGAME_INVALID, /* size 0, seemingly unused */
  DR_MINIGAME_DUEL, /* 6 */
  DR_MINIGAME_DK, /* 7 - DONKEY */
  DR_MINIGAME_INVALID, /* 8 - 12 games? */
  DR_MINIGAME_INVALID, /* 9 - 1 game? */
  DR_MINIGAME_INVALID, /* 10 - 2 games? */
};

static const dr_scene_name_t MP7_SCENE_NAMES[] =
{
  // { 0x00, "" },
  // { 0x01, "" },
  // { 0x02, "" },
  // { 0x03, "" },
  // { 0x04, "" },
  // { 0x05, "" },
  { 0x06, "Mini-Game explanation", true },

  { 0x07, "Catchy Tunes", false },
  { 0x08, "Bubble Brawl", false },
  { 0x09, "Track & Yield", false },
  { 0x0a, "Fun Run", false },
  { 0x0b, "Cointagious", false },
  { 0x0c, "Snow Ride", false },

  // { 0x0d, "" },

  { 0x0e, "Picture This", false },
  { 0x0f, "Ghost in the Hall", false },
  { 0x10, "Big Dripper", false },
  { 0x11, "Target Tag", false },
  { 0x12, "Pokey Pummel", false },
  { 0x13, "Take Me Ohm", false },
  { 0x14, "Kart Wheeled", false },
  { 0x15, "Balloon Busters", false },
  { 0x16, "Clock Watchers", false },
  { 0x17, "Dart Attack", false },
  { 0x18, "Oil Crisis", false },

  // { 0x19, "" },

  { 0x1a, "La Bomba", false },
  { 0x1b, "Spray Anything", false },
  { 0x1c, "Balloonatic", false },
  { 0x1d, "Spinner Cell", false },
  { 0x1e, "Think Tank", false },
  { 0x1f, "Flashfright", false },
  { 0x20, "Coin-op Bop", false },
  { 0x21, "Easy Pickings", false },
  { 0x22, "Wheel of Woe", false },
  { 0x23, "Boxing Day", false },
  { 0x24, "Be My Chum!", false },
  { 0x25, "StratosFEAR!", false },
  { 0x26, "Pogo-a-go-go", false },
  { 0x27, "Buzzstormer", false },
  { 0x28, "Tile and Error", false },
  { 0x29, "Battery Ram", false },
  { 0x2a, "Cardinal Rule", false },
  { 0x2b, "Ice Moves", false },
  { 0x2c, "Bumper Crop", false },
  { 0x2d, "Hop-O-Matic 4000", false },
  { 0x2e, "Wingin' It", false },
  { 0x2f, "Sphere Factor", false },
  { 0x30, "Herbicidal Maniac", false },
  { 0x31, "Pyramid Scheme", false },
  { 0x32, "World Piece", false },
  { 0x33, "Warp Pipe Dreams", false },
  { 0x34, "Weight for It", false },
  { 0x35, "Helipopper", false },
  { 0x36, "Monty's Revenge", false },
  { 0x37, "Deck Hands", false },
  { 0x38, "Mad Props", false },
  { 0x39, "Gimme a Sign", false },
  { 0x3a, "Bridge Work", false },
  { 0x3b, "Spin Doctor", false },
  { 0x3c, "Hip Hop Drop", false },
  { 0x3d, "Air Farce", false },
  { 0x3e, "The Final Countdown", false },
  { 0x3f, "Royal Rumpus", false },
  { 0x40, "Light Speed", false },
  { 0x41, "Apes of Wrath", false },
  { 0x42, "Fish & Cheeps", false },
  { 0x43, "Camp Ukiki", false },
  { 0x44, "Funstacle Course!", false },
  { 0x45, "Funderwall!", false },
  { 0x46, "Magmagical Journey!", false },
  { 0x47, "Tunnel of Lava!", false },
  { 0x48, "Treasure Dome!", false },
  { 0x49, "Slot-O-Whirl!", false },
  { 0x4a, "Peel Out", false },
  { 0x4b, "Bananas Faster", false },
  { 0x4c, "Stump Change", false },
  { 0x4d, "Jump, Man", false },
  { 0x4e, "Vine Country", false },
  { 0x4f, "A Bridge Too Short", false },
  { 0x50, "Spider Stomp", false },
  { 0x51, "Stick and Spin", false },

  // { 0x52, "" },
  // { 0x53, "" },
  // { 0x54, "" },
  // { 0x55, "" },
  // { 0x56, "" },
  // { 0x57, "" },
  // { 0x58, "" },
  // { 0x59, "" },
  // { 0x5a, "" },

  { 0x5b, "Bowser's Lovely Lift!", false },

  // { 0x5c, "" },

  { 0x5d, "Mathemortician", false },

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
  // { 0x71, "" },
  // { 0x72, "" },
  { 0x73, "Mini-Game results", true },

  { -1, nullptr },
};

static DrGcnHostConfig makeConfig(const std::string &game)
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 7 (USA) (Rev 1).rvz").toStdString();
  if (!game.empty())
    config.game = game;

  config.cheats.cave = MP7_CAVE;
  config.cheats.cave_addr = MP7_CAVE_ADDR;
  config.cheats.cave_size = MP7_CAVE_SIZE;
  config.cheats.cheat_board = nullptr;
  config.cheats.hooks = MP7_HOOK_BOARD;

  config.values.scene = { 0x802f2f3c, DR_VALUE_TYPE_S32 };
  config.values.character[0] = { 0x80290c48, DR_VALUE_TYPE_S16 };
  config.values.character[1] = { 0x80290c52, DR_VALUE_TYPE_S16 };
  config.values.character[2] = { 0x80290c5c, DR_VALUE_TYPE_S16 };
  config.values.character[3] = { 0x80290c66, DR_VALUE_TYPE_S16 };
  config.values.controller[0] = { 0x80290c4a, DR_VALUE_TYPE_S16 };
  config.values.controller[1] = { 0x80290c54, DR_VALUE_TYPE_S16 };
  config.values.controller[2] = { 0x80290c5e, DR_VALUE_TYPE_S16 };
  config.values.controller[3] = { 0x80290c68, DR_VALUE_TYPE_S16 };
  config.values.difficulty[0] = { 0x80290c4c, DR_VALUE_TYPE_S16 };
  config.values.difficulty[1] = { 0x80290c56, DR_VALUE_TYPE_S16 };
  config.values.difficulty[2] = { 0x80290c60, DR_VALUE_TYPE_S16 };
  config.values.difficulty[3] = { 0x80290c6a, DR_VALUE_TYPE_S16 };
  config.values.team[0] = { 0x80290c4e, DR_VALUE_TYPE_S16 };
  config.values.team[1] = { 0x80290c58, DR_VALUE_TYPE_S16 };
  config.values.team[2] = { 0x80290c62, DR_VALUE_TYPE_S16 };
  config.values.team[3] = { 0x80290c6c, DR_VALUE_TYPE_S16 };
  config.values.bot[0] = { 0x80290c50, DR_VALUE_TYPE_S16 };
  config.values.bot[1] = { 0x80290c5a, DR_VALUE_TYPE_S16 };
  config.values.bot[2] = { 0x80290c64, DR_VALUE_TYPE_S16 };
  config.values.bot[3] = { 0x80290c6e, DR_VALUE_TYPE_S16 };
  config.values.result[0] = { 0x80290cca, DR_VALUE_TYPE_S16 };
  config.values.result[1] = { 0x80290dda, DR_VALUE_TYPE_S16 };
  config.values.result[2] = { 0x80290eea, DR_VALUE_TYPE_S16 };
  config.values.result[3] = { 0x80290ffa, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[0] = { 0x80290cc8, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[1] = { 0x80290dd8, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[2] = { 0x80290ee8, DR_VALUE_TYPE_S16 };
  config.values.bonus_result[3] = { 0x80290ff8, DR_VALUE_TYPE_S16 };
  config.values.coins[0] = { 0x80290cbe, DR_VALUE_TYPE_S16 };
  config.values.coins[1] = { 0x80290dce, DR_VALUE_TYPE_S16 };
  config.values.coins[2] = { 0x80290ede, DR_VALUE_TYPE_S16 };
  config.values.coins[3] = { 0x80290fee, DR_VALUE_TYPE_S16 };
  config.values.stars[0] = { 0x80290cd0, DR_VALUE_TYPE_S16 };
  config.values.stars[1] = { 0x80290de0, DR_VALUE_TYPE_S16 };
  config.values.stars[2] = { 0x80290ef0, DR_VALUE_TYPE_S16 };
  config.values.stars[3] = { 0x80291000, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[0] = { 0x80290cc6, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[1] = { 0x80290dd6, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[2] = { 0x80290ee6, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[3] = { 0x80290ff6, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP7_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.minigame_id = { 0x80291558, DR_VALUE_TYPE_S16 };

  config.scene_miniexplain = 0x06;
  config.scene_miniresults = 0x73;

  config.scene_names = MP7_SCENE_NAMES;

  config.char_to_dr = mp7_char_to_dr;

  config.minigame_type_to_dr = MP7_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP7_MINIGAME_TYPE_TO_DR) / sizeof(*MP7_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP7_HOST_STATE;

  /* Both roulette list builders are diverted into the cave, which says which
   * one ran; candidates follow it so a mic roulette offers only mic games. */
  config.mic_lists = true;

  /* save_files: MP7's memory-card file name still needs filling in */

  return config;
}

MarioParty7Host::MarioParty7Host(QObject *parent, const std::string &game)
  : MarioPartyGcnHost(makeConfig(game), parent)
{
}
