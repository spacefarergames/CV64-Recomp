/**
 * @file cv64_input_plugin.h
 * @brief Castlevania 64 PC Recomp - Built-in Input Plugin Header
 * 
 * Defines the mupen64plus input plugin interface for our custom XInput/keyboard
 * input handler. This bypasses SDL input issues by using Windows input APIs directly.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_INPUT_PLUGIN_H
#define CV64_INPUT_PLUGIN_H

#include "cv64_types.h"
#include "cv64_m64p_integration.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Plugin Export Macros
 *===========================================================================*/

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#define EXPORT __attribute__((visibility("default")))
#define CALL
#endif

/*===========================================================================
 * Controller Plugin Types (from m64p_plugin.h)
 *===========================================================================*/

/* Controller pak types */
#define PLUGIN_NONE         1
#define PLUGIN_MEMPAK       2
#define PLUGIN_RUMBLE_PAK   3
#define PLUGIN_TRANSFER_PAK 4
#define PLUGIN_RAW          5

/* Controller command types */
#define RD_GETSTATUS        0x00
#define RD_READKEYS         0x01
#define RD_READPAK          0x02
#define RD_WRITEPAK         0x03
#define RD_RESETCONTROLLER  0xFF
#define RD_READEEPROM       0x04
#define RD_WRITEEPROM       0x05

/* CONTROL structure - controller state for mupen64plus v2.6.0 */
/* Must match mupen64plus-core v2.6.0 m64p_plugin.h exactly! */
typedef struct {
    int Present;    /* 1 = controller plugged in, 0 = not present */
    int RawData;    /* 1 = plugin needs raw data, 0 = uses standard API */
    int Plugin;     /* Controller pak type: 1=None, 2=MemPak, 3=RumblePak, 4=TransferPak, 5=Raw */
    int Type;       /* Controller type: 0=Standard, 1=VRU */
} CONTROL;

/* CONTROL_INFO structure - passed during InitiateControllers */
/* mupen64plus v2.6.0 ONLY has Controls pointer, nothing else! */
typedef struct {
    CONTROL *Controls;      /* Pointer to an array of 4 CONTROL structures */
} CONTROL_INFO;

/* BUTTONS union - N64 controller button state */
typedef union {
    unsigned int Value;
    struct {
        unsigned R_DPAD       : 1;
        unsigned L_DPAD       : 1;
        unsigned D_DPAD       : 1;
        unsigned U_DPAD       : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG       : 1;
        unsigned B_BUTTON     : 1;
        unsigned A_BUTTON     : 1;

        unsigned R_CBUTTON    : 1;
        unsigned L_CBUTTON    : 1;
        unsigned D_CBUTTON    : 1;
        unsigned U_CBUTTON    : 1;
        unsigned R_TRIG       : 1;
        unsigned L_TRIG       : 1;
        unsigned Reserved1    : 1;
        unsigned Reserved2    : 1;

        signed   X_AXIS       : 8;
        signed   Y_AXIS       : 8;
    };
} BUTTONS;

/*===========================================================================
 * Plugin Function Types
 *===========================================================================*/

typedef m64p_error (CALL *ptr_InputPluginStartup)(m64p_dynlib_handle, void*, void(*)(void*, int, const char*));
typedef m64p_error (CALL *ptr_InputPluginShutdown)(void);
typedef m64p_error (CALL *ptr_InputPluginGetVersion)(m64p_plugin_type*, int*, int*, const char**, int*);
typedef void (CALL *ptr_InitiateControllers)(CONTROL_INFO);
typedef void (CALL *ptr_GetKeys)(int, BUTTONS*);
typedef void (CALL *ptr_ControllerCommand)(int, unsigned char*);
typedef int (CALL *ptr_RomOpen)(void);
typedef void (CALL *ptr_RomClosed)(void);
typedef void (CALL *ptr_ReadController)(int, unsigned char*);

/* Structure containing all plugin function pointers */
typedef struct {
    ptr_InputPluginStartup PluginStartup;
    ptr_InputPluginShutdown PluginShutdown;
    ptr_InputPluginGetVersion PluginGetVersion;
    ptr_InitiateControllers InitiateControllers;
    ptr_GetKeys GetKeys;
    ptr_ControllerCommand ControllerCommand;
    ptr_RomOpen RomOpen;
    ptr_RomClosed RomClosed;
    ptr_ReadController ReadController;
} CV64_InputPluginFunctions;

/*===========================================================================
 * Plugin API Exports
 *===========================================================================*/

/* Standard mupen64plus plugin exports */
EXPORT m64p_error CALL inputPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                           void (*DebugCallback)(void *, int, const char *));
EXPORT m64p_error CALL inputPluginShutdown(void);
EXPORT m64p_error CALL inputPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                              int *APIVersion, const char **PluginNamePtr, int *Capabilities);

/* Input-specific exports */
EXPORT void CALL inputInitiateControllers(CONTROL_INFO ControlInfo);
EXPORT void CALL inputGetKeys(int Control, BUTTONS *Keys);
EXPORT void CALL inputControllerCommand(int Control, unsigned char *Command);
EXPORT int CALL inputRomOpen(void);
EXPORT void CALL inputRomClosed(void);
EXPORT void CALL inputReadController(int Control, unsigned char *Command);

/* Internal function to get plugin function pointers */
void CV64_GetInputPluginFunctions(CV64_InputPluginFunctions* funcs);

#ifdef __cplusplus
}
#endif

#endif /* CV64_INPUT_PLUGIN_H */
