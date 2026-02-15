/**
 * @file cv64_performance_optimizations.h
 * @brief Castlevania 64 PC Recomp - Performance Optimization API
 * 
 * Provides aggressive performance optimizations for:
 * - GlideN64 renderer (texture cache, framebuffer, draw calls)
 * - RSP processing (audio HLE, task scheduling)
 * - Frame pacing and VSync management
 * - Memory management and caching
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_PERFORMANCE_OPTIMIZATIONS_H
#define CV64_PERFORMANCE_OPTIMIZATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Performance Configuration
 *===========================================================================*/

/**
 * VSync mode settings
 */
typedef enum {
    CV64_VSYNC_OFF = 0,       ///< VSync disabled (tearing possible, max FPS)
    CV64_VSYNC_ON = 1,        ///< VSync enabled (60 FPS locked, no tearing)
    CV64_VSYNC_ADAPTIVE = 2,  ///< Adaptive VSync (tears only if < 60 FPS)
    CV64_VSYNC_HALF = 3       ///< Half refresh rate (30 FPS)
} CV64_VSyncMode;

/**
 * Performance configuration structure
 */
typedef struct {
    // Renderer optimizations
    bool enableTextureCaching;           ///< Cache textures to reduce uploads
    bool enableFramebufferOptimization;  ///< Optimize framebuffer operations
    bool enableDrawCallBatching;         ///< Batch draw calls to reduce overhead
    uint32_t maxTextureSize;             ///< Max texture dimension (1024, 2048, 4096)
    float mipmapBias;                    ///< Mipmap LOD bias (-2.0 to 2.0)
    
    // RSP optimizations
    bool enableParallelAudioMixing;      ///< Process audio on worker threads
    bool enableRSPTaskPrioritization;    ///< Prioritize audio over graphics
    uint32_t audioBufferSize;            ///< Audio buffer size in samples
    uint32_t maxRSPTasksPerFrame;        ///< Limit RSP tasks per frame
    
    // VSync and frame pacing
    CV64_VSyncMode vsyncMode;            ///< VSync mode
    uint32_t targetFPS;                  ///< Target frame rate (30, 60, 120, etc.)
    bool allowFrameSkip;                 ///< Allow frame skipping when slow
    uint32_t maxConsecutiveFrameSkips;   ///< Max frames to skip in a row
    
    // Memory optimizations
    bool enableMemoryPooling;            ///< Use memory pools for allocations
    size_t textureMemoryBudget;          ///< Max texture cache memory in bytes
    bool preallocateBuffers;             ///< Preallocate common buffers
    
    // Rendering quality (can be reduced for performance)
    uint32_t multisampleSamples;         ///< MSAA samples (0, 2, 4, 8)
    uint32_t anisotropicFiltering;       ///< Anisotropic filtering level (0-16)
    bool enablePostProcessing;           ///< Enable post-processing effects
} CV64_PerformanceConfig;

/**
 * Performance statistics
 */
typedef struct {
    // Frame timing
    double avgFrameTimeMs;               ///< Average frame time in milliseconds
    double avgRenderTimeMs;              ///< Average render time in milliseconds
    double currentFPS;                   ///< Current FPS
    
    // Frame counters
    uint64_t totalFrames;                ///< Total frames rendered
    uint64_t droppedFrames;              ///< Frames skipped due to performance
    
    // Rendering stats
    uint64_t textureUploads;             ///< Texture uploads per second
    uint64_t drawCalls;                  ///< Draw calls per second
    uint64_t textureCacheHits;           ///< Texture cache hits
    uint64_t textureCacheMisses;         ///< Texture cache misses
    size_t textureCacheSize;             ///< Number of cached textures
    double textureCacheMemoryMB;         ///< Texture cache memory in MB
    
    // RSP stats
    uint64_t rspAudioTasks;              ///< RSP audio tasks processed
    uint64_t rspGraphicsTasks;           ///< RSP graphics tasks processed
    
    // Performance flags
    bool lowPerformanceMode;             ///< System is in low perf mode
} CV64_PerformanceStats;

/*===========================================================================
 * Initialization
 *===========================================================================*/

/**
 * Initialize performance optimization system
 * 
 * @return true on success, false on failure
 */
bool CV64_Perf_Initialize(void);

/**
 * Shutdown performance optimization system
 */
void CV64_Perf_Shutdown(void);

/*===========================================================================
 * Configuration
 *===========================================================================*/

/**
 * Set performance configuration
 * 
 * @param config Configuration structure (NULL to use defaults)
 */
void CV64_Perf_SetConfig(const CV64_PerformanceConfig* config);

/**
 * Get current performance configuration
 * 
 * @return Pointer to current configuration (read-only)
 */
const CV64_PerformanceConfig* CV64_Perf_GetConfig(void);

/**
 * Enable/disable aggressive optimizations for low-end systems
 * 
 * When enabled:
 * - Reduces texture quality
 * - Disables expensive effects
 * - Enables frame skipping
 * - Increases audio buffer size
 * 
 * @param enable true to enable, false to restore defaults
 */
void CV64_Perf_EnableAggressiveOptimizations(bool enable);

/*===========================================================================
 * VSync Control
 *===========================================================================*/

/**
 * Initialize VSync control system
 * 
 * @return true if VSync control is available
 */
bool CV64_Perf_InitVSync(void);

/**
 * Set VSync mode
 * 
 * @param mode VSync mode to set
 */
void CV64_Perf_SetVSyncMode(CV64_VSyncMode mode);

/**
 * Get current VSync mode
 * 
 * @return Current VSync mode
 */
CV64_VSyncMode CV64_Perf_GetVSyncMode(void);

/*===========================================================================
 * Texture Cache Optimization
 *===========================================================================*/

/**
 * Check if texture is in cache
 * 
 * @param addr Texture address in RDRAM
 * @param width Texture width
 * @param height Texture height
 * @param format Texture format
 * @param crc Texture CRC for validation
 * @param outTextureId Output OpenGL texture ID if cached
 * @return true if texture found in cache
 */
bool CV64_Perf_CacheTexture(uint32_t addr, uint32_t width, uint32_t height,
                            uint32_t format, uint32_t crc, unsigned int* outTextureId);

/**
 * Add texture to cache
 * 
 * @param addr Texture address in RDRAM
 * @param width Texture width
 * @param height Texture height
 * @param format Texture format
 * @param crc Texture CRC for validation
 * @param textureId OpenGL texture ID
 */
void CV64_Perf_AddToTextureCache(uint32_t addr, uint32_t width, uint32_t height,
                                 uint32_t format, uint32_t crc, unsigned int textureId);

/**
 * Clear texture cache and free all cached textures
 */
void CV64_Perf_ClearTextureCache(void);

/*===========================================================================
 * RSP Task Optimization
 *===========================================================================*/

/**
 * Process RSP task with optimizations
 * 
 * @param task Pointer to RSP task structure
 * @param taskType Task type (0x01 = graphics, 0x02 = audio)
 */
void CV64_Perf_ProcessRSPTask(void* task, uint32_t taskType);

/*===========================================================================
 * Draw Call Batching
 *===========================================================================*/

/**
 * Begin batching draw calls
 * Call this at the start of frame rendering
 */
void CV64_Perf_BeginDrawCallBatching(void);

/**
 * Add a draw call to the batch
 * 
 * @param mode OpenGL primitive mode (GL_TRIANGLES, etc.)
 * @param vertexCount Number of vertices
 * @param startIndex Starting vertex index
 * @param texture OpenGL texture ID
 * @param state Render state hash
 */
void CV64_Perf_AddDrawCall(unsigned int mode, uint32_t vertexCount, uint32_t startIndex,
                           unsigned int texture, uint32_t state);

/**
 * Flush all batched draw calls
 * Call this at the end of frame rendering
 */
void CV64_Perf_FlushDrawCalls(void);

/*===========================================================================
 * Frame Timing
 *===========================================================================*/

/**
 * Mark beginning of frame
 * Call this at the start of each frame
 */
void CV64_Perf_BeginFrame(void);

/**
 * Mark end of frame
 * Call this at the end of each frame
 */
void CV64_Perf_EndFrame(void);

/**
 * Check if current frame should be skipped for performance
 * 
 * @return true if frame should be skipped
 */
bool CV64_Perf_ShouldSkipFrame(void);

/*===========================================================================
 * Statistics
 *===========================================================================*/

/**
 * Get performance statistics
 * 
 * @param stats Output structure for statistics
 */
void CV64_Perf_GetStats(CV64_PerformanceStats* stats);

/**
 * Reset performance statistics
 */
void CV64_Perf_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_PERFORMANCE_OPTIMIZATIONS_H */
