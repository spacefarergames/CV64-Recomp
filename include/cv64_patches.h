/**
 * @file cv64_patches.h
 * @brief Castlevania 64 PC Recomp - Patch System
 * 
 * Central patch management system for all game enhancements.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_PATCHES_H
#define CV64_PATCHES_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Patch IDs
 *===========================================================================*/

typedef enum CV64_PatchID {
    CV64_PATCH_NONE = 0,
    
    /* Camera Patches */
    CV64_PATCH_DPAD_CAMERA,             /**< D-PAD camera control */
    CV64_PATCH_FREE_CAMERA,             /**< Free camera movement */
    CV64_PATCH_FIRST_PERSON_SMOOTH,     /**< Smooth first-person camera */
    
    /* Control Patches */
    CV64_PATCH_MODERN_CONTROLS,         /**< Modern twin-stick controls */
    CV64_PATCH_KEYBOARD_MOUSE,          /**< Full KB/M support */
    CV64_PATCH_ANALOG_MOVEMENT,         /**< Enhanced analog movement */
    
    /* Graphics Patches */
    CV64_PATCH_WIDESCREEN,              /**< Widescreen support */
    CV64_PATCH_HD_UI,                   /**< High-resolution UI */
    CV64_PATCH_ENHANCED_FOG,            /**< Better fog rendering */
    CV64_PATCH_DRAW_DISTANCE,           /**< Extended draw distance */
    CV64_PATCH_FRAMERATE_UNLOCK,        /**< Unlock framerate */
    
    /* Gameplay Patches */
    CV64_PATCH_SKIP_INTRO,              /**< Skip intro sequence */
    CV64_PATCH_FAST_TEXT,               /**< Speed up text display */
    CV64_PATCH_INSTANT_MAP,             /**< Instant map display */
    CV64_PATCH_SAVE_ANYWHERE,           /**< Save anywhere feature */
    
    /* Audio Patches */
    CV64_PATCH_CD_QUALITY_MUSIC,        /**< CD quality music */
    CV64_PATCH_ENHANCED_SFX,            /**< Enhanced sound effects */
    
    /* Bug Fixes */
    CV64_PATCH_FIX_SOFTLOCKS,           /**< Fix known softlocks */
    CV64_PATCH_FIX_COLLISION,           /**< Fix collision bugs */
    CV64_PATCH_FIX_CAMERA_CLIP,         /**< Fix camera clipping */
    
    CV64_PATCH_COUNT
} CV64_PatchID;

/*===========================================================================
 * Patch State
 *===========================================================================*/

typedef enum CV64_PatchState {
    CV64_PATCH_STATE_DISABLED = 0,
    CV64_PATCH_STATE_ENABLED,
    CV64_PATCH_STATE_ERROR,
    CV64_PATCH_STATE_INCOMPATIBLE
} CV64_PatchState;

/*===========================================================================
 * Patch Info Structure
 *===========================================================================*/

typedef struct CV64_PatchInfo {
    CV64_PatchID id;
    const char* name;
    const char* description;
    const char* author;
    const char* version;
    CV64_PatchState state;
    bool requires_restart;
    CV64_PatchID* dependencies;
    u32 dependency_count;
    CV64_PatchID* conflicts;
    u32 conflict_count;
} CV64_PatchInfo;

/*===========================================================================
 * Patch System API
 *===========================================================================*/

/**
 * @brief Initialize patch system
 * @return TRUE on success
 */
CV64_API bool CV64_Patches_Init(void);

/**
 * @brief Shutdown patch system
 */
CV64_API void CV64_Patches_Shutdown(void);

/**
 * @brief Enable a patch
 * @param patch_id Patch to enable
 * @return TRUE on success
 */
CV64_API bool CV64_Patches_Enable(CV64_PatchID patch_id);

/**
 * @brief Disable a patch
 * @param patch_id Patch to disable
 * @return TRUE on success
 */
CV64_API bool CV64_Patches_Disable(CV64_PatchID patch_id);

/**
 * @brief Check if a patch is enabled
 * @param patch_id Patch to check
 * @return TRUE if enabled
 */
CV64_API bool CV64_Patches_IsEnabled(CV64_PatchID patch_id);

/**
 * @brief Get patch information
 * @param patch_id Patch ID
 * @return Pointer to patch info, or NULL
 */
CV64_API const CV64_PatchInfo* CV64_Patches_GetInfo(CV64_PatchID patch_id);

/**
 * @brief Get all patch info
 * @param out_count Output number of patches
 * @return Array of patch info
 */
CV64_API const CV64_PatchInfo* CV64_Patches_GetAll(u32* out_count);

/**
 * @brief Apply all enabled patches
 * @return Number of patches applied
 */
CV64_API u32 CV64_Patches_ApplyAll(void);

/**
 * @brief Load patch configuration from file
 * @param filepath Path to config (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Patches_LoadConfig(const char* filepath);

/**
 * @brief Save patch configuration to file
 * @param filepath Path to config (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Patches_SaveConfig(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* CV64_PATCHES_H */
