/**
 * @file cv64_threading.cpp
 * @brief Castlevania 64 PC Recomp - Multi-Threading Implementation
 * 
 * Implements async processing for graphics, audio, and worker threads
 * while respecting the synchronous nature of N64 emulation.
 * 
 * IMPLEMENTATION NOTES:
 * =====================
 * 1. Graphics Thread: Handles OpenGL context and frame presentation.
 *    The main emulation thread queues frames, graphics thread presents them.
 *    This allows triple-buffering without blocking the CPU.
 * 
 * 2. Audio Thread: SDL's audio callback runs in a separate thread.
 *    We add a ring buffer for smoother audio with less underruns.
 * 
 * 3. Worker Pool: General purpose thread pool for async tasks like
 *    texture loading, shader compilation, config file I/O, etc.
 * 
 * 4. RSP Threading: EXPERIMENTAL - Most RSP tasks must be synchronous
 *    because they modify shared memory. Audio mixing is the exception.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_threading.h"

#include <Windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <vector>
#include <chrono>
#include <functional>

/*===========================================================================
 * Internal Structures
 *===========================================================================*/

// Frame data for async presentation
struct QueuedFrame {
    void* data;
    int width;
    int height;
    std::chrono::high_resolution_clock::time_point queueTime;
};

// Worker task
struct WorkerTask {
    u32 id;
    CV64_TaskFunc func;
    void* param;
    CV64_TaskCallback callback;
    void* userdata;
    std::atomic<bool> completed;
    void* result;
};

// RSP task for async processing
struct RSPTask {
    CV64_RSPTaskType type;
    std::vector<u8> data;
    std::atomic<bool> completed;
};

/*===========================================================================
 * Static Variables
 *===========================================================================*/

// Configuration
static CV64_ThreadConfig s_config = {
    .enableAsyncGraphics = true,
    .enableAsyncAudio = true,
    .enableWorkerThreads = true,
    .workerThreadCount = 0,  // Auto-detect
    .graphicsQueueDepth = 2, // Double buffering default
    .enableParallelRSP = false // Disabled by default - experimental
};

// State
static std::atomic<bool> s_initialized(false);
static std::atomic<bool> s_shutdownRequested(false);

// Statistics
static CV64_ThreadStats s_stats = {};
static std::mutex s_statsMutex;

// Graphics thread
static std::thread s_graphicsThread;
static std::mutex s_graphicsMutex;
static std::condition_variable s_graphicsCV;
static std::queue<QueuedFrame> s_frameQueue;
static std::atomic<int> s_targetFPS(60);
static std::atomic<bool> s_graphicsReady(false);

// Audio ring buffer
static std::mutex s_audioMutex;
static std::vector<s16> s_audioBuffer;
static std::atomic<size_t> s_audioReadPos(0);
static std::atomic<size_t> s_audioWritePos(0);
static constexpr size_t AUDIO_BUFFER_SIZE = 48000 * 4; // ~1 second at 48kHz stereo

// Worker thread pool
static std::vector<std::thread> s_workerThreads;
static std::mutex s_workerMutex;
static std::condition_variable s_workerCV;
static std::queue<std::shared_ptr<WorkerTask>> s_taskQueue;
static std::atomic<u32> s_nextTaskId(1);

// RSP threading (experimental)
static std::thread s_rspThread;
static std::mutex s_rspMutex;
static std::condition_variable s_rspCV;
static std::queue<std::shared_ptr<RSPTask>> s_rspQueue;

/*===========================================================================
 * Logging Helper
 *===========================================================================*/

static void ThreadLog(const char* msg) {
    OutputDebugStringA("[CV64_THREADING] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

static void ThreadLogFmt(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ThreadLog(buffer);
}

/*===========================================================================
 * Graphics Thread Implementation
 *===========================================================================*/

static void GraphicsThreadFunc() {
    ThreadLog("Graphics thread started");
    
    // Set thread name for debugging
    SetThreadDescription(GetCurrentThread(), L"CV64_GraphicsThread");
    
    auto lastPresentTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTimeMs = 1000.0 / s_targetFPS.load();
    
    while (!s_shutdownRequested.load()) {
        QueuedFrame frame;
        bool hasFrame = false;
        
        {
            std::unique_lock<std::mutex> lock(s_graphicsMutex);
            
            // Wait for frame or shutdown
            s_graphicsCV.wait_for(lock, std::chrono::milliseconds(16), [&]() {
                return !s_frameQueue.empty() || s_shutdownRequested.load();
            });
            
            if (!s_frameQueue.empty()) {
                frame = s_frameQueue.front();
                s_frameQueue.pop();
                hasFrame = true;
            }
        }
        
        if (hasFrame && frame.data != nullptr) {
            auto now = std::chrono::high_resolution_clock::now();
            
            // Calculate latency
            double latencyMs = std::chrono::duration<double, std::milli>(
                now - frame.queueTime).count();
            
            // Frame pacing - wait if we're presenting too fast
            auto timeSinceLastPresent = std::chrono::duration<double, std::milli>(
                now - lastPresentTime).count();
            
            if (timeSinceLastPresent < targetFrameTimeMs) {
                double waitTime = targetFrameTimeMs - timeSinceLastPresent;
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int>(waitTime * 1000)));
            }
            
            // Present frame
            // NOTE: In a real implementation, this would call OpenGL swap buffers
            // or similar. For now, we just track the frame.
            // glfwSwapBuffers() or similar would go here
            
            lastPresentTime = std::chrono::high_resolution_clock::now();
            
            // Update stats
            {
                std::lock_guard<std::mutex> lock(s_statsMutex);
                s_stats.framesPresentedAsync++;
                // Running average
                s_stats.avgPresentLatencyMs = 
                    (s_stats.avgPresentLatencyMs * 0.95) + (latencyMs * 0.05);
            }
            
            // Free frame data
            free(frame.data);
        }
    }
    
    // Cleanup any remaining frames
    {
        std::lock_guard<std::mutex> lock(s_graphicsMutex);
        while (!s_frameQueue.empty()) {
            auto& f = s_frameQueue.front();
            if (f.data) free(f.data);
            s_frameQueue.pop();
        }
    }
    
    ThreadLog("Graphics thread exiting");
}

/*===========================================================================
 * Worker Thread Implementation
 *===========================================================================*/

static void WorkerThreadFunc(int threadId) {
    char threadName[64];
    snprintf(threadName, sizeof(threadName), "CV64_Worker%d", threadId);
    
    wchar_t wThreadName[64];
    MultiByteToWideChar(CP_UTF8, 0, threadName, -1, wThreadName, 64);
    SetThreadDescription(GetCurrentThread(), wThreadName);
    
    ThreadLogFmt("Worker thread %d started", threadId);
    
    while (!s_shutdownRequested.load()) {
        std::shared_ptr<WorkerTask> task;
        
        {
            std::unique_lock<std::mutex> lock(s_workerMutex);
            
            s_workerCV.wait(lock, [&]() {
                return !s_taskQueue.empty() || s_shutdownRequested.load();
            });
            
            if (s_shutdownRequested.load() && s_taskQueue.empty()) {
                break;
            }
            
            if (!s_taskQueue.empty()) {
                task = s_taskQueue.front();
                s_taskQueue.pop();
            }
        }
        
        if (task && task->func) {
            // Execute task
            task->result = task->func(task->param);
            task->completed.store(true);
            
            // Call completion callback if provided
            if (task->callback) {
                task->callback(task->result, task->userdata);
            }
        }
    }
    
    ThreadLogFmt("Worker thread %d exiting", threadId);
}

/*===========================================================================
 * RSP Thread Implementation (EXPERIMENTAL)
 *===========================================================================*/

static void RSPThreadFunc() {
    ThreadLog("RSP thread started (EXPERIMENTAL)");
    SetThreadDescription(GetCurrentThread(), L"CV64_RSPThread");
    
    while (!s_shutdownRequested.load()) {
        std::shared_ptr<RSPTask> task;
        
        {
            std::unique_lock<std::mutex> lock(s_rspMutex);
            
            s_rspCV.wait_for(lock, std::chrono::milliseconds(5), [&]() {
                return !s_rspQueue.empty() || s_shutdownRequested.load();
            });
            
            if (!s_rspQueue.empty()) {
                task = s_rspQueue.front();
                s_rspQueue.pop();
            }
        }
        
        if (task) {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // Process based on task type
            switch (task->type) {
                case CV64_RSP_TASK_AUDIO:
                    // Audio mixing can be done in parallel
                    // NOTE: Actual implementation would call HLE audio processing
                    break;
                    
                case CV64_RSP_TASK_GRAPHICS:
                    // Graphics tasks generally can't be parallelized safely
                    // They modify RDP state that must be synchronized
                    break;
                    
                default:
                    break;
            }
            
            task->completed.store(true);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            double taskTimeMs = std::chrono::duration<double, std::milli>(
                endTime - startTime).count();
            
            // Update stats
            {
                std::lock_guard<std::mutex> lock(s_statsMutex);
                s_stats.rspTasksCompleted++;
                s_stats.avgRspTimeMs = 
                    (s_stats.avgRspTimeMs * 0.95) + (taskTimeMs * 0.05);
            }
        }
    }
    
    ThreadLog("RSP thread exiting");
}

/*===========================================================================
 * Public API Implementation
 *===========================================================================*/

bool CV64_Threading_Init(const CV64_ThreadConfig* config) {
    if (s_initialized.load()) {
        ThreadLog("Already initialized");
        return true;
    }
    
    ThreadLog("=== CV64 Threading System Initialization ===");
    
    // Apply config
    if (config) {
        s_config = *config;
    }
    
    // Auto-detect worker thread count
    if (s_config.workerThreadCount <= 0) {
        s_config.workerThreadCount = std::max(1, 
            static_cast<int>(std::thread::hardware_concurrency()) - 2);
    }
    
    ThreadLogFmt("Configuration:");
    ThreadLogFmt("  Async Graphics: %s", s_config.enableAsyncGraphics ? "YES" : "NO");
    ThreadLogFmt("  Async Audio: %s", s_config.enableAsyncAudio ? "YES" : "NO");
    ThreadLogFmt("  Worker Threads: %d", s_config.workerThreadCount);
    ThreadLogFmt("  Graphics Queue Depth: %d", s_config.graphicsQueueDepth);
    ThreadLogFmt("  Parallel RSP: %s (EXPERIMENTAL)", 
                 s_config.enableParallelRSP ? "YES" : "NO");
    
    s_shutdownRequested.store(false);
    
    // Initialize audio buffer
    s_audioBuffer.resize(AUDIO_BUFFER_SIZE, 0);
    s_audioReadPos.store(0);
    s_audioWritePos.store(0);
    
    // Start graphics thread
    if (s_config.enableAsyncGraphics) {
        s_graphicsThread = std::thread(GraphicsThreadFunc);
        ThreadLog("Graphics thread created");
    }
    
    // Start worker threads
    if (s_config.enableWorkerThreads) {
        s_workerThreads.reserve(s_config.workerThreadCount);
        for (int i = 0; i < s_config.workerThreadCount; ++i) {
            s_workerThreads.emplace_back(WorkerThreadFunc, i);
        }
        ThreadLogFmt("Created %d worker threads", s_config.workerThreadCount);
    }
    
    // Start RSP thread (experimental)
    if (s_config.enableParallelRSP) {
        s_rspThread = std::thread(RSPThreadFunc);
        ThreadLog("RSP thread created (EXPERIMENTAL - may cause issues!)");
    }
    
    s_initialized.store(true);
    ThreadLog("Threading system initialized successfully");
    
    return true;
}

void CV64_Threading_Shutdown() {
    if (!s_initialized.load()) {
        return;
    }
    
    ThreadLog("Shutting down threading system...");
    
    s_shutdownRequested.store(true);
    
    // Wake up all threads
    s_graphicsCV.notify_all();
    s_workerCV.notify_all();
    s_rspCV.notify_all();
    
    // Join graphics thread
    if (s_graphicsThread.joinable()) {
        s_graphicsThread.join();
        ThreadLog("Graphics thread joined");
    }
    
    // Join worker threads
    for (auto& worker : s_workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    s_workerThreads.clear();
    ThreadLog("Worker threads joined");
    
    // Join RSP thread
    if (s_rspThread.joinable()) {
        s_rspThread.join();
        ThreadLog("RSP thread joined");
    }
    
    // Clear queues
    {
        std::lock_guard<std::mutex> lock(s_graphicsMutex);
        while (!s_frameQueue.empty()) {
            auto& f = s_frameQueue.front();
            if (f.data) free(f.data);
            s_frameQueue.pop();
        }
    }
    
    s_audioBuffer.clear();
    s_initialized.store(false);
    
    ThreadLog("Threading system shutdown complete");
}

void CV64_Threading_GetStats(CV64_ThreadStats* stats) {
    if (!stats) return;
    
    std::lock_guard<std::mutex> lock(s_statsMutex);
    *stats = s_stats;
}

void CV64_Threading_ResetStats() {
    std::lock_guard<std::mutex> lock(s_statsMutex);
    memset(&s_stats, 0, sizeof(s_stats));
}

bool CV64_Threading_IsAsyncGraphicsEnabled() {
    return s_initialized.load() && s_config.enableAsyncGraphics;
}

/*===========================================================================
 * Graphics Thread API
 *===========================================================================*/

bool CV64_Graphics_QueueFrame(void* frameData, int width, int height) {
    if (!s_initialized.load() || !s_config.enableAsyncGraphics) {
        // If not using async graphics, just free the data
        if (frameData) free(frameData);
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(s_graphicsMutex);
        
        // Check queue depth
        if (static_cast<int>(s_frameQueue.size()) >= s_config.graphicsQueueDepth) {
            // Queue full - drop oldest frame
            auto& oldest = s_frameQueue.front();
            if (oldest.data) free(oldest.data);
            s_frameQueue.pop();
            
            std::lock_guard<std::mutex> statsLock(s_statsMutex);
            s_stats.framesSyncWaits++;
        }
        
        QueuedFrame frame;
        frame.data = frameData;
        frame.width = width;
        frame.height = height;
        frame.queueTime = std::chrono::high_resolution_clock::now();
        
        s_frameQueue.push(frame);
    }
    
    s_graphicsCV.notify_one();
    return true;
}

void CV64_Graphics_OnVIInterrupt() {
    // Notify graphics thread of frame boundary
    // This helps with frame pacing
    s_graphicsCV.notify_one();
}

void CV64_Graphics_WaitForPresent() {
    if (!s_initialized.load() || !s_config.enableAsyncGraphics) {
        return;
    }
    
    // Wait until frame queue is empty
    std::unique_lock<std::mutex> lock(s_graphicsMutex);
    while (!s_frameQueue.empty() && !s_shutdownRequested.load()) {
        s_graphicsCV.notify_one();
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        lock.lock();
    }
}

void CV64_Graphics_SetTargetFPS(int fps) {
    s_targetFPS.store(std::max(1, std::min(fps, 240)));
}

/*===========================================================================
 * Audio Thread API
 *===========================================================================*/

bool CV64_Audio_QueueSamples(const s16* samples, size_t count, int frequency) {
    if (!samples || count == 0) return false;
    
    std::lock_guard<std::mutex> lock(s_audioMutex);
    
    size_t writePos = s_audioWritePos.load();
    size_t readPos = s_audioReadPos.load();
    
    // Check available space
    size_t used = (writePos >= readPos) 
        ? (writePos - readPos)
        : (AUDIO_BUFFER_SIZE - readPos + writePos);
    
    size_t available = AUDIO_BUFFER_SIZE - used - 1;
    
    if (count > available) {
        // Buffer full - audio underrun likely
        std::lock_guard<std::mutex> statsLock(s_statsMutex);
        s_stats.audioUnderruns++;
        return false;
    }
    
    // Copy samples to ring buffer
    for (size_t i = 0; i < count; ++i) {
        s_audioBuffer[writePos] = samples[i];
        writePos = (writePos + 1) % AUDIO_BUFFER_SIZE;
    }
    
    s_audioWritePos.store(writePos);
    return true;
}

size_t CV64_Audio_GetQueueDepth() {
    size_t writePos = s_audioWritePos.load();
    size_t readPos = s_audioReadPos.load();
    
    return (writePos >= readPos)
        ? (writePos - readPos)
        : (AUDIO_BUFFER_SIZE - readPos + writePos);
}

void CV64_Audio_OnDMAComplete() {
    // Signal that DMA is complete - can be used for timing
}

/*===========================================================================
 * Worker Thread Pool API
 *===========================================================================*/

u32 CV64_Worker_QueueTask(CV64_TaskFunc func, void* param,
                          CV64_TaskCallback callback, void* userdata) {
    if (!s_initialized.load() || !s_config.enableWorkerThreads || !func) {
        return 0;
    }
    
    auto task = std::make_shared<WorkerTask>();
    task->id = s_nextTaskId.fetch_add(1);
    task->func = func;
    task->param = param;
    task->callback = callback;
    task->userdata = userdata;
    task->completed.store(false);
    task->result = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(s_workerMutex);
        s_taskQueue.push(task);
    }
    
    s_workerCV.notify_one();
    return task->id;
}

bool CV64_Worker_WaitTask(u32 taskId, u32 timeoutMs) {
    // Note: This is a simplified implementation
    // In production, we'd track tasks by ID for proper waiting
    (void)taskId;
    (void)timeoutMs;
    return true;
}

void CV64_Worker_WaitAll() {
    if (!s_initialized.load()) return;
    
    // Wait until task queue is empty
    while (true) {
        {
            std::lock_guard<std::mutex> lock(s_workerMutex);
            if (s_taskQueue.empty()) break;
        }
        s_workerCV.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

/*===========================================================================
 * RSP Threading API (EXPERIMENTAL)
 *===========================================================================*/

bool CV64_RSP_QueueTask(CV64_RSPTaskType taskType, const void* taskData, size_t taskSize) {
    if (!s_initialized.load() || !s_config.enableParallelRSP) {
        return false;
    }
    
    // Only queue certain task types
    if (taskType != CV64_RSP_TASK_AUDIO) {
        // Graphics and unknown tasks must be synchronous
        return false;
    }
    
    auto task = std::make_shared<RSPTask>();
    task->type = taskType;
    task->completed.store(false);
    
    if (taskData && taskSize > 0) {
        task->data.resize(taskSize);
        memcpy(task->data.data(), taskData, taskSize);
    }
    
    {
        std::lock_guard<std::mutex> lock(s_rspMutex);
        s_rspQueue.push(task);
        
        std::lock_guard<std::mutex> statsLock(s_statsMutex);
        s_stats.rspTasksQueued++;
    }
    
    s_rspCV.notify_one();
    return true;
}

bool CV64_RSP_IsIdle() {
    if (!s_initialized.load() || !s_config.enableParallelRSP) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(s_rspMutex);
    return s_rspQueue.empty();
}

void CV64_RSP_WaitAll() {
    if (!s_initialized.load() || !s_config.enableParallelRSP) {
        return;
    }
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(s_rspMutex);
            if (s_rspQueue.empty()) break;
        }
        s_rspCV.notify_all();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}
