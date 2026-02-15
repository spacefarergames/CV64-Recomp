/**
 * @file cv64_memory_hook.h
 * @brief Castlevania 64 PC Recomp - Memory Hook API
 * 
 * Interface for reading/writing N64 RDRAM to hook into game state.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_MEMORY_HOOK_H
#define CV64_MEMORY_HOOK_H

#include "cv64_types.h"
#include "cv64_memory_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Memory Initialization
 *===========================================================================*/

/**
 * @brief Set the RDRAM pointer from mupen64plus
 * @param rdram Pointer to emulated N64 RDRAM
 * @param size Size of RDRAM (typically 4MB or 8MB with Expansion Pak)
 */
CV64_API void CV64_Memory_SetRDRAM(u8* rdram, u32 size);

/**
 * @brief Get the current RDRAM pointer
 */
CV64_API u8* CV64_Memory_GetRDRAM(void);

/**
 * @brief Get the current RDRAM size
 * @return Size in bytes (0x400000 for 4MB, 0x800000 for 8MB)
 */
CV64_API u32 CV64_Memory_GetRDRAMSize(void);

/**
 * @brief Check if memory system is initialized
 */
CV64_API bool CV64_Memory_IsInitialized(void);

/*===========================================================================
 * Controller Access
 *===========================================================================*/

/**
 * @brief Read controller state from N64 memory
 * @param controller_id Controller index (0-3)
 * @param out Output controller state
 * @return true on success
 */
CV64_API bool CV64_Memory_ReadController(u32 controller_id, CV64_N64_Controller* out);

/**
 * @brief Write controller state to N64 memory
 * @param controller_id Controller index (0-3)
 * @param in Controller state to write
 * @return true on success
 */
CV64_API bool CV64_Memory_WriteController(u32 controller_id, const CV64_N64_Controller* in);

/*===========================================================================
 * Camera Access
 *===========================================================================*/

/**
 * @brief Read camera manager data
 * @param out Output camera data
 * @return true on success
 */
CV64_API bool CV64_Memory_ReadCameraData(CV64_N64_CameraMgrData* out);

/**
 * @brief Write camera position to game
 */
CV64_API bool CV64_Memory_WriteCameraPosition(f32 x, f32 y, f32 z);

/**
 * @brief Write camera look-at target to game
 */
CV64_API bool CV64_Memory_WriteCameraLookAt(f32 x, f32 y, f32 z);

/**
 * @brief Get player's current yaw angle
 */
CV64_API s32 CV64_Memory_ReadPlayerYaw(void);

/**
 * @brief Write player camera yaw angle directly to game camera data
 * @param yaw N64 binary angle (0x0000-0xFFFF = 0-360 degrees, stored as s32)
 * @return true on success
 */
CV64_API bool CV64_Memory_WritePlayerYaw(s32 yaw);

/**
 * @brief Accumulate a yaw offset for mode-0 free camera rotation.
 *        Applied automatically each frame via FrameUpdate when camera mode is 0.
 * @param delta Binary angle delta to add (positive = left, negative = right)
 */
CV64_API void CV64_Memory_AddCameraYawOffset(s32 delta);

/**
 * @brief Reset the accumulated camera yaw offset to zero.
 */
CV64_API void CV64_Memory_ResetCameraYawOffset(void);

/**
 * @brief Accumulate a camera zoom offset for distance adjustment.
 *        Applied automatically each frame via FrameUpdate.
 * @param delta Zoom delta (positive = farther away, negative = closer)
 */
CV64_API void CV64_Memory_AddCameraZoomOffset(f32 delta);

/**
 * @brief Reset the accumulated camera zoom offset to default (1.0x).
 */
CV64_API void CV64_Memory_ResetCameraZoomOffset(void);

/**
 * @brief Get the current camera zoom multiplier.
 * @return Zoom multiplier (1.0 = default, <1.0 = closer, >1.0 = farther)
 */
CV64_API f32 CV64_Memory_GetCameraZoomMultiplier(void);

/**
 * @brief Get current camera mode
 */
CV64_API u32 CV64_Memory_GetCameraMode(void);

/*===========================================================================
 * Game State
 *===========================================================================*/

/**
 * @brief Check if currently in gameplay (not menu/cutscene)
 */
CV64_API bool CV64_Memory_IsInGameplay(void);

/**
 * @brief Check if D-PAD should control camera
 */
CV64_API bool CV64_Memory_ShouldDPadControlCamera(void);

/*===========================================================================
 * Frame Update
 *===========================================================================*/

/**
 * @brief Update memory hooks each frame
 * Call this from the main emulation frame callback
 */
CV64_API void CV64_Memory_FrameUpdate(void);

/*===========================================================================
 * Gameshark Cheats
 *===========================================================================*/

/**
 * @brief Enable/disable camera patch cheat
 * 
 * This enables the CV64 Recomp camera control system:
 *   - D-PAD controls camera rotation
 *   - Right analog stick controls camera (modern controllers)
 *   - Mouse camera control (if enabled in settings)
 * 
 * This is the MAIN FEATURE of CV64 Recomp!
 * Enabled by default.
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetCameraPatchCheat(bool enabled);

/**
 * @brief Check if camera patch cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsCameraPatchCheatEnabled(void);

/**
 * @brief Enable/disable lag reduction cheat
 * 
 * Gameshark codes to reduce lag frames when many objects/enemies on screen:
 *   800FAFB0 0000  - Reduce object processing lag threshold
 *   800FAFB2 0001  - Force single-frame processing
 *   800F8E8C 0000  - Disable particle lag accumulation
 * 
 * This helps maintain stable FPS in areas with many enemies like:
 *   - Forest of Silence (werewolves)
 *   - Castle Center (bone pillars, zombies)
 *   - Tower areas (multiple enemies)
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetLagReductionCheat(bool enabled);

/**
 * @brief Check if lag reduction cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsLagReductionCheatEnabled(void);

/**
 * @brief Enable/disable Forest of Silence FPS fix
 * 
 * Gameshark codes specifically for Forest of Silence area optimization:
 *   80073850 0001  - Reduce fog computation frequency
 *   80073852 0002  - Limit distant object rendering
 *   8007386C 0010  - Reduce shadow calculations
 *   8009D280 0000  - Disable redundant background updates
 * 
 * The Forest of Silence has notorious FPS drops due to:
 *   - Dense fog calculations
 *   - Large open areas with many trees
 *   - Wolf enemy AI and pathfinding
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetForestFpsFixCheat(bool enabled);

/**
 * @brief Check if Forest FPS fix cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsForestFpsFixCheatEnabled(void);

/**
 * @brief Enable/disable infinite health cheat
 * 
 * Gameshark code:
 *   8109D0CC 0064  - Set HP to 100
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetInfiniteHealthCheat(bool enabled);

/**
 * @brief Check if infinite health cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsInfiniteHealthCheatEnabled(void);

/**
 * @brief Enable/disable infinite sub-weapon cheat
 * 
 * Gameshark code:
 *   8009D0E0 0063  - Set sub-weapon ammo to 99
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetInfiniteSubweaponCheat(bool enabled);

/**
 * @brief Check if infinite sub-weapon cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsInfiniteSubweaponCheatEnabled(void);

/**
 * @brief Enable/disable moon jump cheat
 * 
 * When L button is held, allows higher jumps.
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetMoonJumpCheat(bool enabled);

/**
 * @brief Check if moon jump cheat is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsMoonJumpCheatEnabled(void);

/*===========================================================================
 * Difficulty Toggle
 * Uses sym: save_difficulty at 0x80389CC8 (u8: 0=Easy, 1=Normal)
 *===========================================================================*/

/**
 * @brief Toggle game difficulty between Easy and Normal.
 * Writes directly to save_difficulty in RDRAM during gameplay.
 * @return The new difficulty (0=Easy, 1=Normal), or -1 on failure
 */
CV64_API s32 CV64_Memory_ToggleDifficulty(void);

/**
 * @brief Set game difficulty.
 * @param difficulty 0=Easy, 1=Normal
 * @return true on success
 */
CV64_API bool CV64_Memory_SetDifficulty(u8 difficulty);

/**
 * @brief Get the current difficulty.
 * @return 0=Easy, 1=Normal, or -1 if unavailable
 */
CV64_API s32 CV64_Memory_GetDifficulty(void);

/**
 * @brief Get a human-readable difficulty name.
 * @param difficulty Difficulty value (0 or 1)
 * @return "Easy", "Normal", or "Unknown"
 */
CV64_API const char* CV64_Memory_GetDifficultyName(u8 difficulty);

/**
 * @brief Enable/disable extended draw distance
 * 
 * Pushes fog start and end distances further out to increase visibility.
 * Each map's original fog values are captured on first entry, then scaled
 * by the draw distance multiplier (default 2.5x).
 * 
 * This lets you see much further in all areas, especially effective in:
 *   - Forest of Silence (removes close fog)
 *   - Castle Wall (see more of the approach)
 *   - Castle Keep (wider view)
 * 
 * @param enabled true to enable, false to disable
 */
CV64_API void CV64_Memory_SetExtendedDrawDistance(bool enabled);

/**
 * @brief Check if extended draw distance is enabled
 * @return true if enabled
 */
CV64_API bool CV64_Memory_IsExtendedDrawDistanceEnabled(void);

/**
 * @brief Set the draw distance multiplier
 * Controls how much farther fog is pushed out (1.0 = normal, 2.5 = default enhanced).
 * Clamped to range [1.0, 10.0].
 * @param multiplier Fog distance scale factor
 */
CV64_API void CV64_Memory_SetDrawDistanceMultiplier(f32 multiplier);

/**
 * @brief Get the current draw distance multiplier
 * @return Current multiplier value
 */
CV64_API f32 CV64_Memory_GetDrawDistanceMultiplier(void);

/**
 * @brief Apply all cheats based on current cheat settings
 * Called automatically during CV64_Memory_FrameUpdate
 */
CV64_API void CV64_Memory_ApplyAllCheats(void);

/*===========================================================================
 * Game Info (for Window Title Display & PC HUD)
 *===========================================================================*/

/**
 * @brief Detailed gameplay state enumeration
 */
typedef enum CV64_GameState {
    CV64_STATE_NOT_RUNNING = 0,     /**< Emulator not started / RDRAM unavailable */
    CV64_STATE_TITLE_SCREEN,        /**< Map 0x00 — Title Screen */
    CV64_STATE_CHARACTER_SELECT,    /**< Map 0x01 — Character Select */
    CV64_STATE_GAMEPLAY,            /**< Active gameplay (player can move) */
    CV64_STATE_CUTSCENE,            /**< Cutscene playing */
    CV64_STATE_MENU,                /**< Pause menu / inventory open */
    CV64_STATE_READING_TEXT,        /**< Reading a sign / NPC text */
    CV64_STATE_FIRST_PERSON,        /**< First-person view mode (C-Up) */
    CV64_STATE_DOOR_TRANSITION,     /**< Walking through a door */
    CV64_STATE_FADING,              /**< Screen fade in/out (map transition) */
    CV64_STATE_SOFT_RESET,          /**< Soft reset in progress */
} CV64_GameState;

/**
 * @brief Subweapon type enumeration
 */
typedef enum CV64_SubweaponType {
    CV64_SUBWEAPON_NONE = 0,
    CV64_SUBWEAPON_KNIFE = 1,
    CV64_SUBWEAPON_HOLY_WATER = 2,
    CV64_SUBWEAPON_CROSS = 3,
    CV64_SUBWEAPON_AXE = 4,
} CV64_SubweaponType;

/**
 * @brief Game information structure for window title display
 */
typedef struct CV64_GameInfo {
    /* Identity */
    char mapName[64];           /**< Current map/stage name */
    char characterName[32];     /**< Player character name (Reinhardt/Carrie) */
    s16 mapId;                  /**< Current map ID */
    s16 mapEntrance;            /**< Current map entrance ID */

    /* Player stats (verified from CASTLEVANIA.sym) */
    s16 health;                 /**< Player current health (-1 if unavailable) */
    s16 maxHealth;              /**< Player max health (-1 if unavailable) */
    u32 gold;                   /**< Player gold (sym: Player_gold) */
    u8  jewels;                 /**< Red jewels (sym: Player_red_jewels) */
    u16 subweapon;              /**< Current subweapon type */
    u16 subweaponAmmo;          /**< Current subweapon ammo count */
    u8  powerupLevel;           /**< current_PowerUp_level */
    u8  alternateCostume;       /**< alternate_costume */
    u8  difficulty;             /**< 0=Easy, 1=Normal */
    u8  saveFileNumber;         /**< Current save file number */

    /* Time */
    u32 gameTime;               /**< Raw game_time counter (sym: game_time) */
    bool isDaytime;             /**< True if daytime in-game */

    /* State */
    CV64_GameState gameState;   /**< Detailed game state */
    bool isInGameplay;          /**< True if in active gameplay (not menu/cutscene) */
    bool isSoftReset;           /**< True if soft reset in progress */
    u16  cutsceneId;            /**< Current cutscene ID (0 = none) */

    /* Camera */
    u32 cameraMode;             /**< Current camera mode (0-7) */
    bool isFirstPerson;         /**< True if in first-person view */
    bool isRLocked;             /**< True if camera locked with R button */

    /* Player position */
    f32 playerX;                /**< Player X from current_player_position */
    f32 playerY;                /**< Player Y */
    f32 playerZ;                /**< Player Z */

    /* Process meter (frame budget) */
    f32 processMeterGreen;      /**< CPU gameplay usage (0.0-1.0) */
    f32 processMeterBlue;       /**< CPU graphics usage (0.0-1.0) */

    /* Boss info */
    bool bossBarVisible;        /**< True if boss health bar is visible */

    /* Fog info */
    u16 fogDistStart;           /**< Fog start distance */
    u16 fogDistEnd;             /**< Fog end distance */
    f32 ambientBrightness;      /**< Map ambient brightness (0.0-1.0) */

    /* Controller pak */
    s16 contPakFileNo;          /**< contPak_file_no (-1 if unavailable) */

    /* Enemy target */
    bool isEnemyTargetActive;   /**< True if lock-on reticle is on an enemy */

    /* Player status flags */
    bool hasMaxHealth;          /**< True if PLAYER_HAS_MAX_HEALTH is set */
} CV64_GameInfo;

/**
 * @brief Get the name of a map by ID
 * @param mapId Map ID (0x00 - 0x1F)
 * @return Map name string, or "Unknown Area" if invalid
 */
CV64_API const char* CV64_Memory_GetMapName(s16 mapId);

/**
 * @brief Get the current map name
 * @return Current map name, or "Unknown Area" if not in game
 */
CV64_API const char* CV64_Memory_GetCurrentMapName(void);

/**
 * @brief Get the current map ID
 * @return Map ID (0x00-0x1F), or -1 if not available
 */
CV64_API s16 CV64_Memory_GetCurrentMapId(void);

/**
 * @brief Get player's current health
 * @return Current HP, or -1 if not available
 */
CV64_API s16 CV64_Memory_GetPlayerHealth(void);

/**
 * @brief Get player's max health
 * @return Max HP, or -1 if not available
 */
CV64_API s16 CV64_Memory_GetPlayerMaxHealth(void);

/**
 * @brief Get character name (Reinhardt or Carrie)
 * @return Character name string
 */
CV64_API const char* CV64_Memory_GetCharacterName(void);

/**
 * @brief Get full game info structure for window title display
 * @param info Output structure to fill
 * @return true if game is running and info is valid
 */
CV64_API bool CV64_Memory_GetGameInfo(CV64_GameInfo* info);

/*===========================================================================
 * Day/Night Cycle
 *===========================================================================*/

/**
 * @brief Get the raw game time counter
 * @return game_time value, or 0 if unavailable
 */
CV64_API u32 CV64_Memory_GetGameTime(void);

/**
 * @brief Check if it's currently daytime in-game
 * Uses the game's own day/night boundary from game_time
 * @return true if daytime
 */
CV64_API bool CV64_Memory_IsDaytime(void);

/**
 * @brief Get a human-readable time-of-day string
 * @return "Day", "Night", "Dawn", "Dusk", or "Unknown"
 */
CV64_API const char* CV64_Memory_GetTimeOfDayString(void);

/*===========================================================================
 * Enhanced Game State Detection
 *===========================================================================*/

/**
 * @brief Get the detailed game state
 * Multi-signal detection using inPlayerGameplayLoop, cutscene_ID,
 * current_opened_menu, PLAYER_IS_READING_TEXT, fade state, etc.
 * @return Current CV64_GameState
 */
CV64_API CV64_GameState CV64_Memory_GetGameState(void);

/**
 * @brief Get subweapon name string
 * @param type Subweapon type ID
 * @return "Knife", "Holy Water", "Cross", "Axe", or "None"
 */
CV64_API const char* CV64_Memory_GetSubweaponName(u16 type);

/**
 * @brief Check if a map transition is occurring (fading)
 * @return true if currently in a fade transition
 */
CV64_API bool CV64_Memory_IsMapTransition(void);

/**
 * @brief Callback type for map change events
 */
typedef void (*CV64_MapChangeCallback)(s16 oldMapId, s16 newMapId);

/**
 * @brief Register a callback for map change events
 * @param callback Function to call when map changes
 */
CV64_API void CV64_Memory_SetMapChangeCallback(CV64_MapChangeCallback callback);

/*===========================================================================
 * Process Meter (Frame Budget Diagnostics)
 * Uses verified sym addresses: processBar_sizeDivisor, greenBarSize, etc.
 *===========================================================================*/

/**
 * @brief Process meter frame budget information
 */
typedef struct CV64_ProcessMeterInfo {
    f32 greenBarSize;           /**< CPU usage for gameplay logic (0.0-1.0 scale) */
    f32 blueBarSize;            /**< CPU usage for graphics/RDP (0.0-1.0 scale) */
    f32 sizeDivisor;            /**< Frame budget divisor (higher = more budget) */
    u32 numDivisions;           /**< Number of time slice divisions */
} CV64_ProcessMeterInfo;

/**
 * @brief Read the process meter data for frame budget diagnostics
 * @param out Output structure to fill
 * @return true if data was read successfully
 */
CV64_API bool CV64_Memory_GetProcessMeter(CV64_ProcessMeterInfo* out);

/*===========================================================================
 * Boss Health Monitor
 * Uses sym: boss_bar_fill_color (0x801804B0), HUD_parameters (0x80279B08)
 *===========================================================================*/

/**
 * @brief Boss health bar information
 */
typedef struct CV64_BossHealthInfo {
    bool bossBarVisible;        /**< True if boss health bar is on screen */
    u32  bossBarFillColor;      /**< Boss bar fill color (RGBA, 0 = not visible) */
} CV64_BossHealthInfo;

/**
 * @brief Read boss health bar information
 * @param out Output structure to fill
 * @return true if data was read successfully
 */
CV64_API bool CV64_Memory_GetBossHealth(CV64_BossHealthInfo* out);

/*===========================================================================
 * Fog & Ambient Lighting Parameters
 * Uses verified sym addresses: map_fog_color, fog distances, ambient brightness
 *===========================================================================*/

/**
 * @brief Fog and ambient lighting parameters
 */
typedef struct CV64_FogInfo {
    u32  fogColor;              /**< map_fog_color (RGB packed) */
    u16  fogDistanceStart;      /**< fog_distance_start */
    u16  fogDistanceEnd;        /**< fog_distance_end */
    f32  ambientBrightness;     /**< map_ambient_color_brightness (0.0-1.0) */
    u32  diffuseColor;          /**< map_diffuse_color */
    bool lightingUpdateDisabled;/**< dont_update_map_lighting flag */
} CV64_FogInfo;

/**
 * @brief Read fog and ambient lighting parameters
 * @param out Output structure to fill
 * @return true if data was read successfully
 */
CV64_API bool CV64_Memory_GetFogInfo(CV64_FogInfo* out);

/*===========================================================================
 * Save File Monitoring
 *===========================================================================*/

/**
 * @brief Get the current controller pak file number
 * @return contPak_file_no value, or -1 if unavailable
 */
CV64_API s16 CV64_Memory_GetContPakFileNo(void);

/*===========================================================================
 * Enemy Target / Lock-On Awareness
 * Uses sym: ptr_enemyTargetGfx (0x800D5F18), enemyTargetGfx (0x80342644)
 *===========================================================================*/

/**
 * @brief Check if an enemy is currently locked on (target reticle visible)
 * @return true if an enemy target is active
 */
CV64_API bool CV64_Memory_IsEnemyTargetActive(void);

/**
 * @brief Get the enemy target graphics pointer
 * Non-zero when lock-on reticle is displayed on an enemy
 * @return N64 pointer to active target, or 0 if none
 */
CV64_API u32 CV64_Memory_GetEnemyTargetPtr(void);

/*===========================================================================
 * Player Status Flags
 *===========================================================================*/

/**
 * @brief Check if player currently has max health
 * Uses sym: PLAYER_HAS_MAX_HEALTH at 0x80180490
 * @return true if player has maximum health
 */
CV64_API bool CV64_Memory_PlayerHasMaxHealth(void);

/*===========================================================================
 * Camera Hook (MIPS Stub Injection)
 *
 * Installs a small MIPS stub into unused RDRAM at 0x80600000 that intercepts
 * the guLookAtF call inside figure_updateMatrices. When enabled, the stub
 * replaces the eye position arguments (a1/a2/a3) with values written by C++
 * before jumping to the real guLookAtF. This lets us control the camera at
 * the exact moment the view matrix is computed — the game cannot overwrite it.
 *===========================================================================*/

/**
 * @brief Install the MIPS stub into RDRAM and patch the JAL call site.
 * Should be called once when gameplay begins.
 * @return true if the stub was written and the JAL was patched successfully
 */
CV64_API bool CV64_Memory_InstallCameraHook(void);

/**
 * @brief Write the override eye position into shared RDRAM each frame.
 * The MIPS stub will pick up these values on the next guLookAtF call.
 * @param x Eye X position
 * @param y Eye Y position
 * @param z Eye Z position
 * @param enabled true to override camera, false to let the game control it
 */
CV64_API void CV64_Memory_UpdateCameraOverride(f32 x, f32 y, f32 z, bool enabled);

/*===========================================================================
 * Debug
 *===========================================================================*/

/**
 * @brief Dump current memory state to debug output
 */
CV64_API void CV64_Memory_DumpState(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_MEMORY_HOOK_H */
