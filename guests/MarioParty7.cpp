#include "MarioParty7.h"

#include <cstring>

// DONKEY_KONG maps to 0x08 (Boo) as MP7 has no DK.
static const uint16_t MP7_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x06, 0x05,
};

static const dr_mp_minigame_t MP7_MINIGAMES[] = {
  /* 4-Player */
  { "Catchy Tunes", DR_MINIGAME_4P, 0x00, 0x07, DR_NO_QUIRKS },
  { "Bubble Brawl", DR_MINIGAME_4P, 0x01, 0x08, DR_NO_QUIRKS },
  { "Track & Yield", DR_MINIGAME_4P, 0x02, 0x09, DR_NO_QUIRKS },
  { "Fun Run", DR_MINIGAME_4P, 0x03, 0x0A, DR_NO_QUIRKS },
  { "Cointagious", DR_MINIGAME_4P, 0x04, 0x0B, DR_NO_QUIRKS },
  { "Snow Ride", DR_MINIGAME_4P, 0x05, 0x0C, DR_NO_QUIRKS },
  { "Picture This", DR_MINIGAME_4P, 0x07, 0x0E, DR_NO_QUIRKS },
  { "Ghost in the Hall", DR_MINIGAME_4P, 0x08, 0x0F, DR_NO_QUIRKS },
  { "Big Dripper", DR_MINIGAME_4P, 0x09, 0x10, DR_NO_QUIRKS },
  { "Target Tag", DR_MINIGAME_4P, 0x0A, 0x11, DR_NO_QUIRKS },
  { "Pokey Pummel", DR_MINIGAME_4P, 0x0B, 0x12, DR_NO_QUIRKS },
  { "Take Me Ohm", DR_MINIGAME_4P, 0x0C, 0x13, DR_NO_QUIRKS },
  { "Kart Wheeled", DR_MINIGAME_4P, 0x0D, 0x14, DR_NO_QUIRKS },

  /* 4-Player Mic */
  { "Balloon Busters", DR_MINIGAME_INVALID, 0x0E, 0x15, DR_NO_QUIRKS },
  { "Clock Watchers", DR_MINIGAME_INVALID, 0x0F, 0x16, DR_NO_QUIRKS },
  { "Dart Attack", DR_MINIGAME_INVALID, 0x10, 0x17, DR_NO_QUIRKS },
  { "Oil Crisis", DR_MINIGAME_INVALID, 0x12, 0x18, DR_NO_QUIRKS },
  { "Mathemortician", DR_MINIGAME_INVALID, 0x57, 0x5D, DR_NO_QUIRKS },

  /* 1-vs.-3 */
  { "La Bomba", DR_MINIGAME_1V3, 0x14, 0x1A, DR_NO_QUIRKS },
  { "Spray Anything", DR_MINIGAME_1V3, 0x15, 0x1B, DR_NO_QUIRKS },
  { "Balloonatic", DR_MINIGAME_1V3, 0x16, 0x1C, DR_NO_QUIRKS },
  { "Spinner Cell", DR_MINIGAME_1V3, 0x17, 0x1D, DR_NO_QUIRKS },
  { "Think Tank", DR_MINIGAME_1V3, 0x18, 0x1E, DR_NO_QUIRKS },
  { "Flashfright", DR_MINIGAME_1V3, 0x19, 0x1F, DR_NO_QUIRKS },
  { "Coin-op Bop", DR_MINIGAME_1V3, 0x1A, 0x20, DR_NO_QUIRKS },
  { "Easy Pickings", DR_MINIGAME_1V3, 0x1B, 0x21, DR_NO_QUIRKS },
  { "Pogo-a-go-go", DR_MINIGAME_1V3, 0x20, 0x26, DR_NO_QUIRKS },

  /* 1-vs.-3 Mic */
  { "Wheel of Woe", DR_MINIGAME_INVALID, 0x1C, 0x22, DR_NO_QUIRKS },
  { "Boxing Day", DR_MINIGAME_INVALID, 0x1D, 0x23, DR_NO_QUIRKS },
  { "Be My Chum!", DR_MINIGAME_INVALID, 0x1E, 0x24, DR_NO_QUIRKS },
  { "StratosFEAR!", DR_MINIGAME_INVALID, 0x1F, 0x25, DR_NO_QUIRKS },

  /* 2-vs.-2 */
  { "Buzzstormer", DR_MINIGAME_2V2, 0x21, 0x27, DR_NO_QUIRKS },
  { "Tile and Error", DR_MINIGAME_2V2, 0x22, 0x28, DR_NO_QUIRKS },
  { "Battery Ram", DR_MINIGAME_2V2, 0x23, 0x29, DR_NO_QUIRKS },
  { "Cardinal Rule", DR_MINIGAME_2V2, 0x24, 0x2A, DR_NO_QUIRKS },
  { "Bumper Crop", DR_MINIGAME_2V2, 0x26, 0x2C, DR_NO_QUIRKS },
  { "Hop-O-Matic 4000", DR_MINIGAME_2V2, 0x27, 0x2D, DR_NO_QUIRKS },
  { "Wingin' It", DR_MINIGAME_2V2, 0x28, 0x2E, DR_NO_QUIRKS },
  { "Sphere Factor", DR_MINIGAME_2V2, 0x29, 0x2F, DR_NO_QUIRKS },
  { "Herbicidal Maniac", DR_MINIGAME_2V2, 0x2A, 0x30, DR_NO_QUIRKS },
  { "Pyramid Scheme", DR_MINIGAME_2V2, 0x2B, 0x31, DR_NO_QUIRKS },
  { "World Piece", DR_MINIGAME_2V2, 0x2C, 0x32, DR_NO_QUIRKS },
  { "Spider Stomp", DR_MINIGAME_2V2, 0x4A, 0x50, DR_NO_QUIRKS },

  /* Battle */
  { "Helipopper", DR_MINIGAME_BATTLE, 0x2F, 0x35, DR_NO_QUIRKS },
  { "Monty's Revenge", DR_MINIGAME_BATTLE, 0x30, 0x36, DR_NO_QUIRKS },
  { "Deck Hands", DR_MINIGAME_BATTLE, 0x31, 0x37, DR_NO_QUIRKS },
  { "Air Farce", DR_MINIGAME_BATTLE, 0x37, 0x3D, DR_NO_QUIRKS },
  { "The Final Countdown", DR_MINIGAME_BATTLE, 0x38, 0x3E, DR_NO_QUIRKS },

  /* Duel */
  { "Warp Pipe Dreams", DR_MINIGAME_DUEL, 0x2D, 0x33, DR_NO_QUIRKS },
  { "Weight for It", DR_MINIGAME_DUEL, 0x2E, 0x34, DR_NO_QUIRKS },
  { "Mad Props", DR_MINIGAME_DUEL, 0x32, 0x38, DR_NO_QUIRKS },
  { "Gimme a Sign", DR_MINIGAME_DUEL, 0x33, 0x39, DR_NO_QUIRKS },
  { "Bridge Work", DR_MINIGAME_DUEL, 0x34, 0x3A, DR_NO_QUIRKS },
  { "Spin Doctor", DR_MINIGAME_DUEL, 0x35, 0x3B, DR_NO_QUIRKS },
  { "Hip Hop Drop", DR_MINIGAME_DUEL, 0x36, 0x3C, DR_NO_QUIRKS },
  { "Royal Rumpus", DR_MINIGAME_DUEL, 0x39, 0x3F, DR_NO_QUIRKS },
  { "Light Speed", DR_MINIGAME_DUEL, 0x3A, 0x40, DR_NO_QUIRKS },
  { "Apes of Wrath", DR_MINIGAME_DUEL, 0x3B, 0x41, DR_NO_QUIRKS },
  { "Fish & Cheeps", DR_MINIGAME_DUEL, 0x3C, 0x42, DR_NO_QUIRKS },
  { "Camp Ukiki", DR_MINIGAME_DUEL, 0x3D, 0x43, DR_NO_QUIRKS },

  /* DK minigames (single-player) */
  { "Jump, Man", DR_MINIGAME_INVALID, 0x47, 0x4D, DR_NO_QUIRKS },
  { "Vine Country", DR_MINIGAME_INVALID, 0x48, 0x4E, DR_NO_QUIRKS },
  { "A Bridge Too Short", DR_MINIGAME_INVALID, 0x49, 0x4F, DR_NO_QUIRKS },

  /* Bowser minigames (single-player) */
  { "Tunnel of Lava!", DR_MINIGAME_INVALID, 0x41, 0x47, DR_NO_QUIRKS },
  { "Treasure Dome!", DR_MINIGAME_INVALID, 0x42, 0x48, DR_NO_QUIRKS },
  { "Slot-O-Whirl!", DR_MINIGAME_INVALID, 0x43, 0x49, DR_NO_QUIRKS },

  /* DK minigames (multiplayer) */
  { "Peel Out", DR_MINIGAME_INVALID, 0x44, 0x4A, DR_NO_QUIRKS },
  { "Bananas Faster", DR_MINIGAME_INVALID, 0x45, 0x4B, DR_NO_QUIRKS },
  { "Stump Change", DR_MINIGAME_INVALID, 0x46, 0x4C, DR_NO_QUIRKS },

  /* Bowser minigames (multiplayer) */
  { "Funstacle Course!", DR_MINIGAME_INVALID, 0x3E, 0x44, DR_NO_QUIRKS },
  { "Funderwall!", DR_MINIGAME_INVALID, 0x3F, 0x45, DR_NO_QUIRKS },
  { "Magmagical Journey!", DR_MINIGAME_INVALID, 0x40, 0x46, DR_NO_QUIRKS },

  /* Rare / Boss */
  { "Ice Moves", DR_MINIGAME_INVALID, 0x25, 0x2B, DR_NO_QUIRKS },
  { "Stick and Spin", DR_MINIGAME_INVALID, 0x4B, 0x51, DR_NO_QUIRKS },
  { "Bowser's Lovely Lift!", DR_MINIGAME_INVALID, 0x55, 0x5B, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS },
};

static MpGcnConfig buildConfig()
{
  MpGcnConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 7 (USA) (Rev 1)").toStdString();
  config.state = (dr_state_directory() + "/mp7.state.zip").toStdString();

  config.scene_miniexplain = 0x06;
  config.scene_miniresults = 0x73;

  config.scene_addr = 0x802f2f3c;
  config.minigame_addr = 0x80291558;

  const size_t character_addr[4]    = { 0x80290c48, 0x80290c52, 0x80290c5c, 0x80290c66 };
  const size_t controller_addr[4]   = { 0x80290c4a, 0x80290c54, 0x80290c5e, 0x80290c68 };
  const size_t difficulty_addr[4]   = { 0x80290c4c, 0x80290c56, 0x80290c60, 0x80290c6a };
  const size_t team_addr[4]         = { 0x80290c4e, 0x80290c58, 0x80290c62, 0x80290c6c };
  const size_t bot_addr[4]          = { 0x80290c50, 0x80290c5a, 0x80290c64, 0x80290c6e };
  const size_t bonus_result_addr[4] = { 0x80290cc8, 0x80290dd8, 0x80290ee8, 0x80290ff8 };
  const size_t result_addr[4]       = { 0x80290cca, 0x80290dda, 0x80290eea, 0x80290ffa };
  memcpy(config.character_addr, character_addr, sizeof(character_addr));
  memcpy(config.controller_addr, controller_addr, sizeof(controller_addr));
  memcpy(config.difficulty_addr, difficulty_addr, sizeof(difficulty_addr));
  memcpy(config.team_addr, team_addr, sizeof(team_addr));
  memcpy(config.bot_addr, bot_addr, sizeof(bot_addr));
  memcpy(config.bonus_result_addr, bonus_result_addr, sizeof(bonus_result_addr));
  memcpy(config.result_addr, result_addr, sizeof(result_addr));

  config.character_ids = MP7_CHARACTER_IDS;
  config.minigames = MP7_MINIGAMES;

  return config;
}

MarioParty7::MarioParty7(QRetro *sharedCore, QObject *parent)
  : MarioPartyGcn(buildConfig(), sharedCore, parent)
{
}
