#include "MarioParty3.h"

#include <QRetroDirectories.h>

static const uint8_t MP3_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  [DR_CHARACTER_INVALID] = 0xFF,
  [DR_CHARACTER_MARIO] = 0x00,
  [DR_CHARACTER_LUIGI] = 0x01,
  [DR_CHARACTER_PEACH] = 0x02,
  [DR_CHARACTER_YOSHI] = 0x03,
  [DR_CHARACTER_WARIO] = 0x04,
  [DR_CHARACTER_DONKEY_KONG] = 0x05,
  [DR_CHARACTER_WALUIGI] = 0x06,
  [DR_CHARACTER_DAISY] = 0x07,
};

static const dr_mp_minigame_t MP3_MINIGAMES[] =
{
  // 1v3
  { "Hand, Line and Sinker", DR_MINIGAME_1V3, 0x01, 0x01, DR_NO_QUIRKS },
  { "Coconut Conk", DR_MINIGAME_1V3, 0x02, 0x02, DR_NO_QUIRKS },
  { "Spotlight Swim", DR_MINIGAME_1V3, 0x03, 0x03, DR_NO_QUIRKS },
  { "Boulder Ball", DR_MINIGAME_1V3, 0x04, 0x04, DR_NO_QUIRKS },
  { "Crazy Cogs", DR_MINIGAME_1V3, 0x05, 0x05, DR_NO_QUIRKS },
  { "Hide and Sneak", DR_MINIGAME_1V3, 0x06, 0x06, DR_NO_QUIRKS },
  { "Ridiculous Relay", DR_MINIGAME_1V3, 0x07, 0x07, DR_NO_QUIRKS },
  { "Thwomp Pull", DR_MINIGAME_1V3, 0x08, 0x08, DR_NO_QUIRKS },
  { "River Raiders", DR_MINIGAME_1V3, 0x09, 0x09, DR_NO_QUIRKS },
  { "Tidal Toss", DR_MINIGAME_1V3, 0x0A, 0x0A, DR_NO_QUIRKS },

  // 2v2
  { "Eatsa Pizza", DR_MINIGAME_2V2, 0x0B, 0x0B, DR_NO_QUIRKS },
  { "Baby Bowser Broadside", DR_MINIGAME_2V2, 0x0C, 0x0C, DR_NO_QUIRKS },
  { "Pump, Pump and Away", DR_MINIGAME_2V2, 0x0D, 0x0D, DR_NO_QUIRKS },
  { "Hyper Hydrants", DR_MINIGAME_2V2, 0x0E, 0x0E, DR_NO_QUIRKS },
  { "Picking Panic", DR_MINIGAME_2V2, 0x0F, 0x0F, DR_NO_QUIRKS },
  { "Cosmic Coaster", DR_MINIGAME_2V2, 0x10, 0x10, DR_NO_QUIRKS },
  { "Puddle Paddle", DR_MINIGAME_2V2, 0x11, 0x11, DR_NO_QUIRKS },
  { "Etch 'n' Catch", DR_MINIGAME_2V2, 0x12, 0x12, DR_NO_QUIRKS },
  { "Log Jam", DR_MINIGAME_2V2, 0x13, 0x13, DR_NO_QUIRKS },
  { "Slot Synch", DR_MINIGAME_2V2, 0x14, 0x14, DR_NO_QUIRKS },

  // 4p
  { "Treadmill Grill", DR_MINIGAME_4P, 0x15, 0x15, DR_NO_QUIRKS },
  { "Toadstool Titan", DR_MINIGAME_4P, 0x16, 0x16, DR_NO_QUIRKS },
  { "Aces High", DR_MINIGAME_4P, 0x17, 0x17, DR_NO_QUIRKS },
  { "Bounce 'n' Trounce", DR_MINIGAME_4P, 0x18, 0x18, DR_NO_QUIRKS },
  { "Ice Rink Risk", DR_MINIGAME_4P, 0x19, 0x19, DR_NO_QUIRKS },
  { "Locked Out", DR_MINIGAME_BATTLE, 0x1A, 0x1A, DR_NO_QUIRKS }, // yep
  { "Chip Shot Challenge", DR_MINIGAME_4P, 0x1B, 0x1B, DR_NO_QUIRKS },
  { "Parasol Plummet", DR_MINIGAME_4P, 0x1C, 0x1C, DR_NO_QUIRKS },
  { "Messy Memory", DR_MINIGAME_4P, 0x1D, 0x1D, DR_NO_QUIRKS },
  { "Picture Imperfect", DR_MINIGAME_4P, 0x1E, 0x1E, DR_NO_QUIRKS },
  { "Mario's Puzzle Party", DR_MINIGAME_4P, 0x1F, 0x1F, DR_NO_QUIRKS },
  { "The Beat Goes On", DR_MINIGAME_4P, 0x20, 0x20, DR_NO_QUIRKS },
  { "M.P.I.Q.", DR_MINIGAME_4P, 0x21, 0x21, DR_NO_QUIRKS },
  { "Curtain Call", DR_MINIGAME_4P, 0x22, 0x22, DR_NO_QUIRKS },
  { "Water Whirled", DR_MINIGAME_4P, 0x23, 0x23, DR_NO_QUIRKS },
  { "Frigid Bridges", DR_MINIGAME_4P, 0x24, 0x24, DR_NO_QUIRKS },
  { "Awful Tower", DR_MINIGAME_4P, 0x25, 0x25, DR_NO_QUIRKS },
  { "Cheep Cheep Chase", DR_MINIGAME_4P, 0x26, 0x26, DR_NO_QUIRKS },
  { "Pipe Cleaners", DR_MINIGAME_4P, 0x27, 0x27, DR_NO_QUIRKS },
  { "Snowball Summit", DR_MINIGAME_4P, 0x28, 0x28, DR_NO_QUIRKS },

  // battle
  { "All Fired Up", DR_MINIGAME_BATTLE, 0x29, 0x29, DR_NO_QUIRKS },
  { "Stacked Deck", DR_MINIGAME_BATTLE, 0x2A, 0x2A, DR_NO_QUIRKS },
  { "Three Door Monty", DR_MINIGAME_BATTLE, 0x2B, 0x2B, DR_NO_QUIRKS },
  { "Rockin' Raceway", DR_MINIGAME_4P, 0x2C, 0x2C, DR_NO_QUIRKS }, // yep
  { "Merry-Go-Chomp", DR_MINIGAME_BATTLE, 0x2D, 0x2D, DR_NO_QUIRKS },
  { "Slap Down", DR_MINIGAME_BATTLE, 0x2E, 0x2E, DR_NO_QUIRKS },
  { "Storm Chasers", DR_MINIGAME_BATTLE, 0x2F, 0x2F, DR_NO_QUIRKS },
  { "Eye Sore", DR_MINIGAME_BATTLE, 0x30, 0x30, DR_NO_QUIRKS },

  // duel
  { "Vine With Me", DR_MINIGAME_DUEL, 0x31, 0x31, DR_NO_QUIRKS },
  { "Popgun Pick-Off", DR_MINIGAME_DUEL, 0x32, 0x32, DR_NO_QUIRKS },
  { "End of the Line", DR_MINIGAME_DUEL, 0x33, 0x33, DR_NO_QUIRKS },
  { "Bowser Toss", DR_MINIGAME_DUEL, 0x34, 0x34, DR_NO_QUIRKS },
  { "Baby Bowser Bonkers", DR_MINIGAME_DUEL, 0x35, 0x35, DR_NO_QUIRKS },
  { "Motor Rooter", DR_MINIGAME_DUEL, 0x36, 0x36, DR_NO_QUIRKS },
  { "Silly Screws", DR_MINIGAME_DUEL, 0x37, 0x37, DR_NO_QUIRKS },
  { "Crowd Cover", DR_MINIGAME_DUEL, 0x38, 0x38, DR_NO_QUIRKS },
  { "Tick Tock Hop", DR_MINIGAME_DUEL, 0x39, 0x39, DR_NO_QUIRKS },
  { "Fowl Play", DR_MINIGAME_DUEL, 0x3A, 0x3A, DR_NO_QUIRKS },

  // item
  { "Winner's Wheel", DR_MINIGAME_ITEM, 0x3B, 0x3B, DR_NO_QUIRKS },
  { "Hey, Batter, Batter!", DR_MINIGAME_ITEM, 0x3C, 0x3C, DR_NO_QUIRKS },
  { "Bobbing Bow-loons", DR_MINIGAME_ITEM, 0x3D, 0x3D, DR_NO_QUIRKS },
  { "Dorrie Dip", DR_MINIGAME_ITEM, 0x3E, 0x3E, DR_NO_QUIRKS },
  { "Swinging with Sharks", DR_MINIGAME_ITEM, 0x3F, 0x3F, DR_NO_QUIRKS },
  { "Swing 'n' Swipe", DR_MINIGAME_ITEM, 0x40, 0x40, DR_NO_QUIRKS },

  // misc mini-games

  // 41 unused (chance time)
  { "Stardust Battle", DR_MINIGAME_SPECIAL, 0x42, 0x41, DR_NO_QUIRKS },

  { "Game Guy's Roulette", DR_MINIGAME_GAME_GUY, 0x43, 0x42, DR_NO_QUIRKS },
  { "Game Guy's Lucky 7", DR_MINIGAME_GAME_GUY, 0x44, 0x43, DR_NO_QUIRKS },
  { "Game Guy's Magic Boxes", DR_MINIGAME_GAME_GUY, 0x45, 0x44, DR_NO_QUIRKS },
  { "Game Guy's Sweet Surprise", DR_MINIGAME_GAME_GUY, 0x46, 0x45, DR_NO_QUIRKS },

  { "Dizzy Dinghies", DR_MINIGAME_SPECIAL, 0x47, 0x46, DR_NO_QUIRKS },
  { "Mario's Puzzle Party Pro", DR_MINIGAME_SPECIAL, 0x48, 0x1F, DR_NO_QUIRKS },

  { nullptr, DR_MINIGAME_INVALID, -1, -1, DR_NO_QUIRKS },
};

static MpN64Config buildConfig()
{
  return {
    .core = dr_core_path(DR_CORE_MUPEN64PLUSNEXT).toStdString(),
    .game = (dr_roms_directory() + "/Mario Party 3 (USA).z64").toStdString(),
    .state = (dr_state_directory() + "/mp3.state.zip").toStdString(),

    .scene_miniexplain = 0x70,
    .scene_miniresults = 0x71,

    .scene_addr    = 0x800ce200,
    .minigame_addr = 0x800cd06b,

    .controller_addr   = { 0x800d1109, 0x800d1141, 0x800d1179, 0x800d11b1 },
    .difficulty_addr   = { 0x800d110a, 0x800d1142, 0x800d117a, 0x800d11b2 },
    .team_addr         = { 0x800d110b, 0x800d1143, 0x800d117b, 0x800d11b3 },
    .bot_addr          = { 0x800d110f, 0x800d1147, 0x800d117f, 0x800d11b7 },
    .character_addr    = { 0x800d1108, 0x800d1140, 0x800d1178, 0x800d11b0 },
    .bonus_result_addr = { 0x800d110c, 0x800d1144, 0x800d117c, 0x800d11b4 },
    .result_addr       = { 0x800d1112, 0x800d114a, 0x800d1182, 0x800d11ba },

    .character_ids = MP3_CHARACTER_IDS,
    .minigames = MP3_MINIGAMES,
  };
}

MarioParty3::MarioParty3(QObject *parent)
  : MarioPartyN64(buildConfig(), parent)
{
}
