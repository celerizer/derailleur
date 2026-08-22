#include "MarioParty4Host.h"

#include <asm/mp4.h>

#include <QRetroDirectories.h>

/* dr_character -> native id, mirroring MP4_CHARACTER_IDS in guests/MarioParty4.cpp */
static const uint16_t MP4_CHARACTER_IDS[DR_CHARACTER_SIZE] = {
  0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x06,
};

/* Native roulette type byte -> dr_minigame_type */
static const dr_minigame_type MP4_MINIGAME_TYPE_TO_DR[] = {
  DR_MINIGAME_4P, /* 0 */
  DR_MINIGAME_1V3, /* 1 */
  DR_MINIGAME_2V2, /* 2 */
  DR_MINIGAME_SPECIAL, /* bowser */
  DR_MINIGAME_BATTLE, /* 4 */
  DR_MINIGAME_SPECIAL, /* chance time */
  DR_MINIGAME_SPECIAL, /* duel */
  DR_MINIGAME_SPECIAL, /* extra */
  DR_MINIGAME_SPECIAL, /* the final battle */
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
  config.values.bonus_result[0] = { 0x8018fc5e, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[1] = { 0x8018fc8e, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[2] = { 0x8018fcbe, DR_VALUE_TYPE_U16 };
  config.values.bonus_result[3] = { 0x8018fcee, DR_VALUE_TYPE_U16 };
  config.values.result[0] = { 0x8018fc60, DR_VALUE_TYPE_U16 };
  config.values.result[1] = { 0x8018fc90, DR_VALUE_TYPE_U16 };
  config.values.result[2] = { 0x8018fcc0, DR_VALUE_TYPE_U16 };
  config.values.result[3] = { 0x8018fcf0, DR_VALUE_TYPE_U16 };

  config.values.minigame_id = { 0x8018fd2c, DR_VALUE_TYPE_S16 };

  config.scene_miniexplain = 0x03;
  config.scene_miniresults = 0x54;

  config.character_ids = MP4_CHARACTER_IDS;
  config.minigame_type_to_dr = MP4_MINIGAME_TYPE_TO_DR;
  config.minigame_type_to_dr_size =
    sizeof(MP4_MINIGAME_TYPE_TO_DR) / sizeof(*MP4_MINIGAME_TYPE_TO_DR);

  config.host_state_addr = MP4_HOST_STATE;

  return config;
}

MarioParty4Host::MarioParty4Host(QObject *parent)
  : MarioPartyGcnHost(makeConfig(), parent)
{
}
