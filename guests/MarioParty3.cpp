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
  { "Tick Tock Hop", DR_MINIGAME_DUEL, 0x39, 0x39, DR_NO_QUIRKS },

  { "Game Guy's Lucky 7", DR_MINIGAME_GAME_GUY, 0x44, 0x43, DR_NO_QUIRKS },

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
