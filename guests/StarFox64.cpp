#include "StarFox64.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>

/* ---- Match start / end ---- */

/* u8 bool: in VS mode */
static const dr_value_t SF64_VERSUS_MODE = { 0x801778E8, DR_VALUE_TYPE_U8 };

/* u8 bool: set once the countdown reaches "GO" */
static const dr_value_t SF64_VS_MATCH_START = { 0x80177E7C, DR_VALUE_TYPE_U8 };

/* u8 bool: set the moment a winning condition is met */
static const dr_value_t SF64_VS_MATCH_WON = { 0x80177E74, DR_VALUE_TYPE_U8 };

/* u8 bool: set ~60 frames after SF64_VS_MATCH_WON, when results begin */
static const dr_value_t SF64_VS_MATCH_OVER = { 0x80178750, DR_VALUE_TYPE_U8 };

/* s32 VS match state (see sf64_vs_state) */
static const dr_value_t SF64_VS_MATCH_STATE = { 0x80178754, DR_VALUE_TYPE_S32 };

/* s32 winning player (0-3), or SF64_VS_WINNER_TIE */
static const dr_value_t SF64_VS_WINNER = { 0x801787A8, DR_VALUE_TYPE_S32 };
static const int SF64_VS_WINNER_TIE = 99;

/* s32 Time Trial clock: minutes, seconds (0-59), hundredths (0-99); time is up when all are 0 */
static const dr_value_t SF64_VS_COUNTDOWN[3] = {
  { 0x80178768, DR_VALUE_TYPE_S32 },
  { 0x8017876C, DR_VALUE_TYPE_S32 },
  { 0x80178770, DR_VALUE_TYPE_S32 }
};

/* The source names these only VS_STATE_<n>; names describe what each state does. */
typedef enum
{
  SF64_VS_STATE_INIT = 0,
  SF64_VS_STATE_SETUP = 1,         /* split screens set up, music started */
  SF64_VS_STATE_CHOOSE_VEHICLE = 2, /* only when Landmaster/on-foot are unlocked */
  SF64_VS_STATE_COUNTDOWN = 3,     /* 3-2-1-GO */
  SF64_VS_STATE_PLAYING = 4,
  SF64_VS_STATE_WINNER = 5,        /* winner decided, 60-frame pause */
  SF64_VS_STATE_RESULTS = 6,       /* 6-9 are results screens */
  SF64_VS_STATE_RESULTS_TALLY = 8, /* point/time tally; multi-player Point Match or Time Trial */
  SF64_VS_STATE_POST_MENU = 10,
  SF64_VS_STATE_PLAY_AGAIN = 11,
  SF64_VS_STATE_RETURN_TO_VS_MENU = 12,
  SF64_VS_STATE_EXIT_TO_TITLE = 13,
  SF64_VS_STATE_IDLE = 14,
  SF64_VS_STATE_UNUSED = 20
} sf64_vs_state;

/* ---- Game type / map ---- */

/* s32 match type (see sf64_vs_match_type) */
static const dr_value_t SF64_VS_MATCH_TYPE = { 0x801778AC, DR_VALUE_TYPE_S32 };

/* s32 stage (see sf64_vs_stage) */
static const dr_value_t SF64_VERSUS_STAGE = { 0x8017789C, DR_VALUE_TYPE_S32 };

/* s32 points to win, 1-5 (default 3) */
static const dr_value_t SF64_VS_POINTS_TO_WIN = { 0x801778A4, DR_VALUE_TYPE_S32 };

/* s32 Time Trial limit 0-4; the match lasts this + 1 minutes */
static const dr_value_t SF64_VS_TIME_TRIAL_LIMIT = { 0x801778C8, DR_VALUE_TYPE_S32 };

/* s32 camera count: 1 = single screen, 4 = split screen (VS gameplay) */
static const dr_value_t SF64_CAM_COUNT = { 0x801778A8, DR_VALUE_TYPE_S32 };

typedef enum
{
  SF64_VS_MATCH_POINT = 0,
  SF64_VS_MATCH_BATTLE_ROYAL = 1,
  SF64_VS_MATCH_TIME_TRIAL = 2
} sf64_vs_match_type;

typedef enum
{
  SF64_VS_STAGE_CORNERIA = 0,
  SF64_VS_STAGE_KATINA = 1,
  SF64_VS_STAGE_SECTOR_Z = 2
} sf64_vs_stage;

/* ---- Player data (player i is controller port i) ---- */

/* u8 bool per port: controller plugged in */
static const dr_value_t SF64_CONTROLLER_PLUGGED[4] = {
  { 0x800DD8B0, DR_VALUE_TYPE_U8 },
  { 0x800DD8B1, DR_VALUE_TYPE_U8 },
  { 0x800DD8B2, DR_VALUE_TYPE_U8 },
  { 0x800DD8B3, DR_VALUE_TYPE_U8 }
};

/* OSContPad per port (6-byte stride): u16 buttons, s8 stick x, s8 stick y */
static const dr_value_t SF64_CONTROLLER_BUTTONS[4] = {
  { 0x800DD898, DR_VALUE_TYPE_U16 },
  { 0x800DD89E, DR_VALUE_TYPE_U16 },
  { 0x800DD8A4, DR_VALUE_TYPE_U16 },
  { 0x800DD8AA, DR_VALUE_TYPE_U16 }
};
static const dr_value_t SF64_CONTROLLER_STICK_X[4] = {
  { 0x800DD89A, DR_VALUE_TYPE_S8 },
  { 0x800DD8A0, DR_VALUE_TYPE_S8 },
  { 0x800DD8A6, DR_VALUE_TYPE_S8 },
  { 0x800DD8AC, DR_VALUE_TYPE_S8 }
};
static const dr_value_t SF64_CONTROLLER_STICK_Y[4] = {
  { 0x800DD89B, DR_VALUE_TYPE_S8 },
  { 0x800DD8A1, DR_VALUE_TYPE_S8 },
  { 0x800DD8A7, DR_VALUE_TYPE_S8 },
  { 0x800DD8AD, DR_VALUE_TYPE_S8 }
};

/* s32 bool per player: port had no controller when the VS menu was opened */
static const dr_value_t SF64_PLAYER_INACTIVE[4] = {
  { 0x80161A18, DR_VALUE_TYPE_S32 },
  { 0x80161A1C, DR_VALUE_TYPE_S32 },
  { 0x80161A20, DR_VALUE_TYPE_S32 },
  { 0x80161A24, DR_VALUE_TYPE_S32 }
};

/* s32 per player: starting vehicle choice (see sf64_form_option). Written while
 * the match state is SF64_VS_STATE_SETUP or SF64_VS_STATE_CHOOSE_VEHICLE. */
static const dr_value_t SF64_PLAYER_FORM_OPTION[4] = {
  { 0x80178780, DR_VALUE_TYPE_S32 },
  { 0x80178784, DR_VALUE_TYPE_S32 },
  { 0x80178788, DR_VALUE_TYPE_S32 },
  { 0x8017878C, DR_VALUE_TYPE_S32 }
};

/* s32 bool: Landmaster selectable */
static const dr_value_t SF64_UNLOCK_LANDMASTER = { 0x8017875C, DR_VALUE_TYPE_S32 };

/* s32 bool: on-foot selectable (only counts if Landmaster is also unlocked) */
static const dr_value_t SF64_UNLOCK_ON_FOOT = { 0x80178760, DR_VALUE_TYPE_S32 };

/* s32 per player: counts down from 150; the game picks a vehicle at random at 0 */
static const dr_value_t SF64_PLAYER_RESPAWN_TIMER[4] = {
  { 0x801787F8, DR_VALUE_TYPE_S32 },
  { 0x801787FC, DR_VALUE_TYPE_S32 },
  { 0x80178800, DR_VALUE_TYPE_S32 },
  { 0x80178804, DR_VALUE_TYPE_S32 }
};

typedef enum
{
  SF64_FORM_OPTION_NONE = 0, /* not chosen yet */
  SF64_FORM_OPTION_ARWING = 1,
  SF64_FORM_OPTION_LANDMASTER = 2,
  SF64_FORM_OPTION_ON_FOOT = 3
} sf64_form_option;

/* s32 per player: vehicle (see sf64_player_form) */
static const dr_value_t SF64_PLAYER_FORM[4] = {
  { 0x80177870, DR_VALUE_TYPE_S32 },
  { 0x80177874, DR_VALUE_TYPE_S32 },
  { 0x80177878, DR_VALUE_TYPE_S32 },
  { 0x8017787C, DR_VALUE_TYPE_S32 }
};

/* s32 per player: handicap (see sf64_handicap) */
static const dr_value_t SF64_HANDICAP[4] = {
  { 0x80177888, DR_VALUE_TYPE_S32 },
  { 0x8017788C, DR_VALUE_TYPE_S32 },
  { 0x80177890, DR_VALUE_TYPE_S32 },
  { 0x80177894, DR_VALUE_TYPE_S32 }
};

/* s32 per player: laser strength (see sf64_laser_strength) */
static const dr_value_t SF64_LASER_STRENGTH[4] = {
  { 0x80161AA8, DR_VALUE_TYPE_S32 },
  { 0x80161AAC, DR_VALUE_TYPE_S32 },
  { 0x80161AB0, DR_VALUE_TYPE_S32 },
  { 0x80161AB4, DR_VALUE_TYPE_S32 }
};

/* s32 per player: score or kill count; flickers +/-1 during the HUD point animation */
static const dr_value_t SF64_VS_POINTS[4] = {
  { 0x80177DB8, DR_VALUE_TYPE_S32 },
  { 0x80177DBC, DR_VALUE_TYPE_S32 },
  { 0x80177DC0, DR_VALUE_TYPE_S32 },
  { 0x80177DC4, DR_VALUE_TYPE_S32 }
};

/* s32[10] per killer: player numbers (0-3) that killer shot down, in order. Each entry
 * is the first element; index n is at + n * 4. */
static const dr_value_t SF64_VS_KILLS[4] = {
  { 0x80177DD0, DR_VALUE_TYPE_S32 },
  { 0x80177DF8, DR_VALUE_TYPE_S32 },
  { 0x80177E20, DR_VALUE_TYPE_S32 },
  { 0x80177E48, DR_VALUE_TYPE_S32 }
};
static const unsigned SF64_VS_KILLS_COUNT = 10;

/* Player* to Player[gCamCount] */
static const dr_value_t SF64_PLAYER = { 0x80178280, DR_VALUE_TYPE_POINTER };

/* Player struct size and field offsets */
static const size_t SF64_PLAYER_SIZE = 0x4E0;
static const size_t SF64_PLAYER_POS = 0x074;     /* Vec3f x, y, z */
static const size_t SF64_PLAYER_NUM = 0x1C4;     /* s32 player number 0-3 */
static const size_t SF64_PLAYER_STATE = 0x1C8;   /* s32 sf64_player_state */
static const size_t SF64_PLAYER_FORM_OFS = 0x1CC; /* s32 sf64_player_form */
static const size_t SF64_PLAYER_SHIELDS = 0x264; /* s32 0-255 */
static const size_t SF64_PLAYER_DAMAGE = 0x268;  /* s32 damage queued to subtract from shields */

typedef enum
{
  SF64_PLAYER_FORM_ARWING = 0,
  SF64_PLAYER_FORM_LANDMASTER = 1,
  SF64_PLAYER_FORM_BLUE_MARINE = 2, /* unused in VS */
  SF64_PLAYER_FORM_ON_FOOT = 3,
  SF64_PLAYER_FORM_NONE = 255       /* unused in VS */
} sf64_player_form;

typedef enum
{
  SF64_HANDICAP_NONE = 0, /* 255 shields */
  SF64_HANDICAP_LOW = 1,  /* 191 shields */
  SF64_HANDICAP_HIGH = 2  /* 127 shields */
} sf64_handicap;

typedef enum
{
  SF64_LASER_SINGLE = 0,
  SF64_LASER_TWIN = 1,
  SF64_LASER_HYPER = 2,
  SF64_LASER_UNKNOWN = 3
} sf64_laser_strength;

typedef enum
{
  SF64_PLAYER_STATE_STANDBY = 0,
  SF64_PLAYER_STATE_INIT = 1,
  SF64_PLAYER_STATE_LEVEL_INTRO = 2,
  SF64_PLAYER_STATE_ACTIVE = 3,
  SF64_PLAYER_STATE_DOWN = 4, /* shot down */
  SF64_PLAYER_STATE_U_TURN = 5,
  SF64_PLAYER_STATE_NEXT = 6,
  SF64_PLAYER_STATE_LEVEL_COMPLETE = 7,
  SF64_PLAYER_STATE_ENTER_WARP_ZONE = 8,
  SF64_PLAYER_STATE_START_360 = 9,
  SF64_PLAYER_STATE_GREAT_FOX_REPAIR = 10,
  SF64_PLAYER_STATE_ANDROSS_MOUTH = 11,
  SF64_PLAYER_STATE_UNKNOWN = 12,
  SF64_PLAYER_STATE_VS_STANDBY = 13
} sf64_player_state;

/* ---- Laser / effect colors ---- */

/* Effect colors baked into code as a lui/ori pair: the u16 immediates (low half of
 * each instruction) are lui = R G, ori = B A. Addresses point at the immediates. */
typedef struct
{
  dr_value_t rg;
  dr_value_t ba;
} sf64_lui_ori_color_t;

/* Arwing laser */
static const sf64_lui_ori_color_t SF64_LASER_COLOR[4] = {
  { { 0x80039892, DR_VALUE_TYPE_U16 }, { 0x80039896, DR_VALUE_TYPE_U16 } },
  { { 0x800398BE, DR_VALUE_TYPE_U16 }, { 0x800398C2, DR_VALUE_TYPE_U16 } },
  { { 0x800398EA, DR_VALUE_TYPE_U16 }, { 0x800398EE, DR_VALUE_TYPE_U16 } },
  { { 0x80039916, DR_VALUE_TYPE_U16 }, { 0x8003991A, DR_VALUE_TYPE_U16 } }
};

/* Arwing charge glow (alpha 128) */
static const sf64_lui_ori_color_t SF64_CHARGE_ARWING_COLOR[4] = {
  { { 0x80055136, DR_VALUE_TYPE_U16 }, { 0x8005513A, DR_VALUE_TYPE_U16 } },
  { { 0x8005515A, DR_VALUE_TYPE_U16 }, { 0x8005515E, DR_VALUE_TYPE_U16 } },
  { { 0x8005517E, DR_VALUE_TYPE_U16 }, { 0x80055182, DR_VALUE_TYPE_U16 } },
  { { 0x800551A2, DR_VALUE_TYPE_U16 }, { 0x800551A6, DR_VALUE_TYPE_U16 } }
};

/* Landmaster charge glow (alpha 128) */
static const sf64_lui_ori_color_t SF64_CHARGE_LANDMASTER_COLOR[4] = {
  { { 0x8005590E, DR_VALUE_TYPE_U16 }, { 0x80055912, DR_VALUE_TYPE_U16 } },
  { { 0x80055932, DR_VALUE_TYPE_U16 }, { 0x80055936, DR_VALUE_TYPE_U16 } },
  { { 0x80055956, DR_VALUE_TYPE_U16 }, { 0x8005595A, DR_VALUE_TYPE_U16 } },
  { { 0x8005597A, DR_VALUE_TYPE_U16 }, { 0x8005597E, DR_VALUE_TYPE_U16 } }
};

/* Fired charge shot orb (alpha 255) */
static const sf64_lui_ori_color_t SF64_CHARGE_SHOT_COLOR[4] = {
  { { 0x8003AB2E, DR_VALUE_TYPE_U16 }, { 0x8003AB32, DR_VALUE_TYPE_U16 } },
  { { 0x8003AB52, DR_VALUE_TYPE_U16 }, { 0x8003AB56, DR_VALUE_TYPE_U16 } },
  { { 0x8003AB76, DR_VALUE_TYPE_U16 }, { 0x8003AB7A, DR_VALUE_TYPE_U16 } },
  { { 0x8003AB9A, DR_VALUE_TYPE_U16 }, { 0x8003AB9E, DR_VALUE_TYPE_U16 } }
};

/* Charge shot / bomb impact burst (alpha filled in at runtime, low byte 00) */
static const sf64_lui_ori_color_t SF64_IMPACT_BURST_COLOR[4] = {
  { { 0x80077F42, DR_VALUE_TYPE_U16 }, { 0x80077F56, DR_VALUE_TYPE_U16 } },
  { { 0x80077F76, DR_VALUE_TYPE_U16 }, { 0x80077F8A, DR_VALUE_TYPE_U16 } },
  { { 0x80077FAA, DR_VALUE_TYPE_U16 }, { 0x80077FBE, DR_VALUE_TYPE_U16 } },
  { { 0x80077FDE, DR_VALUE_TYPE_U16 }, { 0x80077FF2, DR_VALUE_TYPE_U16 } }
};

/* Bomb in flight (alpha 128) */
static const sf64_lui_ori_color_t SF64_BOMB_FLIGHT_COLOR[4] = {
  { { 0x80039FF6, DR_VALUE_TYPE_U16 }, { 0x80039FFA, DR_VALUE_TYPE_U16 } },
  { { 0x8003A01A, DR_VALUE_TYPE_U16 }, { 0x8003A01E, DR_VALUE_TYPE_U16 } },
  { { 0x8003A03E, DR_VALUE_TYPE_U16 }, { 0x8003A042, DR_VALUE_TYPE_U16 } },
  { { 0x8003A062, DR_VALUE_TYPE_U16 }, { 0x8003A066, DR_VALUE_TYPE_U16 } }
};

/* Bomb explosion sphere (alpha filled in at runtime, low byte 00) */
static const sf64_lui_ori_color_t SF64_BOMB_EXPLOSION_COLOR[4] = {
  { { 0x8003A16E, DR_VALUE_TYPE_U16 }, { 0x8003A182, DR_VALUE_TYPE_U16 } },
  { { 0x8003A1A2, DR_VALUE_TYPE_U16 }, { 0x8003A1B6, DR_VALUE_TYPE_U16 } },
  { { 0x8003A1D6, DR_VALUE_TYPE_U16 }, { 0x8003A1EA, DR_VALUE_TYPE_U16 } },
  { { 0x8003A20A, DR_VALUE_TYPE_U16 }, { 0x8003A21E, DR_VALUE_TYPE_U16 } }
};

/* Every lui/ori-colored effect, recolored per player */
static const sf64_lui_ori_color_t *const SF64_LUI_ORI_COLORS[] = {
  SF64_LASER_COLOR,
  SF64_CHARGE_ARWING_COLOR,
  SF64_CHARGE_LANDMASTER_COLOR,
  SF64_CHARGE_SHOT_COLOR,
  SF64_IMPACT_BURST_COLOR,
  SF64_BOMB_FLIGHT_COLOR,
  SF64_BOMB_EXPLOSION_COLOR
};

/* ---- Engine glow ---- */

/* Display_PlayerEngineGlow_Draw passes gLevelType (planet red / space blue) to
 * Display_DrawEngineGlow, so every player shares one color. These two patches
 * reload the Player* and pass player->num instead, picking a per-player entry. */
static const dr_value_t SF64_ENGINE_GLOW_LOAD_PLAYER = { 0x800548EC, DR_VALUE_TYPE_U32 };
static const uint32_t SF64_ENGINE_GLOW_LOAD_PLAYER_OP = 0x8FA40028; /* lw a0, 0x28(sp) */
static const dr_value_t SF64_ENGINE_GLOW_LOAD_NUM = { 0x800548F4, DR_VALUE_TYPE_U32 };
static const uint32_t SF64_ENGINE_GLOW_LOAD_NUM_OP = 0x8C8401C4; /* lw a0, 0x1C4(a0) */

/* P2's original color is a single li, so its patched lui/ori takes the slot that
 * loaded the SetEnvColor command into t3; store from t0 (same command) instead. */
static const dr_value_t SF64_ENGINE_GLOW_P2_STORE = { 0x80054710, DR_VALUE_TYPE_U32 };
static const uint32_t SF64_ENGINE_GLOW_P2_STORE_OP = 0xAC480000; /* sw t0, 0(v0) */

/* Per-player glow color as whole lui/ori instructions; the low halves (RRGG, BBAA)
 * are filled in from the player's color, with alpha FF */
typedef struct
{
  dr_value_t lui;
  uint32_t lui_op;
  dr_value_t ori;
  uint32_t ori_op;
} sf64_engine_glow_t;

static const sf64_engine_glow_t SF64_ENGINE_GLOW_COLOR[4] = {
  { { 0x800546D8, DR_VALUE_TYPE_U32 }, 0x3C090000, { 0x800546DC, DR_VALUE_TYPE_U32 }, 0x35290000 }, /* t1 */
  { { 0x800546FC, DR_VALUE_TYPE_U32 }, 0x3C0C0000, { 0x80054700, DR_VALUE_TYPE_U32 }, 0x358C0000 }, /* t4 */
  { { 0x80054720, DR_VALUE_TYPE_U32 }, 0x3C0F0000, { 0x80054724, DR_VALUE_TYPE_U32 }, 0x35EF0000 }, /* t7 */
  { { 0x80054744, DR_VALUE_TYPE_U32 }, 0x3C080000, { 0x80054748, DR_VALUE_TYPE_U32 }, 0x35080000 }  /* t0 */
};

/* u8 per player: Landmaster/on-foot shot color */
static const dr_value_t SF64_SHOT_COLOR_R[4] = {
  { 0x800C9C00, DR_VALUE_TYPE_U8 },
  { 0x800C9C01, DR_VALUE_TYPE_U8 },
  { 0x800C9C02, DR_VALUE_TYPE_U8 },
  { 0x800C9C03, DR_VALUE_TYPE_U8 }
};
static const dr_value_t SF64_SHOT_COLOR_G[4] = {
  { 0x800C9C04, DR_VALUE_TYPE_U8 },
  { 0x800C9C05, DR_VALUE_TYPE_U8 },
  { 0x800C9C06, DR_VALUE_TYPE_U8 },
  { 0x800C9C07, DR_VALUE_TYPE_U8 }
};
static const dr_value_t SF64_SHOT_COLOR_B[4] = {
  { 0x800C9C08, DR_VALUE_TYPE_U8 },
  { 0x800C9C09, DR_VALUE_TYPE_U8 },
  { 0x800C9C0A, DR_VALUE_TYPE_U8 },
  { 0x800C9C0B, DR_VALUE_TYPE_U8 }
};

/* u8 per player: bomb light color */
static const dr_value_t SF64_BOMB_COLOR_R[4] = {
  { 0x800C9C18, DR_VALUE_TYPE_U8 },
  { 0x800C9C19, DR_VALUE_TYPE_U8 },
  { 0x800C9C1A, DR_VALUE_TYPE_U8 },
  { 0x800C9C1B, DR_VALUE_TYPE_U8 }
};
static const dr_value_t SF64_BOMB_COLOR_G[4] = {
  { 0x800C9C1C, DR_VALUE_TYPE_U8 },
  { 0x800C9C1D, DR_VALUE_TYPE_U8 },
  { 0x800C9C1E, DR_VALUE_TYPE_U8 },
  { 0x800C9C1F, DR_VALUE_TYPE_U8 }
};
static const dr_value_t SF64_BOMB_COLOR_B[4] = {
  { 0x800C9C20, DR_VALUE_TYPE_U8 },
  { 0x800C9C21, DR_VALUE_TYPE_U8 },
  { 0x800C9C22, DR_VALUE_TYPE_U8 },
  { 0x800C9C23, DR_VALUE_TYPE_U8 }
};

/* ---- HUD colors ---- */

/* s32 RGB per player: minimap (radar) marker. The table is s32[8][4] RGBA, paired
 * as normal + dim blink entries, and players don't map to it in order. */
static const dr_value_t SF64_RADAR_COLOR[4][3] = {
  { { 0x800D1E14, DR_VALUE_TYPE_S32 }, { 0x800D1E18, DR_VALUE_TYPE_S32 }, { 0x800D1E1C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E74, DR_VALUE_TYPE_S32 }, { 0x800D1E78, DR_VALUE_TYPE_S32 }, { 0x800D1E7C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E54, DR_VALUE_TYPE_S32 }, { 0x800D1E58, DR_VALUE_TYPE_S32 }, { 0x800D1E5C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E34, DR_VALUE_TYPE_S32 }, { 0x800D1E38, DR_VALUE_TYPE_S32 }, { 0x800D1E3C, DR_VALUE_TYPE_S32 } }
};

/* s32 RGB per player: dim minimap marker your own marker blinks to (alpha 128) */
static const dr_value_t SF64_RADAR_BLINK_COLOR[4][3] = {
  { { 0x800D1E24, DR_VALUE_TYPE_S32 }, { 0x800D1E28, DR_VALUE_TYPE_S32 }, { 0x800D1E2C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E84, DR_VALUE_TYPE_S32 }, { 0x800D1E88, DR_VALUE_TYPE_S32 }, { 0x800D1E8C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E64, DR_VALUE_TYPE_S32 }, { 0x800D1E68, DR_VALUE_TYPE_S32 }, { 0x800D1E6C, DR_VALUE_TYPE_S32 } },
  { { 0x800D1E44, DR_VALUE_TYPE_S32 }, { 0x800D1E48, DR_VALUE_TYPE_S32 }, { 0x800D1E4C, DR_VALUE_TYPE_S32 } }
};

/* s32 RGB per player: player number in the center of the screen */
static const dr_value_t SF64_PLAYER_NUMBER_COLOR[4][3] = {
  { { 0x800D2150, DR_VALUE_TYPE_S32 }, { 0x800D2160, DR_VALUE_TYPE_S32 }, { 0x800D2170, DR_VALUE_TYPE_S32 } },
  { { 0x800D2154, DR_VALUE_TYPE_S32 }, { 0x800D2164, DR_VALUE_TYPE_S32 }, { 0x800D2174, DR_VALUE_TYPE_S32 } },
  { { 0x800D2158, DR_VALUE_TYPE_S32 }, { 0x800D2168, DR_VALUE_TYPE_S32 }, { 0x800D2178, DR_VALUE_TYPE_S32 } },
  { { 0x800D215C, DR_VALUE_TYPE_S32 }, { 0x800D216C, DR_VALUE_TYPE_S32 }, { 0x800D217C, DR_VALUE_TYPE_S32 } }
};

/* s32 RGB per player: results screen kill icons */
static const dr_value_t SF64_KILL_ICON_COLOR[4][3] = {
  { { 0x800D4CD8, DR_VALUE_TYPE_S32 }, { 0x800D4CE8, DR_VALUE_TYPE_S32 }, { 0x800D4CF8, DR_VALUE_TYPE_S32 } },
  { { 0x800D4CDC, DR_VALUE_TYPE_S32 }, { 0x800D4CEC, DR_VALUE_TYPE_S32 }, { 0x800D4CFC, DR_VALUE_TYPE_S32 } },
  { { 0x800D4CE0, DR_VALUE_TYPE_S32 }, { 0x800D4CF0, DR_VALUE_TYPE_S32 }, { 0x800D4D00, DR_VALUE_TYPE_S32 } },
  { { 0x800D4CE4, DR_VALUE_TYPE_S32 }, { 0x800D4CF4, DR_VALUE_TYPE_S32 }, { 0x800D4D04, DR_VALUE_TYPE_S32 } }
};

static const char *SF64_HIRES_DIR =
  "system/Mupen64plus/hires_texture/STARFOX64/GLideNHQ";

/* Per-player 44x44 icon: a top and a bottom 44x20 half, then a 44x4 strip */
static const struct
{
  int y;      /* native y offset within the 44x44 icon */
  int height; /* native height */
} SF64_PLAYER_ICON_PARTS[3] = { { 0, 20 }, { 20, 20 }, { 40, 4 } };

static const char *SF64_PLAYER_ICON_FILES[4][3] = {
  { "STARFOX64#3D2BB480#0#2_all.png", "STARFOX64#C66369FF#0#2_all.png", "STARFOX64#4A886CC7#0#2_all.png" },
  { "STARFOX64#47D8A788#0#2_all.png", "STARFOX64#2D714D9E#0#2_all.png", "STARFOX64#C917D4E0#0#2_all.png" },
  { "STARFOX64#8FAE74EB#0#2_all.png", "STARFOX64#6E178103#0#2_all.png", "STARFOX64#4D5A6F17#0#2_all.png" },
  { "STARFOX64#8B5291E1#0#2_all.png", "STARFOX64#1E17CB0A#0#2_all.png", "STARFOX64#3E02E479#0#2_all.png" }
};

/* Icons are written at this integer multiple of the native size (GLideN64 needs
 * an exact integer upscale). */
static const int SF64_HIRES_SCALE = 2;

static const dr_mp_minigame_t SF64_MINIGAMES[] = {
  /* minigame_id is the match type, scene_id the vehicle; the stage is random */
  { "SF64: Point Match", DR_MINIGAME_4P, SF64_VS_MATCH_POINT, SF64_PLAYER_FORM_ARWING, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { "SF64: Battle Royal", DR_MINIGAME_4P, SF64_VS_MATCH_BATTLE_ROYAL, SF64_PLAYER_FORM_ARWING, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { "SF64: Time Trial", DR_MINIGAME_4P, SF64_VS_MATCH_TIME_TRIAL, SF64_PLAYER_FORM_ARWING, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { "SF64: Landmaster Point Match", DR_MINIGAME_4P, SF64_VS_MATCH_POINT, SF64_PLAYER_FORM_LANDMASTER, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { "SF64: Landmaster Battle Royal", DR_MINIGAME_4P, SF64_VS_MATCH_BATTLE_ROYAL, SF64_PLAYER_FORM_LANDMASTER, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { "SF64: Landmaster Time Trial", DR_MINIGAME_4P, SF64_VS_MATCH_TIME_TRIAL, SF64_PLAYER_FORM_LANDMASTER, DR_NO_QUIRKS, DR_FLAG_NO_BOTS },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

StarFox64::StarFox64(QObject *parent)
  : DrGuest(parent)
{
  m_retro = new DrRetroN64(this);
  m_retro->init(coreId(), rom());
}

const dr_mp_minigame_t *StarFox64::minigames() const
{
  return SF64_MINIGAMES;
}

/* Pre-boot (every launch): lay down the hires icon textures before the core
 * (re)scans them. Player i is controller port i, so icons go by in-game slot. */
void StarFox64::onBeforeBoot(const DrGameData &data)
{
  for (unsigned i = 0; i < 4; i++)
    writePlayerIcon(dr_player_slot(m_players[i], i), data.players[i].character);
}

void StarFox64::doApplyGameData(const DrGameData &data)
{
  (void)data; /* players cached by the base; m_minigame set by the base */
  m_minigameFrames = 0;
  m_winnerIndex = -1;
  m_formOption = SF64_FORM_OPTION_NONE;
  for (unsigned i = 0; i < 4; i++)
    m_slotToIndex[i] = -1;

  loadState(state());

  /* Engine glow: pick the color by player number rather than level type */
  m_retro->writeValueForFrames(SF64_ENGINE_GLOW_LOAD_PLAYER_OP, SF64_ENGINE_GLOW_LOAD_PLAYER, 30);
  m_retro->writeValueForFrames(SF64_ENGINE_GLOW_LOAD_NUM_OP, SF64_ENGINE_GLOW_LOAD_NUM, 30);
  m_retro->writeValueForFrames(SF64_ENGINE_GLOW_P2_STORE_OP, SF64_ENGINE_GLOW_P2_STORE, 30);

  /* Recolor each player's lasers, engine glow, shots, bombs and HUD to match their
   * character. */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = dr_player_slot(m_players[i], i);
    const dr_color_t color = dr_character_color(m_players[i].character);

    m_slotToIndex[slot] = static_cast<int>(i);

    /* Lasers, charge glows/shots, bombs and bursts: lui = R G, ori = B A (keep
     * the game's alpha byte, which is 00 where it's filled in at runtime) */
    for (const sf64_lui_ori_color_t *effect : SF64_LUI_ORI_COLORS)
    {
      int64_t ba = 0;
      m_retro->readValue(&ba, effect[slot].ba);
      m_retro->writeValue((color.red << 8) | color.green, effect[slot].rg);
      m_retro->writeValue((color.blue << 8) | (ba & 0xFF), effect[slot].ba);
    }

    /* Engine glow color for this player number */
    const sf64_engine_glow_t &glow = SF64_ENGINE_GLOW_COLOR[slot];
    m_retro->writeValueForFrames(glow.lui_op | (color.red << 8) | color.green, glow.lui, 30);
    m_retro->writeValueForFrames(glow.ori_op | (color.blue << 8) | 0xFF, glow.ori, 30);

    m_retro->writeValue(color.red, SF64_SHOT_COLOR_R[slot]);
    m_retro->writeValue(color.green, SF64_SHOT_COLOR_G[slot]);
    m_retro->writeValue(color.blue, SF64_SHOT_COLOR_B[slot]);

    m_retro->writeValue(color.red, SF64_BOMB_COLOR_R[slot]);
    m_retro->writeValue(color.green, SF64_BOMB_COLOR_G[slot]);
    m_retro->writeValue(color.blue, SF64_BOMB_COLOR_B[slot]);

    /* HUD: radar marker (blink entry at half brightness, like the game's), player
     * number and kill icons */
    const unsigned rgb[3] = { color.red, color.green, color.blue };
    for (unsigned c = 0; c < 3; c++)
    {
      m_retro->writeValue(rgb[c], SF64_RADAR_COLOR[slot][c]);
      m_retro->writeValue(rgb[c] / 2, SF64_RADAR_BLINK_COLOR[slot][c]);
      m_retro->writeValue(rgb[c], SF64_PLAYER_NUMBER_COLOR[slot][c]);
      m_retro->writeValue(rgb[c], SF64_KILL_ICON_COLOR[slot][c]);
    }
  }

  /* Match type and everyone's vehicle come from the mini-game; the stage is a
   * random one that vehicle can play on (no Landmaster in Sector Z's space). */
  if (m_minigame)
  {
    const int form = m_minigame->scene_id;
    const int stages = form == SF64_PLAYER_FORM_LANDMASTER ? 2 : 3;
    const int stage = dr_rand() % stages;

    m_retro->writeValue(m_minigame->minigame_id, SF64_VS_MATCH_TYPE);
    m_retro->writeValue(stage, SF64_VERSUS_STAGE);
    for (unsigned i = 0; i < 4; i++)
      m_retro->writeValueForFrames(form, SF64_PLAYER_FORM[i], 240);

    /* The vehicle choice is also picked in run() during match setup */
    switch (form)
    {
    case SF64_PLAYER_FORM_LANDMASTER:
      m_formOption = SF64_FORM_OPTION_LANDMASTER;
      break;
    case SF64_PLAYER_FORM_ON_FOOT:
      m_formOption = SF64_FORM_OPTION_ON_FOOT;
      break;
    default:
      m_formOption = SF64_FORM_OPTION_ARWING;
      break;
    }
  }

  startMinigame();
}

void StarFox64::run()
{
  m_retro->tickFrameWrites();

  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* Starting vehicle: during setup / vehicle choice, unlock the vehicles and pick
   * the mini-game's for everyone */
  int64_t state = 0;
  if (m_retro->readValue(&state, SF64_VS_MATCH_STATE) == DR_OK &&
      (state == SF64_VS_STATE_SETUP || state == SF64_VS_STATE_CHOOSE_VEHICLE) &&
      m_formOption != SF64_FORM_OPTION_NONE)
  {
    m_retro->writeValue(1, SF64_UNLOCK_LANDMASTER);
    m_retro->writeValue(m_formOption == SF64_FORM_OPTION_ON_FOOT, SF64_UNLOCK_ON_FOOT);
    for (unsigned i = 0; i < 4; i++)
      m_retro->writeValue(m_formOption, SF64_PLAYER_FORM_OPTION[i]);
  }

  /* End as soon as the winner is decided (start of the 60-frame pause). Guard a
   * few frames so a stale state from the loaded savestate can't finish us instantly. */
  if (m_minigameFrames < 30 ||
      m_retro->readValue(&state, SF64_VS_MATCH_STATE) != DR_OK ||
      state < SF64_VS_STATE_WINNER + 1)
    return;

  /* Winner is the in-game player slot (0-3), or SF64_VS_WINNER_TIE */
  int64_t winner = SF64_VS_WINNER_TIE;
  m_retro->readValue(&winner, SF64_VS_WINNER);
  m_winnerIndex = winner >= 0 && winner < 4 ? m_slotToIndex[winner] : -1;
  log(DR_LOG_INFO, qPrintable(QString("match over: winner slot %1 -> player %2")
    .arg(winner).arg(m_winnerIndex)));

  finishMinigame();
}

dr_minigame_result_t StarFox64::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };

  if (static_cast<int>(index) == m_winnerIndex)
    result.coins = 10;

  return result;
}

void StarFox64::writePlayerIcon(unsigned slot, dr_character character)
{
  if (slot >= 4)
    return;

  const QString destDir = QString::fromUtf8(SF64_HIRES_DIR);
  QDir().mkpath(destDir);

  /* N64 hosts get the 28x28 N64 player panels (black-barred to fill the icon);
   * GameCube/Wii hosts, or a character without a panel, get the GCN heads. */
  const QString panelPath = QString(":/assets/player-panel/n64/%1.png").arg(static_cast<int>(character));
  const bool usePanel = m_hostPlatform == DR_HOST_PLATFORM_N64 && QFile::exists(panelPath);
  QImage art(usePanel ? panelPath : dr_player_icon_32px(m_hostPlatform, character));
  if (art.isNull())
  {
    log(DR_LOG_WARN, qPrintable(QString("no player icon for character %1")
      .arg(static_cast<int>(character))));
    return;
  }

  /* Frame the 44x44 icon with a 1px (native) border in the player's color, fit
   * the art inside it (aspect preserved, centered), then chop it into its parts.
   * The game's own scaling crops ~half a native px off the bottom and a full
   * native px off the right, so pad those edges to keep the border visible. */
  const int S = SF64_HIRES_SCALE;
  const int w = 44 * S;
  const int h = 44 * S;
  const int top = S;
  const int left = S;
  const int bottom = S + S / 2;
  const int right = 2 * S;
  const int innerW = w - left - right;
  const int innerH = h - top - bottom;
  const QImage scaled = art.scaled(innerW, innerH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  const dr_color_t color = dr_character_color(character);
  const QColor border(color.red, color.green, color.blue);
  QImage icon(w, h, QImage::Format_ARGB32);
  icon.fill(border);
  {
    QPainter p(&icon);
    /* Source mode so a transparent fill actually clears the border color */
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(left, top, innerW, innerH, usePanel ? Qt::black : Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawImage(left + (innerW - scaled.width()) / 2, top + (innerH - scaled.height()) / 2, scaled);
  }

  for (unsigned part = 0; part < 3; part++)
  {
    const char *file = SF64_PLAYER_ICON_FILES[slot][part];
    if (!file)
      continue;
    const QString dest = destDir + "/" + QString::fromUtf8(file);
    const QImage img = icon.copy(0, SF64_PLAYER_ICON_PARTS[part].y * S, w,
      SF64_PLAYER_ICON_PARTS[part].height * S);
    if (!img.save(dest, "PNG"))
      log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(dest)));
  }
}
