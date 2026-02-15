/**
 * @file cv64_anim_bridge.h
 * @brief External interface for CV64AnimBridge callback registration
 * 
 * Allows the main application to register callbacks into the GLideN64
 * animation bridge without depending on GLideN64-internal headers.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_ANIM_BRIDGE_H
#define CV64_ANIM_BRIDGE_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invoked on each logic tick with per-actor bone data.
 */
typedef void (*CV64_AnimBridge_CaptureCallback)(
    u32 actor_addr, u8 num_limbs,
    const s16* limb_rx, const s16* limb_ry, const s16* limb_rz,
    f32 pos_x, f32 pos_y, f32 pos_z,
    s16 pitch, s16 yaw, s16 roll);

/** Callback invoked once per logic tick (before actor captures) */
typedef void (*CV64_AnimBridge_LogicTickCallback)(void);

/** Callback invoked when interpolated poses should be computed */
typedef void (*CV64_AnimBridge_UpdateCallback)(f32 alpha);

/** Callback invoked on map change to flush state */
typedef void (*CV64_AnimBridge_MapChangeCallback)(void);

/**
 * @brief Callback invoked during writeback to retrieve the interpolated
 *        pose computed by cv64_anim_interp.
 * @param actor_addr  N64 modelInfo address (entity ID)
 * @param num_limbs   Expected number of limbs
 * @param out_limb_rx Output array for per-limb rotation X (s16), size num_limbs
 * @param out_limb_ry Output array for per-limb rotation Y (s16), size num_limbs
 * @param out_limb_rz Output array for per-limb rotation Z (s16), size num_limbs
 * @param out_pos_x   Output root position X
 * @param out_pos_y   Output root position Y
 * @param out_pos_z   Output root position Z
 * @param out_pitch   Output root rotation pitch (s16)
 * @param out_yaw     Output root rotation yaw (s16)
 * @param out_roll    Output root rotation roll (s16)
 * @return true if pose was written to outputs, false to fall back to inline lerp
 */
typedef bool (*CV64_AnimBridge_GetPoseCallback)(
    u32 actor_addr, u8 num_limbs,
    s16* out_limb_rx, s16* out_limb_ry, s16* out_limb_rz,
    f32* out_pos_x, f32* out_pos_y, f32* out_pos_z,
    s16* out_pitch, s16* out_yaw, s16* out_roll);

/**
 * @brief Register callbacks into the animation bridge.
 * Pass NULL for any callback to leave it unregistered.
 */
void CV64_AnimBridge_SetCallbacks(
    CV64_AnimBridge_CaptureCallback   onCapture,
    CV64_AnimBridge_LogicTickCallback  onLogicTick,
    CV64_AnimBridge_UpdateCallback     onUpdate,
    CV64_AnimBridge_MapChangeCallback  onMapChange,
    CV64_AnimBridge_GetPoseCallback    onGetPose);

/**
 * @brief Enable or disable the animation bridge at runtime.
 * When disabled, no interpolation or writeback occurs.
 */
void CV64_AnimBridge_SetEnabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* CV64_ANIM_BRIDGE_H */
