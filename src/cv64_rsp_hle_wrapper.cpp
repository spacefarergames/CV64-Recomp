/**
 * @file cv64_rsp_hle_wrapper.cpp  
 * @brief Wrapper for RSP-HLE static plugin integration
 * 
 * This file provides the wrapper functions that bridge the mupen64plus core
 * to the statically linked RSP-HLE plugin. Symbol renaming is handled
 * in the RSP-HLE static library project via preprocessor definitions.
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS
#ifdef CV64_USE_RSP_HLE

#include "../include/cv64_rsp_hle_static.h"
#include <Windows.h>
#include <cstdio>

/* Log helper */
static void RspHleWrapperLog(const char* msg) {
    OutputDebugStringA("[CV64_RSPHLE] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

static bool g_rsphle_initialized = false;
static m64p_dynlib_handle g_core_handle = nullptr;

extern "C" {

/* Wrapper startup that gets called before using the plugin */
m64p_error CV64_RspHle_Startup(m64p_dynlib_handle CoreHandle, void* Context,
                                void (*DebugCallback)(void*, int, const char*)) {
    if (g_rsphle_initialized) {
        return M64ERR_SUCCESS;
    }
    
    RspHleWrapperLog("Starting RSP-HLE...");
    g_core_handle = CoreHandle;
    
    m64p_error result = rsphle_PluginStartup(CoreHandle, Context, DebugCallback);
    if (result == M64ERR_SUCCESS) {
        g_rsphle_initialized = true;
        RspHleWrapperLog("RSP-HLE started successfully");
    } else {
        char msg[128];
        sprintf(msg, "RSP-HLE startup failed with error: %d", (int)result);
        RspHleWrapperLog(msg);
    }
    
    return result;
}

m64p_error CV64_RspHle_Shutdown(void) {
    if (!g_rsphle_initialized) {
        return M64ERR_SUCCESS;
    }
    
    RspHleWrapperLog("Shutting down RSP-HLE...");
    m64p_error result = rsphle_PluginShutdown();
    g_rsphle_initialized = false;
    g_core_handle = nullptr;
    return result;
}

} /* extern "C" */

#endif /* CV64_USE_RSP_HLE */
#endif /* CV64_STATIC_MUPEN64PLUS */
