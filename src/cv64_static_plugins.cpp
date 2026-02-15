/**
 * @file cv64_static_plugins.cpp
 * @brief Static plugin registration - embeds all plugins directly in the EXE
 * 
 * This file registers statically linked plugins with the mupen64plus core,
 * completely bypassing DLL loading for a self-contained executable.
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS

#include "../include/cv64_static_plugins.h"
#include <Windows.h>
#include <cstdio>

/* Include core plugin types */
extern "C" {
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_plugin.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/plugin/plugin.h"
}

/*===========================================================================
 * External Core Functions (C linkage)
 *===========================================================================*/

extern "C" {
    /* Core API functions for static plugin initialization */
    extern m64p_dynlib_handle CoreGetStaticHandle(void);
    extern void* GetDebugCallContext(void);
    extern void (*GetDebugCallback(void))(void*, int, const char*);
}

/*===========================================================================
 * Static Helper
 *===========================================================================*/

static void StaticPluginLog(const char* msg) {
    OutputDebugStringA("[CV64_STATIC_PLUGIN] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Audio Plugin Implementation (SDL-based for real audio output)
 *===========================================================================*/

/* SDL Audio plugin functions (from cv64_audio_sdl.cpp) */
extern "C" {
    extern m64p_error cv64audio_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern void cv64audio_AiDacrateChanged(int);
    extern void cv64audio_AiLenChanged(void);
    extern int cv64audio_InitiateAudio(AUDIO_INFO);
    extern void cv64audio_ProcessAList(void);
    extern void cv64audio_RomClosed(void);
    extern int cv64audio_RomOpen(void);
    extern void cv64audio_SetSpeedFactor(int);
    extern void cv64audio_VolumeUp(void);
    extern void cv64audio_VolumeDown(void);
    extern int cv64audio_VolumeGetLevel(void);
    extern void cv64audio_VolumeSetLevel(int);
    extern void cv64audio_VolumeMute(void);
    extern const char* cv64audio_VolumeGetString(void);
    extern void cv64audio_Shutdown(void);
}

#define AUDIO_GetVersion       cv64audio_PluginGetVersion
#define AUDIO_AiDacrateChanged cv64audio_AiDacrateChanged
#define AUDIO_AiLenChanged     cv64audio_AiLenChanged
#define AUDIO_InitiateAudio    cv64audio_InitiateAudio
#define AUDIO_ProcessAList     cv64audio_ProcessAList
#define AUDIO_RomClosed        cv64audio_RomClosed
#define AUDIO_RomOpen          cv64audio_RomOpen
#define AUDIO_SetSpeedFactor   cv64audio_SetSpeedFactor
#define AUDIO_VolumeUp         cv64audio_VolumeUp
#define AUDIO_VolumeDown       cv64audio_VolumeDown
#define AUDIO_VolumeGetLevel   cv64audio_VolumeGetLevel
#define AUDIO_VolumeSetLevel   cv64audio_VolumeSetLevel
#define AUDIO_VolumeMute       cv64audio_VolumeMute
#define AUDIO_VolumeGetString  cv64audio_VolumeGetString
#define AUDIO_PLUGIN_NAME      "CV64 SDL Audio"

/*===========================================================================
 * Configuration: Choose between real plugins or dummy fallbacks
 * 
 * Define CV64_USE_GLIDEN64 to use real GLideN64 graphics
 * Define CV64_USE_RSP_HLE to use real RSP-HLE 
 * Define CV64_USE_CUSTOM_INPUT to use our custom input plugin
 * 
 * If not defined, dummy plugins are used for testing.
 *===========================================================================*/

/* NOTE: To enable real plugins, define these in mupen64plus-static.props:
 * - CV64_USE_GLIDEN64 for real GLideN64 graphics
 * - CV64_USE_RSP_HLE for real RSP-HLE
 * - CV64_USE_CUSTOM_INPUT for our custom input plugin
 */

/*===========================================================================
 * GLideN64 Graphics Plugin (or dummy fallback)
 *===========================================================================*/

#ifdef CV64_USE_GLIDEN64
/* Real GLideN64 functions (from cv64_gliden64_wrapper.cpp) */
extern "C" {
    extern m64p_error gliden64_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern m64p_error gliden64_PluginStartup(m64p_dynlib_handle, void*, void (*)(void*, int, const char*));
    extern m64p_error gliden64_PluginShutdown(void);
    extern void gliden64_ChangeWindow(void);
    extern int gliden64_InitiateGFX(GFX_INFO);
    extern void gliden64_MoveScreen(int, int);
    extern void gliden64_ProcessDList(void);
    extern void gliden64_ProcessRDPList(void);
    extern void gliden64_RomClosed(void);
    extern int gliden64_RomOpen(void);
    extern void gliden64_ShowCFB(void);
    extern void gliden64_UpdateScreen(void);
    extern void gliden64_ViStatusChanged(void);
    extern void gliden64_ViWidthChanged(void);
    extern void gliden64_ReadScreen2(void *, int *, int *, int);
    extern void gliden64_SetRenderingCallback(void (*)(int));
    extern void gliden64_ResizeVideoOutput(int, int);
    extern void gliden64_FBRead(unsigned int);
    extern void gliden64_FBWrite(unsigned int, unsigned int);
    extern void gliden64_FBGetFrameBufferInfo(void *);
}
#define GFX_GetVersion       gliden64_PluginGetVersion
#define GFX_ChangeWindow     gliden64_ChangeWindow
#define GFX_InitiateGFX      gliden64_InitiateGFX
#define GFX_MoveScreen       gliden64_MoveScreen
#define GFX_ProcessDList     gliden64_ProcessDList
#define GFX_ProcessRDPList   gliden64_ProcessRDPList
#define GFX_RomClosed        gliden64_RomClosed
#define GFX_RomOpen          gliden64_RomOpen
#define GFX_ShowCFB          gliden64_ShowCFB
#define GFX_UpdateScreen     gliden64_UpdateScreen
#define GFX_ViStatusChanged  gliden64_ViStatusChanged
#define GFX_ViWidthChanged   gliden64_ViWidthChanged
#define GFX_ReadScreen2      gliden64_ReadScreen2
#define GFX_SetRenderingCallback gliden64_SetRenderingCallback
#define GFX_ResizeVideoOutput gliden64_ResizeVideoOutput
#define GFX_FBRead           gliden64_FBRead
#define GFX_FBWrite          gliden64_FBWrite
#define GFX_FBGetFrameBufferInfo gliden64_FBGetFrameBufferInfo
#define GFX_PLUGIN_NAME      "GLideN64 (Static)"
#else
/* Dummy video plugin fallback */
extern "C" {
    extern m64p_error dummyvideo_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern void dummyvideo_ChangeWindow(void);
    extern int dummyvideo_InitiateGFX(GFX_INFO);
    extern void dummyvideo_MoveScreen(int, int);
    extern void dummyvideo_ProcessDList(void);
    extern void dummyvideo_ProcessRDPList(void);
    extern void dummyvideo_RomClosed(void);
    extern int dummyvideo_RomOpen(void);
    extern void dummyvideo_ShowCFB(void);
    extern void dummyvideo_UpdateScreen(void);
    extern void dummyvideo_ViStatusChanged(void);
    extern void dummyvideo_ViWidthChanged(void);
    extern void dummyvideo_ReadScreen2(void *, int *, int *, int);
    extern void dummyvideo_SetRenderingCallback(void (*)(int));
    extern void dummyvideo_ResizeVideoOutput(int, int);
    extern void dummyvideo_FBRead(unsigned int);
    extern void dummyvideo_FBWrite(unsigned int, unsigned int);
    extern void dummyvideo_FBGetFrameBufferInfo(void *);
}
#define GFX_GetVersion       dummyvideo_PluginGetVersion
#define GFX_ChangeWindow     dummyvideo_ChangeWindow
#define GFX_InitiateGFX      dummyvideo_InitiateGFX
#define GFX_MoveScreen       dummyvideo_MoveScreen
#define GFX_ProcessDList     dummyvideo_ProcessDList
#define GFX_ProcessRDPList   dummyvideo_ProcessRDPList
#define GFX_RomClosed        dummyvideo_RomClosed
#define GFX_RomOpen          dummyvideo_RomOpen
#define GFX_ShowCFB          dummyvideo_ShowCFB
#define GFX_UpdateScreen     dummyvideo_UpdateScreen
#define GFX_ViStatusChanged  dummyvideo_ViStatusChanged
#define GFX_ViWidthChanged   dummyvideo_ViWidthChanged
#define GFX_ReadScreen2      dummyvideo_ReadScreen2
#define GFX_SetRenderingCallback dummyvideo_SetRenderingCallback
#define GFX_ResizeVideoOutput dummyvideo_ResizeVideoOutput
#define GFX_FBRead           dummyvideo_FBRead
#define GFX_FBWrite          dummyvideo_FBWrite
#define GFX_FBGetFrameBufferInfo dummyvideo_FBGetFrameBufferInfo
#define GFX_PLUGIN_NAME      "Dummy Video (Static)"
#endif

/*===========================================================================
 * RSP-HLE Plugin (or dummy fallback)
 *===========================================================================*/

#ifdef CV64_USE_RSP_HLE
/* Real RSP-HLE functions (renamed with rsphle_ prefix in static lib build) */
extern "C" {
    extern m64p_error rsphle_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern m64p_error rsphle_PluginStartup(m64p_dynlib_handle, void*, void (*)(void*, int, const char*));
    extern m64p_error rsphle_PluginShutdown(void);
    extern unsigned int rsphle_DoRspCycles(unsigned int);
    extern void rsphle_InitiateRSP(RSP_INFO, unsigned int *);
    extern void rsphle_RomClosed(void);
}
#define RSP_GetVersion   rsphle_PluginGetVersion
#define RSP_DoRspCycles  rsphle_DoRspCycles
#define RSP_InitiateRSP  rsphle_InitiateRSP
#define RSP_RomClosed    rsphle_RomClosed
#define RSP_PLUGIN_NAME  "RSP-HLE (Static)"
#else
/* Dummy RSP plugin fallback */
extern "C" {
    extern m64p_error dummyrsp_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern unsigned int dummyrsp_DoRspCycles(unsigned int);
    extern void dummyrsp_InitiateRSP(RSP_INFO, unsigned int *);
    extern void dummyrsp_RomClosed(void);
}
#define RSP_GetVersion   dummyrsp_PluginGetVersion
#define RSP_DoRspCycles  dummyrsp_DoRspCycles
#define RSP_InitiateRSP  dummyrsp_InitiateRSP
#define RSP_RomClosed    dummyrsp_RomClosed
#define RSP_PLUGIN_NAME  "Dummy RSP (Static)"
#endif

/*===========================================================================
 * Custom Input Plugin (or dummy fallback)
 *===========================================================================*/

#ifdef CV64_USE_CUSTOM_INPUT
/* Our custom input plugin functions (from cv64_input_plugin.cpp) */
extern "C" {
    extern m64p_error inputPluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern m64p_error inputPluginStartup(m64p_dynlib_handle, void*, void (*)(void*, int, const char*));
    extern m64p_error inputPluginShutdown(void);
    extern void inputControllerCommand(int, unsigned char *);
    extern void inputGetKeys(int, BUTTONS *);
    extern void inputInitiateControllers(CONTROL_INFO);
    extern void inputReadController(int, unsigned char *);
    extern void inputRomClosed(void);
    extern int inputRomOpen(void);
    extern void inputSDL_KeyDown(int, int);
    extern void inputSDL_KeyUp(int, int);
    /* VRU functions */
    extern void inputSendVRUWord(uint16_t, uint16_t*, uint8_t);
    extern void inputSetMicState(int);
    extern void inputReadVRUResults(uint16_t*, uint16_t*, uint16_t*, uint16_t*, uint16_t*, uint16_t*);
    extern void inputClearVRUWords(uint8_t);
    extern void inputSetVRUWordMask(uint8_t, uint8_t*);
}
/* RenderCallback stub for input plugin */
static void InputRenderCallback(void) { }

#define INPUT_GetVersion         inputPluginGetVersion
#define INPUT_ControllerCommand  inputControllerCommand
#define INPUT_GetKeys            inputGetKeys
#define INPUT_InitiateControllers inputInitiateControllers
#define INPUT_ReadController     inputReadController
#define INPUT_RomClosed          inputRomClosed
#define INPUT_RomOpen            inputRomOpen
#define INPUT_SDL_KeyDown        inputSDL_KeyDown
#define INPUT_SDL_KeyUp          inputSDL_KeyUp
#define INPUT_RenderCallback     InputRenderCallback
#define INPUT_SendVRUWord        inputSendVRUWord
#define INPUT_SetMicState        inputSetMicState
#define INPUT_ReadVRUResults     inputReadVRUResults
#define INPUT_ClearVRUWords      inputClearVRUWords
#define INPUT_SetVRUWordMask     inputSetVRUWordMask
#define INPUT_PLUGIN_NAME        "CV64 Input (Static)"
#else
/* Dummy input plugin fallback */
extern "C" {
    extern m64p_error dummyinput_PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *);
    extern void dummyinput_ControllerCommand(int, unsigned char *);
    extern void dummyinput_GetKeys(int, BUTTONS *);
    extern void dummyinput_InitiateControllers(CONTROL_INFO);
    extern void dummyinput_ReadController(int, unsigned char *);
    extern void dummyinput_RomClosed(void);
    extern int dummyinput_RomOpen(void);
    extern void dummyinput_SDL_KeyDown(int, int);
    extern void dummyinput_SDL_KeyUp(int, int);
    extern void dummyinput_RenderCallback(void);
    extern void dummyinput_SendVRUWord(uint16_t, uint16_t*, uint8_t);
    extern void dummyinput_SetMicState(int);
    extern void dummyinput_ReadVRUResults(uint16_t*, uint16_t*, uint16_t*, uint16_t*, uint16_t*, uint16_t*);
    extern void dummyinput_ClearVRUWords(uint8_t);
    extern void dummyinput_SetVRUWordMask(uint8_t, uint8_t*);
}
#define INPUT_GetVersion         dummyinput_PluginGetVersion
#define INPUT_ControllerCommand  dummyinput_ControllerCommand
#define INPUT_GetKeys            dummyinput_GetKeys
#define INPUT_InitiateControllers dummyinput_InitiateControllers
#define INPUT_ReadController     dummyinput_ReadController
#define INPUT_RomClosed          dummyinput_RomClosed
#define INPUT_RomOpen            dummyinput_RomOpen
#define INPUT_SDL_KeyDown        dummyinput_SDL_KeyDown
#define INPUT_SDL_KeyUp          dummyinput_SDL_KeyUp
#define INPUT_RenderCallback     dummyinput_RenderCallback
#define INPUT_SendVRUWord        dummyinput_SendVRUWord
#define INPUT_SetMicState        dummyinput_SetMicState
#define INPUT_ReadVRUResults     dummyinput_ReadVRUResults
#define INPUT_ClearVRUWords      dummyinput_ClearVRUWords
#define INPUT_SetVRUWordMask     dummyinput_SetVRUWordMask
#define INPUT_PLUGIN_NAME        "Dummy Input (Static)"
#endif

/*===========================================================================
 * Static Plugin Registration
 *===========================================================================*/

/* Debug callback for GLideN64 */
static void GlideN64DebugCallback(void* context, int level, const char* message) {
    char buffer[1024];
    const char* levelStr = "???";
    switch (level) {
        case 1: levelStr = "ERROR"; break;
        case 2: levelStr = "WARNING"; break;
        case 3: levelStr = "INFO"; break;
        case 4: levelStr = "STATUS"; break;
        case 5: levelStr = "VERBOSE"; break;
    }
    sprintf(buffer, "[GLideN64][%s] %s", levelStr, message);
    StaticPluginLog(buffer);
}

bool CV64_StaticGFX_Init(void) {
    char msg[256];
    sprintf(msg, "Registering static GFX plugin (%s)...", GFX_PLUGIN_NAME);
    StaticPluginLog(msg);
    
#ifdef CV64_USE_GLIDEN64
    /* Initialize GLideN64 plugin first - this sets up function pointers */
    StaticPluginLog("Calling gliden64_PluginStartup...");
    m64p_dynlib_handle coreHandle = CoreGetStaticHandle();
    if (coreHandle == NULL) {
        StaticPluginLog("ERROR: CoreGetStaticHandle returned NULL");
        return false;
    }
    /* Use GLideN64-specific debug callback for better filtering */
    m64p_error startup_err = gliden64_PluginStartup(coreHandle, GetDebugCallContext(), GlideN64DebugCallback);
    if (startup_err != M64ERR_SUCCESS) {
        sprintf(msg, "gliden64_PluginStartup failed with error: %d", (int)startup_err);
        StaticPluginLog(msg);
        return false;
    }
    StaticPluginLog("gliden64_PluginStartup succeeded");
#endif
    
    gfx_plugin_functions funcs;
    funcs.getVersion = GFX_GetVersion;
    funcs.changeWindow = GFX_ChangeWindow;
    funcs.initiateGFX = GFX_InitiateGFX;
    funcs.moveScreen = GFX_MoveScreen;
    funcs.processDList = GFX_ProcessDList;
    funcs.processRDPList = GFX_ProcessRDPList;
    funcs.romClosed = GFX_RomClosed;
    funcs.romOpen = GFX_RomOpen;
    funcs.showCFB = GFX_ShowCFB;
    funcs.updateScreen = GFX_UpdateScreen;
    funcs.viStatusChanged = GFX_ViStatusChanged;
    funcs.viWidthChanged = GFX_ViWidthChanged;
    funcs.readScreen = GFX_ReadScreen2;
    funcs.setRenderingCallback = GFX_SetRenderingCallback;
    funcs.resizeVideoOutput = GFX_ResizeVideoOutput;
    funcs.fBRead = GFX_FBRead;
    funcs.fBWrite = GFX_FBWrite;
    funcs.fBGetFrameBufferInfo = GFX_FBGetFrameBufferInfo;
    
    m64p_error err = CoreRegisterGfxPlugin(&funcs);
    if (err != M64ERR_SUCCESS) {
        char msg[128];
        sprintf(msg, "Failed to register GFX plugin: error %d", (int)err);
        StaticPluginLog(msg);
        return false;
    }
    
    StaticPluginLog("GFX plugin registered");
    return true;
}

void CV64_StaticGFX_Shutdown(void) {
#ifdef CV64_USE_GLIDEN64
    gliden64_PluginShutdown();
#endif
    CoreRegisterGfxPlugin(NULL);
    StaticPluginLog("GFX plugin unregistered");
}

bool CV64_StaticRSP_Init(void) {
    char msg[256];
    sprintf(msg, "Registering static RSP plugin (%s)...", RSP_PLUGIN_NAME);
    StaticPluginLog(msg);
    
#ifdef CV64_USE_RSP_HLE
    /* Initialize RSP-HLE plugin first - this sets up internal function pointers */
    /* Get the core handle for the static build */
    StaticPluginLog("Calling rsphle_PluginStartup...");
    m64p_dynlib_handle coreHandle = CoreGetStaticHandle();
    if (coreHandle == NULL) {
        StaticPluginLog("ERROR: CoreGetStaticHandle returned NULL");
        return false;
    }
    /* Pass the same debug callback that the core uses */
    m64p_error startup_err = rsphle_PluginStartup(coreHandle, GetDebugCallContext(), GetDebugCallback());
    if (startup_err != M64ERR_SUCCESS) {
        sprintf(msg, "rsphle_PluginStartup failed with error: %d", (int)startup_err);
        StaticPluginLog(msg);
        return false;
    }
    StaticPluginLog("rsphle_PluginStartup succeeded");
#endif
    
    rsp_plugin_functions funcs;
    funcs.getVersion = RSP_GetVersion;
    funcs.doRspCycles = RSP_DoRspCycles;
    funcs.initiateRSP = RSP_InitiateRSP;
    funcs.romClosed = RSP_RomClosed;
    
    m64p_error err = CoreRegisterRspPlugin(&funcs);
    if (err != M64ERR_SUCCESS) {
        char errmsg[128];
        sprintf(errmsg, "Failed to register RSP plugin: error %d", (int)err);
        StaticPluginLog(errmsg);
        return false;
    }
    
    StaticPluginLog("RSP plugin registered");
    return true;
}

void CV64_StaticRSP_Shutdown(void) {
#ifdef CV64_USE_RSP_HLE
    rsphle_PluginShutdown();
#endif
    CoreRegisterRspPlugin(NULL);
    StaticPluginLog("RSP plugin unregistered");
}

bool CV64_StaticAudio_Init(void) {
    char msg[256];
    sprintf(msg, "Registering static Audio plugin (%s)...", AUDIO_PLUGIN_NAME);
    StaticPluginLog(msg);
    
    audio_plugin_functions funcs;
    funcs.getVersion = AUDIO_GetVersion;
    funcs.aiDacrateChanged = AUDIO_AiDacrateChanged;
    funcs.aiLenChanged = AUDIO_AiLenChanged;
    funcs.initiateAudio = AUDIO_InitiateAudio;
    funcs.processAList = AUDIO_ProcessAList;
    funcs.romClosed = AUDIO_RomClosed;
    funcs.romOpen = AUDIO_RomOpen;
    funcs.setSpeedFactor = AUDIO_SetSpeedFactor;
    funcs.volumeUp = AUDIO_VolumeUp;
    funcs.volumeDown = AUDIO_VolumeDown;
    funcs.volumeGetLevel = AUDIO_VolumeGetLevel;
    funcs.volumeSetLevel = AUDIO_VolumeSetLevel;
    funcs.volumeMute = AUDIO_VolumeMute;
    funcs.volumeGetString = AUDIO_VolumeGetString;
    
    m64p_error err = CoreRegisterAudioPlugin(&funcs);
    if (err != M64ERR_SUCCESS) {
        char errmsg[128];
        sprintf(errmsg, "Failed to register Audio plugin: error %d", (int)err);
        StaticPluginLog(errmsg);
        return false;
    }
    
    StaticPluginLog("Audio plugin registered");
    return true;
}

void CV64_StaticAudio_Shutdown(void) {
    cv64audio_Shutdown();
    CoreRegisterAudioPlugin(NULL);
    StaticPluginLog("Audio plugin unregistered");
}

bool CV64_StaticInput_Init(void) {
    char msg[256];
    sprintf(msg, "Registering static Input plugin (%s)...", INPUT_PLUGIN_NAME);
    StaticPluginLog(msg);
    
#ifdef CV64_USE_CUSTOM_INPUT
    /* Initialize custom input plugin if it has a startup function */
    StaticPluginLog("Calling inputPluginStartup...");
    m64p_dynlib_handle coreHandle = CoreGetStaticHandle();
    m64p_error startup_err = inputPluginStartup(coreHandle, GetDebugCallContext(), GetDebugCallback());
    if (startup_err != M64ERR_SUCCESS) {
        sprintf(msg, "inputPluginStartup failed with error: %d", (int)startup_err);
        StaticPluginLog(msg);
        return false;
    }
    StaticPluginLog("inputPluginStartup succeeded");
#endif
    
    input_plugin_functions funcs;
    funcs.getVersion = INPUT_GetVersion;
    funcs.controllerCommand = INPUT_ControllerCommand;
    funcs.getKeys = INPUT_GetKeys;
    funcs.initiateControllers = INPUT_InitiateControllers;
    funcs.readController = INPUT_ReadController;
    funcs.romClosed = INPUT_RomClosed;
    funcs.romOpen = INPUT_RomOpen;
    funcs.keyDown = INPUT_SDL_KeyDown;
    funcs.keyUp = INPUT_SDL_KeyUp;
    funcs.renderCallback = INPUT_RenderCallback;
    funcs.sendVRUWord = INPUT_SendVRUWord;
    funcs.setMicState = INPUT_SetMicState;
    funcs.readVRUResults = INPUT_ReadVRUResults;
    funcs.clearVRUWords = INPUT_ClearVRUWords;
    funcs.setVRUWordMask = INPUT_SetVRUWordMask;
    
    m64p_error err = CoreRegisterInputPlugin(&funcs);
    if (err != M64ERR_SUCCESS) {
        char errmsg[128];
        sprintf(errmsg, "Failed to register Input plugin: error %d", (int)err);
        StaticPluginLog(errmsg);
        return false;
    }
    
    StaticPluginLog("Input plugin registered");
    return true;
}

void CV64_StaticInput_Shutdown(void) {
#ifdef CV64_USE_CUSTOM_INPUT
    /* Shutdown input plugin if it has cleanup */
    extern m64p_error inputPluginShutdown(void);
    inputPluginShutdown();
#endif
    CoreRegisterInputPlugin(NULL);
    StaticPluginLog("Input plugin unregistered");
}

bool CV64_RegisterStaticPlugins(void) {
    StaticPluginLog("=== Registering all static plugins ===");
    
    /* IMPORTANT: Plugin registration order matters!
     * 1. GFX first - provides display list and RDP list processing
     * 2. Audio second - provides audio list processing  
     * 3. Input third - provides controller input
     * 4. RSP last - may depend on callbacks from GFX/Audio
     * 
     * Each plugin's PluginStartup must complete before registration.
     */
    
    /* Register in order: GFX, Audio, Input, RSP */
    if (!CV64_StaticGFX_Init()) {
        StaticPluginLog("ERROR: Failed to register GFX plugin");
        return false;
    }
    
    if (!CV64_StaticAudio_Init()) {
        StaticPluginLog("WARNING: Failed to register Audio plugin, continuing...");
    }
    
    if (!CV64_StaticInput_Init()) {
        StaticPluginLog("WARNING: Failed to register Input plugin, continuing...");
    }
    
    if (!CV64_StaticRSP_Init()) {
        StaticPluginLog("ERROR: Failed to register RSP plugin");
        return false;
    }
    
    StaticPluginLog("=== All static plugins registered ===");
    return true;
}

#endif /* CV64_STATIC_MUPEN64PLUS */
