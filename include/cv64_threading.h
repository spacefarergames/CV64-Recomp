/**
 * @file cv64_threading.h
 * @brief Castlevania 64 PC Recomp - Multi-Threading Support
 * 
 * This module provides asynchronous processing for graphics, audio, and RSP
 * operations where possible within the constraints of N64 emulation.
 * 
 * ARCHITECTURE OVERVIEW:
 * ======================
 * N64 emulation is inherently synchronous because:
 * - RSP must complete before RDP can render
 * - Audio DMA timing is tied to VI interrupts  
 * - Memory is shared between all components
 * 
 * However, we CAN offload:
 * - OpenGL rendering (GLideN64 already does some of this internally)
 * - Audio mixing and output (SDL handles this)
 * - Texture uploads and shader compilation
 * - Frame presentation (triple buffering)
 * 
 * THREADING MODEL:
 * ================
 * Main Thread: R4300 CPU emulation (drives everything)
 * Graphics Thread: OpenGL context, frame presentation
 * Audio Thread: SDL audio callback (already separate)
 * Worker Threads: Async tasks (texture loading, etc.)
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_THREADING_H
#define CV64_THREADING_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Thread Manager Configuration
 *===========================================================================*/

/**
 * @brief Threading configuration options
 */
typedef struct {
    bool enableAsyncGraphics;      ///< Enable async frame presentation
    bool enableAsyncAudio;         ///< Enable async audio mixing (SDL default)
    bool enableWorkerThreads;      ///< Enable worker thread pool
    int workerThreadCount;         ///< Number of worker threads (0 = auto)
    int graphicsQueueDepth;        ///< Max queued frames (1-3, for triple buffering)
    bool enableParallelRSP;        ///< EXPERIMENTAL: Parallel RSP tasks
} CV64_ThreadConfig;

/**
 * @brief Thread synchronization statistics
 */
typedef struct {
    u64 framesPresentedAsync;      ///< Frames presented asynchronously
    u64 framesSyncWaits;           ///< Times we had to wait for GPU
    u64 audioUnderruns;            ///< Audio buffer underruns
    u64 rspTasksQueued;            ///< RSP tasks queued
    u64 rspTasksCompleted;         ///< RSP tasks completed
    double avgPresentLatencyMs;    ///< Average presentation latency
    double avgRspTimeMs;           ///< Average RSP task time
} CV64_ThreadStats;

/*===========================================================================
 * Core Threading API
 *===========================================================================*/

/**
 * @brief Initialize the threading system
 * @param config Threading configuration (NULL for defaults)
 * @return true on success
 */
bool CV64_Threading_Init(const CV64_ThreadConfig* config);

/**
 * @brief Shutdown the threading system
 * Waits for all threads to complete and releases resources.
 */
void CV64_Threading_Shutdown(void);

/**
 * @brief Get current threading statistics
 * @param stats Output statistics structure
 */
void CV64_Threading_GetStats(CV64_ThreadStats* stats);

/**
 * @brief Reset threading statistics
 */
void CV64_Threading_ResetStats(void);

/**
 * @brief Check if async graphics is active
 * @return true if async graphics thread is running
 */
bool CV64_Threading_IsAsyncGraphicsEnabled(void);

/*===========================================================================
 * Graphics Thread API
 *===========================================================================*/

/**
 * @brief Queue a frame for async presentation
 * This allows the CPU to continue while the GPU presents.
 * 
 * @param frameData Pointer to frame data (ownership transferred)
 * @param width Frame width
 * @param height Frame height
 * @return true if queued successfully, false if queue full
 */
bool CV64_Graphics_QueueFrame(void* frameData, int width, int height);

/**
 * @brief Signal that VI interrupt occurred (frame boundary)
 * Used to synchronize frame timing.
 */
void CV64_Graphics_OnVIInterrupt(void);

/**
 * @brief Wait for GPU to finish presenting all queued frames
 * Call this before modifying RDRAM that GPU might be reading.
 */
void CV64_Graphics_WaitForPresent(void);

/**
 * @brief Set the target frame rate for presentation
 * @param fps Target FPS (typically 60 for NTSC, 50 for PAL)
 */
void CV64_Graphics_SetTargetFPS(int fps);

/*===========================================================================
 * Audio Thread API
 *===========================================================================*/

/**
 * @brief Queue audio samples for async playback
 * @param samples Audio sample data (copied)
 * @param count Number of samples
 * @param frequency Sample rate
 * @return true if queued successfully
 */
bool CV64_Audio_QueueSamples(const s16* samples, size_t count, int frequency);

/**
 * @brief Get current audio queue depth
 * @return Number of samples currently queued
 */
size_t CV64_Audio_GetQueueDepth(void);

/**
 * @brief Signal audio DMA complete
 * Called when AI DMA transfer finishes.
 */
void CV64_Audio_OnDMAComplete(void);

/*===========================================================================
 * Worker Thread Pool API
 *===========================================================================*/

/**
 * @brief Task completion callback
 */
typedef void (*CV64_TaskCallback)(void* result, void* userdata);

/**
 * @brief Task function signature
 */
typedef void* (*CV64_TaskFunc)(void* param);

/**
 * @brief Queue a task for async execution
 * @param func Task function to execute
 * @param param Parameter to pass to task
 * @param callback Callback when complete (can be NULL)
 * @param userdata User data for callback
 * @return Task ID (0 on failure)
 */
u32 CV64_Worker_QueueTask(CV64_TaskFunc func, void* param, 
                          CV64_TaskCallback callback, void* userdata);

/**
 * @brief Wait for a specific task to complete
 * @param taskId Task ID returned from QueueTask
 * @param timeoutMs Timeout in milliseconds (0 = infinite)
 * @return true if task completed, false on timeout
 */
bool CV64_Worker_WaitTask(u32 taskId, u32 timeoutMs);

/**
 * @brief Wait for all queued tasks to complete
 */
void CV64_Worker_WaitAll(void);

/*===========================================================================
 * RSP Threading (EXPERIMENTAL)
 *===========================================================================*/

/**
 * @brief RSP task types for parallel processing
 */
typedef enum {
    CV64_RSP_TASK_GRAPHICS,   ///< Graphics microcode (can't really parallelize)
    CV64_RSP_TASK_AUDIO,      ///< Audio processing (can parallelize mixing)
    CV64_RSP_TASK_UNKNOWN     ///< Unknown task type
} CV64_RSPTaskType;

/**
 * @brief Queue RSP task for async processing
 * NOTE: This is EXPERIMENTAL and may cause issues!
 * 
 * @param taskType Type of RSP task
 * @param taskData Task data pointer
 * @param taskSize Task data size
 * @return true if queued (task will be processed async)
 */
bool CV64_RSP_QueueTask(CV64_RSPTaskType taskType, const void* taskData, size_t taskSize);

/**
 * @brief Check if RSP task is complete
 * @return true if no pending RSP tasks
 */
bool CV64_RSP_IsIdle(void);

/**
 * @brief Wait for all RSP tasks to complete
 */
void CV64_RSP_WaitAll(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_THREADING_H */
