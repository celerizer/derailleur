#ifndef PARTYSTUFFER_CHARACTER_MAP_H
#define PARTYSTUFFER_CHARACTER_MAP_H

/* Editable character dereference map, US revision 0, MP1 / MP2 / MP3.
 * This is data for a future inject-character command, not an injector.
 * Keep names stable: they are proposed character-package asset keys.
 * Mainfs IDs are signed low16 indices, NOT full file handles.
 * Resolve directory through ps_character_slots, then compose
 * ((unsigned long)directory << 16) | (unsigned long)file_index,
 * only after checking both values >= 0.
 * -1 means NO KNOWN MAPPING (absent OR unresolved), never a writable ID.
 * inferred_mask: bit0 MP1, bit1 MP2, bit2 MP3. These pairings require review.
 * unknown_* keys preserve coverage without inventing an action/asset identity.
 * Do not infer equivalence between separate unknown rows by proximity.
 * IDs identify destinations; they do NOT convert models/rigs/MTNX or audio.
 * MP2 costumes and extra textures are retained as unresolved entries.
 * Daisy mainfs omitted; her MP3 fallback mapping is NOT ordinary.
 * Daisy sound IDs are included explicitly in the separate sound table.
 * Portraits and other assets outside character directories are not covered.
 *
 * Sources: decomp/mpmod/anims.md (current hand-maintained animation names,
 * all 158/209/160 animation indices, '?' carried into inferred_mask);
 * MP3 include/enums.h and src/14EA0.c character resolver;
 * vanilla character directory signatures; Waluigi hi/mid/low byte matches
 * MP3 00A0/00A1/00A2; MP1 converted-model README gives 009E/009F.
 * MP2 D1/D2 equal MP3 A0/A1 for Luigi; ovl_56 36CAE0 data lists
 * D3/D2/D1 as the model detail triplet, confirming D3 as low.
 * Sound provenance is given beside the sound table below.
 */

enum { PS_GAME_MP1 = 0, PS_GAME_MP2 = 1, PS_GAME_MP3 = 2, PS_GAME_COUNT = 3 };
enum { PS_ASSET_ANIMATION, PS_ASSET_MODEL, PS_ASSET_TEXTURE };

typedef struct PsCharacterSlot {
    const char *name;
    signed character_id;       /* e.g. inject-character ... 1 ... = Luigi */
    signed directory[PS_GAME_COUNT];
} PsCharacterSlot;

static const PsCharacterSlot ps_character_slots[] = {
    { "mario",   0x0000, { 0x0001, 0x0002, 0x0002 } },
    { "luigi",   0x0001, { 0x0002, 0x0003, 0x0003 } },
    { "peach",   0x0002, { 0x0006, 0x0007, 0x0007 } },
    { "yoshi",   0x0003, { 0x0003, 0x0004, 0x0004 } },
    { "wario",   0x0004, { 0x0004, 0x0005, 0x0005 } },
    { "dk",      0x0005, { 0x0005, 0x0006, 0x0006 } },
    { "waluigi", 0x0006, { -1, -1, 0x0008 } }
};

typedef struct PsCharacterFile {
    const char *name;
    signed kind;
    signed mp1;
    signed mp2;
    signed mp3;
    unsigned inferred_mask;
} PsCharacterFile;

/* name, kind, MP1 low16, MP2 low16, MP3 low16, inferred mask */
static const PsCharacterFile ps_character_files[] = {
    { "animation.anticipate", PS_ASSET_ANIMATION, -1, -1, 0x0072, 0x00 },
    { "animation.bag_grab", PS_ASSET_ANIMATION, 0x0078, 0x0038, -1, 0x00 },
    { "animation.ball_idle", PS_ASSET_ANIMATION, -1, 0x005B, -1, 0x00 },
    { "animation.ball_run", PS_ASSET_ANIMATION, -1, 0x005D, -1, 0x00 },
    { "animation.ball_walk", PS_ASSET_ANIMATION, -1, 0x005C, -1, 0x00 },
    { "animation.band_conduct", PS_ASSET_ANIMATION, 0x009D, 0x00C6, -1, 0x00 },
    { "animation.band_drum_hit", PS_ASSET_ANIMATION, -1, 0x00A8, -1, 0x00 },
    { "animation.band_drum_idle", PS_ASSET_ANIMATION, -1, 0x00A6, -1, 0x00 },
    { "animation.band_drum_play", PS_ASSET_ANIMATION, -1, 0x00A7, -1, 0x00 },
    { "animation.band_flute_hit", PS_ASSET_ANIMATION, -1, 0x00AB, -1, 0x00 },
    { "animation.band_flute_idle", PS_ASSET_ANIMATION, -1, 0x00A9, -1, 0x00 },
    { "animation.band_flute_play", PS_ASSET_ANIMATION, -1, 0x00AA, -1, 0x00 },
    { "animation.basketball_run", PS_ASSET_ANIMATION, 0x0073, -1, -1, 0x00 },
    { "animation.basketball_walk", PS_ASSET_ANIMATION, 0x0072, -1, -1, 0x00 },
    { "animation.bat_hold", PS_ASSET_ANIMATION, -1, -1, 0x0018, 0x00 },
    { "animation.bat_swing", PS_ASSET_ANIMATION, 0x0046, 0x001B, 0x0019, 0x01 },
    { "animation.bobbingbow_draw", PS_ASSET_ANIMATION, -1, -1, 0x006E, 0x00 },
    { "animation.bobbingbow_win", PS_ASSET_ANIMATION, -1, -1, 0x006F, 0x00 },
    { "animation.bobsled_mount", PS_ASSET_ANIMATION, -1, 0x003A, -1, 0x00 },
    { "animation.bomb_hold", PS_ASSET_ANIMATION, -1, 0x0061, -1, 0x00 },
    { "animation.bomb_hold_left", PS_ASSET_ANIMATION, -1, 0x0063, -1, 0x00 },
    { "animation.bomb_hold_right", PS_ASSET_ANIMATION, -1, 0x0062, -1, 0x00 },
    { "animation.bomb_throw", PS_ASSET_ANIMATION, -1, 0x0064, -1, 0x00 },
    { "animation.bomb_throw_far", PS_ASSET_ANIMATION, -1, 0x0067, -1, 0x00 },
    { "animation.bomb_throw_far_left", PS_ASSET_ANIMATION, -1, 0x0069, -1, 0x00 },
    { "animation.bomb_throw_far_right", PS_ASSET_ANIMATION, -1, 0x0068, -1, 0x00 },
    { "animation.bomb_throw_left", PS_ASSET_ANIMATION, -1, 0x0066, -1, 0x00 },
    { "animation.bomb_throw_right", PS_ASSET_ANIMATION, -1, 0x0065, -1, 0x00 },
    { "animation.bow_draw", PS_ASSET_ANIMATION, -1, 0x0045, -1, 0x00 },
    { "animation.bow_fire", PS_ASSET_ANIMATION, -1, 0x0046, -1, 0x00 },
    { "animation.bow_walk", PS_ASSET_ANIMATION, -1, 0x0044, -1, 0x00 },
    { "animation.bow_walk_drawn", PS_ASSET_ANIMATION, -1, 0x0060, -1, 0x00 },
    { "animation.bowling_throw", PS_ASSET_ANIMATION, 0x006A, 0x0034, -1, 0x00 },
    { "animation.bowling_walk", PS_ASSET_ANIMATION, -1, 0x0036, -1, 0x00 },
    { "animation.bungee", PS_ASSET_ANIMATION, 0x0081, -1, -1, 0x00 },
    { "animation.cake_grab", PS_ASSET_ANIMATION, -1, 0x0049, -1, 0x00 },
    { "animation.cake_idle", PS_ASSET_ANIMATION, -1, 0x005A, -1, 0x00 },
    { "animation.cake_miss", PS_ASSET_ANIMATION, -1, 0x006B, -1, 0x00 },
    { "animation.cake_place", PS_ASSET_ANIMATION, -1, 0x004A, -1, 0x00 },
    { "animation.cannon_idle", PS_ASSET_ANIMATION, -1, 0x005E, 0x0027, 0x00 },
    { "animation.cannon_shoot", PS_ASSET_ANIMATION, -1, 0x005F, -1, 0x00 },
    { "animation.chicken_catch", PS_ASSET_ANIMATION, -1, -1, 0x0099, 0x00 },
    { "animation.chicken_win", PS_ASSET_ANIMATION, -1, -1, 0x009A, 0x00 },
    { "animation.climb", PS_ASSET_ANIMATION, -1, 0x008B, -1, 0x00 },
    { "animation.count", PS_ASSET_ANIMATION, -1, 0x00A2, -1, 0x00 },
    { "animation.crane_grab", PS_ASSET_ANIMATION, 0x0058, 0x0027, -1, 0x01 },
    { "animation.crane_hold", PS_ASSET_ANIMATION, 0x0059, 0x0028, -1, 0x01 },
    { "animation.crane_idle", PS_ASSET_ANIMATION, 0x0057, 0x0026, -1, 0x00 },
    { "animation.crane_lose", PS_ASSET_ANIMATION, -1, 0x0093, -1, 0x00 },
    { "animation.crane_win", PS_ASSET_ANIMATION, -1, 0x0092, -1, 0x00 },
    { "animation.crawl", PS_ASSET_ANIMATION, 0x0008, -1, -1, 0x00 },
    { "animation.crawl_start", PS_ASSET_ANIMATION, 0x001A, -1, -1, 0x00 },
    { "animation.cycle", PS_ASSET_ANIMATION, 0x0053, 0x0022, -1, 0x00 },
    { "animation.dance_a", PS_ASSET_ANIMATION, -1, 0x007A, -1, 0x00 },
    { "animation.dance_b", PS_ASSET_ANIMATION, -1, 0x007B, -1, 0x00 },
    { "animation.dance_down", PS_ASSET_ANIMATION, -1, 0x0077, -1, 0x00 },
    { "animation.dance_fail", PS_ASSET_ANIMATION, -1, 0x0075, -1, 0x00 },
    { "animation.dance_idle", PS_ASSET_ANIMATION, -1, 0x0074, -1, 0x00 },
    { "animation.dance_left", PS_ASSET_ANIMATION, -1, 0x0079, -1, 0x00 },
    { "animation.dance_loop", PS_ASSET_ANIMATION, -1, -1, 0x0063, 0x00 },
    { "animation.dance_right", PS_ASSET_ANIMATION, -1, 0x0078, -1, 0x00 },
    { "animation.dance_up", PS_ASSET_ANIMATION, -1, 0x0076, -1, 0x00 },
    { "animation.dance_z", PS_ASSET_ANIMATION, -1, 0x007C, -1, 0x00 },
    { "animation.dash_left", PS_ASSET_ANIMATION, 0x005B, 0x002A, -1, 0x01 },
    { "animation.dash_right", PS_ASSET_ANIMATION, 0x005A, 0x0029, -1, 0x01 },
    { "animation.dig", PS_ASSET_ANIMATION, 0x0040, -1, 0x003E, 0x00 },
    { "animation.dizzy_sit", PS_ASSET_ANIMATION, 0x002F, 0x0016, 0x0015, 0x00 },
    { "animation.dizzy_stand", PS_ASSET_ANIMATION, 0x000B, 0x0008, 0x0008, 0x00 },
    { "animation.dizzy_walk", PS_ASSET_ANIMATION, -1, 0x00BF, -1, 0x00 },
    { "animation.drowned", PS_ASSET_ANIMATION, 0x0014, 0x000C, 0x000B, 0x00 },
    { "animation.drowning", PS_ASSET_ANIMATION, 0x0013, 0x000B, -1, 0x00 },
    { "animation.drowning_hold", PS_ASSET_ANIMATION, 0x0026, -1, -1, 0x00 },
    { "animation.duck", PS_ASSET_ANIMATION, 0x0007, -1, -1, 0x00 },
    { "animation.duck_stand", PS_ASSET_ANIMATION, 0x001B, -1, -1, 0x00 },
    { "animation.earthquake", PS_ASSET_ANIMATION, -1, 0x007D, -1, 0x00 },
    { "animation.face_player", PS_ASSET_ANIMATION, 0x0095, -1, -1, 0x00 },
    { "animation.fall", PS_ASSET_ANIMATION, -1, 0x006D, 0x0029, 0x00 },
    { "animation.fall_setup", PS_ASSET_ANIMATION, 0x0082, -1, -1, 0x00 },
    { "animation.falldown", PS_ASSET_ANIMATION, 0x001E, 0x0012, 0x0011, 0x00 },
    { "animation.fishing_catch", PS_ASSET_ANIMATION, 0x0032, 0x0017, -1, 0x00 },
    { "animation.fishing_fail", PS_ASSET_ANIMATION, 0x0034, 0x0018, -1, 0x00 },
    { "animation.fishing_neutral", PS_ASSET_ANIMATION, 0x0033, -1, -1, 0x00 },
    { "animation.fishing_reel", PS_ASSET_ANIMATION, 0x0031, -1, 0x0017, 0x00 },
    { "animation.fishing_windup", PS_ASSET_ANIMATION, 0x0030, -1, 0x0016, 0x00 },
    { "animation.float", PS_ASSET_ANIMATION, 0x0011, 0x000A, -1, 0x00 },
    { "animation.getup_from_back", PS_ASSET_ANIMATION, 0x002B, -1, -1, 0x00 },
    { "animation.getup_from_belly", PS_ASSET_ANIMATION, 0x001D, 0x0011, 0x0010, 0x00 },
    { "animation.getup_from_belly_long", PS_ASSET_ANIMATION, 0x0069, 0x0033, 0x0024, 0x00 },
    { "animation.getup_from_butt", PS_ASSET_ANIMATION, 0x001C, 0x0010, 0x000F, 0x00 },
    { "animation.getup_from_butt_long", PS_ASSET_ANIMATION, 0x0068, 0x0032, 0x0023, 0x00 },
    { "animation.givemeabrake_pull", PS_ASSET_ANIMATION, -1, 0x007F, -1, 0x00 },
    { "animation.golf_idle", PS_ASSET_ANIMATION, -1, -1, 0x005E, 0x00 },
    { "animation.golf_swing", PS_ASSET_ANIMATION, -1, -1, 0x005F, 0x00 },
    { "animation.hammer_idle", PS_ASSET_ANIMATION, 0x0041, 0x0019, -1, 0x00 },
    { "animation.hammer_swing_horizontal", PS_ASSET_ANIMATION, 0x0045, -1, -1, 0x00 },
    { "animation.hammer_swing_vertical", PS_ASSET_ANIMATION, 0x0044, 0x001A, -1, 0x00 },
    { "animation.hipdrop", PS_ASSET_ANIMATION, 0x0009, 0x0006, 0x0006, 0x00 },
    { "animation.hipdrop_loop", PS_ASSET_ANIMATION, 0x000A, 0x0007, 0x0007, 0x00 },
    { "animation.hit_back", PS_ASSET_ANIMATION, 0x0064, 0x0030, 0x0021, 0x00 },
    { "animation.hit_front", PS_ASSET_ANIMATION, 0x0063, 0x002F, 0x0020, 0x00 },
    { "animation.hit_jump", PS_ASSET_ANIMATION, 0x0067, 0x0031, 0x0022, 0x00 },
    { "animation.hold_idle", PS_ASSET_ANIMATION, 0x0024, 0x0013, 0x0012, 0x00 },
    { "animation.hold_run", PS_ASSET_ANIMATION, 0x0022, -1, -1, 0x00 },
    { "animation.hold_swim", PS_ASSET_ANIMATION, 0x0015, -1, -1, 0x00 },
    { "animation.hold_tiptoe", PS_ASSET_ANIMATION, 0x0021, -1, -1, 0x00 },
    { "animation.hold_walk", PS_ASSET_ANIMATION, 0x0020, -1, -1, 0x00 },
    { "animation.honeycomb_jump", PS_ASSET_ANIMATION, -1, 0x00A5, -1, 0x00 },
    { "animation.honeycomb_panic", PS_ASSET_ANIMATION, -1, 0x006A, -1, 0x00 },
    { "animation.hurt_hold_butt", PS_ASSET_ANIMATION, 0x0049, 0x001C, 0x001A, 0x00 },
    { "animation.hurt_lean_back", PS_ASSET_ANIMATION, 0x0056, 0x0025, 0x001B, 0x00 },
    { "animation.idle", PS_ASSET_ANIMATION, 0x0000, 0x0000, 0x0000, 0x00 },
    { "animation.idle_look_downleft", PS_ASSET_ANIMATION, -1, 0x007E, -1, 0x00 },
    { "animation.idle_look_left", PS_ASSET_ANIMATION, -1, 0x0048, -1, 0x00 },
    { "animation.idle_look_right", PS_ASSET_ANIMATION, -1, 0x0047, -1, 0x00 },
    { "animation.idle_simple", PS_ASSET_ANIMATION, -1, -1, 0x009B, 0x00 },
    { "animation.jackhammer", PS_ASSET_ANIMATION, 0x0054, 0x0023, -1, 0x00 },
    { "animation.jump", PS_ASSET_ANIMATION, 0x0005, 0x0004, 0x0004, 0x00 },
    { "animation.jump_begin", PS_ASSET_ANIMATION, 0x0023, -1, -1, 0x00 },
    { "animation.jump_start", PS_ASSET_ANIMATION, 0x0062, 0x002E, 0x001F, 0x00 },
    { "animation.key_use", PS_ASSET_ANIMATION, 0x006C, -1, -1, 0x00 },
    { "animation.kick", PS_ASSET_ANIMATION, 0x0006, 0x0005, 0x0005, 0x00 },
    { "animation.limbo", PS_ASSET_ANIMATION, 0x0070, -1, -1, 0x00 },
    { "animation.limbo_fall", PS_ASSET_ANIMATION, 0x0071, -1, -1, 0x00 },
    { "animation.limbo_low", PS_ASSET_ANIMATION, 0x006F, -1, -1, 0x00 },
    { "animation.limbo_struggle", PS_ASSET_ANIMATION, 0x0092, -1, -1, 0x00 },
    { "animation.look_around", PS_ASSET_ANIMATION, 0x0093, 0x003F, -1, 0x00 },
    { "animation.lose", PS_ASSET_ANIMATION, 0x0010, -1, 0x0036, 0x00 },
    { "animation.lose_old", PS_ASSET_ANIMATION, -1, 0x00C8, -1, 0x00 },
    { "animation.mallet_drop", PS_ASSET_ANIMATION, -1, 0x0073, -1, 0x00 },
    { "animation.mallet_raise", PS_ASSET_ANIMATION, -1, 0x0072, -1, 0x00 },
    { "animation.mallet_ready", PS_ASSET_ANIMATION, -1, 0x0071, -1, 0x00 },
    { "animation.pickup", PS_ASSET_ANIMATION, 0x001F, -1, -1, 0x00 },
    { "animation.plunge", PS_ASSET_ANIMATION, 0x002E, 0x0015, 0x0014, 0x00 },
    { "animation.psychic_loop", PS_ASSET_ANIMATION, -1, 0x00AE, -1, 0x00 },
    { "animation.psychic_start", PS_ASSET_ANIMATION, -1, 0x00AD, -1, 0x00 },
    { "animation.punch", PS_ASSET_ANIMATION, 0x0004, 0x0003, 0x0003, 0x00 },
    { "animation.result_bad", PS_ASSET_ANIMATION, 0x003D, -1, -1, 0x00 },
    { "animation.result_bad_2", PS_ASSET_ANIMATION, 0x003E, -1, -1, 0x00 },
    { "animation.result_good", PS_ASSET_ANIMATION, 0x0039, 0x0051, 0x0032, 0x00 },
    { "animation.result_good_2", PS_ASSET_ANIMATION, 0x003B, 0x0052, 0x0033, 0x00 },
    { "animation.result_good_small", PS_ASSET_ANIMATION, 0x003A, -1, -1, 0x00 },
    { "animation.run", PS_ASSET_ANIMATION, 0x0003, 0x0002, 0x0002, 0x00 },
    { "animation.run_left", PS_ASSET_ANIMATION, 0x0061, 0x002D, 0x001E, 0x01 },
    { "animation.run_panic", PS_ASSET_ANIMATION, -1, -1, 0x0070, 0x00 },
    { "animation.run_right", PS_ASSET_ANIMATION, 0x0060, 0x002C, 0x001D, 0x01 },
    { "animation.shockdroporroll_pull", PS_ASSET_ANIMATION, -1, 0x0080, -1, 0x00 },
    { "animation.shyguysays1_fail", PS_ASSET_ANIMATION, 0x0080, -1, -1, 0x00 },
    { "animation.skate_idle", PS_ASSET_ANIMATION, 0x004A, 0x001D, -1, 0x00 },
    { "animation.skate_jump", PS_ASSET_ANIMATION, 0x004C, 0x001F, -1, 0x00 },
    { "animation.skate_jump_start", PS_ASSET_ANIMATION, 0x006E, 0x0035, -1, 0x00 },
    { "animation.skate_step", PS_ASSET_ANIMATION, 0x004B, 0x001E, -1, 0x00 },
    { "animation.skid", PS_ASSET_ANIMATION, 0x0018, 0x000F, 0x000E, 0x00 },
    { "animation.sky_flag_left", PS_ASSET_ANIMATION, -1, 0x004D, -1, 0x00 },
    { "animation.sky_flag_right", PS_ASSET_ANIMATION, -1, 0x004C, -1, 0x00 },
    { "animation.sky_idle", PS_ASSET_ANIMATION, -1, 0x004B, -1, 0x00 },
    { "animation.sky_win", PS_ASSET_ANIMATION, -1, 0x006C, -1, 0x00 },
    { "animation.slide_back", PS_ASSET_ANIMATION, 0x002A, -1, -1, 0x00 },
    { "animation.slide_belly", PS_ASSET_ANIMATION, 0x0028, -1, -1, 0x00 },
    { "animation.slide_butt", PS_ASSET_ANIMATION, 0x0029, -1, -1, 0x00 },
    { "animation.slide_butt_kick", PS_ASSET_ANIMATION, 0x002D, -1, -1, 0x00 },
    { "animation.sneaknsnore_fail", PS_ASSET_ANIMATION, -1, 0x0085, -1, 0x00 },
    { "animation.sneaknsnore_hide", PS_ASSET_ANIMATION, -1, 0x0084, -1, 0x00 },
    { "animation.sneaknsnore_idle", PS_ASSET_ANIMATION, -1, 0x0082, -1, 0x00 },
    { "animation.sneaknsnore_walk", PS_ASSET_ANIMATION, -1, 0x0083, -1, 0x00 },
    { "animation.spotlight_high", PS_ASSET_ANIMATION, -1, -1, 0x005C, 0x00 },
    { "animation.spotlight_low", PS_ASSET_ANIMATION, -1, -1, 0x005A, 0x00 },
    { "animation.spotlight_med", PS_ASSET_ANIMATION, -1, -1, 0x005B, 0x00 },
    { "animation.standoff_draw", PS_ASSET_ANIMATION, -1, 0x006F, -1, 0x00 },
    { "animation.standoff_holster", PS_ASSET_ANIMATION, -1, 0x0070, -1, 0x00 },
    { "animation.standoff_idle", PS_ASSET_ANIMATION, -1, 0x006E, -1, 0x00 },
    { "animation.superstar", PS_ASSET_ANIMATION, 0x0097, 0x0091, -1, 0x00 },
    { "animation.superstar_2", PS_ASSET_ANIMATION, -1, 0x0053, -1, 0x00 },
    { "animation.swim", PS_ASSET_ANIMATION, 0x000C, 0x0009, 0x0009, 0x00 },
    { "animation.swim_down", PS_ASSET_ANIMATION, 0x0019, -1, -1, 0x00 },
    { "animation.swim_hit", PS_ASSET_ANIMATION, 0x0016, 0x000D, 0x000C, 0x01 },
    { "animation.swim_idle", PS_ASSET_ANIMATION, 0x0017, 0x000E, 0x000D, 0x01 },
    { "animation.swim_idle_look", PS_ASSET_ANIMATION, 0x0037, -1, -1, 0x00 },
    { "animation.swim_punch", PS_ASSET_ANIMATION, 0x0012, -1, -1, 0x00 },
    { "animation.swim_throw", PS_ASSET_ANIMATION, 0x002C, -1, -1, 0x00 },
    { "animation.think", PS_ASSET_ANIMATION, -1, 0x00AC, 0x002C, 0x00 },
    { "animation.think_callout", PS_ASSET_ANIMATION, -1, 0x00BB, 0x002E, 0x00 },
    { "animation.think_decide", PS_ASSET_ANIMATION, 0x009B, 0x00BA, 0x002D, 0x01 },
    { "animation.throw", PS_ASSET_ANIMATION, 0x005F, 0x002B, 0x001C, 0x01 },
    { "animation.tightrope_idle", PS_ASSET_ANIMATION, 0x0079, 0x0039, -1, 0x00 },
    { "animation.tightrope_teeter", PS_ASSET_ANIMATION, 0x004E, 0x0021, -1, 0x00 },
    { "animation.tightrope_walk", PS_ASSET_ANIMATION, 0x004D, 0x0020, -1, 0x00 },
    { "animation.tiptoe", PS_ASSET_ANIMATION, 0x0002, -1, -1, 0x00 },
    { "animation.tired", PS_ASSET_ANIMATION, 0x0027, 0x0014, 0x0013, 0x01 },
    { "animation.trip", PS_ASSET_ANIMATION, 0x000D, -1, -1, 0x00 },
    { "animation.trip_2", PS_ASSET_ANIMATION, 0x000E, -1, -1, 0x00 },
    { "animation.tug", PS_ASSET_ANIMATION, 0x007E, 0x003D, -1, 0x00 },
    { "animation.tug_losing", PS_ASSET_ANIMATION, 0x007F, 0x003E, -1, 0x00 },
    { "animation.walk", PS_ASSET_ANIMATION, 0x0001, 0x0001, 0x0001, 0x00 },
    { "animation.waterski_idle", PS_ASSET_ANIMATION, -1, -1, 0x004F, 0x00 },
    { "animation.waterski_left", PS_ASSET_ANIMATION, -1, -1, 0x0051, 0x00 },
    { "animation.waterski_right", PS_ASSET_ANIMATION, -1, -1, 0x0050, 0x00 },
    { "animation.win", PS_ASSET_ANIMATION, 0x000F, 0x004E, 0x002F, 0x00 },
    { "animation.win_2", PS_ASSET_ANIMATION, 0x0038, 0x004F, 0x0030, 0x00 },
    { "animation.win_old", PS_ASSET_ANIMATION, -1, 0x00C7, -1, 0x00 },
    { "animation.windup", PS_ASSET_ANIMATION, -1, 0x0081, -1, 0x00 },
    { "animation.yoshi_hit", PS_ASSET_ANIMATION, 0x006B, -1, -1, 0x00 },
    { "animation.unknown_mp3_10", PS_ASSET_ANIMATION, -1, -1, 0x000A, 0x00 },
    { "animation.unknown_mp1_37", PS_ASSET_ANIMATION, 0x0025, -1, -1, 0x00 },
    { "animation.unknown_mp1_53", PS_ASSET_ANIMATION, 0x0035, -1, -1, 0x00 },
    { "animation.unknown_mp1_54", PS_ASSET_ANIMATION, 0x0036, -1, -1, 0x00 },
    { "animation.unknown_mp2_55", PS_ASSET_ANIMATION, -1, 0x0037, -1, 0x00 },
    { "animation.unknown_mp2_59", PS_ASSET_ANIMATION, -1, 0x003B, -1, 0x00 },
    { "animation.unknown_mp2_60", PS_ASSET_ANIMATION, -1, 0x003C, -1, 0x00 },
    { "animation.unknown_mp1_60", PS_ASSET_ANIMATION, 0x003C, -1, -1, 0x00 },
    { "animation.unknown_mp1_63", PS_ASSET_ANIMATION, 0x003F, -1, -1, 0x00 },
    { "animation.unknown_mp2_64", PS_ASSET_ANIMATION, -1, 0x0040, -1, 0x00 },
    { "animation.unknown_mp3_37", PS_ASSET_ANIMATION, -1, 0x0041, 0x0025, 0x00 },
    { "animation.unknown_mp2_66", PS_ASSET_ANIMATION, 0x0084, 0x0042, -1, 0x01 },
    { "animation.unknown_mp1_66", PS_ASSET_ANIMATION, 0x0042, -1, -1, 0x00 },
    { "animation.unknown_mp3_38", PS_ASSET_ANIMATION, -1, -1, 0x0026, 0x00 },
    { "animation.unknown_mp2_67", PS_ASSET_ANIMATION, -1, 0x0043, -1, 0x00 },
    { "animation.unknown_mp1_67", PS_ASSET_ANIMATION, 0x0043, -1, -1, 0x00 },
    { "animation.unknown_mp3_40", PS_ASSET_ANIMATION, -1, -1, 0x0028, 0x00 },
    { "animation.unknown_mp1_71", PS_ASSET_ANIMATION, 0x0047, -1, -1, 0x00 },
    { "animation.unknown_mp3_42", PS_ASSET_ANIMATION, -1, 0x008D, 0x002A, 0x00 },
    { "animation.unknown_mp1_72", PS_ASSET_ANIMATION, 0x0048, -1, -1, 0x00 },
    { "animation.unknown_mp3_43", PS_ASSET_ANIMATION, -1, 0x0098, 0x002B, 0x00 },
    { "animation.unknown_mp1_79", PS_ASSET_ANIMATION, 0x004F, -1, -1, 0x00 },
    { "animation.unknown_mp3_49", PS_ASSET_ANIMATION, -1, -1, 0x0031, 0x00 },
    { "animation.unknown_mp2_80", PS_ASSET_ANIMATION, -1, 0x0050, -1, 0x00 },
    { "animation.unknown_mp1_80", PS_ASSET_ANIMATION, 0x0050, -1, -1, 0x00 },
    { "animation.unknown_mp1_81", PS_ASSET_ANIMATION, 0x0051, -1, -1, 0x00 },
    { "animation.unknown_mp1_82", PS_ASSET_ANIMATION, 0x0052, -1, -1, 0x00 },
    { "animation.unknown_mp3_52", PS_ASSET_ANIMATION, -1, -1, 0x0034, 0x00 },
    { "animation.unknown_mp3_53", PS_ASSET_ANIMATION, -1, 0x0054, 0x0035, 0x00 },
    { "animation.unknown_mp2_85", PS_ASSET_ANIMATION, -1, 0x0055, -1, 0x00 },
    { "animation.unknown_mp3_55", PS_ASSET_ANIMATION, -1, -1, 0x0037, 0x00 },
    { "animation.unknown_mp2_86", PS_ASSET_ANIMATION, -1, 0x0056, -1, 0x00 },
    { "animation.unknown_mp3_56", PS_ASSET_ANIMATION, -1, 0x0057, 0x0038, 0x00 },
    { "animation.unknown_mp3_57", PS_ASSET_ANIMATION, -1, 0x0058, 0x0039, 0x00 },
    { "animation.unknown_mp3_58", PS_ASSET_ANIMATION, -1, 0x0059, 0x003A, 0x00 },
    { "animation.unknown_mp3_59", PS_ASSET_ANIMATION, -1, -1, 0x003B, 0x00 },
    { "animation.unknown_mp3_60", PS_ASSET_ANIMATION, -1, -1, 0x003C, 0x00 },
    { "animation.unknown_mp3_61", PS_ASSET_ANIMATION, -1, -1, 0x003D, 0x00 },
    { "animation.unknown_mp1_92", PS_ASSET_ANIMATION, 0x005C, -1, -1, 0x00 },
    { "animation.unknown_mp3_63", PS_ASSET_ANIMATION, -1, -1, 0x003F, 0x00 },
    { "animation.unknown_mp1_93", PS_ASSET_ANIMATION, 0x005D, -1, -1, 0x00 },
    { "animation.unknown_mp3_64", PS_ASSET_ANIMATION, -1, -1, 0x0040, 0x00 },
    { "animation.unknown_mp1_94", PS_ASSET_ANIMATION, 0x005E, -1, -1, 0x00 },
    { "animation.unknown_mp3_65", PS_ASSET_ANIMATION, -1, -1, 0x0041, 0x00 },
    { "animation.unknown_mp3_66", PS_ASSET_ANIMATION, -1, -1, 0x0042, 0x00 },
    { "animation.unknown_mp3_67", PS_ASSET_ANIMATION, -1, -1, 0x0043, 0x00 },
    { "animation.unknown_mp3_68", PS_ASSET_ANIMATION, -1, -1, 0x0044, 0x00 },
    { "animation.unknown_mp3_69", PS_ASSET_ANIMATION, -1, -1, 0x0045, 0x00 },
    { "animation.unknown_mp3_70", PS_ASSET_ANIMATION, -1, -1, 0x0046, 0x00 },
    { "animation.unknown_mp3_71", PS_ASSET_ANIMATION, -1, -1, 0x0047, 0x00 },
    { "animation.unknown_mp3_72", PS_ASSET_ANIMATION, -1, -1, 0x0048, 0x00 },
    { "animation.unknown_mp3_73", PS_ASSET_ANIMATION, -1, -1, 0x0049, 0x00 },
    { "animation.unknown_mp1_101", PS_ASSET_ANIMATION, 0x0065, -1, -1, 0x00 },
    { "animation.unknown_mp3_74", PS_ASSET_ANIMATION, -1, -1, 0x004A, 0x00 },
    { "animation.unknown_mp3_75", PS_ASSET_ANIMATION, -1, -1, 0x004B, 0x00 },
    { "animation.unknown_mp1_102", PS_ASSET_ANIMATION, 0x0066, -1, -1, 0x00 },
    { "animation.unknown_mp3_76", PS_ASSET_ANIMATION, -1, -1, 0x004C, 0x00 },
    { "animation.unknown_mp3_77", PS_ASSET_ANIMATION, -1, -1, 0x004D, 0x00 },
    { "animation.unknown_mp3_78", PS_ASSET_ANIMATION, -1, -1, 0x004E, 0x00 },
    { "animation.unknown_mp3_82", PS_ASSET_ANIMATION, -1, -1, 0x0052, 0x00 },
    { "animation.unknown_mp3_83", PS_ASSET_ANIMATION, -1, -1, 0x0053, 0x00 },
    { "animation.unknown_mp3_84", PS_ASSET_ANIMATION, -1, -1, 0x0054, 0x00 },
    { "animation.unknown_mp1_109", PS_ASSET_ANIMATION, 0x006D, -1, -1, 0x00 },
    { "animation.unknown_mp3_85", PS_ASSET_ANIMATION, -1, -1, 0x0055, 0x00 },
    { "animation.unknown_mp3_86", PS_ASSET_ANIMATION, -1, -1, 0x0056, 0x00 },
    { "animation.unknown_mp3_87", PS_ASSET_ANIMATION, -1, -1, 0x0057, 0x00 },
    { "animation.unknown_mp3_88", PS_ASSET_ANIMATION, -1, -1, 0x0058, 0x00 },
    { "animation.unknown_mp3_89", PS_ASSET_ANIMATION, -1, -1, 0x0059, 0x00 },
    { "animation.unknown_mp3_93", PS_ASSET_ANIMATION, -1, -1, 0x005D, 0x00 },
    { "animation.unknown_mp1_116", PS_ASSET_ANIMATION, 0x0074, -1, -1, 0x00 },
    { "animation.unknown_mp1_117", PS_ASSET_ANIMATION, 0x0075, -1, -1, 0x00 },
    { "animation.unknown_mp3_96", PS_ASSET_ANIMATION, -1, -1, 0x0060, 0x00 },
    { "animation.unknown_mp1_118", PS_ASSET_ANIMATION, 0x0076, -1, -1, 0x00 },
    { "animation.unknown_mp3_97", PS_ASSET_ANIMATION, -1, -1, 0x0061, 0x00 },
    { "animation.unknown_mp1_119", PS_ASSET_ANIMATION, 0x0077, -1, -1, 0x00 },
    { "animation.unknown_mp3_98", PS_ASSET_ANIMATION, -1, -1, 0x0062, 0x00 },
    { "animation.unknown_mp3_100", PS_ASSET_ANIMATION, -1, -1, 0x0064, 0x00 },
    { "animation.unknown_mp3_101", PS_ASSET_ANIMATION, -1, -1, 0x0065, 0x00 },
    { "animation.unknown_mp1_122", PS_ASSET_ANIMATION, 0x007A, -1, -1, 0x00 },
    { "animation.unknown_mp3_102", PS_ASSET_ANIMATION, -1, -1, 0x0066, 0x00 },
    { "animation.unknown_mp1_123", PS_ASSET_ANIMATION, 0x007B, -1, -1, 0x00 },
    { "animation.unknown_mp3_103", PS_ASSET_ANIMATION, -1, -1, 0x0067, 0x00 },
    { "animation.unknown_mp1_124", PS_ASSET_ANIMATION, 0x007C, -1, -1, 0x00 },
    { "animation.unknown_mp3_104", PS_ASSET_ANIMATION, -1, -1, 0x0068, 0x00 },
    { "animation.unknown_mp3_105", PS_ASSET_ANIMATION, -1, -1, 0x0069, 0x00 },
    { "animation.unknown_mp1_125", PS_ASSET_ANIMATION, 0x007D, -1, -1, 0x00 },
    { "animation.unknown_mp3_106", PS_ASSET_ANIMATION, -1, -1, 0x006A, 0x00 },
    { "animation.unknown_mp3_107", PS_ASSET_ANIMATION, -1, -1, 0x006B, 0x00 },
    { "animation.unknown_mp3_108", PS_ASSET_ANIMATION, -1, -1, 0x006C, 0x00 },
    { "animation.unknown_mp3_109", PS_ASSET_ANIMATION, -1, -1, 0x006D, 0x00 },
    { "animation.unknown_mp3_113", PS_ASSET_ANIMATION, -1, -1, 0x0071, 0x00 },
    { "animation.unknown_mp1_131", PS_ASSET_ANIMATION, 0x0083, -1, -1, 0x00 },
    { "animation.unknown_mp3_115", PS_ASSET_ANIMATION, -1, -1, 0x0073, 0x00 },
    { "animation.unknown_mp1_133", PS_ASSET_ANIMATION, 0x0085, -1, -1, 0x00 },
    { "animation.unknown_mp3_116", PS_ASSET_ANIMATION, -1, -1, 0x0074, 0x00 },
    { "animation.unknown_mp3_117", PS_ASSET_ANIMATION, -1, -1, 0x0075, 0x00 },
    { "animation.unknown_mp2_134", PS_ASSET_ANIMATION, -1, 0x0086, -1, 0x00 },
    { "animation.unknown_mp1_134", PS_ASSET_ANIMATION, 0x0086, -1, -1, 0x00 },
    { "animation.unknown_mp3_118", PS_ASSET_ANIMATION, -1, -1, 0x0076, 0x00 },
    { "animation.unknown_mp2_135", PS_ASSET_ANIMATION, -1, 0x0087, -1, 0x00 },
    { "animation.unknown_mp1_135", PS_ASSET_ANIMATION, 0x0087, -1, -1, 0x00 },
    { "animation.unknown_mp3_119", PS_ASSET_ANIMATION, -1, -1, 0x0077, 0x00 },
    { "animation.unknown_mp2_136", PS_ASSET_ANIMATION, -1, 0x0088, -1, 0x00 },
    { "animation.unknown_mp1_136", PS_ASSET_ANIMATION, 0x0088, -1, -1, 0x00 },
    { "animation.unknown_mp3_120", PS_ASSET_ANIMATION, -1, -1, 0x0078, 0x00 },
    { "animation.unknown_mp3_121", PS_ASSET_ANIMATION, -1, -1, 0x0079, 0x00 },
    { "animation.unknown_mp2_137", PS_ASSET_ANIMATION, -1, 0x0089, -1, 0x00 },
    { "animation.unknown_mp1_137", PS_ASSET_ANIMATION, 0x0089, -1, -1, 0x00 },
    { "animation.unknown_mp3_122", PS_ASSET_ANIMATION, -1, -1, 0x007A, 0x00 },
    { "animation.unknown_mp2_138", PS_ASSET_ANIMATION, -1, 0x008A, -1, 0x00 },
    { "animation.unknown_mp1_138", PS_ASSET_ANIMATION, 0x008A, -1, -1, 0x00 },
    { "animation.unknown_mp3_123", PS_ASSET_ANIMATION, -1, -1, 0x007B, 0x00 },
    { "animation.unknown_mp1_139", PS_ASSET_ANIMATION, 0x008B, -1, -1, 0x00 },
    { "animation.unknown_mp3_124", PS_ASSET_ANIMATION, -1, -1, 0x007C, 0x00 },
    { "animation.unknown_mp2_140", PS_ASSET_ANIMATION, -1, 0x008C, -1, 0x00 },
    { "animation.unknown_mp1_140", PS_ASSET_ANIMATION, 0x008C, -1, -1, 0x00 },
    { "animation.unknown_mp3_125", PS_ASSET_ANIMATION, -1, -1, 0x007D, 0x00 },
    { "animation.unknown_mp3_126", PS_ASSET_ANIMATION, -1, -1, 0x007E, 0x00 },
    { "animation.unknown_mp1_141", PS_ASSET_ANIMATION, 0x008D, -1, -1, 0x00 },
    { "animation.unknown_mp3_127", PS_ASSET_ANIMATION, -1, -1, 0x007F, 0x00 },
    { "animation.unknown_mp2_142", PS_ASSET_ANIMATION, -1, 0x008E, -1, 0x00 },
    { "animation.unknown_mp1_142", PS_ASSET_ANIMATION, 0x008E, -1, -1, 0x00 },
    { "animation.unknown_mp3_128", PS_ASSET_ANIMATION, -1, -1, 0x0080, 0x00 },
    { "animation.unknown_mp2_143", PS_ASSET_ANIMATION, -1, 0x008F, -1, 0x00 },
    { "animation.unknown_mp1_143", PS_ASSET_ANIMATION, 0x008F, -1, -1, 0x00 },
    { "animation.unknown_mp3_129", PS_ASSET_ANIMATION, -1, -1, 0x0081, 0x00 },
    { "animation.unknown_mp3_130", PS_ASSET_ANIMATION, -1, -1, 0x0082, 0x00 },
    { "animation.unknown_mp2_144", PS_ASSET_ANIMATION, -1, 0x0090, -1, 0x00 },
    { "animation.unknown_mp1_144", PS_ASSET_ANIMATION, 0x0090, -1, -1, 0x00 },
    { "animation.unknown_mp3_131", PS_ASSET_ANIMATION, -1, -1, 0x0083, 0x00 },
    { "animation.unknown_mp1_145", PS_ASSET_ANIMATION, 0x0091, -1, -1, 0x00 },
    { "animation.unknown_mp3_132", PS_ASSET_ANIMATION, -1, -1, 0x0084, 0x00 },
    { "animation.unknown_mp3_133", PS_ASSET_ANIMATION, -1, -1, 0x0085, 0x00 },
    { "animation.unknown_mp3_134", PS_ASSET_ANIMATION, -1, -1, 0x0086, 0x00 },
    { "animation.unknown_mp3_135", PS_ASSET_ANIMATION, -1, -1, 0x0087, 0x00 },
    { "animation.unknown_mp2_148", PS_ASSET_ANIMATION, -1, 0x0094, -1, 0x00 },
    { "animation.unknown_mp1_148", PS_ASSET_ANIMATION, 0x0094, -1, -1, 0x00 },
    { "animation.unknown_mp3_136", PS_ASSET_ANIMATION, -1, -1, 0x0088, 0x00 },
    { "animation.unknown_mp2_149", PS_ASSET_ANIMATION, -1, 0x0095, -1, 0x00 },
    { "animation.unknown_mp3_137", PS_ASSET_ANIMATION, -1, -1, 0x0089, 0x00 },
    { "animation.unknown_mp3_138", PS_ASSET_ANIMATION, -1, -1, 0x008A, 0x00 },
    { "animation.unknown_mp2_150", PS_ASSET_ANIMATION, -1, 0x0096, -1, 0x00 },
    { "animation.unknown_mp1_150", PS_ASSET_ANIMATION, 0x0096, -1, -1, 0x00 },
    { "animation.unknown_mp3_139", PS_ASSET_ANIMATION, -1, -1, 0x008B, 0x00 },
    { "animation.unknown_mp2_151", PS_ASSET_ANIMATION, 0x0098, 0x0097, -1, 0x01 },
    { "animation.unknown_mp3_140", PS_ASSET_ANIMATION, -1, -1, 0x008C, 0x00 },
    { "animation.unknown_mp3_141", PS_ASSET_ANIMATION, -1, -1, 0x008D, 0x00 },
    { "animation.unknown_mp3_142", PS_ASSET_ANIMATION, -1, 0x0099, 0x008E, 0x04 },
    { "animation.unknown_mp2_154", PS_ASSET_ANIMATION, -1, 0x009A, -1, 0x00 },
    { "animation.unknown_mp3_143", PS_ASSET_ANIMATION, -1, -1, 0x008F, 0x00 },
    { "animation.unknown_mp2_155", PS_ASSET_ANIMATION, -1, 0x009B, -1, 0x00 },
    { "animation.unknown_mp2_156", PS_ASSET_ANIMATION, -1, 0x009C, -1, 0x00 },
    { "animation.unknown_mp1_156", PS_ASSET_ANIMATION, 0x009C, -1, -1, 0x00 },
    { "animation.unknown_mp3_144", PS_ASSET_ANIMATION, -1, -1, 0x0090, 0x00 },
    { "animation.unknown_mp2_157", PS_ASSET_ANIMATION, -1, 0x009D, -1, 0x00 },
    { "animation.unknown_mp2_158", PS_ASSET_ANIMATION, -1, 0x009E, -1, 0x00 },
    { "animation.unknown_mp3_145", PS_ASSET_ANIMATION, -1, -1, 0x0091, 0x00 },
    { "animation.unknown_mp2_159", PS_ASSET_ANIMATION, -1, 0x009F, -1, 0x00 },
    { "animation.unknown_mp2_160", PS_ASSET_ANIMATION, -1, 0x00A0, -1, 0x00 },
    { "animation.unknown_mp3_146", PS_ASSET_ANIMATION, -1, -1, 0x0092, 0x00 },
    { "animation.unknown_mp2_161", PS_ASSET_ANIMATION, -1, 0x00A1, -1, 0x00 },
    { "animation.unknown_mp3_147", PS_ASSET_ANIMATION, -1, -1, 0x0093, 0x00 },
    { "animation.unknown_mp2_163", PS_ASSET_ANIMATION, -1, 0x00A3, -1, 0x00 },
    { "animation.unknown_mp3_148", PS_ASSET_ANIMATION, -1, 0x00A4, 0x0094, 0x04 },
    { "animation.unknown_mp2_175", PS_ASSET_ANIMATION, -1, 0x00AF, -1, 0x00 },
    { "animation.unknown_mp2_176", PS_ASSET_ANIMATION, 0x0099, 0x00B0, -1, 0x01 },
    { "animation.unknown_mp2_177", PS_ASSET_ANIMATION, -1, 0x00B1, -1, 0x00 },
    { "animation.unknown_mp2_178", PS_ASSET_ANIMATION, 0x009A, 0x00B2, -1, 0x01 },
    { "animation.unknown_mp2_179", PS_ASSET_ANIMATION, -1, 0x00B3, -1, 0x00 },
    { "animation.unknown_mp2_180", PS_ASSET_ANIMATION, -1, 0x00B4, -1, 0x00 },
    { "animation.unknown_mp2_181", PS_ASSET_ANIMATION, -1, 0x00B5, -1, 0x00 },
    { "animation.unknown_mp2_182", PS_ASSET_ANIMATION, -1, 0x00B6, -1, 0x00 },
    { "animation.unknown_mp2_183", PS_ASSET_ANIMATION, -1, 0x00B7, -1, 0x00 },
    { "animation.unknown_mp2_184", PS_ASSET_ANIMATION, -1, 0x00B8, -1, 0x00 },
    { "animation.unknown_mp2_185", PS_ASSET_ANIMATION, -1, 0x00B9, -1, 0x00 },
    { "animation.unknown_mp2_188", PS_ASSET_ANIMATION, -1, 0x00BC, -1, 0x00 },
    { "animation.unknown_mp2_189", PS_ASSET_ANIMATION, -1, 0x00BD, -1, 0x00 },
    { "animation.unknown_mp2_190", PS_ASSET_ANIMATION, -1, 0x00BE, -1, 0x00 },
    { "animation.unknown_mp2_192", PS_ASSET_ANIMATION, -1, 0x00C0, -1, 0x00 },
    { "animation.unknown_mp2_193", PS_ASSET_ANIMATION, -1, 0x00C1, -1, 0x00 },
    { "animation.unknown_mp2_194", PS_ASSET_ANIMATION, -1, 0x00C2, -1, 0x00 },
    { "animation.unknown_mp2_195", PS_ASSET_ANIMATION, -1, 0x00C3, -1, 0x00 },
    { "animation.unknown_mp2_196", PS_ASSET_ANIMATION, -1, 0x00C4, -1, 0x00 },
    { "animation.unknown_mp2_197", PS_ASSET_ANIMATION, -1, 0x00C5, -1, 0x00 },
    { "animation.unknown_mp2_201", PS_ASSET_ANIMATION, -1, 0x00C9, -1, 0x00 },
    { "animation.unknown_mp2_202", PS_ASSET_ANIMATION, -1, 0x00CA, -1, 0x00 },
    { "animation.unknown_mp2_203", PS_ASSET_ANIMATION, -1, 0x00CB, -1, 0x00 },
    { "animation.unknown_mp2_204", PS_ASSET_ANIMATION, -1, 0x00CC, -1, 0x00 },
    { "animation.unknown_mp2_205", PS_ASSET_ANIMATION, -1, 0x00CD, -1, 0x00 },
    { "animation.unknown_mp2_206", PS_ASSET_ANIMATION, -1, 0x00CE, -1, 0x00 },
    { "animation.unknown_mp2_207", PS_ASSET_ANIMATION, -1, 0x00CF, -1, 0x00 },
    { "animation.unknown_mp2_208", PS_ASSET_ANIMATION, -1, 0x00D0, -1, 0x00 },
    { "animation.unknown_mp3_149", PS_ASSET_ANIMATION, -1, -1, 0x0095, 0x00 },
    { "animation.unknown_mp3_150", PS_ASSET_ANIMATION, -1, -1, 0x0096, 0x00 },
    { "animation.unknown_mp3_151", PS_ASSET_ANIMATION, -1, -1, 0x0097, 0x00 },
    { "animation.unknown_mp3_152", PS_ASSET_ANIMATION, -1, -1, 0x0098, 0x00 },
    { "animation.unknown_mp3_156", PS_ASSET_ANIMATION, -1, -1, 0x009C, 0x00 },
    { "animation.unknown_mp3_157", PS_ASSET_ANIMATION, -1, -1, 0x009D, 0x00 },
    { "animation.unknown_mp3_158", PS_ASSET_ANIMATION, -1, -1, 0x009E, 0x00 },
    { "animation.unknown_mp3_159", PS_ASSET_ANIMATION, 0x0055, 0x0024, 0x009F, 0x01 },
    { "model.high", PS_ASSET_MODEL, 0x009E, 0x00D1, 0x00A0, 0x00 },
    { "model.medium", PS_ASSET_MODEL, -1, 0x00D2, 0x00A1, 0x00 },
    { "model.low", PS_ASSET_MODEL, 0x009F, 0x00D3, 0x00A2, 0x00 },
    { "texture.unknown_mp1_00a0", PS_ASSET_TEXTURE, 0x00A0, -1, -1, 0x00 },
    { "texture.unknown_mp1_00a1", PS_ASSET_TEXTURE, 0x00A1, -1, -1, 0x00 },
    { "texture.unknown_mp1_00a2", PS_ASSET_TEXTURE, 0x00A2, -1, -1, 0x00 },
    { "model.unknown_mp2_00d4", PS_ASSET_MODEL, -1, 0x00D4, -1, 0x00 },
    { "model.unknown_mp2_00d5", PS_ASSET_MODEL, -1, 0x00D5, -1, 0x00 },
    { "model.unknown_mp2_00d6", PS_ASSET_MODEL, -1, 0x00D6, -1, 0x00 },
    { "model.unknown_mp2_00d7", PS_ASSET_MODEL, -1, 0x00D7, -1, 0x00 },
    { "model.unknown_mp2_00d8", PS_ASSET_MODEL, -1, 0x00D8, -1, 0x00 },
    { "model.unknown_mp2_00d9", PS_ASSET_MODEL, -1, 0x00D9, -1, 0x00 },
    { "model.unknown_mp2_00da", PS_ASSET_MODEL, -1, 0x00DA, -1, 0x00 },
    { "model.unknown_mp2_00db", PS_ASSET_MODEL, -1, 0x00DB, -1, 0x00 },
    { "model.unknown_mp2_00dc", PS_ASSET_MODEL, -1, 0x00DC, -1, 0x00 },
    { "model.unknown_mp2_00dd", PS_ASSET_MODEL, -1, 0x00DD, -1, 0x00 },
    { "texture.unknown_mp2_00de", PS_ASSET_TEXTURE, -1, 0x00DE, -1, 0x00 },
    { "texture.unknown_mp2_00df", PS_ASSET_TEXTURE, -1, 0x00DF, -1, 0x00 },
    { "texture.unknown_mp2_00e0", PS_ASSET_TEXTURE, -1, 0x00E0, -1, 0x00 },
    { "texture.unknown_mp2_00e1", PS_ASSET_TEXTURE, -1, 0x00E1, -1, 0x00 },
    { "texture.unknown_mp2_00e2", PS_ASSET_TEXTURE, -1, 0x00E2, -1, 0x00 },
    { "texture.unknown_mp2_00e3", PS_ASSET_TEXTURE, -1, 0x00E3, -1, 0x00 },
    { "texture.unknown_mp2_00e4", PS_ASSET_TEXTURE, -1, 0x00E4, -1, 0x00 },
    { "texture.unknown_mp2_00e5", PS_ASSET_TEXTURE, -1, 0x00E5, -1, 0x00 },
    { "texture.unknown_mp2_00e6", PS_ASSET_TEXTURE, -1, 0x00E6, -1, 0x00 },
    { "texture.unknown_mp2_00e7", PS_ASSET_TEXTURE, -1, 0x00E7, -1, 0x00 },
    { "texture.unknown_mp2_00e8", PS_ASSET_TEXTURE, -1, 0x00E8, -1, 0x00 },
    { "texture.unknown_mp2_00e9", PS_ASSET_TEXTURE, -1, 0x00E9, -1, 0x00 },
    { "texture.unknown_mp2_00ea", PS_ASSET_TEXTURE, -1, 0x00EA, -1, 0x00 },
    { "texture.unknown_mp2_00eb", PS_ASSET_TEXTURE, -1, 0x00EB, -1, 0x00 },
    { "texture.unknown_mp3_00a3", PS_ASSET_TEXTURE, -1, -1, 0x00A3, 0x00 },
    { "texture.unknown_mp3_00a4", PS_ASSET_TEXTURE, -1, -1, 0x00A4, 0x00 },
    { "texture.unknown_mp3_00a5", PS_ASSET_TEXTURE, -1, -1, 0x00A5, 0x00 },
    { "texture.unknown_mp3_00a6", PS_ASSET_TEXTURE, -1, -1, 0x00A6, 0x00 },
    { "texture.unknown_mp3_00a7", PS_ASSET_TEXTURE, -1, -1, 0x00A7, 0x00 },
    { "texture.unknown_mp3_00a8", PS_ASSET_TEXTURE, -1, -1, 0x00A8, 0x00 },
    { "texture.unknown_mp3_00a9", PS_ASSET_TEXTURE, -1, -1, 0x00A9, 0x00 },
    { "texture.unknown_mp3_00aa", PS_ASSET_TEXTURE, -1, -1, 0x00AA, 0x00 },
    { "texture.unknown_mp3_00ab", PS_ASSET_TEXTURE, -1, -1, 0x00AB, 0x00 },
};

/* Sound IDs have an explicit namespace: code-derived groups use effect IDs;
 * user-identified voices use exported samples/NNNN.wav IDs.
 * MP2 shared actions intentionally use the same exported sample ID.
 * Explicit character order: Mario, Luigi, Peach, Yoshi, Wario, DK, Waluigi, Daisy.
 * bank is sfx_N. A -1 bank or effect means absent/unresolved; do not inject.
 * There is no arithmetic relationship assumed between character effects.
 * Never interpret an exported-sample ID as an effect-table index.
 */
#define PS_SOUND_CHARACTER_COUNT 8
enum { PS_SOUND_EFFECT, PS_SOUND_EXPORTED_SAMPLE };

typedef struct PsCharacterSoundRef {
    signed bank;
    signed id[PS_SOUND_CHARACTER_COUNT];
} PsCharacterSoundRef;

typedef struct PsCharacterSound {
    const char *name;
    signed id_kind; /* PS_SOUND_EFFECT or PS_SOUND_EXPORTED_SAMPLE */
    PsCharacterSoundRef game[PS_GAME_COUNT];
} PsCharacterSound;

/* Voice IDs supplied by the user are exported sample IDs (MP1 and MP3). DK's unnumbered block is interpreted
 * as FA win, FB lose, FC superstar, FD despair (between Yoshi and Wario).
 * good_choice/got_item omit Yoshi and DK; every entry is explicit.
 * Daisy (column 7) is mapped for sounds only, not mainfs injection.
 */
static const PsCharacterSound ps_character_sounds[] = {
    { "sound.unknown_mp3_story_02c6", PS_SOUND_EFFECT, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x005F, 0x0064, 0x006A, 0x006C, 0x0073, 0x006F, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CC, 0x02CD, -1 } } /* MP3 */
    } },
    { "sound.get_star", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { 0x0000, { 0x01D3, 0x01D4, 0x01D5, 0x01D6, 0x01D7, 0x01D8, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x005F, 0x0064, 0x006A, 0x006C, 0x0073, 0x006F, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E2, 0x00E9, 0x00F0, -1, 0x00FE, -1, 0x0105, 0x010B } } /* MP3 */
    } },
    { "sound.lose", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { 0x0000, { 0x01D9, 0x01DA, 0x01DB, 0x01DC, 0x01DD, 0x01DE, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x0060, 0x0065, 0x0069, 0x006E, 0x0074, 0x0070, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E3, 0x00EA, 0x00F1, -1, 0x00FF, 0x00FB, 0x0106, 0x010C } } /* MP3 */
    } },
    { "sound.win", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { 0x0000, { 0x01DF, 0x01E0, 0x01E1, 0x01E2, -1, -1, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x0061, 0x0064, 0x0068, 0x006C, 0x0073, 0x006F, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E4, 0x00EB, 0x00F2, 0x00F7, 0x0100, 0x00FA, 0x0107, 0x010D } } /* MP3 */
    } },
    { "sound.superstar", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { 0x0000, { 0x01E3, 0x01E4, -1, -1, 0x01E5, 0x01E6, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x0062, 0x0066, 0x006A, 0x006D, 0x0075, 0x0071, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E7, 0x00EE, 0x00F5, 0x00F8, 0x0103, 0x00FC, 0x010A, 0x0110 } } /* MP3 */
    } },
    { "sound.despair", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { 0x0000, { 0x01E7, 0x01E8, 0x01E9, -1, 0x01EA, 0x01EB, -1, -1 } }, /* MP1 */
        { 0x0000, { 0x0063, 0x0067, 0x006B, 0x006E, 0x0076, 0x0072, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E6, 0x00ED, 0x00F4, 0x00F9, 0x0102, 0x00FD, 0x0109, 0x010F } } /* MP3 */
    } },
    { "sound.superstar_story", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP1 */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E5, 0x00EC, 0x00F3, -1, 0x0101, -1, 0x0108, 0x010E } } /* MP3 */
    } },
    { "sound.despair_2", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP1 */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x00E8, 0x00EF, 0x00F6, -1, 0x0104, -1, -1, -1 } } /* MP3 */
    } },
    { "sound.good_choice", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP1 */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x0121, 0x0122, 0x0123, -1, 0x0124, -1, 0x0125, 0x0126 } } /* MP3 */
    } },
    { "sound.got_item", PS_SOUND_EXPORTED_SAMPLE, {
        /*              Mario   Luigi   Peach   Yoshi   Wario   DK      Waluigi Daisy */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP1 */
        { -1, { -1, -1, -1, -1, -1, -1, -1, -1 } }, /* MP2 */
        { 0x0000, { 0x0127, 0x0128, 0x0129, -1, 0x012A, -1, 0x012B, 0x012C } } /* MP3 */
    } },
};

#define PS_CHARACTER_SLOT_COUNT (sizeof(ps_character_slots) / sizeof(ps_character_slots[0]))
#define PS_CHARACTER_FILE_COUNT (sizeof(ps_character_files) / sizeof(ps_character_files[0]))
#define PS_CHARACTER_SOUND_COUNT (sizeof(ps_character_sounds) / sizeof(ps_character_sounds[0]))

/* Editing: rename unknown keys once identified; combine equivalent rows,
 * retaining each game's index; clear an inferred bit only after verification.
 * Set id_kind correctly: user voice IDs are exported samples/NNNN.wav;
 * code-derived effect IDs require PS_SOUND_EFFECT. Do not mix namespaces.
 * This header supersedes the stale C000-based animation map for NEW consumers;
 * the old header is left untouched for any existing users.
 */
#endif
