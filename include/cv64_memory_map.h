/**
 * @file cv64_memory_map.h
 * @brief Castlevania 64 PC Recomp - Memory Address Mappings
 * 
 * Memory addresses extracted from cv64 decomp project.
 * These addresses are used to hook into the N64 game's memory
 * for our camera patch and other enhancements.
 * 
 * Source: C:\Users\patte\cv64\linker\symbol_addrs.txt
 * 
 * SAFETY NOTES:
 * - All memory access functions validate pointers and bounds
 * - Read functions return 0 or safe defaults on failure
 * - Write functions return bool to indicate success/failure
 * - Uses 8MB RDRAM size to support CV64's extended addresses
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_MEMORY_MAP_H
#define CV64_MEMORY_MAP_H

#include "cv64_types.h"
#include <string.h>  /* For memcpy in safe type-punning */

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * N64 Memory Layout
 *===========================================================================*/

#define N64_RDRAM_BASE          0x80000000
#define N64_RDRAM_SIZE_4MB      0x00400000  /* 4MB RDRAM (standard) */
#define N64_RDRAM_SIZE_8MB      0x00800000  /* 8MB RDRAM (expansion pak) */
#define N64_RDRAM_SIZE          N64_RDRAM_SIZE_8MB  /* Use 8MB for safety - CV64 addresses extend past 4MB */
#define N64_RDRAM_END           0x807FFFFF

/* Convert N64 address to RDRAM offset - handles both 0x80XXXXXX and raw offsets */
#define N64_ADDR_TO_OFFSET(addr) ((addr) & 0x007FFFFF)

/* Validate that an address is within RDRAM bounds */
#define N64_ADDR_IS_VALID(addr, size, rdram_size) \
    (N64_ADDR_TO_OFFSET(addr) + (size) <= (rdram_size))

/*===========================================================================
 * System Work Structure (Main Game State)
 * Address: 0x80363AB8
 * This is THE key structure containing all game state
 *===========================================================================*/

#define CV64_ADDR_SYSTEM_WORK   0x80363AB8

/* Offsets within system_work structure (verified from CASTLEVANIA.sym) */
#define CV64_SYS_OFFSET_CONTROLLERS     0x00000  /* Controller[4] array */
#define CV64_SYS_OFFSET_SAVE_STRUCT     0x2612C  /* SaveStruct_gameplay (0x80389BE4) */
#define CV64_SYS_OFFSET_CHARACTER       0x26184  /* s16 character (getCurrentCharacter reads this) */
#define CV64_SYS_OFFSET_HEALTH          0x26186  /* u16 Player_health (0x80389C3E) */
#define CV64_SYS_OFFSET_MAX_HEALTH      0x26188  /* u16 Player_max_health (0x80389C40) */
#define CV64_SYS_OFFSET_DIFFICULTY      0x26210  /* save_difficulty (0x80389CC8) */
#define CV64_SYS_OFFSET_MAP_ID_COPY     0x261D8  /* map_ID_copy (0x80389C90) */
#define CV64_SYS_OFFSET_PLAYER_MODULE   0x26224  /* ptr_PlayerModule (0x80389CDC) */
#define CV64_SYS_OFFSET_PLAYER_PTR      0x26228  /* Player* ptr_PlayerObject (0x80389CE0) */
#define CV64_SYS_OFFSET_POWERUP_LEVEL   0x26234  /* current_PowerUp_level (0x80389CEC) */
#define CV64_SYS_OFFSET_CAMERA_MGR_PTR  0x263C8  /* PTR_MODULE_006A / cameraMgr (0x80389E80) */
#define CV64_SYS_OFFSET_MAP_ID          0x26428  /* map_ID (0x80389EE0) */
#define CV64_SYS_OFFSET_CURRENT_MENU    0x26436  /* current_opened_menu (0x80389EEE) */

/*===========================================================================
 * Controller Structure
 * Size: 0x10 (16 bytes) per controller
 *===========================================================================*/

#define CV64_CONTROLLER_SIZE    0x10
#define CV64_MAX_CONTROLLERS    4

/* Offsets within Controller structure */
#define CV64_CTRL_OFFSET_IS_CONNECTED   0x00  /* u16 */
#define CV64_CTRL_OFFSET_BTNS_HELD      0x02  /* u16 */
#define CV64_CTRL_OFFSET_BTNS_PRESSED   0x04  /* u16 */
#define CV64_CTRL_OFFSET_JOY_X          0x06  /* s16 (-80 to +80) */
#define CV64_CTRL_OFFSET_JOY_Y          0x08  /* s16 (-80 to +80) */
#define CV64_CTRL_OFFSET_JOY_ANG        0x0A  /* s16 angle */
#define CV64_CTRL_OFFSET_JOY_HELD       0x0C  /* s16 magnitude */

/* N64 Controller Button Masks */
#define CV64_BTN_A          0x8000
#define CV64_BTN_B          0x4000
#define CV64_BTN_Z          0x2000
#define CV64_BTN_START      0x1000
#define CV64_BTN_DPAD_UP    0x0800
#define CV64_BTN_DPAD_DOWN  0x0400
#define CV64_BTN_DPAD_LEFT  0x0200
#define CV64_BTN_DPAD_RIGHT 0x0100
#define CV64_BTN_L          0x0020
#define CV64_BTN_R          0x0010
#define CV64_BTN_C_UP       0x0008
#define CV64_BTN_C_DOWN     0x0004
#define CV64_BTN_C_LEFT     0x0002
#define CV64_BTN_C_RIGHT    0x0001

/*===========================================================================
 * Camera Manager Structure (cameraMgr_data)
 * This controls all camera behavior in the game
 *===========================================================================*/

/* Offsets within cameraMgr_data structure */
#define CV64_CAM_OFFSET_STATE           0x00  /* u32 player_camera_state */
#define CV64_CAM_OFFSET_MODE            0x04  /* u32 camera_mode */
#define CV64_CAM_OFFSET_PREV_MODE       0x08  /* u32 previous_camera_mode */
#define CV64_CAM_OFFSET_CAMERA_PTR      0x0C  /* Camera* camera_player */
#define CV64_CAM_OFFSET_CTRL_PTR        0x10  /* playerCameraController* */
#define CV64_CAM_OFFSET_MODE_TEXT       0x14  /* Model* camera_mode_text */
#define CV64_CAM_OFFSET_POSITION        0x18  /* Vec3f camera_position */
#define CV64_CAM_OFFSET_LOOK_AT         0x24  /* Vec3f camera_look_at_direction */
#define CV64_CAM_OFFSET_DISTANCE        0x70  /* Vec3f camera_distance_to_player */
#define CV64_CAM_OFFSET_FP_LOOK_AT      0x7C  /* Vec3f first_person_camera_look_at_dir */
#define CV64_CAM_OFFSET_PLAYER_YAW      0x88  /* s32 player_angle_yaw */

/* Camera Mode Constants */
#define CV64_CAMERA_MODE_NORMAL         0
#define CV64_CAMERA_MODE_BATTLE         1
#define CV64_CAMERA_MODE_ACTION         2
#define CV64_CAMERA_MODE_BOSS           3
#define CV64_CAMERA_MODE_READING        4
#define CV64_CAMERA_MODE_FIRST_PERSON   5
#define CV64_CAMERA_MODE_CUTSCENE       6
#define CV64_CAMERA_MODE_LOCKED         7

/*===========================================================================
 * Camera Pointers (verified from CASTLEVANIA.sym)
 * sym: common_cameras_array at 0x8009B430 (array of camera ptrs)
 * sym: ptr_Camera_player at 0x8009B438
 *===========================================================================*/

#define CV64_ADDR_COMMON_CAMERAS_ARRAY  0x8009B430  /* sym: common_cameras_array */
#define CV64_ADDR_CAMERA_GAME_VIEW      0x8009B438  /* sym: ptr_Camera_player */

/* Offsets within Camera structure */
#define CV64_CAM_STRUCT_OFFSET_TYPE         0x00  /* s16 type */
#define CV64_CAM_STRUCT_OFFSET_FLAGS        0x02  /* u16 flags */
#define CV64_CAM_STRUCT_OFFSET_POSITION     0x40  /* Vec3f position */
#define CV64_CAM_STRUCT_OFFSET_ANGLE        0x52  /* Angle (s16 x3) */
#define CV64_CAM_STRUCT_OFFSET_LOOK_AT      0x58  /* Vec3f look_at_direction */
#define CV64_CAM_STRUCT_OFFSET_MATRIX       0x68  /* Mat4f matrix */

/*===========================================================================
 * Camera Functions (for hooking)
 *===========================================================================*/

#define CV64_FUNC_CAMERA_CREATE             0x80004C04
#define CV64_FUNC_CAMERA_SET_PARAMS         0x80004BB4
#define CV64_FUNC_CAMERA_MGR_LOOP_GAMEPLAY  0x80066CE0
#define CV64_FUNC_CAMERA_MGR_LOOP_CUTSCENE  0x80066E94
#define CV64_FUNC_CAMERA_MGR_CALC_STATE     0x80066F10
#define CV64_FUNC_CAMERA_MGR_CALC_COLLISION 0x80067B08
#define CV64_FUNC_CAMERA_MGR_ENTER_FP       0x800684F0
#define CV64_FUNC_CAMERA_MGR_LEAVE_FP       0x80068520
#define CV64_FUNC_CAMERA_MGR_FP_MAIN        0x800685E8
#define CV64_FUNC_CAMERA_MGR_SET_READING    0x800698A0

/*===========================================================================
 * Controller Functions
 *===========================================================================*/

#define CV64_FUNC_CONTROLLER_INIT           0x80016A60
#define CV64_FUNC_CONTROLLER_READ_DATA      0x80016B6C
#define CV64_FUNC_CONTROLLER_CLEAR_PRESSED  0x80016C88
#define CV64_FUNC_UPDATE_PLAYER_CONTROLS    0x800219B8

/*===========================================================================
 * Player Functions
 *===========================================================================*/

#define CV64_FUNC_PLAYER_UPDATE_ANIM        0x80022AC8
#define CV64_FUNC_PLAYER_ANIMATE_FRAME      0x80022C90
#define CV64_FUNC_PLAYER_CALC_PHYSICS       0x80023D88

/*===========================================================================
 * Save / Game State Data (within system_work)
 * Verified against CASTLEVANIA.sym decomp symbols and v1.0 USA cheat codes
 * Source: CASTLEVANIA.sym symbol table + mupencheat.txt (CRC 4BCDFF47)
 *===========================================================================*/

/* SaveStruct_gameplay base — sym: SaveStruct_gameplay */
#define CV64_ADDR_SAVE_STRUCT           0x80389BE4  /* system_work + 0x2612C */

/* Player character (s16): 0 = Reinhardt, 1 = Carrie
 * Verified: getCurrentCharacter() at 0x8003CA24 reads s16 from 0x80389C3C
 * ROM: LUI v0,0x8039 / JR RA / LH v0,-0x63C4(v0) */
#define CV64_ADDR_SAVE_CHARACTER        0x80389C3C  /* system_work + 0x26184 */

/* Player_health (u16) — sym: Player_health */
#define CV64_ADDR_SAVE_HEALTH           0x80389C3E  /* system_work + 0x26186 */

/* Player_max_health (u16) — follows Player_health in SaveStruct_gameplay */
#define CV64_ADDR_SAVE_MAX_HEALTH       0x80389C40  /* system_work + 0x26188 */

/* Sub-weapon type — cheat verified at 0x80389C42 */
#define CV64_ADDR_SAVE_SUBWEAPON        0x80389C42  /* system_work + 0x2618A */

/* Player_gold — sym: Player_gold */
#define CV64_ADDR_SAVE_GOLD             0x80389C44  /* system_work + 0x2618C */

/* Sub-weapon ammo (u16) — cheat: "Infinite Throwing Weapon" */
#define CV64_ADDR_SAVE_SUBWEAPON_AMMO   0x80389C48  /* system_work + 0x26190 */

/* Player_red_jewels — sym: Player_red_jewels */
#define CV64_ADDR_SAVE_JEWELS           0x80389C49  /* system_work + 0x26191 */

/* Inventory items start — cheat: "Have All Items" */
#define CV64_ADDR_SAVE_INVENTORY        0x80389C4A  /* system_work + 0x26192 */

/* Player status — cheat: 0x80389C88 */
#define CV64_ADDR_SAVE_STATUS           0x80389C88  /* system_work + 0x261D0 */

/* map_ID_copy — sym: map_ID_copy */
#define CV64_ADDR_SAVE_MAP              0x80389C90  /* system_work + 0x261D8 */

/* map_entrance_ID_copy — sym: map_entrance_ID_copy */
#define CV64_ADDR_SAVE_MAP_ENTRANCE     0x80389C92  /* system_work + 0x261DA */

/* PAUSE_MENU_IS_CLOSED — sym */
#define CV64_ADDR_PAUSE_CLOSED          0x80389CC4  /* system_work + 0x2620C */

/* save_difficulty — sym: save_difficulty */
#define CV64_ADDR_SAVE_DIFFICULTY       0x80389CC8  /* system_work + 0x26210 */

/* save_file_number — sym: save_file_number */
#define CV64_ADDR_SAVE_FILE_NUMBER      0x80389CDB  /* system_work + 0x26223 */

/* current_PowerUp_level — sym: current_PowerUp_level */
#define CV64_ADDR_SAVE_POWERUP          0x80389CEC  /* system_work + 0x26234 */

/* alternate_costume — sym: alternate_costume */
#define CV64_ADDR_SAVE_COSTUME          0x80389CEE  /* system_work + 0x26236 */

/* Door/gate flags (u16 array) — cheat: "Open All Doors" at 0x80389BD0 */
#define CV64_ADDR_SAVE_DOOR_FLAGS       0x80389BD0  /* system_work + 0x26118 */

/* map_ID (active) — sym: map_ID */
#define CV64_ADDR_MAP_ID                0x80389EE0  /* system_work + 0x26428 */

/* current_opened_menu — sym: current_opened_menu */
#define CV64_ADDR_CURRENT_MENU          0x80389EEE  /* system_work + 0x26436 */

/*===========================================================================
 * Player Object and Position Data (verified from CASTLEVANIA.sym)
 *===========================================================================*/

/* ptr_PlayerModule — sym: ptr_PlayerModule (N64 pointer) */
#define CV64_ADDR_PLAYER_MODULE_PTR     0x80389CDC  /* system_work + 0x26224 */

/* Player object pointer (N64 pointer to heap-allocated player actor) */
#define CV64_ADDR_PLAYER_PTR            0x80389CE0  /* system_work + 0x26228 */

/* PlayerData (fixed BSS address) — sym: PlayerData */
#define CV64_ADDR_PLAYER_DATA           0x80350798  /* Fixed address */

/* player_model_info (fixed BSS address) — sym: player_model_info
 * Position at offsets +0x40 (X), +0x44 (Y), +0x48 (Z)
 * Verified: modelInfo_setPosVec3s (0x80009330) stores to 0x40/0x44/0x48 */
#define CV64_ADDR_PLAYER_MODEL_INFO     0x80350990  /* Fixed address */

/* current_player_position (Vec3f) — sym: current_player_position
 * Cached copy: X at +0, Y at +4, Z at +8 */
#define CV64_ADDR_CURRENT_PLAYER_POS    0x8009E1B0  /* Fixed address */

/* ptr_PlayerData — sym: ptr_PlayerData
 * Used by getPlayerPosAndDirVectors (reads +0x0C then +0x40/0x44/0x48) */
#define CV64_ADDR_PTR_PLAYER_DATA       0x800D7AF8  /* Fixed address */

/* inPlayerGameplayLoop — sym: useful to check if player is in gameplay */
#define CV64_ADDR_IN_GAMEPLAY_LOOP      0x800D7980  /* u32, Fixed address */

/* Offsets within modelInfo structure (verified from ROM disassembly) */
#define CV64_MODEL_OFFSET_CHILD_PTR     0x000C  /* pointer to child/model data */
#define CV64_MODEL_OFFSET_POS_X         0x0040  /* f32 position X */
#define CV64_MODEL_OFFSET_POS_Y         0x0044  /* f32 position Y */
#define CV64_MODEL_OFFSET_POS_Z         0x0048  /* f32 position Z */

/*===========================================================================
 * Controller Data (verified from CASTLEVANIA.sym)
 *===========================================================================*/

/* playerControllerData — sym: playerControllerData */
#define CV64_ADDR_PLAYER_CONTROLLER     0x800D79B0  /* Fixed address */

/*===========================================================================
 * Game Time & Day/Night Cycle (verified from CASTLEVANIA.sym)
 * game_time increments each gameplay frame. Day/night transitions
 * are handled by isDaytime (0x801591E4) / isNightTime (0x80159214)
 *===========================================================================*/

#define CV64_ADDR_GAME_TIME             0x800A7900  /* u32: game_time (sym) */
#define CV64_ADDR_GAME_TIME_PREV        0x800A7904  /* u32: game_time_prev (sym) */

/*===========================================================================
 * Fixed BSS Manager Addresses (verified from CASTLEVANIA.sym)
 * These are critical game structures at fixed addresses — no pointer chasing
 *===========================================================================*/

#define CV64_ADDR_CAMERA_MGR            0x80342230  /* cameraMgr (fixed BSS) */
#define CV64_ADDR_GAMEPLAY_MGR          0x80342148  /* gameplayMgr (fixed BSS) */
#define CV64_ADDR_GAME_STATE_MGR        0x80342060  /* GameStateMgr (fixed BSS) */
#define CV64_ADDR_PLAYER_CAM_CTRL       0x8034272C  /* playerCameraController (fixed BSS) */
#define CV64_ADDR_HUD                   0x80342814  /* HUD module (fixed BSS) */
#define CV64_ADDR_3D_VIEW_CAMERA        0x8034ED58  /* _3d_view_camera (fixed BSS) */
#define CV64_ADDR_CUTSCENE_MGR          0x8034238C  /* cutsceneMgr (fixed BSS) */
#define CV64_ADDR_ENEMY_TARGET_GFX      0x80342644  /* enemyTargetGfx (fixed BSS) */
#define CV64_ADDR_PLAYER_PARAMS         0x80342BB4  /* player_params (fixed BSS) */
#define CV64_ADDR_HUD_PARAMETERS        0x80279B08  /* HUD_parameters (fixed BSS) */

/*===========================================================================
 * Camera State Flags (verified from CASTLEVANIA.sym)
 * Used for accurate gameplay state detection and camera mode awareness
 *===========================================================================*/

#define CV64_ADDR_IN_FIRST_PERSON       0x8009B4B8  /* s32: PLAYER_IS_IN_FIRST_PERSON_VIEW */
#define CV64_ADDR_FP_BTN_PRESSED        0x8009B4B4  /* s32: FIRST_PERSON_VIEW_BUTTON_IS_PRESSED */
#define CV64_ADDR_FP_HOLD_TIME          0x8009B4B0  /* s32: TIME_PLAYER_IS_HOLDING_FIRST_PERSON_BUTTON */
#define CV64_ADDR_PLAYER_IS_STILL       0x8009B4BC  /* s32: PLAYER_IS_STILL */
#define CV64_ADDR_R_LOCK_VIEW           0x8009B4CC  /* s32: PLAYER_IS_LOCKING_VIEW_WITH_R */
#define CV64_ADDR_TIME_IN_FP            0x8009B4D0  /* u16: TIME_IN_FIRST_PERSON_VIEW */
#define CV64_ADDR_FP_WHILE_DOOR         0x8009B4C0  /* s32: FIRST_PERSON_VIEW_WHILE_ENTERING_A_DOOR */

/*===========================================================================
 * Text / Cutscene State (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_READING_TEXT          0x80389CCE  /* PLAYER_IS_READING_TEXT */
#define CV64_ADDR_CAMERA_ZOOM_TEXT      0x80389CD2  /* CAMERA_ZOOMED_IN_WHEN_READING_TEXT */
#define CV64_ADDR_CUTSCENE_ID          0x80389EF8  /* cutscene_ID */
#define CV64_ADDR_CUTSCENE_FLAGS       0x80389F00  /* cutscene_flags */
#define CV64_ADDR_ENTRANCE_CUTSCENE    0x80389EFC  /* entrance_cutscene_ID */

/*===========================================================================
 * Fade / Visual State (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_FADE_SETTINGS        0x80387AD6  /* fade_settings */
#define CV64_ADDR_FADE_COLOR           0x80387AD8  /* fade_color (RGB) */
#define CV64_ADDR_FADE_ALPHA           0x80387ADB  /* fade_alpha */
#define CV64_ADDR_FADE_CURRENT_TIME    0x80387ADC  /* current_fade_time */
#define CV64_ADDR_FADE_MAX_TIME        0x80387ADE  /* max_fade_time */
#define CV64_ADDR_MAP_FADE_OUT_TIME    0x80389EE8  /* map_fade_out_time */
#define CV64_ADDR_MAP_FADE_IN_TIME     0x80389EEA  /* map_fade_in_time */
#define CV64_ADDR_MAP_FADE_IN_COLOR    0x80389EE4  /* map_fade_in_color */

/*===========================================================================
 * Fog & Lighting (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_MAP_FOG_COLOR        0x80387AC8  /* map_fog_color */
#define CV64_ADDR_FOG_DIST_START       0x80387AE0  /* fog_distance_start */
#define CV64_ADDR_FOG_DIST_END         0x80387AE2  /* fog_distance_end */
#define CV64_ADDR_MAP_AMBIENT_BRIGHT   0x80185F80  /* float: map_ambient_color_brightness */
#define CV64_ADDR_MAP_DIFFUSE_COLOR    0x80389ECC  /* map_diffuse_color */
#define CV64_ADDR_MAP_COLOR_SETTINGS   0x8018CDF8  /* map_color_settings */
#define CV64_ADDR_DONT_UPDATE_LIGHTING 0x80185F7C  /* u32: dont_update_map_lighting */

/*===========================================================================
 * Soft Reset (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_IS_SOFT_RESET        0x800947F8  /* u16: isSoftReset */
#define CV64_ADDR_SOFT_RESET_TIMER     0x800947FC  /* u16: softReset_timer */

/*===========================================================================
 * Moon / Skybox (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_MOON_VISIBILITY      0x8018CDD0  /* moonVisibility */
#define CV64_ADDR_DONT_UPDATE_MOON     0x8018CDD2  /* s16: dontUpdateMoonVisibility */

/*===========================================================================
 * Process Meter (verified from CASTLEVANIA.sym)
 * Used for frame budget diagnostics and lag reduction
 *===========================================================================*/

#define CV64_ADDR_PROCESS_BAR_DIVISOR   0x80096AC4  /* float: processBar_sizeDivisor */
#define CV64_ADDR_PROCESS_DIVISIONS     0x80096AC0  /* u32: processMeter_number_of_divisions */
#define CV64_ADDR_PROCESS_GREEN_SIZE    0x800D7140  /* float: processMeter_greenBarSize */
#define CV64_ADDR_PROCESS_BLUE_SIZE     0x800D7144  /* float: processMeter_blueBarSize */
#define CV64_ADDR_PROCESS_GREEN_BEGIN   0x800D7120  /* u64: processMeter_greenBar_beginTime */
#define CV64_ADDR_PROCESS_GREEN_END     0x800D7128  /* u64: processMeter_greenBar_endTime */
#define CV64_ADDR_PROCESS_BLUE_BEGIN    0x800D7130  /* u64: processMeter_blueBar_beginTime */
#define CV64_ADDR_PROCESS_BLUE_END      0x800D7138  /* u64: processMeter_blueBar_endTime */

/*===========================================================================
 * Boss Bar / HUD Graphics (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_BOSS_BAR_FILL_COLOR   0x801804B0  /* boss_bar_fill_color */
#define CV64_ADDR_HUD_CAMERA            0x8034ECB0  /* HUD_camera */

/*===========================================================================
 * Save File / Controller Pak (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_CONTPAK_FILE_NO       0x80389CDA  /* contPak_file_no */

/*===========================================================================
 * Enemy Target / Lock-On System (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_PTR_ENEMY_TARGET_GFX  0x800D5F18  /* ptr_enemyTargetGfx (N64 pointer) */
#define CV64_ADDR_ENEMY_TARGET_GFX      0x80342644  /* enemyTargetGfx (fixed BSS) */
#define CV64_ADDR_ENEMY_ITEM_DROPS      0x800973A0  /* enemy_item_drops_list */

/*===========================================================================
 * Item Pickables / Inventory (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_ITEM_PICKABLES_NAMES  0x8016CA80  /* item_pickables_names (lookup table) */
#define CV64_ADDR_ITEM_MODEL_SETTINGS   0x8016B2C8  /* item_model_settings_list */
#define CV64_ADDR_PICKABLE_ITEMS_FILE   0x80389ED8  /* pickable_items_assets_file_ID */
#define CV64_ADDR_PICKABLE_ITEMS_COLOR  0x80186020  /* pickable_items_color_settings */

/*===========================================================================
 * Misc Gameplay (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_ADDR_SEED_RNG             0x800973C0  /* u32: seed_RNG */
#define CV64_ADDR_HAS_MAX_HEALTH       0x80180490  /* u8: PLAYER_HAS_MAX_HEALTH */
#define CV64_ADDR_PLAYER_ANGLE         0x800D79CE  /* player_angle */
#define CV64_ADDR_JOYSTICK_ANGLE       0x800D79CC  /* joystick_angle */

/*===========================================================================
 * Game State Functions (verified from CASTLEVANIA.sym)
 *===========================================================================*/

#define CV64_FUNC_SETUP_FRAME               0x80000694  /* sym: setup_frame */
#define CV64_FUNC_UPDATE_GAMEPLAY_TIME      0x80018E74  /* sym: updateGameplayTime */
#define CV64_FUNC_UPDATE_GAME_SOUND         0x80012400  /* sym: updateGameSound */
#define CV64_FUNC_FIGURE_UPDATE             0x8000C740  /* sym: figure_update */
#define CV64_FUNC_FIGURE_UPDATE_MATRICES    0x8000C800  /* sym: figure_updateMatrices */
#define CV64_FUNC_GET_CURRENT_CHARACTER     0x8003CA24  /* sym: getCurrentCharacter */
#define CV64_FUNC_GET_PLAYER_CHARACTER      0x8006ACF0  /* sym: getPlayerCharacter */
#define CV64_FUNC_GET_PLAYER_POS_AND_DIR    0x80012338  /* sym: getPlayerPosAndDirVectors */
#define CV64_FUNC_GULOOKATF                 0x8008A6E0  /* sym: guLookAtF — N64 SDK view matrix */

/*===========================================================================
 * Camera Hook (MIPS stub injection)
 * We redirect the guLookAtF call inside figure_updateMatrices to our stub
 * at 0x80600000. The stub reads override eye position from shared RDRAM and
 * loads it into a1/a2/a3 before jumping to the real guLookAtF.
 *===========================================================================*/

#define CV64_ADDR_GULOOKATF_CALL_SITE       0x8000CA1C  /* JAL guLookAtF inside figure_updateMatrices */
#define CV64_ADDR_CAMERA_HOOK_STUB          0x80600000  /* Where our MIPS stub lives in RDRAM */
#define CV64_ADDR_CAMERA_HOOK_DATA          0x80600500  /* Shared data for stub ? C++ */
#define CV64_HOOK_DATA_OFF_EYE_X            0x00        /* f32 override eyeX */
#define CV64_HOOK_DATA_OFF_EYE_Y            0x04        /* f32 override eyeY */
#define CV64_HOOK_DATA_OFF_EYE_Z            0x08        /* f32 override eyeZ */
#define CV64_HOOK_DATA_OFF_ENABLED          0x0C        /* u32 enabled flag (0=off, 1=on) */
#define CV64_JAL_ORIGINAL_GULOOKATF         0x0C0229B8  /* JAL 0x8008A6E0 (original instruction) */
#define CV64_JAL_PATCHED_TO_STUB            0x0C180000  /* JAL 0x80600000 (patched instruction) */

/*===========================================================================
 * Angle Conversion Utilities
 * N64 uses 16-bit angles: 0x0000=0°, 0x4000=90°, 0x8000=180°, 0xC000=270°
 *===========================================================================*/

#define CV64_ANGLE_0        0x0000
#define CV64_ANGLE_45       0x2000
#define CV64_ANGLE_90       0x4000
#define CV64_ANGLE_135      0x6000
#define CV64_ANGLE_180      0x8000
#define CV64_ANGLE_225      0xA000
#define CV64_ANGLE_270      0xC000
#define CV64_ANGLE_315      0xE000

/* Convert N64 s16 angle to degrees */
static inline f32 CV64_AngleToDegrees(s16 angle) {
    return ((f32)angle / 65536.0f) * 360.0f;
}

/* Convert degrees to N64 s16 angle */
static inline s16 CV64_DegreesToAngle(f32 degrees) {
    return (s16)((degrees / 360.0f) * 65536.0f);
}

/* Convert N64 s16 angle to radians */
static inline f32 CV64_AngleToRadians(s16 angle) {
    return ((f32)angle / 65536.0f) * 6.28318530718f;  /* 2*PI */
}

/* Convert radians to N64 s16 angle */
static inline s16 CV64_RadiansToAngle(f32 radians) {
    return (s16)((radians / 6.28318530718f) * 65536.0f);
}

/*===========================================================================
 * Memory Access Structures (for reading from N64 RAM)
 *===========================================================================*/

#pragma pack(push, 1)

/* Controller structure as it appears in N64 RAM */
typedef struct CV64_N64_Controller {
    u16 is_connected;
    u16 btns_held;
    u16 btns_pressed;
    s16 joy_x;
    s16 joy_y;
    s16 joy_ang;
    s16 joy_held;
    u16 padding;
} CV64_N64_Controller;

/* Camera manager data as it appears in N64 RAM */
typedef struct CV64_N64_CameraMgrData {
    u32 player_camera_state;
    u32 camera_mode;
    u32 previous_camera_mode;
    u32 camera_player_ptr;      /* N64 pointer */
    u32 camera_ctrl_ptr;        /* N64 pointer */
    u32 camera_mode_text_ptr;   /* N64 pointer */
    f32 position_x;
    f32 position_y;
    f32 position_z;
    f32 look_at_x;
    f32 look_at_y;
    f32 look_at_z;
    /* ... more fields follow ... */
} CV64_N64_CameraMgrData;

#pragma pack(pop)

/*===========================================================================
 * Memory Access Helper Functions
 *===========================================================================*/

/**
 * @brief Validate RDRAM pointer and address before access
 * @param rdram Pointer to RDRAM buffer (must not be NULL)
 * @param addr N64 address (0x80000000 based or raw offset)
 * @param access_size Size of access in bytes
 * @param rdram_size Size of the RDRAM buffer
 * @return Pointer to the data, or NULL if invalid
 */
static inline void* CV64_ValidateAndGetPtr(u8* rdram, u32 addr, u32 access_size, u32 rdram_size) {
    if (!rdram) return NULL;
    if (rdram_size == 0) return NULL;
    
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + access_size > rdram_size) return NULL;
    
    return rdram + offset;
}

/**
 * @brief Read a value from N64 RDRAM (legacy compatibility wrapper)
 * @param rdram Pointer to RDRAM buffer
 * @param addr N64 address (0x80000000 based)
 * @return Pointer to the data, or NULL if out of range
 * @deprecated Use CV64_ValidateAndGetPtr instead for better safety
 */
static inline void* CV64_ReadN64Memory(u8* rdram, u32 addr) {
    if (!rdram) return NULL;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset < N64_RDRAM_SIZE) {
        return rdram + offset;
    }
    return NULL;
}

/**
 * @brief Read 32-bit value from N64 RDRAM with safety checks
 * mupen64plus stores RDRAM words in host byte order, so 32-bit reads
 * are a direct dereference (no byte swap needed).
 */
static inline u32 CV64_ReadU32(u8* rdram, u32 addr) {
    if (!rdram) return 0;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 4 > N64_RDRAM_SIZE) return 0;
    /* 32-bit aligned access — host order matches N64 word value */
    return *(u32*)(rdram + offset);
}

/**
 * @brief Read 16-bit value from N64 RDRAM with safety checks
 * mupen64plus stores words in host order; to access a 16-bit sub-field
 * we XOR the byte offset with 2 (S16 on little-endian hosts).
 */
static inline u16 CV64_ReadU16(u8* rdram, u32 addr) {
    if (!rdram) return 0;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 2 > N64_RDRAM_SIZE) return 0;
    return *(u16*)(rdram + (offset ^ 2));
}

/**
 * @brief Read 8-bit value from N64 RDRAM with safety checks
 * XOR offset with 3 (S8 on little-endian hosts) for correct byte lane.
 */
static inline u8 CV64_ReadU8(u8* rdram, u32 addr) {
    if (!rdram) return 0;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 1 > N64_RDRAM_SIZE) return 0;
    return *(u8*)(rdram + (offset ^ 3));
}

/**
 * @brief Read float from N64 RDRAM with safety checks
 * Float bits are stored as a 32-bit word in host order.
 */
static inline f32 CV64_ReadF32(u8* rdram, u32 addr) {
    u32 val = CV64_ReadU32(rdram, addr);
    f32 result;
    memcpy(&result, &val, sizeof(f32));  /* Safe type-punning */
    return result;
}

/**
 * @brief Write 32-bit value to N64 RDRAM with safety checks
 * mupen64plus stores words in host order — direct store, no byte swap.
 */
static inline bool CV64_WriteU32(u8* rdram, u32 addr, u32 value) {
    if (!rdram) return false;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 4 > N64_RDRAM_SIZE) return false;
    *(u32*)(rdram + offset) = value;
    return true;
}

/**
 * @brief Write 16-bit value to N64 RDRAM with safety checks
 * XOR offset with 2 (S16) for correct half-word lane.
 */
static inline bool CV64_WriteU16(u8* rdram, u32 addr, u16 value) {
    if (!rdram) return false;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 2 > N64_RDRAM_SIZE) return false;
    *(u16*)(rdram + (offset ^ 2)) = value;
    return true;
}

/**
 * @brief Write 8-bit value to N64 RDRAM with safety checks
 * XOR offset with 3 (S8) for correct byte lane.
 */
static inline bool CV64_WriteU8(u8* rdram, u32 addr, u8 value) {
    if (!rdram) return false;
    u32 offset = N64_ADDR_TO_OFFSET(addr);
    if (offset + 1 > N64_RDRAM_SIZE) return false;
    *(u8*)(rdram + (offset ^ 3)) = value;
    return true;
}

/**
 * @brief Write float to N64 RDRAM with safety checks
 * Float bits stored as a 32-bit word in host order.
 */
static inline bool CV64_WriteF32(u8* rdram, u32 addr, f32 value) {
    u32 val;
    memcpy(&val, &value, sizeof(u32));  /* Safe type-punning */
    return CV64_WriteU32(rdram, addr, val);
}

#ifdef __cplusplus
}
#endif

#endif /* CV64_MEMORY_MAP_H */
