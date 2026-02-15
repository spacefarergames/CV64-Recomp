/**
 * @file cv64_advanced_graphics.h
 * @brief CV64 Advanced Graphics Features
 * 
 * Advanced rendering features including:
 * - Screen Space Ambient Occlusion (SSAO)
 * - Screen Space Reflections (SSR) for flat surfaces
 * - Enhanced visual quality
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_ADVANCED_GRAPHICS_H
#define CV64_ADVANCED_GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Feature Flags
 *===========================================================================*/

/**
 * @brief Enable Screen Space Ambient Occlusion
 * Adds realistic shadows in corners and crevices
 * Performance: ~1-2ms per frame
 */
#ifndef CV64_ENABLE_SSAO
#define CV64_ENABLE_SSAO 1
#endif

/**
 * @brief Enable Screen Space Reflections
 * Adds reflections to flat surfaces (floor, water)
 * Performance: ~2-3ms per frame
 */
#ifndef CV64_ENABLE_SSR
#define CV64_ENABLE_SSR 1
#endif

/*===========================================================================
 * SSAO Configuration
 *===========================================================================*/

/**
 * @brief SSAO sample count (higher = better quality, slower)
 * Recommended: 8-16 samples
 */
#define CV64_SSAO_SAMPLE_COUNT 12

/**
 * @brief SSAO radius (how far to sample around each pixel)
 * Smaller = sharper AO, Larger = softer AO
 * Recommended: 0.05 - 0.2
 */
#define CV64_SSAO_RADIUS 0.12f

/**
 * @brief SSAO intensity (how dark the occlusion is)
 * Range: 0.0 (none) to 2.0 (very strong)
 * Recommended: 0.8 - 1.2
 */
#define CV64_SSAO_INTENSITY 1.0f

/**
 * @brief SSAO bias (prevents self-shadowing)
 * Range: 0.0 - 0.1
 * Recommended: 0.025
 */
#define CV64_SSAO_BIAS 0.025f

/*===========================================================================
 * SSR Configuration
 *===========================================================================*/

/**
 * @brief SSR max ray steps (higher = better quality, slower)
 * Recommended: 16-32
 */
#define CV64_SSR_MAX_STEPS 24

/**
 * @brief SSR ray step size (smaller = more accurate)
 * Recommended: 0.01 - 0.05
 */
#define CV64_SSR_STEP_SIZE 0.02f

/**
 * @brief SSR intensity (reflection strength)
 * Range: 0.0 (none) to 1.0 (mirror)
 * Recommended: 0.3 - 0.6
 */
#define CV64_SSR_INTENSITY 0.4f

/**
 * @brief SSR surface detection threshold
 * How flat a surface must be to receive reflections
 * Range: 0.0 (any angle) to 1.0 (perfectly flat)
 * Recommended: 0.85 - 0.95
 */
#define CV64_SSR_FLATNESS_THRESHOLD 0.90f

/*===========================================================================
 * Quality Presets
 *===========================================================================*/

typedef enum {
    CV64_GRAPHICS_QUALITY_LOW = 0,      // Minimal features, max performance
    CV64_GRAPHICS_QUALITY_MEDIUM = 1,   // Balanced
    CV64_GRAPHICS_QUALITY_HIGH = 2,     // Best quality
    CV64_GRAPHICS_QUALITY_ULTRA = 3     // All features enabled
} CV64_GraphicsQuality;

/*===========================================================================
 * API Functions
 *===========================================================================*/

/**
 * @brief Initialize advanced graphics system
 * @param quality Quality preset to use
 * @return true on success
 */
bool CV64_AdvancedGraphics_Init(CV64_GraphicsQuality quality);

/**
 * @brief Shutdown advanced graphics
 */
void CV64_AdvancedGraphics_Shutdown(void);

/**
 * @brief Update per-frame
 * @param deltaTime Time since last frame in seconds
 */
void CV64_AdvancedGraphics_Update(float deltaTime);

/**
 * @brief Apply SSAO post-process effect
 * @param colorTexture Source color texture
 * @param depthTexture Depth buffer texture
 * @param normalTexture Normal buffer texture (if available)
 * @return Output texture with AO applied
 */
uint32_t CV64_AdvancedGraphics_ApplySSAO(
    uint32_t colorTexture,
    uint32_t depthTexture,
    uint32_t normalTexture
);

/**
 * @brief Apply SSR post-process effect
 * @param colorTexture Source color texture
 * @param depthTexture Depth buffer texture
 * @param normalTexture Normal buffer texture
 * @return Output texture with reflections applied
 */
uint32_t CV64_AdvancedGraphics_ApplySSR(
    uint32_t colorTexture,
    uint32_t depthTexture,
    uint32_t normalTexture
);

/**
 * @brief Enable/disable SSAO
 * @param enabled true to enable
 */
void CV64_AdvancedGraphics_SetSSAOEnabled(bool enabled);

/**
 * @brief Enable/disable SSR
 * @param enabled true to enable
 */
void CV64_AdvancedGraphics_SetSSREnabled(bool enabled);

/**
 * @brief Set SSAO intensity
 * @param intensity Range 0.0 - 2.0
 */
void CV64_AdvancedGraphics_SetSSAOIntensity(float intensity);

/**
 * @brief Set SSR intensity
 * @param intensity Range 0.0 - 1.0
 */
void CV64_AdvancedGraphics_SetSSRIntensity(float intensity);

/**
 * @brief Get current performance metrics
 * @param outSSAOTime SSAO time in milliseconds
 * @param outSSRTime SSR time in milliseconds
 */
void CV64_AdvancedGraphics_GetPerformanceMetrics(
    float* outSSAOTime,
    float* outSSRTime
);

/**
 * @brief Check if advanced graphics are supported
 * @return true if GPU supports required features
 */
bool CV64_AdvancedGraphics_IsSupported(void);

/*===========================================================================
 * Implementation Notes
 *===========================================================================*/

/*
 * SSAO Implementation:
 * - Uses random sampling in hemisphere around each pixel
 * - Samples depth buffer to detect nearby geometry
 * - Darker where geometry is closer (corners, crevices)
 * - Blur pass for smooth result
 * 
 * SSR Implementation:
 * - Ray marching in screen space
 * - Only reflects pixels that are on screen
 * - Fades out at screen edges
 * - Only applies to surfaces with normal pointing up (floors)
 * 
 * Performance:
 * - SSAO: ~1-2ms @ 1080p with 12 samples
 * - SSR: ~2-3ms @ 1080p with 24 steps
 * - Total: ~3-5ms per frame (still maintains 60 FPS)
 * 
 * Quality Presets:
 * - LOW: SSAO disabled, SSR disabled
 * - MEDIUM: SSAO 8 samples, SSR disabled
 * - HIGH: SSAO 12 samples, SSR 16 steps
 * - ULTRA: SSAO 16 samples, SSR 32 steps
 */

#ifdef __cplusplus
}
#endif

#endif /* CV64_ADVANCED_GRAPHICS_H */
