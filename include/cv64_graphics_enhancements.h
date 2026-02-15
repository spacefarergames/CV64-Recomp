/**
 * @file cv64_graphics_enhancements.h
 * @brief CV64 Graphics Pipeline Enhancements
 * 
 * Enhanced lighting, shadows, and rendering optimizations for GLideN64.
 * Includes:
 * - Enhanced N64 lighting calculations
 * - Optimized shadow rendering
 * - Performance improvements for CV64-specific rendering
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_GRAPHICS_ENHANCEMENTS_H
#define CV64_GRAPHICS_ENHANCEMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Lighting Enhancements
 *===========================================================================*/

/**
 * @brief Enhanced ambient light multiplier for CV64
 * CV64 uses darker lighting than other N64 games - this compensates
 */
#define CV64_AMBIENT_LIGHT_BOOST    1.15f

/**
 * @brief Enhanced directional light falloff
 * Makes lights feel more natural in the castle environment
 */
#define CV64_DIRECTIONAL_FALLOFF    0.92f

/**
 * @brief Shadow intensity multiplier
 * CV64's gothic atmosphere benefits from deeper shadows
 */
#define CV64_SHADOW_INTENSITY       0.85f

/*===========================================================================
 * Performance Optimizations
 *===========================================================================*/

/**
 * @brief Enable optimized triangle culling for CV64
 * CV64 has predictable geometry patterns we can optimize
 */
#define CV64_ENABLE_FAST_CULLING    1

/**
 * @brief Enable texture cache optimizations
 * CV64 reuses textures heavily - optimize caching
 */
#define CV64_OPTIMIZE_TEXTURE_CACHE 1

/**
 * @brief Enable dynamic LOD for distant objects
 * Improves performance in large outdoor areas (Forest, Villa)
 */
#define CV64_ENABLE_DYNAMIC_LOD     0  // Disabled - needs more testing

/*===========================================================================
 * Visual Quality Enhancements
 *===========================================================================*/

/**
 * @brief Enhanced fog rendering quality
 * CV64 uses fog heavily - improve gradient quality
 */
#define CV64_ENHANCED_FOG_QUALITY   1

/**
 * @brief Improved shadow edge softness
 * Softer shadow edges look more natural
 */
#define CV64_SOFT_SHADOW_EDGES      1

/**
 * @brief Enhanced texture filtering
 * Better texture quality for close-up views
 */
#define CV64_ENHANCED_FILTERING     1

/*===========================================================================
 * API Functions
 *===========================================================================*/

/**
 * @brief Initialize graphics enhancements
 * Call once at plugin startup
 */
void CV64_Graphics_Init(void);

/**
 * @brief Apply per-frame enhancements
 * Call once per frame before rendering
 */
void CV64_Graphics_ApplyPerFrameEnhancements(void);

/**
 * @brief Enhance lighting calculation for a vertex
 * @param vertex Vertex data to enhance
 * @param lightIndex Light source index
 * @return Enhanced light contribution
 */
float CV64_Graphics_EnhanceLighting(const float* vertex, int lightIndex);

/**
 * @brief Check if current scene benefits from optimizations
 * @return true if optimizations should be applied
 */
bool CV64_Graphics_ShouldOptimize(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_GRAPHICS_ENHANCEMENTS_H */
