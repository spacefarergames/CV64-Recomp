/**
 * @file cv64_m64p_static_wrapper.h
 * @brief Static linking wrapper for mupen64plus core
 * 
 * This header provides extern declarations for calling mupen64plus core
 * functions directly when statically linked.
 */

#ifndef CV64_M64P_STATIC_WRAPPER_H
#define CV64_M64P_STATIC_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include mupen64plus API headers directly from the core */
#define M64P_CORE_PROTOTYPES 1

/* Forward declarations - these will be resolved at link time */

/* Define EXPORT and CALL if not already defined */
#ifndef EXPORT
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif

#ifndef CALL
#ifdef _WIN32
#define CALL __cdecl
#else
#define CALL
#endif
#endif

/* m64p error codes */
typedef enum {
    M64ERR_SUCCESS = 0,
    M64ERR_NOT_INIT,
    M64ERR_ALREADY_INIT,
    M64ERR_INCOMPATIBLE,
    M64ERR_INPUT_ASSERT,
    M64ERR_INPUT_INVALID,
    M64ERR_INPUT_NOT_FOUND,
    M64ERR_NO_MEMORY,
    M64ERR_FILES,
    M64ERR_INTERNAL,
    M64ERR_INVALID_STATE,
    M64ERR_PLUGIN_FAIL,
    M64ERR_SYSTEM_FAIL,
    M64ERR_UNSUPPORTED,
    M64ERR_WRONG_TYPE
} m64p_static_error;

/* Core parameter types */
typedef enum {
    M64CORE_EMU_STATE = 1,
    M64CORE_VIDEO_MODE,
    M64CORE_SAVESTATE_SLOT,
    M64CORE_SPEED_FACTOR,
    M64CORE_SPEED_LIMITER,
    M64CORE_VIDEO_SIZE,
    M64CORE_AUDIO_VOLUME,
    M64CORE_AUDIO_MUTE,
    M64CORE_INPUT_GAMESHARK,
    M64CORE_STATE_LOADCOMPLETE,
    M64CORE_STATE_SAVECOMPLETE
} m64p_static_core_param;

/* Plugin types */
typedef enum {
    M64PLUGIN_NULL = 0,
    M64PLUGIN_RSP = 1,
    M64PLUGIN_GFX,
    M64PLUGIN_AUDIO,
    M64PLUGIN_INPUT,
    M64PLUGIN_CORE
} m64p_static_plugin_type;

/* Command types */
typedef enum {
    M64CMD_NOP = 0,
    M64CMD_ROM_OPEN,
    M64CMD_ROM_CLOSE,
    M64CMD_ROM_GET_HEADER,
    M64CMD_ROM_GET_SETTINGS,
    M64CMD_EXECUTE,
    M64CMD_STOP,
    M64CMD_PAUSE,
    M64CMD_RESUME,
    M64CMD_CORE_STATE_QUERY,
    M64CMD_STATE_LOAD,
    M64CMD_STATE_SAVE,
    M64CMD_STATE_SET_SLOT,
    M64CMD_SEND_SDL_KEYDOWN,
    M64CMD_SEND_SDL_KEYUP,
    M64CMD_SET_FRAME_CALLBACK,
    M64CMD_TAKE_NEXT_SCREENSHOT,
    M64CMD_CORE_STATE_SET,
    M64CMD_READ_SCREEN,
    M64CMD_RESET,
    M64CMD_ADVANCE_FRAME,
    M64CMD_SET_MEDIA_LOADER,
    M64CMD_NETPLAY_INIT,
    M64CMD_NETPLAY_CONTROL_PLAYER,
    M64CMD_NETPLAY_GET_VERSION,
    M64CMD_NETPLAY_CLOSE,
    M64CMD_PIF_OPEN,
    M64CMD_ROM_SET_SETTINGS
} m64p_static_command;

/* Debug memory types */
typedef enum {
    M64P_DBG_MEM_TYPE_RDRAM = 1,
    M64P_DBG_MEM_TYPE_PI_REG,
    M64P_DBG_MEM_TYPE_SI_REG,
    M64P_DBG_MEM_TYPE_VI_REG,
    M64P_DBG_MEM_TYPE_RI_REG,
    M64P_DBG_MEM_TYPE_AI_REG
} m64p_static_dbg_mem_type;

/* Config parameter types */
typedef enum {
    M64TYPE_INT = 1,
    M64TYPE_FLOAT,
    M64TYPE_BOOL,
    M64TYPE_STRING
} m64p_static_type;

/* Handle type */
typedef void* m64p_static_handle;
typedef void* m64p_static_dynlib_handle;

/* Callback types */
typedef void (*ptr_DebugCallback)(void *Context, int level, const char *message);
typedef void (*ptr_StateCallback)(void *Context, int param_type, int new_value);

/*===========================================================================
 * mupen64plus Core API (Static Function Declarations)
 *===========================================================================*/

/* These functions are implemented in the mupen64plus-core static library */

/* Core lifecycle */
m64p_static_error CALL CoreStartup(int APIVersion, const char *ConfigPath, const char *DataPath,
                                    void *Context, ptr_DebugCallback DebugCB,
                                    void *Context2, ptr_StateCallback StateCB);
m64p_static_error CALL CoreShutdown(void);
m64p_static_error CALL CoreDoCommand(int Command, int ParamInt, void *ParamPtr);
m64p_static_error CALL CoreAttachPlugin(int PluginType, m64p_static_dynlib_handle PluginLibHandle);
m64p_static_error CALL CoreDetachPlugin(int PluginType);
const char * CALL CoreErrorMessage(m64p_static_error ReturnCode);

/* Video extension override */
typedef struct {
    unsigned int Functions;
    /* Video extension function pointers */
    void *VidExtFuncs[20];  /* Placeholder - actual struct in m64p_vidext.h */
} m64p_static_video_extension_functions;

m64p_static_error CALL CoreOverrideVidExt(m64p_static_video_extension_functions *VideoFunctionStruct);

/* Config API */
m64p_static_error CALL ConfigOpenSection(const char *SectionName, m64p_static_handle *ConfigSectionHandle);
m64p_static_error CALL ConfigSetParameter(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                          int ParamType, const void *ParamValue);
m64p_static_error CALL ConfigSetDefaultInt(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                           int ParamValue, const char *ParamHelp);
m64p_static_error CALL ConfigSetDefaultFloat(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                             float ParamValue, const char *ParamHelp);
m64p_static_error CALL ConfigSetDefaultBool(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                            int ParamValue, const char *ParamHelp);
m64p_static_error CALL ConfigSetDefaultString(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                              const char * ParamValue, const char *ParamHelp);
m64p_static_error CALL ConfigGetParameter(m64p_static_handle ConfigSectionHandle, const char *ParamName,
                                          int ParamType, void *ParamValue, int MaxSize);
m64p_static_error CALL ConfigSaveFile(void);
m64p_static_error CALL ConfigSaveSection(const char *SectionName);

/* Debug API */
m64p_static_error CALL DebugMemGetPointer(void **Pointer, int Type);
m64p_static_error CALL DebugMemRead32(unsigned int *Value, unsigned int Address);
m64p_static_error CALL DebugMemWrite32(unsigned int Value, unsigned int Address);

#ifdef __cplusplus
}
#endif

#endif /* CV64_M64P_STATIC_WRAPPER_H */
