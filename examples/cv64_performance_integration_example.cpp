/**
 * @file cv64_performance_integration_example.cpp
 * @brief Example integration of performance optimizations into CV64 RMG
 * 
 * This file demonstrates how to integrate the performance optimization
 * system into your main application loop and GlideN64 renderer.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_performance_optimizations.h"
#include "../include/cv64_threading.h"
#include <stdio.h>

/*===========================================================================
 * Example 1: Application Initialization
 *===========================================================================*/

void InitializeCV64Performance() {
    printf("Initializing CV64 Performance Optimizations...\n");
    
    // Step 1: Initialize threading system (if not already done)
    CV64_ThreadConfig threadConfig = {
        .enableAsyncGraphics = true,
        .enableAsyncAudio = true,
        .enableWorkerThreads = true,
        .workerThreadCount = 0,  // Auto-detect based on CPU cores
        .graphicsQueueDepth = 2,  // Double buffering
        .enableParallelRSP = false  // Keep disabled unless you know what you're doing
    };
    
    if (CV64_Threading_Init(&threadConfig)) {
        printf("? Threading system initialized\n");
    }
    
    // Step 2: Initialize performance optimization system
    if (CV64_Perf_Initialize()) {
        printf("? Performance optimizations initialized\n");
    }
    
    // Step 3: Configure performance settings (optional)
    CV64_PerformanceConfig config = {
        // Renderer optimizations (HIGHLY RECOMMENDED)
        .enableTextureCaching = true,
        .enableFramebufferOptimization = true,
        .enableDrawCallBatching = true,
        .maxTextureSize = 2048,  // 1024 for low-end, 4096 for high-end
        .mipmapBias = 0.0f,
        
        // RSP optimizations (RECOMMENDED)
        .enableParallelAudioMixing = true,
        .enableRSPTaskPrioritization = true,
        .audioBufferSize = 2048,
        .maxRSPTasksPerFrame = 16,
        
        // VSync and frame pacing (CHOOSE BEST MODE FOR YOUR MONITOR)
        .vsyncMode = CV64_VSYNC_ADAPTIVE,  // Best choice!
        .targetFPS = 60,
        .allowFrameSkip = false,  // Enable on low-end systems
        .maxConsecutiveFrameSkips = 2,
        
        // Memory optimizations
        .enableMemoryPooling = true,
        .textureMemoryBudget = 512 * 1024 * 1024,  // 512 MB - adjust if needed
        .preallocateBuffers = true,
        
        // Quality settings (adjust based on GPU capability)
        .multisampleSamples = 0,  // 0=off, 2/4/8 for MSAA
        .anisotropicFiltering = 4,  // 0-16, higher = better quality
        .enablePostProcessing = true
    };
    
    CV64_Perf_SetConfig(&config);
    printf("? Performance configuration applied\n");
    
    // Optional: Print current VSync mode
    const char* vsyncModes[] = { "OFF", "ON", "ADAPTIVE", "HALF" };
    printf("  VSync Mode: %s\n", vsyncModes[config.vsyncMode]);
    printf("  Texture Cache: %zu MB budget\n", 
           config.textureMemoryBudget / (1024 * 1024));
    printf("  Max Texture Size: %u x %u\n", 
           config.maxTextureSize, config.maxTextureSize);
}

/*===========================================================================
 * Example 2: Main Game Loop Integration
 *===========================================================================*/

void CV64_MainGameLoop() {
    // This would be called every frame in your main loop
    
    // 1. Mark beginning of frame (for frame timing)
    CV64_Perf_BeginFrame();
    
    // 2. Check if we should skip this frame (optional, for low-end systems)
    if (CV64_Perf_ShouldSkipFrame()) {
        // Skip rendering this frame to catch up
        return;
    }
    
    // 3. Begin draw call batching for this frame
    CV64_Perf_BeginDrawCallBatching();
    
    // 4. Process emulation frame (mupen64plus core does this)
    // CoreDoCommand(M64CMD_ADVANCE_FRAME, ...)
    
    // 5. Render scene (GlideN64 does this)
    // The renderer will call our optimized functions
    
    // 6. Flush all batched draw calls
    CV64_Perf_FlushDrawCalls();
    
    // 7. Mark end of frame
    CV64_Perf_EndFrame();
}

/*===========================================================================
 * Example 3: GlideN64 Texture Upload Hook
 *===========================================================================*/

// This shows how GlideN64 would integrate texture caching
// You would modify GlideN64's texture upload code to call this

#include <gl/GL.h>

GLuint CV64_UploadTextureOptimized(uint32_t tmemAddr, 
                                    uint32_t width, 
                                    uint32_t height,
                                    uint32_t format,
                                    void* pixelData,
                                    uint32_t dataSize) {
    // Calculate CRC for cache validation
    // (Simple example - GlideN64 may have its own CRC calculation)
    uint32_t crc = 0;
    uint8_t* data = (uint8_t*)pixelData;
    for (uint32_t i = 0; i < dataSize; i++) {
        crc = (crc >> 8) ^ data[i];
    }
    
    // Check if texture is in cache
    GLuint cachedTexId;
    if (CV64_Perf_CacheTexture(tmemAddr, width, height, format, crc, &cachedTexId)) {
        // Cache hit! Reuse existing texture
        return cachedTexId;
    }
    
    // Cache miss - need to upload texture
    GLuint newTexId;
    glGenTextures(1, &newTexId);
    glBindTexture(GL_TEXTURE_2D, newTexId);
    
    // Upload texture to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Add to cache for future use
    CV64_Perf_AddToTextureCache(tmemAddr, width, height, format, crc, newTexId);
    
    return newTexId;
}

/*===========================================================================
 * Example 4: RSP Task Processing Hook
 *===========================================================================*/

// This shows how to integrate RSP task optimization
// You would call this from your RSP HLE implementation

void CV64_ProcessRSPTaskOptimized(void* rspTask) {
    // Determine task type
    // In mupen64plus, task->type indicates audio (0x02) or graphics (0x01)
    uint32_t* taskHeader = (uint32_t*)rspTask;
    uint32_t taskType = taskHeader[0];  // Simplified - actual structure is more complex
    
    // Let the optimization system handle it
    // This will automatically:
    // - Batch audio tasks for efficiency
    // - Prioritize audio over graphics
    // - Use worker threads for parallel processing
    CV64_Perf_ProcessRSPTask(rspTask, taskType);
    
    // Then do the actual RSP processing
    // (Your existing RSP HLE code)
}

/*===========================================================================
 * Example 5: Performance Monitoring
 *===========================================================================*/

void CV64_PrintPerformanceStats() {
    CV64_PerformanceStats stats;
    CV64_Perf_GetStats(&stats);
    
    printf("\n==================================================\n");
    printf("           CV64 Performance Statistics\n");
    printf("==================================================\n");
    
    // Frame timing
    printf("\nFrame Timing:\n");
    printf("  Current FPS:      %.1f\n", stats.currentFPS);
    printf("  Avg Frame Time:   %.2f ms\n", stats.avgFrameTimeMs);
    printf("  Total Frames:     %llu\n", stats.totalFrames);
    printf("  Dropped Frames:   %llu (%.2f%%)\n", 
           stats.droppedFrames,
           100.0 * stats.droppedFrames / (stats.totalFrames + 1));
    
    // Texture cache
    printf("\nTexture Cache:\n");
    printf("  Cache Hits:       %llu\n", stats.textureCacheHits);
    printf("  Cache Misses:     %llu\n", stats.textureCacheMisses);
    uint64_t totalAccesses = stats.textureCacheHits + stats.textureCacheMisses;
    if (totalAccesses > 0) {
        printf("  Hit Rate:         %.1f%%\n", 
               100.0 * stats.textureCacheHits / totalAccesses);
    }
    printf("  Cached Textures:  %zu\n", stats.textureCacheSize);
    printf("  Memory Used:      %.2f MB\n", stats.textureCacheMemoryMB);
    
    // Rendering
    printf("\nRendering:\n");
    printf("  Draw Calls:       %llu\n", stats.drawCalls);
    printf("  Texture Uploads:  %llu\n", stats.textureUploads);
    
    // RSP
    printf("\nRSP Tasks:\n");
    printf("  Audio Tasks:      %llu\n", stats.rspAudioTasks);
    printf("  Graphics Tasks:   %llu\n", stats.rspGraphicsTasks);
    
    // Performance mode
    printf("\nPerformance Mode:\n");
    printf("  Low Perf Mode:    %s\n", 
           stats.lowPerformanceMode ? "ACTIVE" : "INACTIVE");
    
    printf("==================================================\n\n");
}

/*===========================================================================
 * Example 6: Dynamic Performance Adjustment
 *===========================================================================*/

void CV64_AdjustPerformanceBasedOnFPS() {
    CV64_PerformanceStats stats;
    CV64_Perf_GetStats(&stats);
    
    // If FPS is too low, enable aggressive optimizations
    if (stats.currentFPS < 50.0 && !stats.lowPerformanceMode) {
        printf("FPS dropped below 50, enabling aggressive optimizations...\n");
        CV64_Perf_EnableAggressiveOptimizations(true);
    }
    
    // If FPS recovers, disable aggressive optimizations
    if (stats.currentFPS > 58.0 && stats.lowPerformanceMode) {
        printf("FPS recovered, disabling aggressive optimizations...\n");
        CV64_Perf_EnableAggressiveOptimizations(false);
    }
}

/*===========================================================================
 * Example 7: VSync Mode Toggling (for debugging)
 *===========================================================================*/

void CV64_CycleVSyncMode() {
    CV64_VSyncMode currentMode = CV64_Perf_GetVSyncMode();
    CV64_VSyncMode nextMode;
    
    switch (currentMode) {
        case CV64_VSYNC_OFF:
            nextMode = CV64_VSYNC_ON;
            break;
        case CV64_VSYNC_ON:
            nextMode = CV64_VSYNC_ADAPTIVE;
            break;
        case CV64_VSYNC_ADAPTIVE:
            nextMode = CV64_VSYNC_HALF;
            break;
        case CV64_VSYNC_HALF:
            nextMode = CV64_VSYNC_OFF;
            break;
        default:
            nextMode = CV64_VSYNC_ADAPTIVE;
    }
    
    const char* modeNames[] = { "OFF", "ON", "ADAPTIVE", "HALF" };
    printf("Changing VSync: %s -> %s\n", 
           modeNames[currentMode], modeNames[nextMode]);
    
    CV64_Perf_SetVSyncMode(nextMode);
}

/*===========================================================================
 * Example 8: Cleanup on Shutdown
 *===========================================================================*/

void ShutdownCV64Performance() {
    printf("Shutting down CV64 Performance Optimizations...\n");
    
    // Print final statistics
    CV64_PrintPerformanceStats();
    
    // Shutdown performance system
    CV64_Perf_Shutdown();
    printf("? Performance optimizations shut down\n");
    
    // Shutdown threading system
    CV64_Threading_Shutdown();
    printf("? Threading system shut down\n");
}

/*===========================================================================
 * Example 9: Configuration Presets
 *===========================================================================*/

void CV64_ApplyPerformancePreset(const char* presetName) {
    CV64_PerformanceConfig config = {};
    
    if (strcmp(presetName, "maximum") == 0) {
        // Maximum quality - for high-end systems
        config.enableTextureCaching = true;
        config.enableDrawCallBatching = true;
        config.maxTextureSize = 4096;
        config.vsyncMode = CV64_VSYNC_ON;
        config.multisampleSamples = 8;
        config.anisotropicFiltering = 16;
        config.enablePostProcessing = true;
        config.textureMemoryBudget = 1024 * 1024 * 1024;  // 1 GB
        printf("Applied 'Maximum Quality' preset\n");
    }
    else if (strcmp(presetName, "balanced") == 0) {
        // Balanced - default settings
        config.enableTextureCaching = true;
        config.enableDrawCallBatching = true;
        config.maxTextureSize = 2048;
        config.vsyncMode = CV64_VSYNC_ADAPTIVE;
        config.multisampleSamples = 0;
        config.anisotropicFiltering = 4;
        config.enablePostProcessing = true;
        config.textureMemoryBudget = 512 * 1024 * 1024;  // 512 MB
        printf("Applied 'Balanced' preset\n");
    }
    else if (strcmp(presetName, "performance") == 0) {
        // Performance - for low-end systems
        config.enableTextureCaching = true;
        config.enableDrawCallBatching = true;
        config.maxTextureSize = 1024;
        config.vsyncMode = CV64_VSYNC_OFF;
        config.multisampleSamples = 0;
        config.anisotropicFiltering = 1;
        config.enablePostProcessing = false;
        config.allowFrameSkip = true;
        config.maxConsecutiveFrameSkips = 3;
        config.textureMemoryBudget = 256 * 1024 * 1024;  // 256 MB
        printf("Applied 'Performance' preset\n");
    }
    
    // Apply common settings
    config.enableParallelAudioMixing = true;
    config.enableRSPTaskPrioritization = true;
    config.audioBufferSize = 2048;
    config.targetFPS = 60;
    
    CV64_Perf_SetConfig(&config);
}

/*===========================================================================
 * Example 10: Complete Integration Template
 *===========================================================================*/

int main() {
    // Initialize everything
    InitializeCV64Performance();
    
    // Optional: Apply a preset
    CV64_ApplyPerformancePreset("balanced");
    
    // Main game loop
    bool running = true;
    int frameCount = 0;
    
    while (running) {
        // Process input, update game state, etc.
        // ...
        
        // Render frame with optimizations
        CV64_MainGameLoop();
        
        frameCount++;
        
        // Every 60 frames (~1 second), check performance
        if (frameCount % 60 == 0) {
            CV64_AdjustPerformanceBasedOnFPS();
        }
        
        // Every 300 frames (~5 seconds), print stats
        if (frameCount % 300 == 0) {
            CV64_PrintPerformanceStats();
        }
    }
    
    // Cleanup
    ShutdownCV64Performance();
    
    return 0;
}
