/**
 * @file cv64_gliden64_optimize.h
 * @brief Castlevania 64 PC Recomp - GLideN64 Optimization Layer
 * 
 * This module provides CV64-specific optimizations for GLideN64 to:
 * - Optimize texture cache usage for CV64's texture patterns
 * - Reduce draw calls where possible
 * - Provide CV64-aware rendering hints
 * - Minimize slowdowns in problematic areas
 * 
 * OPTIMIZATION STRATEGIES:
 * ========================
 * 1. Texture Cache Optimization:
 *    - CV64 uses many repeated textures (castle walls, floors)
 *    - Pre-identify high-frequency textures and prioritize caching
 *    - Batch similar texture uploads
 * 
 * 2. Draw Call Reduction:
 *    - CV64 has many small geometry pieces
 *    - Identify redundant state changes
 *    - Batch compatible draw calls
 * 
 * 3. Framebuffer Optimization:
 *    - CV64 uses framebuffer effects for fog/lighting
 *    - Optimize copy operations for known patterns
 * 
 * 4. Area-Specific Optimizations:
 *    - Villa (complex lighting) - reduce shadow precision
 *    - Castle Center (many textures) - aggressive caching
 *    - Forest (fog heavy) - optimize fog rendering
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_GLIDEN64_OPTIMIZE_H
#define CV64_GLIDEN64_OPTIMIZE_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Optimization Profiles
 *===========================================================================*/

/**
 * @brief Performance optimization profiles
 */
typedef enum {
    CV64_OPT_PROFILE_BALANCED = 0,  ///< Default balanced settings
    CV64_OPT_PROFILE_QUALITY,       ///< Maximum quality (slower)
    CV64_OPT_PROFILE_PERFORMANCE,   ///< Maximum performance (lower quality)
    CV64_OPT_PROFILE_LOW_END,       ///< For integrated graphics
    CV64_OPT_PROFILE_CUSTOM         ///< User-defined settings
} CV64_OptProfile;

/**
 * @brief Optimization configuration
 */
typedef struct {
    CV64_OptProfile profile;
    
    /* Texture Cache Settings */
    u32 textureCacheSizeMB;          ///< Texture cache size in MB (default: 100)
    bool enableTexturePreload;        ///< Preload known CV64 textures
    bool enableTextureBatching;       ///< Batch texture uploads
    u32 textureHashCacheSize;         ///< CRC cache size (default: 4096)
    
    /* Draw Call Settings */
    bool enableDrawCallBatching;      ///< Batch compatible draw calls
    bool enableStateChangeReduction;  ///< Minimize OpenGL state changes
    u32 maxBatchedTriangles;          ///< Max triangles per batch (default: 1024)
    
    /* Framebuffer Settings */
    bool enableFBOptimization;        ///< Optimize framebuffer operations
    bool deferFBCopy;                 ///< Defer framebuffer copies
    bool enableAsyncFBRead;           ///< Async framebuffer readback
    
    /* CV64-Specific */
    bool enableAreaOptimization;      ///< Enable area-specific optimizations
    bool optimizeFogRendering;        ///< Optimize fog for CV64's usage
    bool optimizeShadows;             ///< Reduce shadow complexity
    bool optimizeWaterEffects;        ///< Optimize water reflection/refraction
    
    /* Quality vs Performance */
    u32 lodBias;                      ///< LOD bias (-2 to +2, 0 = default)
    bool enableMipmapping;            ///< Use mipmaps for distant textures
    u32 shadowMapResolution;          ///< Shadow map resolution (256-2048)
} CV64_OptConfig;

/**
 * @brief Runtime optimization statistics
 */
typedef struct {
    /* Texture Stats */
    u64 texturesCached;               ///< Total textures in cache
    u64 textureCacheHits;             ///< Cache hits
    u64 textureCacheMisses;           ///< Cache misses
    double textureCacheHitRate;       ///< Hit rate percentage
    u32 textureCacheUsedMB;           ///< Cache usage in MB
    
    /* Draw Call Stats */
    u64 drawCallsOriginal;            ///< Original draw calls per frame
    u64 drawCallsBatched;             ///< After batching
    u64 drawCallsSaved;               ///< Draw calls saved
    double batchingEfficiency;        ///< Batching efficiency %
    
    /* Framebuffer Stats */
    u64 fbCopiesOriginal;             ///< Original FB copy operations
    u64 fbCopiesOptimized;            ///< After optimization
    u64 fbCopiesDeferred;             ///< Copies deferred
    
    /* Performance */
    double avgFrameTimeMs;            ///< Average frame time
    double avgGPUTimeMs;              ///< Average GPU time
    u64 trianglesPerSecond;           ///< Triangles rendered per second
} CV64_OptStats;

/*===========================================================================
 * CV64 Texture Categories (for prioritized caching)
 *===========================================================================*/

/**
 * @brief CV64 texture categories for cache prioritization
 */
typedef enum {
    CV64_TEX_CATEGORY_UNKNOWN = 0,
    CV64_TEX_CATEGORY_UI,             ///< HUD, menus, text (high priority)
    CV64_TEX_CATEGORY_PLAYER,         ///< Player model textures
    CV64_TEX_CATEGORY_ENEMY,          ///< Enemy textures
    CV64_TEX_CATEGORY_ENVIRONMENT,    ///< Walls, floors, ceilings
    CV64_TEX_CATEGORY_SKY,            ///< Skybox textures
    CV64_TEX_CATEGORY_EFFECT,         ///< Particles, effects
    CV64_TEX_CATEGORY_WATER,          ///< Water surfaces
    CV64_TEX_CATEGORY_SHADOW          ///< Shadow textures
} CV64_TexCategory;

/**
 * @brief Known CV64 texture info (for preloading/prioritization)
 */
typedef struct {
    u32 crc;                          ///< Texture CRC
    CV64_TexCategory category;         ///< Category for prioritization
    u8 priority;                      ///< Cache priority (0-255)
    const char* name;                 ///< Debug name (optional)
} CV64_KnownTexture;

/*===========================================================================
 * CV64 Area Info (for area-specific optimizations)
 *===========================================================================*/

/**
 * @brief CV64 area optimization hints
 */
typedef struct {
    u32 mapId;                        ///< CV64 map ID
    const char* name;                 ///< Area name
    
    /* Optimization hints */
    bool heavyFog;                    ///< Area uses heavy fog
    bool complexLighting;             ///< Area has complex lighting
    bool manyTextures;                ///< Area has many unique textures
    bool waterPresent;                ///< Area has water effects
    bool outdoorArea;                 ///< Outdoor (needs skybox)
    
    /* Recommended settings */
    u32 suggestedCacheSize;           ///< Suggested texture cache for area
    u32 suggestedFogQuality;          ///< 0=low, 1=medium, 2=high
    u32 suggestedShadowQuality;       ///< 0=off, 1=low, 2=medium, 3=high
} CV64_AreaOptHints;

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief Initialize the optimization system
 * @param config Initial configuration (NULL for defaults)
 * @return true on success
 */
bool CV64_Optimize_Init(const CV64_OptConfig* config);

/**
 * @brief Shutdown the optimization system
 */
void CV64_Optimize_Shutdown(void);

/**
 * @brief Apply an optimization profile
 * @param profile Profile to apply
 */
void CV64_Optimize_SetProfile(CV64_OptProfile profile);

/**
 * @brief Get current configuration
 * @param config Output configuration
 */
void CV64_Optimize_GetConfig(CV64_OptConfig* config);

/**
 * @brief Set custom configuration
 * @param config Configuration to apply
 */
void CV64_Optimize_SetConfig(const CV64_OptConfig* config);

/**
 * @brief Get optimization statistics
 * @param stats Output statistics
 */
void CV64_Optimize_GetStats(CV64_OptStats* stats);

/**
 * @brief Reset statistics
 */
void CV64_Optimize_ResetStats(void);

/*===========================================================================
 * Texture Cache Optimization API
 *===========================================================================*/

/**
 * @brief Preload known CV64 textures into cache
 * Call this during loading screens for smoother gameplay
 */
void CV64_Optimize_PreloadTextures(void);

/**
 * @brief Hint that a texture will be used soon
 * @param crc Texture CRC
 * @param category Texture category
 */
void CV64_Optimize_TextureHint(u32 crc, CV64_TexCategory category);

/**
 * @brief Flush low-priority textures from cache
 * Call when entering a new area
 */
void CV64_Optimize_FlushLowPriorityTextures(void);

/**
 * @brief Get texture category for a CRC
 * @param crc Texture CRC
 * @return Texture category
 */
CV64_TexCategory CV64_Optimize_GetTextureCategory(u32 crc);

/*===========================================================================
 * Draw Call Optimization API
 *===========================================================================*/

/**
 * @brief Begin draw call batching for current frame
 */
void CV64_Optimize_BeginBatch(void);

/**
 * @brief End draw call batching and flush
 */
void CV64_Optimize_EndBatch(void);

/**
 * @brief Check if two draw calls can be batched
 * @param state1 First draw call state
 * @param state2 Second draw call state
 * @return true if batchable
 */
bool CV64_Optimize_CanBatch(const void* state1, const void* state2);

/**
 * @brief Update the current RDP state for batching analysis
 * Call this before each draw to track state changes.
 * @param texCRC Current texture CRC
 * @param combine Color combiner mode
 * @param blend Blend mode
 * @param geomMode Geometry mode
 * @param otherL Other mode L bits
 * @param otherH Other mode H bits
 * @param fog Fog enabled
 * @param depth Depth test enabled
 * @param alphaComp Alpha compare enabled
 */
void CV64_Optimize_UpdateRDPState(u32 texCRC, u32 combine, u32 blend,
                                   u32 geomMode, u32 otherL, u32 otherH,
                                   bool fog, bool depth, bool alphaComp);

/**
 * @brief Update scissor state for batching
 * @param x Scissor X
 * @param y Scissor Y
 * @param w Scissor width
 * @param h Scissor height
 */
void CV64_Optimize_UpdateScissor(u16 x, u16 y, u16 w, u16 h);

/**
 * @brief Attempt to batch the current draw call with previous
 * @param triangleCount Number of triangles in this draw
 * @return true if batched, false if state changed (new batch started)
 */
bool CV64_Optimize_TryBatchCurrentDraw(u32 triangleCount);

/*===========================================================================
 * Area-Specific Optimization API
 *===========================================================================*/

/**
 * @brief Notify optimization system of area change
 * @param mapId New area map ID
 */
void CV64_Optimize_OnAreaChange(u32 mapId);

/**
 * @brief Get optimization hints for an area
 * @param mapId Area map ID
 * @return Area hints (NULL if unknown)
 */
const CV64_AreaOptHints* CV64_Optimize_GetAreaHints(u32 mapId);

/**
 * @brief Apply area-specific optimizations
 * @param mapId Area to optimize for
 */
void CV64_Optimize_ApplyAreaSettings(u32 mapId);

/*===========================================================================
 * Frame Hooks (called by rendering pipeline)
 *===========================================================================*/

/**
 * @brief Called at start of frame
 */
void CV64_Optimize_FrameBegin(void);

/**
 * @brief Called at end of frame
 */
void CV64_Optimize_FrameEnd(void);

/**
 * @brief Called before texture load
 * @param crc Texture CRC
 * @param size Texture size in bytes
 * @return true to proceed with load, false to skip (cached)
 */
bool CV64_Optimize_OnTextureLoad(u32 crc, u32 size);

/**
 * @brief Called before draw call
 * @param triangleCount Number of triangles
 * @return true to proceed, false if batched
 */
bool CV64_Optimize_OnDrawCall(u32 triangleCount);

#ifdef __cplusplus
}
#endif

#endif /* CV64_GLIDEN64_OPTIMIZE_H */
