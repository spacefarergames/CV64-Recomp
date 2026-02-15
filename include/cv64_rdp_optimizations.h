/**
 * @file cv64_rdp_optimizations.h
 * @brief Castlevania 64 PC Recomp - RDP Optimization API
 * 
 * Optimizations for the N64's Reality Display Processor (RDP).
 * These optimizations improve rasterization performance in GlideN64.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_RDP_OPTIMIZATIONS_H
#define CV64_RDP_OPTIMIZATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Configuration
 *===========================================================================*/

#define MAX_DL_CACHE_ENTRIES 256

/**
 * RDP optimization configuration
 */
typedef struct {
    bool enableCommandBatching;        ///< Batch RDP commands before processing
    bool enableStateCaching;           ///< Cache RDP state to avoid redundant changes
    bool enableTriangleCulling;        ///< Cull backface and off-screen triangles
    bool enableScissorCulling;         ///< Cull triangles outside scissor box
    bool enableZeroAreaCulling;        ///< Cull degenerate triangles
    bool enableDisplayListCaching;     ///< Cache compiled display lists
    bool enableFillrateOptimization;   ///< Track and optimize fillrate
    uint32_t maxDLCacheSize;           ///< Max cached display lists
} CV64_RDPConfig;

/**
 * RDP performance statistics
 */
typedef struct {
    uint64_t trianglesProcessed;       ///< Total triangles rendered
    uint64_t trianglesCulled;          ///< Triangles culled
    double cullingEffectiveness;       ///< Percentage of triangles culled
    uint64_t stateChanges;             ///< RDP state changes
    uint64_t commandsProcessed;        ///< Total RDP commands processed
    uint64_t dlCacheHits;              ///< Display list cache hits
    uint64_t dlCacheMisses;            ///< Display list cache misses
    double dlCacheHitRate;             ///< Cache hit rate percentage
    size_t dlCacheSize;                ///< Number of cached display lists
    uint32_t overdrawPixels;           ///< Pixels drawn multiple times
} CV64_RDPStats;

/*===========================================================================
 * Initialization
 *===========================================================================*/

/**
 * Initialize RDP optimization system
 * 
 * @return true on success
 */
bool CV64_RDP_Initialize(void);

/**
 * Shutdown RDP optimization system
 */
void CV64_RDP_Shutdown(void);

/*===========================================================================
 * Configuration
 *===========================================================================*/

/**
 * Set RDP configuration
 * 
 * @param config Configuration structure (NULL to use defaults)
 */
void CV64_RDP_SetConfig(const CV64_RDPConfig* config);

/**
 * Get current RDP configuration
 * 
 * @return Pointer to current configuration (read-only)
 */
const CV64_RDPConfig* CV64_RDP_GetConfig(void);

/*===========================================================================
 * Command Batching
 *===========================================================================*/

/**
 * Begin batching RDP commands
 * Call at the start of frame processing
 */
void CV64_RDP_BeginCommandBatch(void);

/**
 * Add command to batch
 * 
 * @param command RDP command byte
 * @param data Command data
 * @param dataSize Size of data in uint32_t elements
 */
void CV64_RDP_AddCommand(uint8_t command, const uint32_t* data, uint32_t dataSize);

/**
 * Flush all batched commands
 * Call at the end of frame processing
 */
void CV64_RDP_FlushCommands(void);

/*===========================================================================
 * State Caching
 *===========================================================================*/

/**
 * Set geometry mode (returns true if state changed)
 * 
 * @param mode Geometry mode flags
 * @return true if state changed, false if already set
 */
bool CV64_RDP_SetGeometryMode(uint32_t mode);

/**
 * Set combine mode (returns true if state changed)
 * 
 * @param mode Combine mode
 * @return true if state changed
 */
bool CV64_RDP_SetCombineMode(uint32_t mode);

/**
 * Set texture image (returns true if state changed)
 * 
 * @param image Texture image address
 * @return true if state changed
 */
bool CV64_RDP_SetTextureImage(uint32_t image);

/*===========================================================================
 * Triangle Culling
 *===========================================================================*/

/**
 * Check if triangle should be culled
 * 
 * @param x0, y0, z0 First vertex
 * @param x1, y1, z1 Second vertex
 * @param x2, y2, z2 Third vertex
 * @return true if triangle should be culled
 */
bool CV64_RDP_ShouldCullTriangle(float x0, float y0, float z0,
                                  float x1, float y1, float z1,
                                  float x2, float y2, float z2);

/*===========================================================================
 * Display List Caching
 *===========================================================================*/

/**
 * Calculate hash for display list
 * 
 * @param address Display list address
 * @param data Display list data
 * @param size Size in bytes
 * @return Hash value
 */
uint32_t CV64_RDP_HashDisplayList(uint32_t address, const uint8_t* data, uint32_t size);

/**
 * Get cached display list
 * 
 * @param address Display list address
 * @param hash Display list hash
 * @return true if found in cache and executed
 */
bool CV64_RDP_GetCachedDisplayList(uint32_t address, uint32_t hash);

/**
 * Cache display list
 * 
 * @param address Display list address
 * @param hash Display list hash
 * @param commands RDP commands
 * @param count Number of commands
 */
void CV64_RDP_CacheDisplayList(uint32_t address, uint32_t hash,
                                const void* commands, uint32_t count);

/**
 * Advance frame counter (for cache LRU)
 */
void CV64_RDP_AdvanceFrame(void);

/*===========================================================================
 * Fillrate Optimization
 *===========================================================================*/

/**
 * Begin fillrate tracking for frame
 */
void CV64_RDP_BeginFillrateTracking(void);

/**
 * Record pixel fill operation
 * 
 * @param pixelCount Number of pixels filled
 */
void CV64_RDP_RecordPixelFill(uint32_t pixelCount);

/**
 * Get overdraw count for current frame
 * 
 * @return Number of overdraw pixels
 */
uint32_t CV64_RDP_GetOverdrawCount(void);

/*===========================================================================
 * Statistics
 *===========================================================================*/

/**
 * Get RDP statistics
 * 
 * @param stats Output structure for statistics
 */
void CV64_RDP_GetStats(CV64_RDPStats* stats);

/**
 * Reset RDP statistics
 */
void CV64_RDP_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_RDP_OPTIMIZATIONS_H */
