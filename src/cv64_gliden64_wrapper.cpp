/**
 * @file cv64_gliden64_wrapper.cpp
 * @brief Wrapper for GLideN64 static plugin integration
 * 
 * This file provides the wrapper functions that bridge the mupen64plus core
 * to the statically linked GLideN64 video plugin. Symbol renaming is handled
 * in the GLideN64 static library project via preprocessor definitions.
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS
#ifdef CV64_USE_GLIDEN64

#include "../include/cv64_gliden64_static.h"
#include <Windows.h>
#include <cstdio>

/* Log helper */
static void GlideN64WrapperLog(const char* msg) {
    OutputDebugStringA("[CV64_GLIDEN64] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/* Plugin startup - called once at initialization */
static bool g_gliden64_initialized = false;
static m64p_dynlib_handle g_core_handle = nullptr;

extern "C" {

/* Wrapper startup that gets called before using the plugin */
m64p_error CV64_GlideN64_Startup(m64p_dynlib_handle CoreHandle, void* Context, 
                                  void (*DebugCallback)(void*, int, const char*)) {
    if (g_gliden64_initialized) {
        return M64ERR_SUCCESS;
    }
    
    GlideN64WrapperLog("Starting GLideN64...");
    g_core_handle = CoreHandle;
    
    m64p_error result = gliden64_PluginStartup(CoreHandle, Context, DebugCallback);
    if (result == M64ERR_SUCCESS) {
        g_gliden64_initialized = true;
        GlideN64WrapperLog("GLideN64 started successfully");
    } else {
        char msg[128];
        sprintf(msg, "GLideN64 startup failed with error: %d", (int)result);
        GlideN64WrapperLog(msg);
    }
    
    return result;
}

m64p_error CV64_GlideN64_Shutdown(void) {
    if (!g_gliden64_initialized) {
        return M64ERR_SUCCESS;
    }
    
    GlideN64WrapperLog("Shutting down GLideN64...");
    m64p_error result = gliden64_PluginShutdown();
    g_gliden64_initialized = false;
    g_core_handle = nullptr;
    return result;
}

} /* extern "C" */

#endif /* CV64_USE_GLIDEN64 */
#endif /* CV64_STATIC_MUPEN64PLUS */
