/**
 * @file cv64_m64p_static.h
 * @brief Static linking declarations for mupen64plus core and plugins
 * 
 * This header declares the mupen64plus functions for static linking.
 * Include this instead of dynamically loading DLLs.
 */

#ifndef CV64_M64P_STATIC_H
#define CV64_M64P_STATIC_H

#include "cv64_m64p_integration.h"

/* Define EXPORT and CALL macros for static linking */
#ifndef EXPORT
  #define EXPORT extern
#endif
#ifndef CALL
  #ifdef _WIN32
    #define CALL __cdecl
  #else
    #define CALL
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * mupen64plus Core API (Static Declarations)
 *===========================================================================*/

/* Core functions */
EXPORT m64p_error CALL CoreStartup(int APIVersion, const char *ConfigPath, const char *DataPath,
                                    void *Context, void (*DebugCallback)(void *, int, const char *),
                                    void *Context2, void (*StateCallback)(void *, m64p_core_param, int));
EXPORT m64p_error CALL CoreShutdown(void);
EXPORT m64p_error CALL CoreDoCommand(m64p_command Command, int ParamInt, void *ParamPtr);
EXPORT m64p_error CALL CoreAttachPlugin(m64p_plugin_type PluginType, m64p_dynlib_handle PluginLibHandle);
EXPORT m64p_error CALL CoreDetachPlugin(m64p_plugin_type PluginType);
EXPORT m64p_error CALL CoreOverrideVidExt(m64p_video_extension_functions *VideoFunctionStruct);
EXPORT const char * CALL CoreErrorMessage(m64p_error ReturnCode);

/* Config functions */
EXPORT m64p_error CALL ConfigOpenSection(const char *SectionName, m64p_handle *ConfigSectionHandle);
EXPORT m64p_error CALL ConfigSetParameter(m64p_handle ConfigSectionHandle, const char *ParamName,
                                          m64p_type ParamType, const void *ParamValue);
EXPORT m64p_error CALL ConfigSetDefaultInt(m64p_handle ConfigSectionHandle, const char *ParamName,
                                           int ParamValue, const char *ParamHelp);
EXPORT m64p_error CALL ConfigSetDefaultFloat(m64p_handle ConfigSectionHandle, const char *ParamName,
                                             float ParamValue, const char *ParamHelp);
EXPORT m64p_error CALL ConfigSetDefaultBool(m64p_handle ConfigSectionHandle, const char *ParamName,
                                            int ParamValue, const char *ParamHelp);
EXPORT m64p_error CALL ConfigSetDefaultString(m64p_handle ConfigSectionHandle, const char *ParamName,
                                              const char * ParamValue, const char *ParamHelp);
EXPORT m64p_error CALL ConfigGetParameter(m64p_handle ConfigSectionHandle, const char *ParamName,
                                          m64p_type ParamType, void *ParamValue, int MaxSize);
EXPORT m64p_error CALL ConfigGetParameterType(m64p_handle ConfigSectionHandle, const char *ParamName,
                                              m64p_type *ParamType);
EXPORT const char * CALL ConfigGetParameterHelp(m64p_handle ConfigSectionHandle, const char *ParamName);
EXPORT m64p_error CALL ConfigSaveSection(const char *SectionName);

/* Debug functions */
EXPORT m64p_error CALL DebugSetCallbacks(void (*DebugCallback)(void *, int, const char *),
                                         void *Context, void (*VI_Callback)(void));
EXPORT m64p_error CALL DebugSetCoreCompare(void (*CoreCompareCallback)(unsigned int), void (*SyncCallback)(void));
EXPORT m64p_error CALL DebugSetRunState(m64p_dbg_runstate State);
EXPORT m64p_error CALL DebugGetState(int *StatePtr);
EXPORT m64p_error CALL DebugStep(void);
EXPORT m64p_error CALL DebugDecodeOp(unsigned int Instruction, char *Buffer, int BufferSize, unsigned int PC);
EXPORT m64p_error CALL DebugMemGetPointer(void **Pointer, m64p_dbg_mem_type Type);
EXPORT m64p_error CALL DebugMemRead64(unsigned long long *Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemRead32(unsigned int *Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemRead16(unsigned short *Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemRead8(unsigned char *Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemWrite64(unsigned long long Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemWrite32(unsigned int Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemWrite16(unsigned short Value, unsigned int Address);
EXPORT m64p_error CALL DebugMemWrite8(unsigned char Value, unsigned int Address);

/* Video Extension Override */
EXPORT m64p_error CALL CoreOverrideVidExt(m64p_video_extension_functions *VideoFunctionStruct);

/*===========================================================================
 * Plugin API (Static Declarations)
 *===========================================================================*/

/* RSP-HLE Plugin */
EXPORT m64p_error CALL RspPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                        void (*DebugCallback)(void *, int, const char *));
EXPORT m64p_error CALL RspPluginShutdown(void);
EXPORT m64p_error CALL RspPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                           int *APIVersion, const char **PluginNamePtr, int *Capabilities);
EXPORT void CALL RspRomClosed(void);
EXPORT unsigned int CALL DoRspCycles(unsigned int Cycles);
EXPORT void CALL InitiateRSP(RSP_INFO Rsp_Info, unsigned int *CycleCount);

/* Video Plugin (GLideN64 or alternative) */
EXPORT m64p_error CALL VideoPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                          void (*DebugCallback)(void *, int, const char *));
EXPORT m64p_error CALL VideoPluginShutdown(void);
EXPORT m64p_error CALL VideoPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                             int *APIVersion, const char **PluginNamePtr, int *Capabilities);
EXPORT void CALL ChangeWindow(void);
EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info);
EXPORT void CALL MoveScreen(int x, int y);
EXPORT void CALL ProcessDList(void);
EXPORT void CALL ProcessRDPList(void);
EXPORT void CALL RomClosed(void);
EXPORT void CALL RomOpen(void);
EXPORT void CALL ShowCFB(void);
EXPORT void CALL UpdateScreen(void);
EXPORT void CALL ViStatusChanged(void);
EXPORT void CALL ViWidthChanged(void);
EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front);
EXPORT void CALL SetRenderingCallback(void (*callback)(int));
EXPORT void CALL FBRead(unsigned int addr);
EXPORT void CALL FBWrite(unsigned int addr, unsigned int size);
EXPORT void CALL FBGetFrameBufferInfo(void *p);

/* Audio Plugin (Optional) */
EXPORT m64p_error CALL AudioPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                          void (*DebugCallback)(void *, int, const char *));
EXPORT m64p_error CALL AudioPluginShutdown(void);
EXPORT m64p_error CALL AudioPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                             int *APIVersion, const char **PluginNamePtr, int *Capabilities);
EXPORT void CALL AiDacrateChanged(int SystemType);
EXPORT void CALL AiLenChanged(void);
EXPORT int  CALL InitiateAudio(AUDIO_INFO Audio_Info);
EXPORT void CALL ProcessAList(void);
EXPORT void CALL RomOpenAudio(void);
EXPORT void CALL RomClosedAudio(void);
EXPORT void CALL SetSpeedFactor(int percentage);
EXPORT void CALL VolumeUp(void);
EXPORT void CALL VolumeDown(void);
EXPORT int  CALL VolumeGetLevel(void);
EXPORT void CALL VolumeSetLevel(int level);
EXPORT void CALL VolumeMute(void);
EXPORT const char * CALL VolumeGetString(void);

/* Input Plugin (Optional - using custom implementation) */
EXPORT m64p_error CALL InputPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                          void (*DebugCallback)(void *, int, const char *));
EXPORT m64p_error CALL InputPluginShutdown(void);
EXPORT m64p_error CALL InputPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                             int *APIVersion, const char **PluginNamePtr, int *Capabilities);
EXPORT void CALL ControllerCommand(int Control, unsigned char *Command);
EXPORT void CALL GetKeys(int Control, BUTTONS *Keys);
EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo);
EXPORT void CALL ReadController(int Control, unsigned char *Command);
EXPORT void CALL RomOpenInput(void);
EXPORT void CALL RomClosedInput(void);
EXPORT void CALL SDL_KeyDown(int keymod, int keysym);
EXPORT void CALL SDL_KeyUp(int keymod, int keysym);

/*===========================================================================
 * Static Plugin Registration
 *===========================================================================*/

/**
 * @brief Initialize mupen64plus with static linking
 * @return TRUE on success
 */
bool CV64_M64P_InitStatic(void* hwnd);

/**
 * @brief Register statically linked plugins
 * @return TRUE on success
 */
bool CV64_M64P_RegisterStaticPlugins(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_M64P_STATIC_H */
