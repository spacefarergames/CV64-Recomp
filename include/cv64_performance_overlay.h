/**
 * @file cv64_performance_overlay.h
 * @brief Performance statistics overlay for debugging
 * 
 * Displays real-time performance metrics including FPS, frame times,
 * threading statistics, and more.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_PERFORMANCE_OVERLAY_H
#define CV64_PERFORMANCE_OVERLAY_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Performance overlay display options
 */
typedef enum {
    CV64_PERF_OVERLAY_OFF = 0,      ///< No overlay
    CV64_PERF_OVERLAY_MINIMAL,      ///< Just FPS
    CV64_PERF_OVERLAY_STANDARD,     ///< FPS + frame time
    CV64_PERF_OVERLAY_DETAILED,     ///< All stats including threading
    CV64_PERF_OVERLAY_GRAPH         ///< Frame time graph
} CV64_PerfOverlayMode;

/**
 * @brief Performance statistics
 */
typedef struct {
    double fps;                     ///< Current FPS
    double avgFrameTimeMs;          ///< Average frame time
    double minFrameTimeMs;          ///< Min frame time (last second)
    double maxFrameTimeMs;          ///< Max frame time (last second)
    u64 totalFrames;                ///< Total frames rendered
    
    // Threading stats
    double avgPresentLatencyMs;     ///< Avg graphics present latency
    u64 audioUnderruns;             ///< Audio buffer underruns
    u64 gpuSyncWaits;               ///< GPU sync waits
    
    // Memory
    size_t rdramUsage;              ///< RDRAM usage in MB
    
    // Emulation
    double cpuUsage;                ///< CPU usage %
    double emulationSpeed;          ///< Speed factor (1.0 = 100%)
} CV64_PerfStats;

/**
 * @brief Initialize the performance overlay
 * @return true on success
 */
bool CV64_PerfOverlay_Init(void);

/**
 * @brief Shutdown the performance overlay
 */
void CV64_PerfOverlay_Shutdown(void);

/**
 * @brief Set the overlay display mode
 * @param mode Display mode
 */
void CV64_PerfOverlay_SetMode(CV64_PerfOverlayMode mode);

/**
 * @brief Get current display mode
 * @return Current mode
 */
CV64_PerfOverlayMode CV64_PerfOverlay_GetMode(void);

/**
 * @brief Toggle through overlay modes
 * Called when user presses hotkey (F3)
 */
void CV64_PerfOverlay_Toggle(void);

/**
 * @brief Update performance statistics
 * Call this every frame from main loop
 * 
 * @param deltaTime Time since last frame (seconds)
 */
void CV64_PerfOverlay_Update(double deltaTime);

/**
 * @brief Render the overlay
 * Call after all game rendering is done
 * 
 * @param width Screen width
 * @param height Screen height
 */
void CV64_PerfOverlay_Render(int width, int height);

/**
 * @brief Get current performance stats
 * @param stats Output statistics structure
 */
void CV64_PerfOverlay_GetStats(CV64_PerfStats* stats);

/**
 * @brief Reset statistics counters
 */
void CV64_PerfOverlay_ResetStats(void);

/**
 * @brief Record a frame time sample
 * @param frameTimeMs Frame time in milliseconds
 */
void CV64_PerfOverlay_RecordFrame(double frameTimeMs);

#ifdef __cplusplus
}
#endif

#endif /* CV64_PERFORMANCE_OVERLAY_H */
