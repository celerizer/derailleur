#include "MarioParty4.h"

#include <cstring>

static const uint16_t MP4_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x06,
};

static const dr_mp_minigame_t MP4_MINIGAMES[] = {
  /* 4P */
  { "Manta Rings", DR_MINIGAME_4P, 0x00, 0x09, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Slime Time", DR_MINIGAME_4P, 0x01, 0x0A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Booksquirm", DR_MINIGAME_4P, 0x02, 0x0B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Trace Race", DR_MINIGAME_BATTLE, 0x03, 0x0C, DR_QUIRK_SAFE_TEXTURE_CACHE, DR_NO_FLAGS },
  { "Mario Medley", DR_MINIGAME_4P, 0x04, 0x0D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Avalanche!", DR_MINIGAME_4P, 0x05, 0x0E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Domination", DR_MINIGAME_4P, 0x06, 0x0F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Paratrooper Plunge", DR_MINIGAME_4P, 0x07, 0x10, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Toad's Quick Draw", DR_MINIGAME_4P, 0x08, 0x11, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Three Throw", DR_MINIGAME_4P, 0x09, 0x12, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Photo Finish", DR_MINIGAME_4P, 0x0A, 0x13, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mr. Blizzard's Brigade", DR_MINIGAME_4P, 0x0B, 0x14, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bob-omb Breakers", DR_MINIGAME_4P, 0x0C, 0x15, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Long Claw of the Law", DR_MINIGAME_4P, 0x0D, 0x16, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stamp Out!", DR_MINIGAME_4P, 0x0E, 0x17, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* 1v3 */
  { "Candlelight Fright", DR_MINIGAME_1V3, 0x0F, 0x18, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Makin' Waves", DR_MINIGAME_1V3, 0x10, 0x19, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hide and Go BOOM!", DR_MINIGAME_1V3, 0x11, 0x1A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Tree Stomp", DR_MINIGAME_1V3, 0x12, 0x1B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fish n' Drips", DR_MINIGAME_1V3, 0x13, 0x1C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Hop or Pop", DR_MINIGAME_1V3, 0x14, 0x1D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Money Belts", DR_MINIGAME_1V3, 0x15, 0x1E, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "GOOOOOOOAL!!", DR_MINIGAME_1V3, 0x16, 0x1F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Blame it on the Crane", DR_MINIGAME_1V3, 0x17, 0x20, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* 2v2 */
  { "The Great Deflate", DR_MINIGAME_2V2, 0x18, 0x21, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Revers-a-Bomb", DR_MINIGAME_2V2, 0x19, 0x22, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Right Oar Left?", DR_MINIGAME_2V2, 0x1A, 0x23, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Cliffhangers", DR_MINIGAME_2V2, 0x1B, 0x24, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Team Treasure Trek", DR_MINIGAME_2V2, 0x1C, 0x25, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Pair-a-sailing", DR_MINIGAME_2V2, 0x1D, 0x26, DR_NO_QUIRKS, DR_FLAG_LUCKY },
  { "Order Up", DR_MINIGAME_2V2, 0x1E, 0x27, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Dungeon Duos", DR_MINIGAME_2V2, 0x1F, 0x28, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beach Volley Folley", DR_MINIGAME_2V2, 0x20, 0x29, DR_NO_QUIRKS, DR_NO_FLAGS }, // special
  { "Cheep Cheep Sweep", DR_MINIGAME_2V2, 0x21, 0x2A, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Bowser */
  { "Darts of Doom", DR_MINIGAME_SPECIAL, 0x22, 0x2B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Fruits of Doom", DR_MINIGAME_SPECIAL, 0x23, 0x2C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Balloon of Doom", DR_MINIGAME_SPECIAL, 0x24, 0x2D, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Battle */
  { "Chain Chomp Fever", DR_MINIGAME_BATTLE, 0x25, 0x2E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Paths of Peril", DR_MINIGAME_BATTLE, 0x26, 0x2F, DR_QUIRK_EFB_TO_TEXTURE, DR_NO_FLAGS },
  { "Bowser's Bigger Blast", DR_MINIGAME_BATTLE, 0x27, 0x30, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Butterfly Blitz", DR_MINIGAME_BATTLE, 0x28, 0x31, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Etc */
  { "Barrel Baron", DR_MINIGAME_SPECIAL, 0x29, 0x32, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mario Speedwagons", DR_MINIGAME_4P, 0x2A, 0x33, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Reversal of Fortune", DR_MINIGAME_SPECIAL, 0x2B, 0x34, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* Story mode duel mini-games */
  { "Bowser Bop", DR_MINIGAME_DUEL, 0x2C, 0x35, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mystic Match 'Em", DR_MINIGAME_DUEL, 0x2D, 0x36, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Archaeologuess", DR_MINIGAME_DUEL, 0x2E, 0x37, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Goomba's Chip Flip", DR_MINIGAME_DUEL, 0x2F, 0x38, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kareening Koopas", DR_MINIGAME_DUEL, 0x30, 0x39, DR_NO_QUIRKS, DR_NO_FLAGS },

  /* More etc */
  { "The Final Battle!", DR_MINIGAME_SPECIAL, 0x31, 0x3A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Jigsaw Jitters", DR_MINIGAME_SPECIAL, 0xFF, 0x3B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Challenge Booksquirm", DR_MINIGAME_SPECIAL, 0xFF, 0x3C, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Rumble Fishing", DR_MINIGAME_BATTLE, 0xFF, 0x3D, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Take a Breather", DR_MINIGAME_4P, 0xFF, 0x3E, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bowser Wrestling", DR_MINIGAME_SPECIAL, 0xFF, 0x3F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Panels of Doom", DR_MINIGAME_SPECIAL, 0xFF, 0x40, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Mushroom Medic", DR_MINIGAME_SPECIAL, 0xFF, 0x41, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Doors of Doom", DR_MINIGAME_SPECIAL, 0xFF, 0x42, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Bob-omb X-ing", DR_MINIGAME_SPECIAL, 0xFF, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Goomba Stomp", DR_MINIGAME_SPECIAL, 0xFF, 0x44, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Panel Panic", DR_MINIGAME_SPECIAL, 0xFF, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

static MpGcnConfig buildConfig()
{
  MpGcnConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 4 (USA) (Rev 1)").toStdString();
  config.state = (dr_state_directory() + "/mp4.state.zip").toStdString();

  config.scene_miniexplain = 0x03;
  config.scene_miniresults = 0x54;

  config.scene_addr = 0x801d3ce0;
  config.minigame_addr = 0x8018fd2c;

  const size_t character_addr[4]    = { 0x8018fc10, 0x8018fc1a, 0x8018fc24, 0x8018fc2e };
  const size_t controller_addr[4]   = { 0x8018fc12, 0x8018fc1c, 0x8018fc26, 0x8018fc30 };
  const size_t difficulty_addr[4]   = { 0x8018fc14, 0x8018fc1e, 0x8018fc28, 0x8018fc32 };
  const size_t team_addr[4]         = { 0x8018fc16, 0x8018fc20, 0x8018fc2a, 0x8018fc34 };
  const size_t bot_addr[4]          = { 0x8018fc18, 0x8018fc22, 0x8018fc2c, 0x8018fc36 };
  const size_t bonus_result_addr[4] = { 0x8018fc5e, 0x8018fc8e, 0x8018fcbe, 0x8018fcee };
  const size_t result_addr[4]       = { 0x8018fc60, 0x8018fc90, 0x8018fcc0, 0x8018fcf0 };
  memcpy(config.character_addr, character_addr, sizeof(character_addr));
  memcpy(config.controller_addr, controller_addr, sizeof(controller_addr));
  memcpy(config.difficulty_addr, difficulty_addr, sizeof(difficulty_addr));
  memcpy(config.team_addr, team_addr, sizeof(team_addr));
  memcpy(config.bot_addr, bot_addr, sizeof(bot_addr));
  memcpy(config.bonus_result_addr, bonus_result_addr, sizeof(bonus_result_addr));
  memcpy(config.result_addr, result_addr, sizeof(result_addr));

  config.coins[0] = { 0x8018fc54, DR_VALUE_TYPE_U16 };
  config.coins[1] = { 0x8018fc84, DR_VALUE_TYPE_U16 };
  config.coins[2] = { 0x8018fcb4, DR_VALUE_TYPE_U16 };
  config.coins[3] = { 0x8018fce4, DR_VALUE_TYPE_U16 };
  config.stars[0] = { 0x8018fc62, DR_VALUE_TYPE_U16 };
  config.stars[1] = { 0x8018fc92, DR_VALUE_TYPE_U16 };
  config.stars[2] = { 0x8018fcc2, DR_VALUE_TYPE_U16 };
  config.stars[3] = { 0x8018fcf2, DR_VALUE_TYPE_U16 };

  config.character_ids = MP4_CHARACTER_IDS;
  config.minigames = MP4_MINIGAMES;

  return config;
}

MarioParty4::MarioParty4(QRetro *sharedCore, QObject *parent)
  : MarioPartyGcn(buildConfig(), sharedCore, parent)
{
}
