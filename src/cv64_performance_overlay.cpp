/**
 * @file cv64_performance_overlay.cpp
 * @brief Performance statistics overlay implementation
 * 
 * Simple text-based overlay using GDI for rendering performance stats.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_performance_overlay.h"
#include "../include/cv64_threading.h"

#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <deque>

#pragma comment(lib, "opengl32.lib")

/*===========================================================================
 * Configuration
 *===========================================================================*/

#define MAX_FRAME_HISTORY 120  // 2 seconds at 60fps
#define GRAPH_WIDTH 200
#define GRAPH_HEIGHT 60

/*===========================================================================
 * State
 *===========================================================================*/

static struct {
    bool initialized;
    CV64_PerfOverlayMode mode;
    
    // Frame timing
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::deque<double> frameTimeHistory;
    
    // Statistics
    CV64_PerfStats stats;
    
    // FPS calculation
    double fpsAccumulator;
    int fpsFrameCount;
    double lastFpsUpdate;
    
} g_overlay = {
    false,
    CV64_PERF_OVERLAY_OFF,
    {},
    {},
    {},
    0.0, 0, 0.0
};

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void DrawText(int x, int y, const char* text, COLORREF color) {
    // Note: This is a placeholder. In a real implementation, you'd use:
    // - OpenGL text rendering (FreeType + texture atlas)
    // - ImGui overlay
    // - GDI text blit to texture
    // For now, we just output to debug console
    (void)x; (void)y; (void)color;
    OutputDebugStringA(text);
}

static void DrawRect(int x, int y, int width, int height, COLORREF color) {
    // Placeholder for rectangle drawing
    (void)x; (void)y; (void)width; (void)height; (void)color;
}

/*===========================================================================
 * Public API
 *===========================================================================*/

bool CV64_PerfOverlay_Init() {
    if (g_overlay.initialized) {
        return true;
    }
    
    g_overlay.mode = CV64_PERF_OVERLAY_OFF;
    g_overlay.lastFrameTime = std::chrono::high_resolution_clock::now();
    g_overlay.frameTimeHistory.clear();
    memset(&g_overlay.stats, 0, sizeof(g_overlay.stats));
    g_overlay.fpsAccumulator = 0.0;
    g_overlay.fpsFrameCount = 0;
    g_overlay.lastFpsUpdate = 0.0;
    
    g_overlay.initialized = true;
    OutputDebugStringA("[CV64_PERF] Performance overlay initialized\n");
    
    return true;
}

void CV64_PerfOverlay_Shutdown() {
    if (!g_overlay.initialized) {
        return;
    }
    
    g_overlay.frameTimeHistory.clear();
    g_overlay.initialized = false;
    
    OutputDebugStringA("[CV64_PERF] Performance overlay shutdown\n");
}

void CV64_PerfOverlay_SetMode(CV64_PerfOverlayMode mode) {
    g_overlay.mode = mode;
    
    const char* modeNames[] = {
        "OFF", "MINIMAL", "STANDARD", "DETAILED", "GRAPH"
    };
    
    char msg[128];
    sprintf_s(msg, "[CV64_PERF] Overlay mode: %s\n", modeNames[mode]);
    OutputDebugStringA(msg);
}

CV64_PerfOverlayMode CV64_PerfOverlay_GetMode() {
    return g_overlay.mode;
}

void CV64_PerfOverlay_Toggle() {
    int nextMode = ((int)g_overlay.mode + 1) % 5;
    CV64_PerfOverlay_SetMode((CV64_PerfOverlayMode)nextMode);
}

void CV64_PerfOverlay_Update(double deltaTime) {
    if (!g_overlay.initialized) {
        return;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    double frameTimeMs = std::chrono::duration<double, std::milli>(
        now - g_overlay.lastFrameTime).count();
    g_overlay.lastFrameTime = now;
    
    // Record frame time
    CV64_PerfOverlay_RecordFrame(frameTimeMs);
    
    // Update FPS
    g_overlay.fpsAccumulator += deltaTime;
    g_overlay.fpsFrameCount++;
    
    if (g_overlay.fpsAccumulator >= 1.0) {
        g_overlay.stats.fps = g_overlay.fpsFrameCount / g_overlay.fpsAccumulator;
        g_overlay.fpsFrameCount = 0;
        g_overlay.fpsAccumulator = 0.0;
        g_overlay.lastFpsUpdate = 0.0;
    }
    
    // Update frame time stats
    if (!g_overlay.frameTimeHistory.empty()) {
        double sum = 0.0;
        double min = 999999.0;
        double max = 0.0;
        
        for (double ft : g_overlay.frameTimeHistory) {
            sum += ft;
            if (ft < min) min = ft;
            if (ft > max) max = ft;
        }
        
        g_overlay.stats.avgFrameTimeMs = sum / g_overlay.frameTimeHistory.size();
        g_overlay.stats.minFrameTimeMs = min;
        g_overlay.stats.maxFrameTimeMs = max;
    }
    
    // Get threading stats if available
    if (CV64_Threading_IsAsyncGraphicsEnabled()) {
        CV64_ThreadStats threadStats;
        CV64_Threading_GetStats(&threadStats);
        
        g_overlay.stats.avgPresentLatencyMs = threadStats.avgPresentLatencyMs;
        g_overlay.stats.audioUnderruns = threadStats.audioUnderruns;
        g_overlay.stats.gpuSyncWaits = threadStats.framesSyncWaits;
    }
    
    g_overlay.stats.totalFrames++;
}

void CV64_PerfOverlay_Render(int width, int height) {
    if (!g_overlay.initialized || g_overlay.mode == CV64_PERF_OVERLAY_OFF) {
        return;
    }
    
    char buffer[512];
    int x = 10;
    int y = 10;
    int lineHeight = 20;
    
    // Save OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    
    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Draw semi-transparent background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    
    int bgHeight = lineHeight * 
        (g_overlay.mode == CV64_PERF_OVERLAY_MINIMAL ? 2 :
         g_overlay.mode == CV64_PERF_OVERLAY_STANDARD ? 4 :
         g_overlay.mode == CV64_PERF_OVERLAY_DETAILED ? 10 : 12);
    
    glBegin(GL_QUADS);
    glVertex2f((float)x - 5, (float)y - 5);
    glVertex2f((float)x + 300, (float)y - 5);
    glVertex2f((float)x + 300, (float)y + bgHeight);
    glVertex2f((float)x - 5, (float)y + bgHeight);
    glEnd();
    
    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // Restore OpenGL state
    glPopAttrib();
    
    // Output to debug console (text rendering would go here with proper font rendering)
    switch (g_overlay.mode) {
        case CV64_PERF_OVERLAY_MINIMAL:
            sprintf_s(buffer, "FPS: %.1f\n", g_overlay.stats.fps);
            OutputDebugStringA(buffer);
            break;
            
        case CV64_PERF_OVERLAY_STANDARD:
            sprintf_s(buffer, 
                "FPS: %.1f\n"
                "Frame Time: %.2f ms (avg)\n"
                "Min/Max: %.2f / %.2f ms\n",
                g_overlay.stats.fps,
                g_overlay.stats.avgFrameTimeMs,
                g_overlay.stats.minFrameTimeMs,
                g_overlay.stats.maxFrameTimeMs);
            OutputDebugStringA(buffer);
            break;
            
        case CV64_PERF_OVERLAY_DETAILED:
            sprintf_s(buffer,
                "=== CV64 Performance ===\n"
                "FPS: %.1f (%.2f ms avg)\n"
                "Frame Time Range: %.2f - %.2f ms\n"
                "Total Frames: %llu\n"
                "\n"
                "=== Threading ===\n"
                "Present Latency: %.2f ms\n"
                "Audio Underruns: %llu\n"
                "GPU Sync Waits: %llu\n",
                g_overlay.stats.fps,
                g_overlay.stats.avgFrameTimeMs,
                g_overlay.stats.minFrameTimeMs,
                g_overlay.stats.maxFrameTimeMs,
                g_overlay.stats.totalFrames,
                g_overlay.stats.avgPresentLatencyMs,
                g_overlay.stats.audioUnderruns,
                g_overlay.stats.gpuSyncWaits);
            OutputDebugStringA(buffer);
            break;
            
        case CV64_PERF_OVERLAY_GRAPH:
            // TODO: Implement frame time graph using OpenGL line rendering
            sprintf_s(buffer, "FPS: %.1f [GRAPH MODE]\n", g_overlay.stats.fps);
            OutputDebugStringA(buffer);
            break;
            
        default:
            break;
    }
}

void CV64_PerfOverlay_GetStats(CV64_PerfStats* stats) {
    if (!stats) return;
    *stats = g_overlay.stats;
}

void CV64_PerfOverlay_ResetStats() {
    g_overlay.stats.totalFrames = 0;
    g_overlay.stats.audioUnderruns = 0;
    g_overlay.stats.gpuSyncWaits = 0;
    g_overlay.frameTimeHistory.clear();
}

void CV64_PerfOverlay_RecordFrame(double frameTimeMs) {
    g_overlay.frameTimeHistory.push_back(frameTimeMs);
    
    // Keep only last N frames
    while (g_overlay.frameTimeHistory.size() > MAX_FRAME_HISTORY) {
        g_overlay.frameTimeHistory.pop_front();
    }
}
