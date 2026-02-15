/**
 * @file cv64_patches.cpp
 * @brief Stub implementation of the CV64 Patch System
 *
 * Provides minimal implementations for the patch API declared in cv64_patches.h.
 * In this recomp build the framerate unlock is always considered active.
 */

#include "../include/cv64_patches.h"

#ifdef __cplusplus
extern "C" {
#endif

static bool s_patches_initialized = false;

bool CV64_Patches_Init(void) {
    s_patches_initialized = true;
    return true;
}

void CV64_Patches_Shutdown(void) {
    s_patches_initialized = false;
}

bool CV64_Patches_IsEnabled(CV64_PatchID patch_id) {
    /* Framerate unlock is always active in the recomp build */
    (void)patch_id;
    return true;
}

#ifdef __cplusplus
}
#endif
