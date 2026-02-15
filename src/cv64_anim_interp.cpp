/**
 * @file cv64_anim_interp.cpp
 * @brief Castlevania 64 PC Recomp - Animation Interpolation Implementation
 * 
 * Interpolates skeletal animation between 30fps game ticks to produce
 * smooth 60fps (or higher) visual output. Game logic is unaffected.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_anim_interp.h"
#include "../include/cv64_patches.h"
#include <string.h>
#include <math.h>
#include <Windows.h>

/*===========================================================================
 * Constants
 *===========================================================================*/

#define PI 3.14159265358979323846f

/** N64 s16 angle range: 0x0000–0xFFFF maps to 0–360 degrees */
#define S16_ANGLE_MAX 65536.0f

/*===========================================================================
 * Static State
 *===========================================================================*/

static CV64_AnimInterpConfig    s_config;
static CV64_AnimInterpEntity    s_entities[CV64_ANIM_MAX_ENTITIES];
static u32                      s_current_tick = 0;
static bool                     s_initialized = false;

/*===========================================================================
 * Logging
 *===========================================================================*/

static void LogInfo(const char* msg) {
    OutputDebugStringA("[CV64_AnimInterp] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Math Helpers
 *===========================================================================*/

/**
 * @brief Linear interpolation for floats
 */
static inline f32 lerp_f32(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

/**
 * @brief Interpolate an s16 angle along the shortest arc.
 * 
 * N64 angles wrap at 16-bit boundaries. This handles the wraparound
 * so that interpolation always takes the shortest path (e.g., from
 * 0xFFF0 to 0x0010 goes through 0x0000, not the long way around).
 */
static inline s16 lerp_angle_s16(s16 a, s16 b, f32 t) {
    /* Compute signed difference (wraps correctly via s16 overflow) */
    s16 diff = (s16)(b - a);
    return (s16)(a + (s16)(diff * t));
}

/**
 * @brief Interpolate a Vec3f
 */
static inline void lerp_vec3f(Vec3f* out, const Vec3f* a, const Vec3f* b, f32 t) {
    out->x = lerp_f32(a->x, b->x, t);
    out->y = lerp_f32(a->y, b->y, t);
    out->z = lerp_f32(a->z, b->z, t);
}

/*===========================================================================
 * Entity Slot Management
 *===========================================================================*/

/**
 * @brief Find an existing entity slot by ID
 * @return Slot pointer or NULL
 */
static CV64_AnimInterpEntity* find_entity(u32 entity_id) {
    for (u32 i = 0; i < CV64_ANIM_MAX_ENTITIES; i++) {
        if (s_entities[i].active && s_entities[i].entity_id == entity_id) {
            return &s_entities[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate a new entity slot
 * @return Slot pointer or NULL if full
 */
static CV64_AnimInterpEntity* alloc_entity(u32 entity_id) {
    for (u32 i = 0; i < CV64_ANIM_MAX_ENTITIES; i++) {
        if (!s_entities[i].active) {
            memset(&s_entities[i], 0, sizeof(CV64_AnimInterpEntity));
            s_entities[i].entity_id = entity_id;
            s_entities[i].active = true;
            return &s_entities[i];
        }
    }
    return NULL;
}

/*===========================================================================
 * Interpolation Core
 *===========================================================================*/

/**
 * @brief Interpolate a single bone transform
 */
static void interp_bone(CV64_BoneTransform* out,
                         const CV64_BoneTransform* prev,
                         const CV64_BoneTransform* curr,
                         f32 alpha,
                         const CV64_AnimInterpConfig* cfg)
{
    /* Position */
    if (cfg->interp_position) {
        lerp_vec3f(&out->position, &prev->position, &curr->position, alpha);
    } else {
        out->position = curr->position;
    }

    /* Rotation (shortest-arc s16 lerp) */
    if (cfg->interp_rotation) {
        out->rot_x = lerp_angle_s16(prev->rot_x, curr->rot_x, alpha);
        out->rot_y = lerp_angle_s16(prev->rot_y, curr->rot_y, alpha);
        out->rot_z = lerp_angle_s16(prev->rot_z, curr->rot_z, alpha);
    } else {
        out->rot_x = curr->rot_x;
        out->rot_y = curr->rot_y;
        out->rot_z = curr->rot_z;
    }

    /* Scale */
    if (cfg->interp_scale) {
        lerp_vec3f(&out->scale, &prev->scale, &curr->scale, alpha);
    } else {
        out->scale = curr->scale;
    }

    out->pad = 0;
}

/**
 * @brief Interpolate an entire skeleton snapshot
 */
static void interp_skeleton(CV64_SkeletonSnapshot* out,
                             const CV64_SkeletonSnapshot* prev,
                             const CV64_SkeletonSnapshot* curr,
                             f32 alpha,
                             const CV64_AnimInterpConfig* cfg)
{
    /* Use bone count from current frame */
    u32 bone_count = curr->bone_count;
    if (bone_count > CV64_ANIM_MAX_BONES) {
        bone_count = CV64_ANIM_MAX_BONES;
    }

    /* If prev has different bone count, don't interpolate — just snap */
    if (prev->bone_count != curr->bone_count) {
        memcpy(out, curr, sizeof(CV64_SkeletonSnapshot));
        return;
    }

    out->bone_count = bone_count;

    /* Root transform */
    if (cfg->interp_position) {
        lerp_vec3f(&out->root_position, &prev->root_position, &curr->root_position, alpha);
    } else {
        out->root_position = curr->root_position;
    }

    if (cfg->interp_rotation) {
        out->root_rot_x = lerp_angle_s16(prev->root_rot_x, curr->root_rot_x, alpha);
        out->root_rot_y = lerp_angle_s16(prev->root_rot_y, curr->root_rot_y, alpha);
        out->root_rot_z = lerp_angle_s16(prev->root_rot_z, curr->root_rot_z, alpha);
    } else {
        out->root_rot_x = curr->root_rot_x;
        out->root_rot_y = curr->root_rot_y;
        out->root_rot_z = curr->root_rot_z;
    }

    out->pad = 0;

    /* Per-bone interpolation */
    for (u32 i = 0; i < bone_count; i++) {
        interp_bone(&out->bones[i], &prev->bones[i], &curr->bones[i], alpha, cfg);
    }
}

/*===========================================================================
 * Public API — System Lifecycle
 *===========================================================================*/

bool CV64_AnimInterp_Init(void) {
    if (s_initialized) {
        return true;
    }

    memset(s_entities, 0, sizeof(s_entities));
    s_current_tick = 0;

    /* Default configuration */
    s_config.enabled          = true;
    s_config.target_fps       = CV64_ANIM_DEFAULT_TARGET_FPS;
    s_config.interp_position  = true;
    s_config.interp_rotation  = true;
    s_config.interp_scale     = true;
    s_config.interp_camera    = true;
    s_config.blend_sharpness  = 0.0f;

    s_initialized = true;
    LogInfo("Animation interpolation system initialized");
    return true;
}

void CV64_AnimInterp_Shutdown(void) {
    if (!s_initialized) {
        return;
    }

    memset(s_entities, 0, sizeof(s_entities));
    s_initialized = false;
    LogInfo("Animation interpolation system shut down");
}

CV64_AnimInterpConfig* CV64_AnimInterp_GetConfig(void) {
    return &s_config;
}

/*===========================================================================
 * Public API — Per-Frame
 *===========================================================================*/

void CV64_AnimInterp_OnLogicTick(void) {
    if (!s_initialized) {
        return;
    }
    s_current_tick++;
}

void CV64_AnimInterp_Capture(
    u32                         entity_id,
    u32                         bone_count,
    const CV64_BoneTransform*   bones,
    const Vec3f*                root_pos,
    s16                         root_rot_x,
    s16                         root_rot_y,
    s16                         root_rot_z)
{
    if (!s_initialized || !s_config.enabled) {
        return;
    }

    if (!bones || bone_count == 0) {
        return;
    }

    // Hard clamp to prevent memory corruption if caller passes too many bones
    if (bone_count > CV64_ANIM_MAX_BONES) {
        bone_count = CV64_ANIM_MAX_BONES;
    }

    /* Check if FRAMERATE_UNLOCK is active — no point interpolating at native rate */
    if (!CV64_Patches_IsEnabled(CV64_PATCH_FRAMERATE_UNLOCK)) {
        return;
    }

    /* Find or allocate slot */
    CV64_AnimInterpEntity* ent = find_entity(entity_id);
    if (!ent) {
        ent = alloc_entity(entity_id);
        if (!ent) {
            return; /* Table full */
        }
    }

    /* Shift curr → prev */
    memcpy(&ent->prev, &ent->curr, sizeof(CV64_SkeletonSnapshot));

    // Safety clamp (double‑defense)
    u32 safe_count = bone_count;
    if (safe_count > CV64_ANIM_MAX_BONES) {
        safe_count = CV64_ANIM_MAX_BONES;
    }

    ent->curr.bone_count = safe_count;
    memcpy(ent->curr.bones, bones, safe_count * sizeof(CV64_BoneTransform));


    if (root_pos) {
        ent->curr.root_position = *root_pos;
    }
    ent->curr.root_rot_x = root_rot_x;
    ent->curr.root_rot_y = root_rot_y;
    ent->curr.root_rot_z = root_rot_z;
    ent->curr.pad = 0;

    /* Mark valid once we have at least 2 captures */
    if (ent->tick_captured > 0) {
        ent->valid = true;
    }
    ent->tick_captured = s_current_tick;
}

void CV64_AnimInterp_Update(f32 alpha) {
    if (!s_initialized || !s_config.enabled) {
        return;
    }

    if (!CV64_Patches_IsEnabled(CV64_PATCH_FRAMERATE_UNLOCK)) {
        return;
    }

    /* Apply blend sharpness: push alpha toward 0 or 1 */
    if (s_config.blend_sharpness > 0.0f) {
        f32 sharp = s_config.blend_sharpness;
        /* Bias alpha toward nearest integer */
        if (alpha < 0.5f) {
            alpha = alpha * (1.0f - sharp);
        } else {
            alpha = 1.0f - (1.0f - alpha) * (1.0f - sharp);
        }
    }

    /* Clamp alpha */
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    for (u32 i = 0; i < CV64_ANIM_MAX_ENTITIES; i++) {
        // Ensure no stale or corrupted slot can be used
        if (i >= CV64_ANIM_MAX_ENTITIES) break;
        CV64_AnimInterpEntity* ent = &s_entities[i];
        if (!ent->active || !ent->valid) {
            continue;
        }

        /* Stale entity — hasn't been captured in a while, skip */
        if (s_current_tick > ent->tick_captured + 2) {
            continue;
        }

        interp_skeleton(&ent->rendered, &ent->prev, &ent->curr, alpha, &s_config);
    }
}

const CV64_SkeletonSnapshot* CV64_AnimInterp_GetPose(u32 entity_id) {
    if (!s_initialized || !s_config.enabled) {
        return NULL;
    }

    CV64_AnimInterpEntity* ent = find_entity(entity_id);
    if (!ent || !ent->valid) {
        return NULL;
    }

    return &ent->rendered;
}

/*===========================================================================
 * Public API — Entity Management
 *===========================================================================*/

void CV64_AnimInterp_RemoveEntity(u32 entity_id) {
    CV64_AnimInterpEntity* ent = find_entity(entity_id);
    if (ent) {
        memset(ent, 0, sizeof(CV64_AnimInterpEntity));
    }
}

void CV64_AnimInterp_RemoveAll(void) {
    memset(s_entities, 0, sizeof(s_entities));
    LogInfo("All interpolation entities cleared (map transition)");
}

u32 CV64_AnimInterp_GetEntityCount(void) {
    u32 count = 0;
    for (u32 i = 0; i < CV64_ANIM_MAX_ENTITIES; i++) {
        if (s_entities[i].active) {
            count++;
        }
    }
    return count;
}
