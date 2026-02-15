/**
 * @file cv64_anim_interp.h
 * @brief Castlevania 64 PC Recomp - Animation Interpolation System
 * 
 * Provides smooth animation playback at 60fps (or any target framerate)
 * by interpolating between the game's native 30fps animation keyframes.
 * 
 * Architecture:
 *   - Game logic continues to tick at the original rate (30fps NTSC / 25fps PAL)
 *   - Each logic tick, bone transforms are captured as "previous" and "current"
 *   - At render time, bones are interpolated using a fractional alpha (0.0–1.0)
 *   - Interpolation is purely visual — hitboxes, timing, and gameplay are unaffected
 * 
 * Covers both animation data segments:
 *   - Player animations (0x67D7B0–0x697040, RAM 0x803B7B50)
 *   - NPC animations   (0x66C3D0–0x67D7B0, RAM 0x803A6770)
 * 
 * Gated by CV64_PATCH_FRAMERATE_UNLOCK — interpolation is only active when
 * the framerate unlock patch is enabled.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_ANIM_INTERP_H
#define CV64_ANIM_INTERP_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Constants
 *===========================================================================*/

/** Maximum bones per skeleton (matches CV64 decomp hierarchy limit) */
#define CV64_ANIM_MAX_BONES         64

/** Maximum interpolated entities tracked simultaneously */
#define CV64_ANIM_MAX_ENTITIES      128

/** Original game tick rate (NTSC) */
#define CV64_ANIM_NATIVE_TICKRATE   30.0

/** Default target render rate */
#define CV64_ANIM_DEFAULT_TARGET_FPS 60.0

/*===========================================================================
 * Bone Transform
 *===========================================================================*/

/**
 * @brief Per-bone transform (position + rotation).
 * 
 * Matches the decomp's skeletal hierarchy format. Rotations are stored
 * as Euler angles in the same s16 format the N64 uses internally.
 */
typedef struct CV64_BoneTransform {
    Vec3f   position;       /**< Bone local position */
    s16     rot_x;          /**< Rotation X (N64 s16 angle units) */
    s16     rot_y;          /**< Rotation Y */
    s16     rot_z;          /**< Rotation Z */
    s16     pad;            /**< Alignment padding */
    Vec3f   scale;          /**< Bone local scale (usually 1,1,1) */
} CV64_BoneTransform;

/*===========================================================================
 * Skeleton Snapshot — one frame of an entire skeleton
 *===========================================================================*/

typedef struct CV64_SkeletonSnapshot {
    Vec3f               root_position;      /**< World-space root position */
    s16                 root_rot_x;         /**< Root rotation X */
    s16                 root_rot_y;         /**< Root rotation Y */
    s16                 root_rot_z;         /**< Root rotation Z */
    s16                 pad;
    u32                 bone_count;         /**< Number of active bones */
    CV64_BoneTransform  bones[CV64_ANIM_MAX_BONES];
} CV64_SkeletonSnapshot;

/*===========================================================================
 * Interpolated Entity
 *===========================================================================*/

/**
 * @brief Tracks the previous/current skeleton state for one entity
 *        and produces an interpolated result at render time.
 */
typedef struct CV64_AnimInterpEntity {
    u32                     entity_id;      /**< N64 actor/object pointer used as key */
    bool                    active;         /**< Slot in use */
    bool                    valid;          /**< Has at least 2 frames captured */
    u32                     tick_captured;  /**< Game tick of last capture */

    CV64_SkeletonSnapshot   prev;           /**< Previous logic-tick snapshot */
    CV64_SkeletonSnapshot   curr;           /**< Current logic-tick snapshot */
    CV64_SkeletonSnapshot   rendered;       /**< Interpolated output (read at render time) */
} CV64_AnimInterpEntity;

/*===========================================================================
 * Interpolation Configuration
 *===========================================================================*/

typedef struct CV64_AnimInterpConfig {
    bool    enabled;                /**< Master enable (tied to FRAMERATE_UNLOCK) */
    f64     target_fps;             /**< Target render framerate */
    bool    interp_position;        /**< Interpolate root position */
    bool    interp_rotation;        /**< Interpolate bone rotations */
    bool    interp_scale;           /**< Interpolate bone scales */
    bool    interp_camera;          /**< Also interpolate camera transforms */
    f32     blend_sharpness;        /**< 0.0 = full lerp, 1.0 = snap (default 0.0) */
} CV64_AnimInterpConfig;

/*===========================================================================
 * System API
 *===========================================================================*/

/**
 * @brief Initialize the animation interpolation system.
 *        Call once at startup after CV64_Patches_Init().
 * @return TRUE on success
 */
CV64_API bool CV64_AnimInterp_Init(void);

/**
 * @brief Shutdown and free all resources.
 */
CV64_API void CV64_AnimInterp_Shutdown(void);

/**
 * @brief Get / set configuration.
 */
CV64_API CV64_AnimInterpConfig* CV64_AnimInterp_GetConfig(void);

/*===========================================================================
 * Per-Frame API (called from the main loop)
 *===========================================================================*/

/**
 * @brief Signal that a game logic tick has just completed.
 * 
 * Call this once per game-logic tick (30 Hz). Internally advances
 * the tick counter so subsequent Capture calls are associated with
 * the new tick.
 */
CV64_API void CV64_AnimInterp_OnLogicTick(void);

/**
 * @brief Capture the current skeleton pose for an entity.
 * 
 * Should be called once per entity per logic tick, after the game
 * has finished updating the entity's animation. Internally shifts
 * curr → prev and writes the new data into curr.
 * 
 * @param entity_id  Unique entity identifier (typically the N64 actor pointer)
 * @param bone_count Number of bones in the skeleton
 * @param bones      Array of bone_count transforms
 * @param root_pos   World-space root position
 * @param root_rot_x Root rotation X
 * @param root_rot_y Root rotation Y
 * @param root_rot_z Root rotation Z
 */
CV64_API void CV64_AnimInterp_Capture(
    u32                     entity_id,
    u32                     bone_count,
    const CV64_BoneTransform* bones,
    const Vec3f*            root_pos,
    s16                     root_rot_x,
    s16                     root_rot_y,
    s16                     root_rot_z
);

/**
 * @brief Compute interpolated poses for all tracked entities.
 * 
 * Call once per render frame, before drawing. Uses the fractional
 * time between the last two logic ticks to produce smooth in-between
 * poses.
 * 
 * @param alpha Interpolation factor (0.0 = prev tick, 1.0 = curr tick).
 *              Typically: accumulated_time / tick_duration
 */
CV64_API void CV64_AnimInterp_Update(f32 alpha);

/**
 * @brief Get the interpolated skeleton for an entity.
 * 
 * Call during rendering to retrieve the smooth pose.
 * Returns NULL if the entity is not tracked or doesn't have
 * enough frames for interpolation yet.
 * 
 * @param entity_id Entity identifier
 * @return Interpolated skeleton snapshot, or NULL
 */
CV64_API const CV64_SkeletonSnapshot* CV64_AnimInterp_GetPose(u32 entity_id);

/*===========================================================================
 * Entity Management
 *===========================================================================*/

/**
 * @brief Remove an entity from tracking.
 * 
 * Call when an actor is destroyed or leaves the scene.
 * 
 * @param entity_id Entity identifier
 */
CV64_API void CV64_AnimInterp_RemoveEntity(u32 entity_id);

/**
 * @brief Remove all tracked entities.
 * 
 * Call on map transitions / overlay swaps to flush stale data.
 */
CV64_API void CV64_AnimInterp_RemoveAll(void);

/**
 * @brief Get the number of currently tracked entities.
 * @return Active entity count
 */
CV64_API u32 CV64_AnimInterp_GetEntityCount(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_ANIM_INTERP_H */
