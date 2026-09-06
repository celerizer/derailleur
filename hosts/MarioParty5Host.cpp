#include "MarioParty5Host.h"

#include <asm/mp5.h>

#include <QRetroDirectories.h>

static dr_character mp5_char_to_dr(unsigned chr)
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
    return DR_CHARACTER_KOOPA_KID;
  case 0x0A:
    return DR_CHARACTER_KOOPA_KID_R;
  case 0x0B:
    return DR_CHARACTER_KOOPA_KID_G;
  case 0x0C:
    return DR_CHARACTER_KOOPA_KID_B;
  }

  return DR_CHARACTER_INVALID;
}

static const dr_minigame_type MP5_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, /* 0 */
  DR_MINIGAME_1V3, /* 1 */
  DR_MINIGAME_2V2, /* 2 */
  DR_MINIGAME_BATTLE, /* 3 */
  DR_MINIGAME_BOWSER, /* KUPPA */
  DR_MINIGAME_INVALID, /* LAST */
  DR_MINIGAME_DUEL, /* 6 */
  DR_MINIGAME_INVALID, /* SD (super duel mode) */
  DR_MINIGAME_DK, /* DONKEY */
};

static const dr_scene_name_t MP5_SCENE_NAMES[] =
{
  { 0x00, "Booting up", true }, // actmanDLL
  { 0x01, "Booting up", false }, // bootdll
  { 0x02, "Card Party", false }, // carddll
  // { 0x03, "decathlondll" },
  { 0x04, "E3 demo boot", true }, // e3bootdll
  { 0x05, "E3 demo setup", true }, // e3setupdll
  { 0x06, "File select", false }, // fileseldll
  { 0x07, "Mini-Game explanation", true }, // instdll
  // { 0x08, "m401dll" },
  // { 0x09, "m416dll" },
  // { 0x0a, "m425dll" },

  { 0x0b, "Beach Volleyball", false }, // m433dll

  // { 0x0c, "m435dll" },
  // { 0x0d, "m438dll" },
  // { 0x0e, "m444dll" },

  { 0x0f, "Coney Island", false }, // m501dll
  { 0x10, "Ground Pound Down", false }, // m502dll
  { 0x11, "Chimp Chase", false }, // m503dll
  { 0x12, "Chomp Romp", false }, // m504dll
  { 0x13, "Pushy Penguins", false }, // m505dll
  { 0x14, "Leaf Leap", false }, // m506dll
  { 0x15, "Night Light Fright", false }, // m507dll
  { 0x16, "Pop-Star Piranhas", false }, // m508dll
  { 0x17, "Mazed & Confused", false }, // m509dll
  { 0x18, "Dinger Derby", false }, // m510dll
  { 0x19, "Hydrostars", false }, // m511dll
  { 0x1a, "Later Skater", false }, // m512dll
  { 0x1b, "Will Flower", false }, // m513dll
  { 0x1c, "Triple Jump", false }, // m514dll
  { 0x1d, "Hotel Goomba", false }, // m515dll
  { 0x1e, "Coin Cache", false }, // m516dll
  { 0x1f, "Flatiator", false }, // m517dll
  { 0x20, "Squared Away", false }, // m518dll
  { 0x21, "Mario Mechs", false }, // m519dll
  { 0x22, "Revolving Fire", false }, // m520dll
  { 0x23, "Clock Stoppers", false }, // m521dll
  { 0x24, "Heat Stroke", false }, // m522dll
  { 0x25, "Beam Team", false }, // m523dll
  { 0x26, "Vicious Vending", false }, // m524dll
  { 0x27, "Big Top Drop", false }, // m525dll
  { 0x28, "Defuse or Lose", false }, // m526dll
  { 0x29, "ID UFO", false }, // m527dll
  { 0x2a, "Mario Can-Can", false }, // m528dll
  { 0x2b, "Handy Hoppers", false }, // m529dll
  { 0x2c, "Berry Basket", false }, // m530dll
  { 0x2d, "Bus Buffer", false }, // m531dll
  { 0x2e, "Rumble Ready", false }, // m532dll
  { 0x2f, "Submarathon", false }, // m533dll
  { 0x30, "Manic Mallets", false }, // m534dll
  { 0x31, "Astro-Logical", false }, // m535dll
  { 0x32, "Bill Blasters", false }, // m536dll
  { 0x33, "Tug-o-Dorrie", false }, // m537dll
  { 0x34, "Twist 'n' Out", false }, // m538dll
  { 0x35, "Lucky Lineup", false }, // m539dll
  { 0x36, "Random Ride", false }, // m540dll
  { 0x37, "Shock Absorbers", false }, // m541dll
  { 0x38, "Countdown Pound", false }, // m542dll
  { 0x39, "Whomp Maze", false }, // m543dll
  { 0x3a, "Shy Guy Showdown", false }, // m544dll
  { 0x3b, "Button Mashers", false }, // m545dll
  { 0x3c, "Get a Rope", false }, // m546dll
  { 0x3d, "Pump 'n' Jump", false }, // m547dll
  { 0x3e, "Head Waiter", false }, // m548dll
  { 0x3f, "Blown Away", false }, // m549dll
  { 0x40, "Merry Poppings", false }, // m550dll
  { 0x41, "Pound Peril", false }, // m551dll
  { 0x42, "Piece Out", false }, // m552dll
  { 0x43, "Bound of Music", false }, // m553dll
  { 0x44, "Wind Wavers", false }, // m554dll
  { 0x45, "Sky Survivor", false }, // m555dll
  { 0x46, "Rain of Fire", false }, // m559dll
  { 0x47, "Cage-in Cookin'", false }, // m560dll
  { 0x48, "Scaldin' Cauldron", false }, // m561dll
  { 0x49, "Frightmare", false }, // m562dll
  { 0x4a, "Flower Shower", false }, // m563dll
  { 0x4b, "Dodge Bomb", false }, // m564dll
  { 0x4c, "Fish Upon a Star", false }, // m565dll
  { 0x4d, "Rumble Fumble", false }, // m566dll
  { 0x4e, "Quilt for Speed", false }, // m567dll
  { 0x4f, "Tube It or Lose It", false }, // m568dll
  { 0x50, "Mathletes", false }, // m569dll
  { 0x51, "Fight Cards", false }, // m570dll
  { 0x52, "Banana Punch", false }, // m571dll
  { 0x53, "Da Vine Climb", false }, // m572dll
  { 0x54, "Mass A-peel", false }, // m573dll
  { 0x55, "Panic Pinball", false }, // m574dll
  { 0x56, "Banking Coins", false }, // m575dll
  { 0x57, "Frozen Frenzy", false }, // m576dll
  { 0x58, "Curvy Curbs", false }, // m577dll
  { 0x59, "Fish Sticks", false }, // m579dll
  { 0x5a, "Ice Hockey", false }, // m580dll

  { 0x5b, "Beach Volleyball setup", false }, // mdbeachdll
  { 0x5c, "Card Party setup", false }, // mdcarddll
  { 0x5d, "Mini-Game mode", false }, // mdminidll
  { 0x5e, "Bonus mode", false }, // mdomakedll
  { 0x5f, "Options", false }, // mdoptiondll
  { 0x60, "Party mode setup", false }, // mdpartydll
  { 0x61, "Mode select", false }, // mdseldll
  { 0x62, "Story mode setup", false }, // mdstorydll
  { 0x63, "Message check", true }, // meschkdll
  { 0x64, "Free Play", false }, // mgfreedll
  { 0x65, "Mini-Game Match", false }, // mgmatchdll
  { 0x66, "Mini-Game Tournament", false }, // mgtourdll
  { 0x67, "Mini-Game Wars", false }, // mgwarsdll
  { 0x68, "Motion check", true }, // motchkdll
  { 0x69, "Party results", true }, // partyresultdll
  // { 0x6a, "rescarddll" },
  { 0x6b, "Mini-Game results", true }, // resultdll
  // { 0x6c, "safdll" },
  // { 0x6d, "sampledll" },
  { 0x6e, "Super Duel", false }, // sd00dll
  { 0x6f, "Super Duel room", false }, // sdroomdll
  { 0x70, "Debug select menu", true }, // selmenuDLL
  // { 0x71, "sequencedll" },
  { 0x72, "Credits", true }, // staffdll
  { 0x73, "Story mode", false }, // storymodedll
  { 0x74, "Story mode results", true }, // storyresultdll
  // { 0x75, "systemdll" },
  { 0x76, "Toy Dream", false }, // w01dll
  { 0x77, "Toy Dream (story)", false }, // w01sdll
  { 0x78, "Rainbow Dream", false }, // w02dll
  { 0x79, "Rainbow Dream (story)", false }, // w02sdll
  { 0x7a, "Pirate Dream", false }, // w03dll
  { 0x7b, "Pirate Dream (story)", false }, // w03sdll
  { 0x7c, "Undersea Dream", false }, // w04dll
  { 0x7d, "Undersea Dream (story)", false }, // w04sdll
  { 0x7e, "Future Dream", false }, // w05dll
  { 0x7f, "Future Dream (story)", false }, // w05sdll
  { 0x80, "Sweet Dream", false }, // w06dll
  { 0x81, "Sweet Dream (story)", false }, // w06sdll
  { 0x82, "Bowser Nightmare", false }, // w07dll
  { 0x83, "Bowser Nightmare (story)", false }, // w07sdll
  { 0x84, "Tutorial board", false }, // w10dll
  // { 0x85, "w20dll" },

  { -1, nullptr },
};

static DrGcnHostConfig makeConfig()
{
  DrGcnHostConfig config = {};

  config.core = dr_core_path(DR_CORE_DOLPHIN).toStdString();
  config.game = (dr_roms_directory() + "/Mario Party 5 (USA).rvz").toStdString();

  config.cheats.cave = MP5_CAVE;
  config.cheats.cave_addr = MP5_CAVE_ADDR;
  config.cheats.cave_size = MP5_CAVE_SIZE;
  config.cheats.cheat_board = nullptr;
  config.cheats.hooks = MP5_HOOK_BOARD;

  config.values.scene = { 0x80288860, DR_VALUE_TYPE_S32 };
  config.values.character[0] = { 0x8022a048, DR_VALUE_TYPE_U16 };
  config.values.character[1] = { 0x8022a052, DR_VALUE_TYPE_U16 };
  config.values.character[2] = { 0x8022a05c, DR_VALUE_TYPE_U16 };
  config.values.character[3] = { 0x8022a066, DR_VALUE_TYPE_U16 };
  config.values.controller[0] = { 0x8022a04a, DR_VALUE_TYPE_U16 };
  config.values.controller[1] = { 0x8022a054, DR_VALUE_TYPE_U16 };
  config.values.controller[2] = { 0x8022a05e, DR_VALUE_TYPE_U16 };
  config.values.controller[3] = { 0x8022a068, DR_VALUE_TYPE_U16 };
  config.values.difficulty[0] = { 0x8022a04c, DR_VALUE_TYPE_U16 };
  config.values.difficulty[1] = { 0x8022a056, DR_VALUE_TYPE_U16 };
  config.values.difficulty[2] = { 0x8022a060, DR_VALUE_TYPE_U16 };
  config.values.difficulty[3] = { 0x8022a06a, DR_VALUE_TYPE_U16 };
  config.values.team[0] = { 0x8022a04e, DR_VALUE_TYPE_U16 };
  config.values.team[1] = { 0x8022a058, DR_VALUE_TYPE_U16 };
  config.values.team[2] = { 0x8022a062, DR_VALUE_TYPE_U16 };
  config.values.team[3] = { 0x8022a06c, DR_VALUE_TYPE_U16 };
  config.values.bot[0] = { 0x8022a050, DR_VALUE_TYPE_U16 };
  config.values.bot[1] = { 0x8022a05a, DR_VALUE_TYPE_U16 };
  config.values.bot[2] = { 0x8022a064, DR_VALUE_TYPE_U16 };
  config.values.bot[3] = { 0x8022a06e, DR_VALUE_TYPE_U16 };
  config.values.result[0] = { 0x8022a09c, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x8022a1a4, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x8022a2ac, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x8022a3b4, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[0] = { 0x8022a09a, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[1] = { 0x8022a1a2, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[2] = { 0x8022a2aa, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[3] = { 0x8022a3b2, DR_VALUE_TYPE_U16 };
  config.values.coins[0] = { 0x8022a090, DR_VALUE_TYPE_S16 };
  config.values.coins[1] = { 0x8022a198, DR_VALUE_TYPE_S16 };
  config.values.coins[2] = { 0x8022a2a0, DR_VALUE_TYPE_S16 };
  config.values.coins[3] = { 0x8022a3a8, DR_VALUE_TYPE_S16 };
  config.values.stars[0] = { 0x8022a0a4, DR_VALUE_TYPE_S16 };
  config.values.stars[1] = { 0x8022a1ac, DR_VALUE_TYPE_S16 };
  config.values.stars[2] = { 0x8022a2b4, DR_VALUE_TYPE_S16 };
  config.values.stars[3] = { 0x8022a3bc, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[0] = { 0x8022a098, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[1] = { 0x8022a1a0, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[2] = { 0x8022a2a8, DR_VALUE_TYPE_S16 };
  config.values.battle_ante[3] = { 0x8022a3b0, DR_VALUE_TYPE_S16 };
  config.values.minigame_id = { 0x8022a4c4, DR_VALUE_TYPE_S16 };
  config.values.title_block = { MP5_TITLE_BLOCK, DR_VALUE_TYPE_POINTER };

  config.scene_miniexplain = 0x07;
  config.scene_miniresults = 0x6b;

  config.scene_names = MP5_SCENE_NAMES;

  config.char_to_dr = mp5_char_to_dr;

  config.minigame_type_to_dr = MP5_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP5_MINIGAME_TYPE_TO_DR) / sizeof(*MP5_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP5_HOST_STATE;

  /* save_files: MP5's memory-card file name still needs filling in */

  return config;
}

MarioParty5Host::MarioParty5Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
}
