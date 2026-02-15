/**
 * @file cv64_dummy_video.cpp
 * @brief Dummy video plugin for static linking
 * 
 * This provides a minimal video plugin implementation that does nothing.
 * It's used when GLideN64 is not available, allowing the emulator to run
 * without graphics (useful for testing core functionality).
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS
#ifndef CV64_USE_GLIDEN64

#include <Windows.h>
#include <cstdio>

extern "C" {
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_plugin.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"
}

static void DummyVideoLog(const char* msg) {
    OutputDebugStringA("[CV64_DUMMY_VIDEO] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

static GFX_INFO g_gfx_info;

extern "C" {

m64p_error dummyvideo_PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                        int *APIVersion, const char **PluginNamePtr, int *Capabilities) {
    if (PluginType) *PluginType = M64PLUGIN_GFX;
    if (PluginVersion) *PluginVersion = 0x010000;
    if (APIVersion) *APIVersion = 0x020200;
    if (PluginNamePtr) *PluginNamePtr = "CV64 Dummy Video";
    if (Capabilities) *Capabilities = 0;
    return M64ERR_SUCCESS;
}

void dummyvideo_ChangeWindow(void) {
    DummyVideoLog("ChangeWindow");
}

int dummyvideo_InitiateGFX(GFX_INFO Gfx_Info) {
    DummyVideoLog("InitiateGFX");
    g_gfx_info = Gfx_Info;
    return 1;
}

void dummyvideo_MoveScreen(int xpos, int ypos) {
    (void)xpos; (void)ypos;
}

void dummyvideo_ProcessDList(void) {
    /* Dummy - no processing */
}

void dummyvideo_ProcessRDPList(void) {
    /* Dummy - no processing */
}

void dummyvideo_RomClosed(void) {
    DummyVideoLog("RomClosed");
}

int dummyvideo_RomOpen(void) {
    DummyVideoLog("RomOpen");
    return 1;
}

void dummyvideo_ShowCFB(void) {
    /* Dummy - no display */
}

void dummyvideo_UpdateScreen(void) {
    /* Dummy - no update */
}

void dummyvideo_ViStatusChanged(void) {
    /* Dummy */
}

void dummyvideo_ViWidthChanged(void) {
    /* Dummy */
}

void dummyvideo_ReadScreen2(void *dest, int *width, int *height, int front) {
    (void)dest; (void)front;
    if (width) *width = 320;
    if (height) *height = 240;
}

void dummyvideo_SetRenderingCallback(void (*callback)(int)) {
    (void)callback;
}

void dummyvideo_ResizeVideoOutput(int width, int height) {
    char msg[128];
    sprintf(msg, "ResizeVideoOutput: %dx%d", width, height);
    DummyVideoLog(msg);
}

void dummyvideo_FBRead(unsigned int addr) {
    (void)addr;
}

void dummyvideo_FBWrite(unsigned int addr, unsigned int size) {
    (void)addr; (void)size;
}

void dummyvideo_FBGetFrameBufferInfo(void *pinfo) {
    (void)pinfo;
}

} /* extern "C" */

#endif /* !CV64_USE_GLIDEN64 */
#endif /* CV64_STATIC_MUPEN64PLUS */
