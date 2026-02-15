/**
 * @file cv64_input_plugin.cpp
 * @brief Castlevania 64 PC Recomp - Built-in Input Plugin
 * 
 * A custom mupen64plus input plugin that uses XInput and keyboard directly,
 * bypassing SDL input issues. This plugin is loaded internally instead of
 * using an external DLL.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_input_plugin.h"
#include "../include/cv64_controller.h"
#include "../include/cv64_memory_hook.h"
#include "../include/cv64_camera_patch.h"
#include <Windows.h>
#include <Xinput.h>
#include <cstring>
#include <cstdio>



#pragma comment(lib, "xinput.lib")

/*===========================================================================
 * Plugin Information
 *===========================================================================*/

#define PLUGIN_VERSION 0x020000  /* Version 2.0.0 */
#define PLUGIN_API_VERSION 0x020100  /* API 2.1.0 - REQUIRED by RMG mupen64plus core */

static const char* s_pluginName = "CV64 Input Plugin";
static bool s_initialized = false;
static m64p_dynlib_handle s_coreLib = nullptr;

/* Controller states */
// Only one controller (P1) should ever be marked as connected, regardless of input source
static CONTROL s_controllers[4];
static bool s_controllerConnected[4] = { true, false, false, false };  /* Only P1 connected */

/* Memory Pak data - 32KB per controller */
static unsigned char s_mempakData[4][32768];
static bool s_mempakInitialized[4] = { false, false, false, false };

/*===========================================================================
 * Helper - Debug Logging
 *===========================================================================*/

static void LogDebug(const char* msg) {
    OutputDebugStringA("[CV64-Input] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Memory Pak Functions
 *===========================================================================*/

static void InitMempak(int controller) {
    if (s_mempakInitialized[controller]) return;
    
    /* Initialize with proper N64 Controller Pak format */
    memset(s_mempakData[controller], 0, sizeof(s_mempakData[controller]));
    
    unsigned char* pak = s_mempakData[controller];
    
    /* Page 0: ID Area */
    pak[0x00] = 0x81;
    pak[0x01] = 0x01;
    for (int i = 0x02; i <= 0x19; i++) pak[i] = (unsigned char)i;
    
    /* Checksum */
    unsigned short checksum = 0;
    for (int i = 0; i < 0x1A; i++) checksum += pak[i];
    pak[0x1A] = (checksum >> 8) & 0xFF;
    pak[0x1B] = checksum & 0xFF;
    pak[0x1C] = (~checksum >> 8) & 0xFF;
    pak[0x1D] = ~checksum & 0xFF;
    
    /* Copy ID to backup blocks */
    for (int block = 1; block < 4; block++) {
        memcpy(&pak[block * 0x20], &pak[0], 0x20);
    }
    
    /* Pages 1-2: Inode table - mark all pages as free (0x03) */
    for (int i = 5; i < 128; i++) {
        int offset = 0x80 + (i * 2);
        pak[offset] = 0x00;
        pak[offset + 1] = 0x03;  /* Free page */
    }
    
    /* Copy to backup (pages 128-255) */
    memcpy(&pak[0x4000], &pak[0], 0x4000);
    
    s_mempakInitialized[controller] = true;
    LogDebug("Memory Pak initialized");
}

static void LoadMempakFromFile(int controller) {
    char filename[MAX_PATH];
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0';
    
    /* Only look in save directory */
    snprintf(filename, sizeof(filename), "%s\\save\\mempak%d.mpk", exePath, controller);
    
    FILE* f = fopen(filename, "rb");
    if (f) {
        fread(s_mempakData[controller], 1, 32768, f);
        fclose(f);
        s_mempakInitialized[controller] = true;
        char msg[256];
        snprintf(msg, sizeof(msg), "Loaded mempak: %s", filename);
        LogDebug(msg);
    } else {
        /* No existing file - initialize empty mempak */
        InitMempak(controller);
        char msg[256];
        snprintf(msg, sizeof(msg), "No mempak found at %s - initialized new", filename);
        LogDebug(msg);
    }
}

static void SaveMempakToFile(int controller) {
    if (!s_mempakInitialized[controller]) return;
    
    char filename[MAX_PATH];
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0';
    
    /* Create save directory if needed */
    char saveDir[MAX_PATH];
    snprintf(saveDir, sizeof(saveDir), "%s\\save", exePath);
    CreateDirectoryA(saveDir, NULL);
    
    snprintf(filename, sizeof(filename), "%s\\save\\mempak%d.mpk", exePath, controller);
    
    FILE* f = fopen(filename, "wb");
    if (f) {
        fwrite(s_mempakData[controller], 1, 32768, f);
        fclose(f);
    }
}

/*===========================================================================
 * Plugin API Functions
 *===========================================================================*/

EXPORT m64p_error CALL inputPluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                           void (*DebugCallback)(void *, int, const char *)) {
    if (s_initialized) {
        return M64ERR_ALREADY_INIT;
    }
    
    
    s_coreLib = CoreLibHandle;
    
    LogDebug("=== CV64 INPUT PLUGIN STARTUP ===");
    LogDebug("CoreLibHandle received, initializing...");
    
    /* Initialize our controller system if not already done */
    CV64_Controller_Init();
    LogDebug("CV64_Controller_Init() called");
    
    /* Load mempak files */
    for (int i = 0; i < 4; i++) {
        LoadMempakFromFile(i);
    }
    
    s_initialized = true;
    LogDebug("=== CV64 INPUT PLUGIN READY ===");
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL inputPluginShutdown(void) {
    if (!s_initialized) {
        return M64ERR_NOT_INIT;
    }
    
    /* Save mempak files */
    for (int i = 0; i < 4; i++) {
        SaveMempakToFile(i);
    }
    
    s_initialized = false;
    LogDebug("Plugin shutdown");
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL inputPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                              int *APIVersion, const char **PluginNamePtr, int *Capabilities) {
    if (PluginType) *PluginType = M64PLUGIN_INPUT;
    if (PluginVersion) *PluginVersion = PLUGIN_VERSION;
    if (APIVersion) *APIVersion = PLUGIN_API_VERSION;
    if (PluginNamePtr) *PluginNamePtr = s_pluginName;
    
    /* CRITICAL: Set VRU capability bit so mupen64plus recognizes VRU support!
     * Capability bits (from mupen64plus source):
     * Bit 0 (0x01) = VRU (Voice Recognition Unit) support
     */
    if (Capabilities) {
        *Capabilities = 0x01;  /* VRU supported */
        
        /* Log this so we can verify it's being called */
        static int callCount = 0;
        callCount++;
        char msg[256];
        snprintf(msg, sizeof(msg), "PluginGetVersion called (#%d): returning Capabilities=0x%02X (VRU bit set)", 
                 callCount, *Capabilities);
        LogDebug(msg);
    }
    
    return M64ERR_SUCCESS;
}

/*===========================================================================
 * Input Plugin Specific Functions
 *===========================================================================*/

/* Store CONTROL_INFO - in mupen64plus v2.6.0 this only contains Controls pointer */
static CONTROL_INFO s_savedControlInfo = { 0 };

EXPORT void CALL inputInitiateControllers(CONTROL_INFO ControlInfo) {
    LogDebug("=== InitiateControllers called ===");
    
    char ptrMsg[256];
    snprintf(ptrMsg, sizeof(ptrMsg), "  Controls: %p", ControlInfo.Controls);
    LogDebug(ptrMsg);
    
    /* Save the control info for later use */
    s_savedControlInfo = ControlInfo;
    
    /* CRITICAL: Initialize our internal controller state */
    for (int i = 0; i < 4; i++) {
        s_controllers[i].Present = (i == 0) ? 1 : 0;
        s_controllers[i].RawData = 0;
        s_controllers[i].Plugin = (i == 0) ? PLUGIN_MEMPAK : PLUGIN_NONE;
        s_controllers[i].Type = 0;  /* Standard controller type */
    }
    
    /* Write our state back to the core's CONTROL array */
    if (ControlInfo.Controls) {
        for (int i = 0; i < 4; i++) {
            ControlInfo.Controls[i].Present = s_controllers[i].Present;
            ControlInfo.Controls[i].RawData = s_controllers[i].RawData;
            ControlInfo.Controls[i].Plugin = s_controllers[i].Plugin;
            ControlInfo.Controls[i].Type = s_controllers[i].Type;
            
            char msg[256];
            snprintf(msg, sizeof(msg), "  Controller %d: Present=%d, Plugin=%d (%s), Type=%d", 
                     i, 
                     ControlInfo.Controls[i].Present, 
                     ControlInfo.Controls[i].Plugin,
                     (ControlInfo.Controls[i].Plugin == PLUGIN_MEMPAK) ? "MEMPAK" : "NONE",
                     ControlInfo.Controls[i].Type);
            LogDebug(msg);
        }
        LogDebug("=== InitiateControllers COMPLETE - Controller 1 ENABLED with MEMPAK ===");
    } else {
        LogDebug("ERROR: Controls pointer is NULL!");
        LogDebug("Controller state initialized internally but core may not recognize controllers");
    }
}

EXPORT void CALL inputGetKeys(int Control, BUTTONS *Keys) {
static bool firstCall = true;
static int callCount = 0;
static int logInterval = 60;  /* Log every 60 frames initially */

if (firstCall) {
    LogDebug("========================================");
    LogDebug("=== inputGetKeys FIRST CALL - INPUT PLUGIN IS ACTIVE! ===");
    LogDebug("========================================");
    firstCall = false;
}

callCount++;
if (callCount == 10) {
    LogDebug("inputGetKeys: 10 calls received - input system working");
} else if (callCount == 100) {
    LogDebug("inputGetKeys: 100 calls received");
    logInterval = 300;  /* Reduce logging frequency */
}
    
if (Control != 0 || !Keys) {
        if (Keys) memset(Keys, 0, sizeof(BUTTONS));
        return; // Only P1 supported
    }
    
Keys->Value = 0;
    
/* Update our controller system - this processes D-PAD and sends it to camera patch */
CV64_Controller_Update(0);

/* Update camera patch state if enabled */
if (CV64_CameraPatch_IsEnabled()) {
    CV64_CameraPatch_Update(1.0f / 60.0f);
}

/* Always run memory frame update (applies yaw offset, cheats, state cache, etc.) */
if (CV64_Memory_IsInitialized()) {
    CV64_Memory_FrameUpdate();
}
    
/* Get state from our controller system */
CV64_Controller* ctrl = CV64_Controller_GetState(0);
if (!ctrl) {
    if (callCount % 300 == 0) {
        LogDebug("WARNING: CV64_Controller_GetState returned NULL!");
    }
    return;
}
    
    u16 btns = ctrl->btns_held;
    s16 stick_x = ctrl->joy_x;
    s16 stick_y = ctrl->joy_y;
    s16 right_x = ctrl->right_joy_x;
    s16 right_y = ctrl->right_joy_y;
    
    /* Log input activity periodically for debugging */
    if (callCount % logInterval == 0 && (btns != 0 || stick_x != 0 || stick_y != 0)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Input: btns=0x%04X stick=(%d,%d) right=(%d,%d)", 
                 btns, stick_x, stick_y, right_x, right_y);
        LogDebug(msg);
    }

    /* Map our buttons to N64 buttons */
    if (btns & CONT_A) Keys->A_BUTTON = 1;
    if (btns & CONT_B) Keys->B_BUTTON = 1;
    if (btns & CONT_G) Keys->Z_TRIG = 1;
    if (btns & CONT_START) Keys->START_BUTTON = 1;
    if (btns & CONT_L) Keys->L_TRIG = 1;
    if (btns & CONT_R) Keys->R_TRIG = 1;

    // C-buttons from button bindings
    if (btns & CONT_E) Keys->U_CBUTTON = 1;
    if (btns & CONT_D) Keys->D_CBUTTON = 1;
    if (btns & CONT_C) Keys->L_CBUTTON = 1;
    if (btns & CONT_F) Keys->R_CBUTTON = 1;

    // Map right analog stick to C-buttons (threshold = 32)
    if (right_y > 32) Keys->U_CBUTTON = 1;
    if (right_y < -32) Keys->D_CBUTTON = 1;
    if (right_x < -32) Keys->L_CBUTTON = 1;
    if (right_x > 32) Keys->R_CBUTTON = 1;

    // Optionally, map D-Pad to C-buttons if desired (uncomment if needed)
    // if (btns & CONT_UP) Keys->U_CBUTTON = 1;
    // if (btns & CONT_DOWN) Keys->D_CBUTTON = 1;
    // if (btns & CONT_LEFT) Keys->L_CBUTTON = 1;
    // if (btns & CONT_RIGHT) Keys->R_CBUTTON = 1;

    // D-Pad
    if (btns & CONT_UP) Keys->U_DPAD = 1;
    if (btns & CONT_DOWN) Keys->D_DPAD = 1;
    if (btns & CONT_LEFT) Keys->L_DPAD = 1;
    if (btns & CONT_RIGHT) Keys->R_DPAD = 1;

    /* Analog stick - clamp to valid N64 range (-80 to 80) */
    if (stick_x > 80) stick_x = 80;
    if (stick_x < -80) stick_x = -80;
    if (stick_y > 80) stick_y = 80;
    if (stick_y < -80) stick_y = -80;

    Keys->X_AXIS = stick_x;
    Keys->Y_AXIS = stick_y;
}

EXPORT void CALL inputControllerCommand(int Control, unsigned char *Command) {
static bool loggedFirstCommand = false;
static int commandCount = 0;
    
if (Control < 0 || Control >= 4 || !Command) {
    return;
}
    
/* Ensure mempak is initialized */
if (!s_mempakInitialized[Control]) {
    InitMempak(Control);
}
    
unsigned char commandType = Command[2];
commandCount++;
    
/* Log first few commands for debugging - show the entire command buffer */
if (commandCount <= 10 || (commandCount <= 30 && commandType == RD_GETSTATUS)) {
    char msg[512];
    snprintf(msg, sizeof(msg), "ControllerCommand[%d] #%d: Type=0x%02X (%s) Buffer=[%02X %02X %02X %02X %02X %02X]", 
             Control, commandCount, commandType,
             commandType == RD_GETSTATUS ? "GETSTATUS" :
             commandType == RD_READKEYS ? "READKEYS" :
             commandType == RD_READPAK ? "READPAK" :
             commandType == RD_WRITEPAK ? "WRITEPAK" :
             "OTHER",
             Command[0], Command[1], Command[2], Command[3], Command[4], Command[5]);
    LogDebug(msg);
}
    
switch (commandType) {
    case RD_GETSTATUS:
        /* CRITICAL: Report controller status to the game */
        /* N64 PIF Protocol for controller status:
         * INPUT: Command[0] = TX bytes, Command[1] = RX bytes, Command[2] = 0x00 (RD_GETSTATUS)
         * OUTPUT: Command[3-4] = Status word, Command[5] = 0x00 (or pak type)
         * 
         * Status word format for controller:
         *   Byte 3: 0x05 = Controller present, no buttons held
         *   Byte 4: 0x00 = No errors
         *   Byte 5: 0x01 = Mempak inserted, 0x02 = Rumble pak, 0x00 = no pak
         */
        if (!loggedFirstCommand) {
            LogDebug("========================================");
            LogDebug("FIRST RD_GETSTATUS - Game checking controller!");
            LogDebug("========================================");
            loggedFirstCommand = true;
        }
            
        if (Control == 0) {
            /* Controller 1: Present with Memory Pak */
            Command[3] = 0x05;  /* Device type: standard controller */
            Command[4] = 0x00;  /* Device status: OK */
            Command[5] = 0x01;  /* Pak type: 0x01 = Memory Pak, 0x00 = no pak */
                
            if (commandCount <= 10) {
                char msg[512];
                snprintf(msg, sizeof(msg), "  Response: Controller %d PRESENT + MEMPAK => [%02X %02X %02X %02X %02X %02X]", 
                         Control,
                         Command[0], Command[1], Command[2], Command[3], Command[4], Command[5]);
                LogDebug(msg);
            }
        } else {
            /* Controllers 2-4: Not present */
            Command[3] = 0x00;  /* No device */
            Command[4] = 0x80;  /* Error: Not present */
            Command[5] = 0x00;  /* No pak */
        }
        break;
            
    case RD_READKEYS:
        /* Already handled by GetKeys */
        break;
            
    case RD_READPAK:
            if (s_controllers[Control].Plugin == PLUGIN_MEMPAK) {
                /* Read from mempak */
                unsigned int address = (Command[3] << 8) | (Command[4] & 0xE0);
                if (address < 32768) {
                    memcpy(&Command[5], &s_mempakData[Control][address], 32);
                }
                /* CRC */
                unsigned char crc = 0;
                for (int i = 0; i < 32; i++) {
                    crc = (crc + Command[5 + i]) & 0xFF;
                }
                Command[37] = crc;
            }
            break;
            
        case RD_WRITEPAK:
            if (s_controllers[Control].Plugin == PLUGIN_MEMPAK) {
                /* Write to mempak */
                unsigned int address = (Command[3] << 8) | (Command[4] & 0xE0);
                if (address < 32768) {
                    memcpy(&s_mempakData[Control][address], &Command[5], 32);
                }
                /* CRC */
                unsigned char crc = 0;
                for (int i = 0; i < 32; i++) {
                    crc = (crc + Command[5 + i]) & 0xFF;
                }
                Command[37] = crc;
            }
            break;
            
        case RD_RESETCONTROLLER:
        case RD_READEEPROM:
        case RD_WRITEEPROM:
            break;
    }
}

EXPORT int CALL inputRomOpen(void) {
    LogDebug("ROM opened");
    return 1;  /* Return 1 for success */
}

EXPORT void CALL inputRomClosed(void) {
    LogDebug("ROM closed");
    
    /* Save mempak data */
    for (int i = 0; i < 4; i++) {
        SaveMempakToFile(i);
    }
}

EXPORT void CALL inputReadController(int Control, unsigned char *Command) {
    /* This is called directly for raw input mode */
    inputControllerCommand(Control, Command);
}

extern "C" {

EXPORT void CALL inputSDL_KeyDown(int keymod, int keysym) {
    /* Not needed - we handle input directly */
}

EXPORT void CALL inputSDL_KeyUp(int keymod, int keysym) {
    /* Not needed - we handle input directly */
}

/*===========================================================================
 * VRU (Voice Recognition Unit) Functions - REQUIRED by RMG mupen64plus core
 * These are stubs since CV64 doesn't use VRU
 * RMG core expects: SendVRUWord, SetMicState, ReadVRUResults, ClearVRUWords, SetVRUWordMask
 * NOTE: Each function MUST be unique to prevent linker COMDAT folding!
 *===========================================================================*/

/* Static volatile variables to prevent COMDAT folding */
static volatile int s_vru_sendword_marker = 0;
static volatile int s_vru_micstate_marker = 1;
static volatile int s_vru_readresults_marker = 2;
static volatile int s_vru_clearwords_marker = 3;
static volatile int s_vru_wordmask_marker = 4;

/* RMG VRU function: SendVRUWord */
EXPORT void CALL inputSendVRUWord(uint16_t length, uint16_t *word, uint8_t lang) {
    /* VRU not implemented */
    (void)s_vru_sendword_marker;
    (void)length;
    (void)word;
    (void)lang;
}

/* RMG VRU function: SetMicState */
EXPORT void CALL inputSetMicState(int state) {
    /* VRU not implemented */
    (void)s_vru_micstate_marker;
    (void)state;
}

/* RMG VRU function: ReadVRUResults */
EXPORT void CALL inputReadVRUResults(uint16_t *error_flags, uint16_t *num_results, uint16_t *mic_level,
                                      uint16_t *voice_level, uint16_t *voice_length, uint16_t *matches) {
    /* VRU not implemented - return no results */
    (void)s_vru_readresults_marker;
    if (error_flags) *error_flags = 0;
    if (num_results) *num_results = 0;
    if (mic_level) *mic_level = 0;
    if (voice_level) *voice_level = 0;
    if (voice_length) *voice_length = 0;
    /* matches array not modified */
    (void)matches;
}

/* RMG VRU function: ClearVRUWords */
EXPORT void CALL inputClearVRUWords(uint8_t length) {
    /* VRU not implemented */
    (void)s_vru_clearwords_marker;
    (void)length;
}

/* RMG VRU function: SetVRUWordMask */
EXPORT void CALL inputSetVRUWordMask(uint8_t length, uint8_t *mask) {
    /* VRU not implemented */
    (void)s_vru_wordmask_marker;
    (void)length;
    (void)mask;
}

} /* extern "C" for input helper functions */

/*===========================================================================
 * Standard mupen64plus Plugin API exports
 * These are the names mupen64plus core looks for via GetProcAddress
 * MUST use extern "C" to prevent C++ name mangling!
 *===========================================================================*/

extern "C" {

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                      void (*DebugCallback)(void *, int, const char *)) {
    return inputPluginStartup(CoreLibHandle, Context, DebugCallback);
}

EXPORT m64p_error CALL PluginShutdown(void) {
    return inputPluginShutdown();
}

/* Only export PluginGetVersion when not statically linking - the static core has its own */
#ifndef CV64_STATIC_MUPEN64PLUS
EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                         int *APIVersion, const char **PluginNamePtr, int *Capabilities) {
    return inputPluginGetVersion(PluginType, PluginVersion, APIVersion, PluginNamePtr, Capabilities);
}
#endif

EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo) {
    inputInitiateControllers(ControlInfo);
}

EXPORT void CALL GetKeys(int Control, BUTTONS *Keys) {
    inputGetKeys(Control, Keys);
}

EXPORT void CALL ControllerCommand(int Control, unsigned char *Command) {
    inputControllerCommand(Control, Command);
}

EXPORT int CALL RomOpen(void) {
    return inputRomOpen();
}

EXPORT void CALL RomClosed(void) {
    inputRomClosed();
}

EXPORT void CALL ReadController(int Control, unsigned char *Command) {
    inputReadController(Control, Command);
}

EXPORT void CALL SDL_KeyDown(int keymod, int keysym) {
    inputSDL_KeyDown(keymod, keysym);
}

EXPORT void CALL SDL_KeyUp(int keymod, int keysym) {
    inputSDL_KeyUp(keymod, keysym);
}

/* VRU Functions - RMG mupen64plus expects these specific names */
/* Each must be unique to prevent COMDAT folding */
EXPORT void CALL SendVRUWord(uint16_t length, uint16_t *word, uint8_t lang) {
    static volatile int marker = 100;
    (void)marker;
    inputSendVRUWord(length, word, lang);
}

EXPORT void CALL SetMicState(int state) {
    static volatile int marker = 101;
    (void)marker;
    inputSetMicState(state);
}

EXPORT void CALL ReadVRUResults(uint16_t *error_flags, uint16_t *num_results, uint16_t *mic_level,
                                 uint16_t *voice_level, uint16_t *voice_length, uint16_t *matches) {
    static volatile int marker = 102;
    (void)marker;
    inputReadVRUResults(error_flags, num_results, mic_level, voice_level, voice_length, matches);
}

EXPORT void CALL ClearVRUWords(uint8_t length) {
    static volatile int marker = 103;
    (void)marker;
    inputClearVRUWords(length);
}

EXPORT void CALL SetVRUWordMask(uint8_t length, uint8_t *mask) {
    static volatile int marker = 104;
    (void)marker;
    inputSetVRUWordMask(length, mask);
}

/* RMG Core additional functions - return controller info when core queries it */
EXPORT int CALL GetControlInfo(int Control, int* Present, int* Plugin) {
    char msg[128];
    snprintf(msg, sizeof(msg), "GetControlInfo called for controller %d", Control);
    LogDebug(msg);
    if (Control < 0 || Control >= 4) return 0;
    
    if (Present) *Present = s_controllers[Control].Present;
    if (Plugin) *Plugin = s_controllers[Control].Plugin;
    
    return 1;
}

/* RenderCallback - called by core to let plugin render on screen display */
EXPORT void CALL RenderCallback(void) {
    /* No-op - we don't render anything */
}

/* WM_KeyDown/WM_KeyUp - Windows message handlers (some cores use these instead of SDL) */
EXPORT void CALL WM_KeyDown(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    (void)lParam;
}

EXPORT void CALL WM_KeyUp(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    (void)lParam;
}

} /* extern "C" */

/*===========================================================================
 * Plugin Registration
 *===========================================================================*/

/* Function to get our plugin function pointers for internal registration */
void CV64_GetInputPluginFunctions(CV64_InputPluginFunctions* funcs) {
    if (!funcs) return;
    
    funcs->PluginStartup = inputPluginStartup;
    funcs->PluginShutdown = inputPluginShutdown;
    funcs->PluginGetVersion = inputPluginGetVersion;
    funcs->InitiateControllers = inputInitiateControllers;
    funcs->GetKeys = inputGetKeys;
    funcs->ControllerCommand = inputControllerCommand;
    funcs->RomOpen = inputRomOpen;
    funcs->RomClosed = inputRomClosed;
    funcs->ReadController = inputReadController;
}
