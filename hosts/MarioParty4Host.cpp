#include "MarioParty4Host.h"

#include <asm/mp4.h>

#include <QRetroDirectories.h>

static dr_character mp4_char_to_dr(unsigned chr)
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
    return DR_CHARACTER_DONKEY_KONG;
  case 0x06:
    return DR_CHARACTER_DAISY;
  case 0x07:
    return DR_CHARACTER_WALUIGI;
  }

  return DR_CHARACTER_INVALID;
}

/* Native roulette type byte -> dr_minigame_type */
static const dr_minigame_type MP4_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, /* 0 */
  DR_MINIGAME_1V3, /* 1 */
  DR_MINIGAME_2V2, /* 2 */
  DR_MINIGAME_INVALID, /* bowser */
  DR_MINIGAME_BATTLE, /* 4 */
  DR_MINIGAME_INVALID, /* chance time */
  DR_MINIGAME_INVALID, /* duel */
  DR_MINIGAME_INVALID, /* extra */
  DR_MINIGAME_INVALID, /* the final battle */
};

static const dr_scene_name_t MP4_SCENE_NAMES[] =
{
  // { 0x00, "_minigameDLL" },
  { 0x01, "Booting up", true }, // bootdll
  { 0x02, "E3 demo setup", true }, // e3setupDLL
  { 0x03, "Mini-Game explanation", true }, // instdll
  // { 0x04, "m300dll" },
  // { 0x05, "m302dll" },
  // { 0x06, "m303dll" },
  // { 0x07, "m330dll" },
  // { 0x08, "m333dll" },

  { 0x09, "Manta Rings", false }, // m401dll
  { 0x0a, "Slime Time", false }, // m402dll
  { 0x0b, "Booksquirm", false }, // m403dll
  { 0x0c, "Trace Race", false }, // m404dll
  { 0x0d, "Mario Medley", false }, // m405dll
  { 0x0e, "Avalanche!", false }, // m406dll
  { 0x0f, "Domination", false }, // m407dll
  { 0x10, "Paratrooper Plunge", false }, // m408dll
  { 0x11, "Toad's Quick Draw", false }, // m409dll
  { 0x12, "Three Throw", false }, // m410dll
  { 0x13, "Photo Finish", false }, // m411dll
  { 0x14, "Mr. Blizzard's Brigade", false }, // m412dll
  { 0x15, "Bob-omb Breakers", false }, // m413dll
  { 0x16, "Long Claw of the Law", false }, // m414dll
  { 0x17, "Stamp Out!", false }, // m415dll
  { 0x18, "Candlelight Fright", false }, // m416dll
  { 0x19, "Makin' Waves", false }, // m417dll
  { 0x1a, "Hide and Go BOOM!", false }, // m418dll
  { 0x1b, "Tree Stomp", false }, // m419dll
  { 0x1c, "Fish n' Drips", false }, // m420dll
  { 0x1d, "Hop or Pop", false }, // m421dll
  { 0x1e, "Money Belts", false }, // m422dll
  { 0x1f, "GOOOOOOOAL!!", false }, // m423dll
  { 0x20, "Blame it on the Crane", false }, // m424dll
  { 0x21, "The Great Deflate", false }, // m425dll
  { 0x22, "Revers-a-Bomb", false }, // m426dll
  { 0x23, "Right Oar Left?", false }, // m427dll
  { 0x24, "Cliffhangers", false }, // m428dll
  { 0x25, "Team Treasure Trek", false }, // m429dll
  { 0x26, "Pair-a-sailing", false }, // m430dll
  { 0x27, "Order Up", false }, // m431dll
  { 0x28, "Dungeon Duos", false }, // m432dll
  { 0x29, "Beach Volley Folley", false }, // m433dll
  { 0x2a, "Cheep Cheep Sweep", false }, // m434dll
  { 0x2b, "Darts of Doom", false }, // m435dll
  { 0x2c, "Fruits of Doom", false }, // m436dll
  { 0x2d, "Balloon of Doom", false }, // m437dll
  { 0x2e, "Chain Chomp Fever", false }, // m438dll
  { 0x2f, "Paths of Peril", false }, // m439dll
  { 0x30, "Bowser's Bigger Blast", false }, // m440dll
  { 0x31, "Butterfly Blitz", false }, // m441dll
  { 0x32, "Barrel Baron", false }, // m442dll
  { 0x33, "Mario Speedwagons", false }, // m443dll
  { 0x34, "Reversal of Fortune", false }, // m444dll
  { 0x35, "Bowser Bop", false }, // m445dll
  { 0x36, "Mystic Match 'Em", false }, // m446dll
  { 0x37, "Archaeologuess", false }, // m447dll
  { 0x38, "Goomba's Chip Flip", false }, // m448dll
  { 0x39, "Kareening Koopas", false }, // m449dll
  { 0x3a, "The Final Battle!", false }, // m450dll
  { 0x3b, "Jigsaw Jitters", false }, // m451dll
  { 0x3c, "Challenge Booksquirm", false }, // m453dll
  { 0x3d, "Rumble Fishing", false }, // m455dll
  { 0x3e, "Take a Breather", false }, // m456dll
  { 0x3f, "Bowser Wrestling", false }, // m457dll
  { 0x40, "Panels of Doom", false }, // m458dll
  { 0x41, "Mushroom Medic", false }, // m459dll
  { 0x42, "Doors of Doom", false }, // m460dll
  { 0x43, "Bob-omb X-ing", false }, // m461dll
  { 0x44, "Goomba Stomp", false }, // m462dll
  { 0x45, "Panel Panic", false }, // m463dll

  { 0x46, "Game setup", false }, // mentdll
  { 0x47, "Message check", true }, // messdll
  { 0x48, "Mini-Game mode", false }, // mgmodedll
  { 0x49, "Model test", true }, // modeltestdll
  { 0x4a, "Mode select", false }, // modeseldll
  { 0x4b, "Extra Room", false }, // mpexdll
  // { 0x4c, "msetupdll" },
  { 0x4d, "Story mode", false }, // mstory2dll
  { 0x4e, "Story mode", false }, // mstory3dll
  { 0x4f, "Story mode", false }, // mstory4dll
  { 0x50, "Story mode", false }, // mstorydll
  // { 0x51, "nisdll" },
  { 0x52, "Options", false }, // option
  { 0x53, "Present Room", false }, // present
  { 0x54, "Mini-Game results", true }, // resultdll
  // { 0x55, "safdll" },
  { 0x56, "Debug select menu", true }, // selmenuDLL
  { 0x57, "Credits", true }, // staffdll
  { 0x58, "Character select", false }, // subchrseldll
  { 0x59, "Toad's Midway Madness", false }, // w01dll
  { 0x5a, "Goomba's Greedy Gala", false }, // w02dll
  { 0x5b, "Shy Guy's Jungle Jam", false }, // w03dll
  { 0x5c, "Boo's Haunted Bash", false }, // w04dll
  { 0x5d, "Koopa's Seaside Soiree", false }, // w05dll
  { 0x5e, "Bowser's Gnarly Party", false }, // w06dll
  { 0x5f, "Tutorial board", false }, // w10dll
  { 0x60, "Mega Board Mayhem", false }, // w20dll
  { 0x61, "Mini Board Mad-Dash", false }, // w21dll
  // { 0x62, "ztardll" },

  { -1, nullptr },
};

static DrGcnHostConfig makeConfig()
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 4 (USA) (Rev 1).rvz").toStdString();

  config.cheats.cave = MP4_CAVE;
  config.cheats.cave_addr = MP4_CAVE_ADDR;
  config.cheats.cave_size = MP4_CAVE_SIZE;
  config.cheats.cheat_board = nullptr;
  config.cheats.hooks = MP4_HOOK_BOARD;

  config.values.scene = { 0x801d3ce0, DR_VALUE_TYPE_S32 };
  config.values.character[0] = { 0x8018fc10, DR_VALUE_TYPE_U16 };
  config.values.character[1] = { 0x8018fc1a, DR_VALUE_TYPE_U16 };
  config.values.character[2] = { 0x8018fc24, DR_VALUE_TYPE_U16 };
  config.values.character[3] = { 0x8018fc2e, DR_VALUE_TYPE_U16 };
  config.values.controller[0] = { 0x8018fc12, DR_VALUE_TYPE_U16 };
  config.values.controller[1] = { 0x8018fc1c, DR_VALUE_TYPE_U16 };
  config.values.controller[2] = { 0x8018fc26, DR_VALUE_TYPE_U16 };
  config.values.controller[3] = { 0x8018fc30, DR_VALUE_TYPE_U16 };
  config.values.difficulty[0] = { 0x8018fc14, DR_VALUE_TYPE_U16 };
  config.values.difficulty[1] = { 0x8018fc1e, DR_VALUE_TYPE_U16 };
  config.values.difficulty[2] = { 0x8018fc28, DR_VALUE_TYPE_U16 };
  config.values.difficulty[3] = { 0x8018fc32, DR_VALUE_TYPE_U16 };
  config.values.team[0] = { 0x8018fc16, DR_VALUE_TYPE_U16 };
  config.values.team[1] = { 0x8018fc20, DR_VALUE_TYPE_U16 };
  config.values.team[2] = { 0x8018fc2a, DR_VALUE_TYPE_U16 };
  config.values.team[3] = { 0x8018fc34, DR_VALUE_TYPE_U16 };
  config.values.bot[0] = { 0x8018fc18, DR_VALUE_TYPE_U16 };
  config.values.bot[1] = { 0x8018fc22, DR_VALUE_TYPE_U16 };
  config.values.bot[2] = { 0x8018fc2c, DR_VALUE_TYPE_U16 };
  config.values.bot[3] = { 0x8018fc36, DR_VALUE_TYPE_U16 };
  config.values.result[0] = { 0x8018fc60, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x8018fc90, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x8018fcc0, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x8018fcf0, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[0] = { 0x8018fc5e, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[1] = { 0x8018fc8e, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[2] = { 0x8018fcbe, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[3] = { 0x8018fcee, DR_VALUE_TYPE_U16 };
  config.values.coins[0] = { 0x8018fc54, DR_VALUE_TYPE_U16 };
  config.values.coins[1] = { 0x8018fc84, DR_VALUE_TYPE_U16 };
  config.values.coins[2] = { 0x8018fcb4, DR_VALUE_TYPE_U16 };
  config.values.coins[3] = { 0x8018fce4, DR_VALUE_TYPE_U16 };
  config.values.stars[0] = { 0x8018fc62, DR_VALUE_TYPE_U16 };
  config.values.stars[1] = { 0x8018fc92, DR_VALUE_TYPE_U16 };
  config.values.stars[2] = { 0x8018fcc2, DR_VALUE_TYPE_U16 };
  config.values.stars[3] = { 0x8018fcf2, DR_VALUE_TYPE_U16 };
  config.values.battle_ante[0] = { 0x8018fc5c, DR_VALUE_TYPE_U16 };
  config.values.battle_ante[1] = { 0x8018fc8c, DR_VALUE_TYPE_U16 };
  config.values.battle_ante[2] = { 0x8018fcbc, DR_VALUE_TYPE_U16 };
  config.values.battle_ante[3] = { 0x8018fcec, DR_VALUE_TYPE_U16 };
  config.values.minigame_id = { 0x8018fd2c, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP4_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };
  config.values.rng = { 0x801d342c, DR_VALUE_TYPE_U32 };

  config.scene_miniexplain = 0x03;
  config.scene_miniresults = 0x54;

  config.scene_names = MP4_SCENE_NAMES;

  config.char_to_dr = mp4_char_to_dr;

  config.minigame_type_to_dr = MP4_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP4_MINIGAME_TYPE_TO_DR) / sizeof(*MP4_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP4_HOST_STATE;

  config.save_files = { "*-GMPE-MARIPA4BOX0.gci" };

  return config;
}

MarioParty4Host::MarioParty4Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
}

dr_minigame_result_t MarioParty4Host::adjustResult(unsigned index,
  const dr_minigame_result_t &result)
{
  dr_minigame_result_t adjusted = result;

  (void)index;

  /* The guest hands back "1 = got through" like every other mini-game type; a
   * Bowser mini-game's board code reads the opposite, marking who was caught. */
  if (m_MinigameType == DR_MINIGAME_BOWSER)
    adjusted.coins = result.coins ? 0 : 1;

  return adjusted;
}
