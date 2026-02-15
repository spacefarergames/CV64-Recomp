/**
 * @file cv64_gliden64_static.h
 * @brief GLideN64 Static Plugin Interface
 * 
 * Declares the GLideN64 plugin functions for static linking.
 * GLideN64 is compiled as a static library and these functions
 * are linked directly into the executable.
 */

#ifndef CV64_GLIDEN64_STATIC_H
#define CV64_GLIDEN64_STATIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_plugin.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"

/*===========================================================================
 * GLideN64 Plugin Exports (from MupenPlusPluginAPI.cpp & CommonPluginAPI.cpp)
 * 
 * These are the actual GLideN64 functions we link against.
 * When building GLideN64 as a static library with GLIDEN64_STATIC_EXPORTS,
 * the functions are renamed via preprocessor to avoid symbol conflicts.
 *===========================================================================*/

/* Plugin lifecycle */
m64p_error gliden64_PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                   void (*DebugCallback)(void *, int, const char *));
m64p_error gliden64_PluginShutdown(void);
m64p_error gliden64_PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                      int *APIVersion, const char **PluginNamePtr, int *Capabilities);

/* Video initialization */
int gliden64_InitiateGFX(GFX_INFO Gfx_Info);
int gliden64_RomOpen(void);
void gliden64_RomClosed(void);

/* Rendering */
void gliden64_ProcessDList(void);
void gliden64_ProcessRDPList(void);
void gliden64_UpdateScreen(void);
void gliden64_ShowCFB(void);

/* Window management */
void gliden64_ChangeWindow(void);
void gliden64_MoveScreen(int xpos, int ypos);
void gliden64_ResizeVideoOutput(int width, int height);

/* Status callbacks */
void gliden64_ViStatusChanged(void);
void gliden64_ViWidthChanged(void);

/* Framebuffer operations */
void gliden64_FBRead(unsigned int addr);
void gliden64_FBWrite(unsigned int addr, unsigned int size);
void gliden64_FBGetFrameBufferInfo(void *pinfo);

/* Screenshot/rendering */
void gliden64_ReadScreen2(void *dest, int *width, int *height, int front);
void gliden64_SetRenderingCallback(void (*callback)(int));

#ifdef __cplusplus
}
#endif

#endif /* CV64_GLIDEN64_STATIC_H */
