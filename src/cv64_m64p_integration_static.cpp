/**
 * @file cv64_m64p_integration_static.cpp
 * @brief Castlevania 64 PC Recomp - Static mupen64plus Integration
 * 
 * This file provides FULLY static linking integration with mupen64plus-core
 * AND all plugins. No DLLs are loaded - everything is embedded in the EXE.
 * 
 * Advantages of fully static linking:
 *   - Single executable - no loose files
 *   - Finer control over emulator behavior
 *   - Direct access to internal structures
 *   - No DLL dependency at runtime
 *   - Easier distribution
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS

#include "../include/cv64_m64p_integration.h"
#include "../include/cv64_static_plugins.h"
#include "../include/cv64_camera_patch.h"
#include "../include/cv64_controller.h"
#include "../include/cv64_vidext.h"
#include "../include/cv64_input_plugin.h"
#include "../include/cv64_embedded_rom.h"
#include "../include/cv64_config_bridge.h"
#include "../include/cv64_bps_patch.h"
#include "../include/cv64_memory_hook.h"
#include "../include/cv64_threading.h"

#include <Windows.h>
#include <string>
#include <filesystem>
#include <cstring>
#include <cstdio>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

/*===========================================================================
 * Static Core API Declarations
 * These functions are linked directly from mupen64plus-core-static.lib
 *===========================================================================*/

/* Config parameter types (needed in C++ scope) */
typedef enum {
    M64TYPE_INT = 1,
    M64TYPE_FLOAT,
    M64TYPE_BOOL,
    M64TYPE_STRING
} m64p_type;

extern "C" {
/* Forward declarations matching mupen64plus-core API */
typedef void* m64p_static_handle;
    
typedef enum {
    M64P_DBG_PTR_RDRAM = 1,
    M64P_DBG_PTR_PI_REG,
    M64P_DBG_PTR_SI_REG,
    M64P_DBG_PTR_VI_REG,
    M64P_DBG_PTR_RI_REG,
    M64P_DBG_PTR_AI_REG
} m64p_dbg_memptr_type;
    
/* Core API functions - resolved at link time */
/* Note: CALL is __cdecl on Windows, matching mupen64plus-core exports */
m64p_error __cdecl CoreStartup(int, const char*, const char*, void*, 
                       void(*)(void*, int, const char*), void*, 
                       void(*)(void*, m64p_core_param, int));
m64p_error __cdecl CoreShutdown(void);
m64p_error __cdecl CoreDoCommand(m64p_command, int, void*);
m64p_error __cdecl CoreAttachPlugin(m64p_plugin_type, m64p_dynlib_handle);
m64p_error __cdecl CoreDetachPlugin(m64p_plugin_type);
const char* __cdecl CoreErrorMessage(m64p_error);
m64p_error __cdecl CoreOverrideVidExt(m64p_video_extension_functions*);
    
    /* Plugin API - for static plugin support */
    m64p_error __cdecl plugin_start(m64p_plugin_type);
    
    /* Config API */
    m64p_error __cdecl ConfigOpenSection(const char*, m64p_static_handle*);
    m64p_error __cdecl ConfigSetParameter(m64p_static_handle, const char*, int, const void*);
    m64p_error __cdecl ConfigSetDefaultInt(m64p_static_handle, const char*, int, const char*);
    m64p_error __cdecl ConfigSetDefaultFloat(m64p_static_handle, const char*, float, const char*);
    m64p_error __cdecl ConfigSetDefaultBool(m64p_static_handle, const char*, int, const char*);
    m64p_error __cdecl ConfigSetDefaultString(m64p_static_handle, const char*, const char*, const char*);
    m64p_error __cdecl ConfigSaveFile(void);
    m64p_error __cdecl ConfigSaveSection(const char*);
    
    /* Debug API */
    void* __cdecl DebugMemGetPointer(m64p_dbg_memptr_type);
    m64p_error __cdecl DebugMemRead32(unsigned int*, unsigned int);
    m64p_error __cdecl DebugMemWrite32(unsigned int, unsigned int);
}

/*===========================================================================
 * Static Variables
 *===========================================================================*/

/* State */
static CV64_IntegrationState s_staticState = CV64_INTEGRATION_UNINITIALIZED;
static std::string s_staticLastError;
static bool s_staticCoreInitialized = false;
static bool s_staticPluginsAttached = false;
static bool s_staticRomLoaded = false;
static std::filesystem::path s_staticRomPath;
static CV64_RomHeader s_staticRomHeader;
static u8* s_staticRdram = NULL;

/* Threading */
static std::thread s_staticEmulationThread;
static std::atomic<bool> s_staticEmulationRunning(false);
static std::atomic<bool> s_staticStopRequested(false);
static std::mutex s_staticStateMutex;

/* Callbacks */
static CV64_FrameCallback s_staticFrameCallback = NULL;
static void* s_staticFrameCallbackContext = NULL;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void StaticSetError(const std::string& error) {
    s_staticLastError = error;
    s_staticState = CV64_INTEGRATION_ERROR;
    OutputDebugStringA("[CV64_STATIC] ERROR: ");
    OutputDebugStringA(error.c_str());
    OutputDebugStringA("\n");
}

static void StaticLogDebug(const std::string& msg) {
    OutputDebugStringA("[CV64_STATIC] ");
    OutputDebugStringA(msg.c_str());
    OutputDebugStringA("\n");
}

static std::filesystem::path StaticGetCoreDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

/*===========================================================================
 * Callbacks
 *===========================================================================*/

static void StaticCoreDebugCallback(void* context, int level, const char* message) {
    const char* levelStr = "???";
    switch (level) {
        case 1: levelStr = "ERROR"; break;
        case 2: levelStr = "WARNING"; break;
        case 3: levelStr = "INFO"; break;
        case 4: levelStr = "STATUS"; break;
        case 5: levelStr = "VERBOSE"; break;
    }
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "[M64P-CORE][%s] %s\n", levelStr, message);
    OutputDebugStringA(buffer);
}

static void StaticCoreStateCallback(void* context, m64p_core_param param, int value) {
    StaticLogDebug("State change: param=" + std::to_string((int)param) + " value=" + std::to_string(value));
}

/*===========================================================================
 * Public API (Static Version)
 * These will be used when CV64_STATIC_MUPEN64PLUS is defined
 *===========================================================================*/

CV64_IntegrationState CV64_M64P_Static_GetState() {
    return s_staticState;
}

const char* CV64_M64P_Static_GetLastError() {
    return s_staticLastError.c_str();
}

/**
 * @brief Get the Windows Temp directory path for our config file
 * @return Path to CASTLEVANIA_TEMP.cfg in the user's temp directory
 */
static std::string GetTempConfigPath() {
    char tempPath[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempPath);
    if (len == 0 || len > MAX_PATH) {
        // Fallback to current directory if GetTempPath fails
        return ".\\";
    }
    return std::string(tempPath);
}

bool CV64_M64P_Static_Initialize(void* hwnd) {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState == CV64_INTEGRATION_PLUGINS_LOADED || s_staticState == CV64_INTEGRATION_RUNNING) {
        return true;
    }
    
    StaticLogDebug("=== CV64 Static mupen64plus Integration ===");
    StaticLogDebug("Initializing with statically linked core...");
    
    std::filesystem::path coreDir = StaticGetCoreDirectory();
    /* Use Windows Temp directory for config file to avoid permission issues
     * and keep the config separate from our CV64 INI files which are authoritative.
     * This prevents "Couldn't open configuration file" errors while still allowing
     * mupen64plus to save its internal state if needed. */
    std::string configPath = GetTempConfigPath();
    std::string dataPath = coreDir.string();
    
    StaticLogDebug("Config path: " + configPath + "CASTLEVANIA_TEMP.cfg");
    StaticLogDebug("Data path: " + dataPath);
    
    /* Initialize core directly (no DLL loading) */
    m64p_error result = CoreStartup(
        0x020001,
        configPath.c_str(),
        dataPath.c_str(),
        nullptr,
        StaticCoreDebugCallback,
        nullptr,
        StaticCoreStateCallback
    );
    
    if (result != M64ERR_SUCCESS) {
        StaticSetError("CoreStartup failed: " + std::string(CoreErrorMessage(result)));
        return false;
    }
    
    s_staticCoreInitialized = true;
    StaticLogDebug("Core initialized successfully (static linking)");
    StaticLogDebug("Config file in temp directory - CV64 INI files are authoritative");
    
    /* Apply CV64 INI configuration BEFORE loading plugins */
    StaticLogDebug("Applying CV64 configuration...");
    if (!CV64_Config_ApplyAll()) {
        StaticLogDebug("Warning: CV64 configuration may not have been fully applied");
    }
    
    /* Initialize video extension BEFORE loading plugins */
    if (hwnd) {
        StaticLogDebug("Initializing video extension...");
        if (!CV64_VidExt_Init((HWND)hwnd)) {
            StaticSetError("Failed to initialize video extension");
            CoreShutdown();
            s_staticCoreInitialized = false;
            return false;
        }
        
        /* Override video extension in core */
        m64p_video_extension_functions* vidExtFuncs = CV64_VidExt_GetFunctions();
        if (vidExtFuncs) {
            result = CoreOverrideVidExt(vidExtFuncs);
            if (result != M64ERR_SUCCESS) {
                StaticLogDebug("Warning: CoreOverrideVidExt failed, using default video");
            } else {
                StaticLogDebug("Video extension override successful");
            }
        }
    }
    
    s_staticState = CV64_INTEGRATION_CORE_LOADED;
    return true;
}

void CV64_M64P_Static_Shutdown() {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState == CV64_INTEGRATION_UNINITIALIZED) {
        return;
    }
    
    StaticLogDebug("Shutting down static integration...");
    
    if (s_staticEmulationRunning) {
        s_staticStopRequested = true;
        CoreDoCommand(M64CMD_STOP, 0, NULL);
        
        if (s_staticEmulationThread.joinable()) {
            s_staticEmulationThread.join();
        }
    }
    
    /* Shutdown static plugins */
    if (s_staticPluginsAttached) {
        CV64_StaticGFX_Shutdown();
        CV64_StaticAudio_Shutdown();
        CV64_StaticInput_Shutdown();
        CV64_StaticRSP_Shutdown();
        s_staticPluginsAttached = false;
    }
    
    if (s_staticCoreInitialized) {
        CoreShutdown();
        s_staticCoreInitialized = false;
    }
    
    s_staticState = CV64_INTEGRATION_UNINITIALIZED;
    s_staticRdram = NULL;
    
    StaticLogDebug("Static integration shutdown complete");
}

u8* CV64_M64P_Static_GetRDRAM() {
    return s_staticRdram;
}

u32 CV64_M64P_Static_ReadWord(u32 address) {
    if (s_staticRdram && address < 0x800000) {
        return *(u32*)(s_staticRdram + address);
    }
    unsigned int value = 0;
    DebugMemRead32(&value, address | 0x80000000);
    return value;
}

void CV64_M64P_Static_WriteWord(u32 address, u32 value) {
    if (s_staticRdram && address < 0x800000) {
        *(u32*)(s_staticRdram + address) = value;
    } else {
        DebugMemWrite32(value, address | 0x80000000);
    }
}

/*===========================================================================
 * Static Plugin Registration (No DLLs - All embedded in EXE)
 *===========================================================================*/

bool CV64_M64P_Static_LoadPlugins() {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState < CV64_INTEGRATION_CORE_LOADED) {
        StaticSetError("Core not initialized");
        return false;
    }
    
    if (s_staticPluginsAttached) {
        return true;
    }
    
    StaticLogDebug("Registering static plugins (embedded in EXE)...");
    
    /* Register all static plugins - NO DLLs needed */
    if (!CV64_RegisterStaticPlugins()) {
        StaticSetError("Failed to register static plugins");
        return false;
    }
    
    s_staticPluginsAttached = true;
    s_staticState = CV64_INTEGRATION_PLUGINS_LOADED;
    StaticLogDebug("All static plugins registered successfully");
    
    return true;
}

/*===========================================================================
 * ROM Loading
 *===========================================================================*/

/**
 * Load the embedded ROM (if available)
 * This is the preferred method - no external files needed
 */
bool CV64_M64P_Static_LoadEmbeddedROM() {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState < CV64_INTEGRATION_PLUGINS_LOADED) {
        StaticSetError("Plugins not loaded");
        return false;
    }
    
    if (s_staticRomLoaded) {
        CoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
        s_staticRomLoaded = false;
    }
    
    StaticLogDebug("Attempting to load embedded ROM...");
    
    /* Get embedded ROM data */
    size_t romSize = 0;
    const uint8_t* romData = CV64_GetEmbeddedRom(&romSize);
    
    if (!romData || romSize == 0) {
        char debugMsg[256];
        sprintf_s(debugMsg, "No embedded ROM available (data=%p, size=%zu)", (void*)romData, romSize);
        StaticSetError(debugMsg);
        return false;
    }
    
    char sizeMsg[256];
    sprintf_s(sizeMsg, "Found embedded ROM: %zu bytes (%.2f MB) at %p, first bytes: %02X %02X %02X %02X", 
              romSize, romSize / (1024.0 * 1024.0), (void*)romData,
              romSize > 0 ? romData[0] : 0,
              romSize > 1 ? romData[1] : 0,
              romSize > 2 ? romData[2] : 0,
              romSize > 3 ? romData[3] : 0);
    StaticLogDebug(sizeMsg);
    
    /* Make a mutable copy of ROM data for patching */
    std::vector<u8> romBuffer(romData, romData + romSize);
    size_t patchedSize = romSize;
    
    /* Apply BPS patches from patches folder */
    StaticLogDebug("Checking for BPS patches...");
    CV64_BPS_Result bpsResult = CV64_BPS_ApplyPatches(romBuffer.data(), &patchedSize, romBuffer.capacity());
    if (bpsResult == CV64_BPS_SUCCESS) {
        if (patchedSize != romSize) {
            romBuffer.resize(patchedSize);
            sprintf_s(sizeMsg, "ROM size changed after patching: %zu -> %zu", romSize, patchedSize);
            StaticLogDebug(sizeMsg);
        }
        StaticLogDebug("BPS patches applied successfully");
    } else if (bpsResult != CV64_BPS_NO_PATCHES_FOUND) {
        sprintf_s(sizeMsg, "BPS patching failed: %s", CV64_BPS_GetErrorMessage(bpsResult));
        StaticLogDebug(sizeMsg);
    }
    
    /* Open ROM in core */
    sprintf_s(sizeMsg, "Calling CoreDoCommand(M64CMD_ROM_OPEN, %d, %p)", (int)romBuffer.size(), (void*)romBuffer.data());
    StaticLogDebug(sizeMsg);
    
    m64p_error result = CoreDoCommand(M64CMD_ROM_OPEN, (int)romBuffer.size(), (void*)romBuffer.data());
    if (result != M64ERR_SUCCESS) {
        StaticSetError("CoreDoCommand(ROM_OPEN) failed for embedded ROM: " + std::string(CoreErrorMessage(result)));
        return false;
    }
    
    /* Get ROM header */
    result = CoreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(s_staticRomHeader), &s_staticRomHeader);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: Could not get ROM header");
    }
    
    s_staticRomPath = "<embedded>";
    s_staticRomLoaded = true;
    s_staticState = CV64_INTEGRATION_ROM_LOADED;
    
    StaticLogDebug("Embedded ROM loaded: " + std::string(s_staticRomHeader.name, 20));
    
    /* Initialize plugins after ROM is loaded - this is critical!
     * plugin_start() calls each plugin's initiation function:
     * - GFX: initiateGFX()
     * - Audio: initiateAudio()
     * - Input: initiateControllers()
     * - RSP: initiateRSP() <- THIS INITIALIZES g_hle memory pointers!
     * Without this, RSP plugin's g_hle.dmem/dram are nullptr causing crashes
     */
    StaticLogDebug("Calling plugin_start for all plugins...");
    result = plugin_start(M64PLUGIN_GFX);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(GFX) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_AUDIO);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(AUDIO) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_INPUT);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(INPUT) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_RSP);
    if (result != M64ERR_SUCCESS) {
        StaticSetError("plugin_start(RSP) failed: " + std::string(CoreErrorMessage(result)));
        return false;
    }
    StaticLogDebug("All plugins started successfully");
    
    return true;
}

bool CV64_M64P_Static_LoadROM(const char* romPath) {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState < CV64_INTEGRATION_PLUGINS_LOADED) {
        StaticSetError("Plugins not loaded");
        return false;
    }
    
    if (s_staticRomLoaded) {
        CoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
        s_staticRomLoaded = false;
    }
    
    StaticLogDebug("Loading ROM: " + std::string(romPath));
    
    /* Read ROM file */
    FILE* romFile = fopen(romPath, "rb");
    if (!romFile) {
        StaticSetError("Failed to open ROM file: " + std::string(romPath));
        return false;
    }
    
    fseek(romFile, 0, SEEK_END);
    size_t romSize = ftell(romFile);
    fseek(romFile, 0, SEEK_SET);
    
    char debugMsg[256];
    sprintf_s(debugMsg, "ROM file size: %zu bytes", romSize);
    StaticLogDebug(debugMsg);
    
    if (romSize < 4096) {
        fclose(romFile);
        StaticSetError("ROM file too small (< 4096 bytes)");
        return false;
    }
    
    std::vector<u8> romData(romSize);
    size_t bytesRead = fread(romData.data(), 1, romSize, romFile);
    fclose(romFile);
    
    if (bytesRead != romSize) {
        sprintf_s(debugMsg, "Failed to read ROM file: read %zu of %zu bytes", bytesRead, romSize);
        StaticSetError(debugMsg);
        return false;
    }
    
    sprintf_s(debugMsg, "ROM data loaded at %p, size %zu, first bytes: %02X %02X %02X %02X",
              (void*)romData.data(), romSize,
              romData.size() > 0 ? romData[0] : 0,
              romData.size() > 1 ? romData[1] : 0,
              romData.size() > 2 ? romData[2] : 0,
              romData.size() > 3 ? romData[3] : 0);
    StaticLogDebug(debugMsg);
    
    /* Apply BPS patches from patches folder */
    StaticLogDebug("Checking for BPS patches...");
    size_t patchedSize = romSize;
    CV64_BPS_Result bpsResult = CV64_BPS_ApplyPatches(romData.data(), &patchedSize, romData.capacity());
    if (bpsResult == CV64_BPS_SUCCESS) {
        if (patchedSize != romSize) {
            romData.resize(patchedSize);
            sprintf_s(debugMsg, "ROM size changed after patching: %zu -> %zu", romSize, patchedSize);
            StaticLogDebug(debugMsg);
            romSize = patchedSize;
        }
        StaticLogDebug("BPS patches applied successfully");
    } else if (bpsResult != CV64_BPS_NO_PATCHES_FOUND) {
        sprintf_s(debugMsg, "BPS patching failed: %s", CV64_BPS_GetErrorMessage(bpsResult));
        StaticLogDebug(debugMsg);
    }
    
    /* Open ROM in core */
    sprintf_s(debugMsg, "Calling CoreDoCommand(M64CMD_ROM_OPEN, %d, %p)", (int)romSize, (void*)romData.data());
    StaticLogDebug(debugMsg);
    
    m64p_error result = CoreDoCommand(M64CMD_ROM_OPEN, (int)romSize, romData.data());
    if (result != M64ERR_SUCCESS) {
        StaticSetError("CoreDoCommand(ROM_OPEN) failed: " + std::string(CoreErrorMessage(result)));
        return false;
    }
    
    /* Get ROM header */
    result = CoreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(s_staticRomHeader), &s_staticRomHeader);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: Could not get ROM header");
    }
    
    s_staticRomPath = romPath;
    s_staticRomLoaded = true;
    s_staticState = CV64_INTEGRATION_ROM_LOADED;
    
    StaticLogDebug("ROM loaded: " + std::string(s_staticRomHeader.name, 20));
    
    /* Initialize plugins after ROM is loaded - this is critical!
     * plugin_start() calls each plugin's initiation function:
     * - GFX: initiateGFX()
     * - Audio: initiateAudio()
     * - Input: initiateControllers()
     * - RSP: initiateRSP() <- THIS INITIALIZES g_hle memory pointers!
     * Without this, RSP plugin's g_hle.dmem/dram are nullptr causing crashes
     */
    StaticLogDebug("Calling plugin_start for all plugins...");
    result = plugin_start(M64PLUGIN_GFX);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(GFX) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_AUDIO);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(AUDIO) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_INPUT);
    if (result != M64ERR_SUCCESS) {
        StaticLogDebug("Warning: plugin_start(INPUT) failed: " + std::string(CoreErrorMessage(result)));
    }
    result = plugin_start(M64PLUGIN_RSP);
    if (result != M64ERR_SUCCESS) {
        StaticSetError("plugin_start(RSP) failed: " + std::string(CoreErrorMessage(result)));
        return false;
    }
    StaticLogDebug("All plugins started successfully");
    
    return true;
}

void CV64_M64P_Static_CloseROM() {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticRomLoaded) {
        CoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
        s_staticRomLoaded = false;
        s_staticRomPath.clear();
        s_staticState = CV64_INTEGRATION_PLUGINS_LOADED;
    }
}

bool CV64_M64P_Static_GetROMHeader(CV64_RomHeader* header) {
    if (!header || !s_staticRomLoaded) return false;
    *header = s_staticRomHeader;
    return true;
}

bool CV64_M64P_Static_IsCV64ROM() {
    if (!s_staticRomLoaded) return false;
    
    /* Check for Castlevania 64 by game ID */
    /* NADE = Akumajou Dracula (Japan) */
    /* NAEE = Castlevania (USA) */
    /* NAEP = Castlevania (PAL) */
    char gameId[5] = {0};
    memcpy(gameId, &s_staticRomHeader.cartridge_id, 2);
    gameId[2] = s_staticRomHeader.country_code;
    gameId[3] = s_staticRomHeader.manufacturer_id;
    
    std::string name(s_staticRomHeader.name, 20);
    return (name.find("CASTLEVANIA") != std::string::npos ||
            name.find("DRACULA") != std::string::npos);
}

/*===========================================================================
 * Emulation Control
 *===========================================================================*/

static void StaticEmulationThreadFunc() {
    StaticLogDebug("Emulation thread started");
    
    /* Note: RDRAM pointer may not be valid until after CoreDoCommand(EXECUTE) starts
     * The frame callback will handle lazy initialization of the memory hook system.
     * We still try here as a fallback. */
    void* rdramPtr = DebugMemGetPointer(M64P_DBG_PTR_RDRAM);
    if (rdramPtr != NULL) {
        s_staticRdram = (u8*)rdramPtr;
        char msg[256];
        snprintf(msg, sizeof(msg), "RDRAM pointer obtained early: %p", s_staticRdram);
        StaticLogDebug(msg);
        
        /* Initialize memory hook system with RDRAM pointer for native patches
         * Use 8MB size - CV64 requires Expansion Pak for our Gameshark cheats
         * Addresses like 0x80387ACE are beyond 4MB RDRAM */
        CV64_Memory_SetRDRAM(s_staticRdram, 0x800000);  // 8MB
        StaticLogDebug("Memory hook system initialized with 8MB RDRAM (Expansion Pak required)");
    } else {
        StaticLogDebug("RDRAM pointer not available yet - frame callback will initialize");
    }
    
    s_staticEmulationRunning = true;
    
    StaticLogDebug("Calling CoreDoCommand(M64CMD_EXECUTE)...");
    
    /* Execute emulation - this blocks until stopped */
    m64p_error result = CoreDoCommand(M64CMD_EXECUTE, 0, NULL);
    
    s_staticEmulationRunning = false;
    s_staticRdram = NULL;
    
    /* Clear memory hook system on shutdown */
    CV64_Memory_SetRDRAM(NULL, 0);
    
    /* Shutdown threading system after emulation ends */
    CV64_Threading_Shutdown();
    
    if (result != M64ERR_SUCCESS && !s_staticStopRequested) {
        StaticSetError("Emulation ended with error: " + std::string(CoreErrorMessage(result)));
    }
    
    StaticLogDebug("Emulation thread ended");
}

bool CV64_M64P_Static_Start() {
    std::lock_guard<std::mutex> lock(s_staticStateMutex);
    
    if (s_staticState < CV64_INTEGRATION_ROM_LOADED) {
        StaticSetError("No ROM loaded");
        return false;
    }
    
    if (s_staticEmulationRunning) {
        return true;
    }
    
    StaticLogDebug("Starting emulation...");
    s_staticStopRequested = false;
    
    /* Initialize threading system before starting emulation */
    CV64_ThreadConfig threadConfig = {
        .enableAsyncGraphics = true,   // Async frame presentation
        .enableAsyncAudio = true,      // SDL audio callback is async
        .enableWorkerThreads = true,   // Worker pool for async tasks
        .workerThreadCount = 0,        // Auto-detect based on CPU cores
        .graphicsQueueDepth = 2,       // Double buffering (safer than triple)
        .enableParallelRSP = false     // Keep RSP synchronous for safety
    };
    
    if (!CV64_Threading_Init(&threadConfig)) {
        StaticLogDebug("WARNING: Threading system init failed, running single-threaded");
    } else {
        StaticLogDebug("Threading system initialized successfully");
    }
    
    /* Start emulation in a separate thread */
    s_staticEmulationThread = std::thread(StaticEmulationThreadFunc);
    
    /* Wait briefly for emulation to actually start */
    int retries = 50;
    while (!s_staticEmulationRunning && retries > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        retries--;
    }
    
    if (s_staticEmulationRunning) {
        s_staticState = CV64_INTEGRATION_RUNNING;
        return true;
    }
    
    StaticSetError("Emulation failed to start");
    return false;
}

void CV64_M64P_Static_Stop() {
    if (!s_staticEmulationRunning) return;
    
    StaticLogDebug("Stopping emulation...");
    s_staticStopRequested = true;
    CoreDoCommand(M64CMD_STOP, 0, NULL);
    
    if (s_staticEmulationThread.joinable()) {
        s_staticEmulationThread.join();
    }
    
    s_staticState = CV64_INTEGRATION_ROM_LOADED;
}

void CV64_M64P_Static_Pause() {
    if (s_staticEmulationRunning) {
        CoreDoCommand(M64CMD_PAUSE, 0, NULL);
        s_staticState = CV64_INTEGRATION_PAUSED;
    }
}

void CV64_M64P_Static_Resume() {
    if (s_staticState == CV64_INTEGRATION_PAUSED) {
        CoreDoCommand(M64CMD_RESUME, 0, NULL);
        s_staticState = CV64_INTEGRATION_RUNNING;
    }
}

void CV64_M64P_Static_Reset(bool hard) {
    if (s_staticEmulationRunning) {
        CoreDoCommand(M64CMD_RESET, hard ? 1 : 0, NULL);
    }
}

void CV64_M64P_Static_AdvanceFrame() {
    if (s_staticEmulationRunning) {
        CoreDoCommand(M64CMD_ADVANCE_FRAME, 0, NULL);
    }
}

bool CV64_M64P_Static_IsRunning() {
    return s_staticEmulationRunning;
}

bool CV64_M64P_Static_IsPaused() {
    return s_staticState == CV64_INTEGRATION_PAUSED;
}

/*===========================================================================
 * Memory Access
 *===========================================================================*/

u8 CV64_M64P_Static_ReadMemory8(u32 address) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        return s_staticRdram[addr ^ 3]; /* XOR for endianness */
    }
    return 0;
}

u16 CV64_M64P_Static_ReadMemory16(u32 address) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        u16* ptr = (u16*)(s_staticRdram + (addr ^ 2)); /* XOR for endianness */
        return *ptr;
    }
    return 0;
}

u32 CV64_M64P_Static_ReadMemory32(u32 address) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        return *(u32*)(s_staticRdram + addr);
    }
    unsigned int value = 0;
    DebugMemRead32(&value, address);
    return value;
}

void CV64_M64P_Static_WriteMemory8(u32 address, u8 value) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        s_staticRdram[addr ^ 3] = value;
    }
}

void CV64_M64P_Static_WriteMemory16(u32 address, u16 value) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        u16* ptr = (u16*)(s_staticRdram + (addr ^ 2));
        *ptr = value;
    }
}

void CV64_M64P_Static_WriteMemory32(u32 address, u32 value) {
    u32 addr = address & 0x00FFFFFF;
    if (s_staticRdram && addr < 0x800000) {
        *(u32*)(s_staticRdram + addr) = value;
    } else {
        DebugMemWrite32(value, address);
    }
}

void* CV64_M64P_Static_GetRDRAMPointer() {
    /* If we don't have RDRAM pointer yet, try to get it now
     * This handles the case where emulation has started but we haven't
     * gotten the pointer yet (lazy initialization) */
    if (s_staticRdram == NULL && s_staticEmulationRunning) {
        void* rdramPtr = DebugMemGetPointer(M64P_DBG_PTR_RDRAM);
        if (rdramPtr != NULL) {
            s_staticRdram = (u8*)rdramPtr;
            char msg[256];
            snprintf(msg, sizeof(msg), "[CV64_STATIC] RDRAM pointer obtained via lazy init: %p", s_staticRdram);
            OutputDebugStringA(msg);
            OutputDebugStringA("\n");
        }
    }
    return s_staticRdram;
}

/*===========================================================================
 * Save States
 *===========================================================================*/

bool CV64_M64P_Static_SaveState(int slot) {
    if (!s_staticEmulationRunning) return false;
    
    CoreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
    m64p_error result = CoreDoCommand(M64CMD_STATE_SAVE, 0, NULL);
    return result == M64ERR_SUCCESS;
}

bool CV64_M64P_Static_LoadState(int slot) {
    if (!s_staticEmulationRunning) return false;
    
    CoreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
    m64p_error result = CoreDoCommand(M64CMD_STATE_LOAD, 0, NULL);
    return result == M64ERR_SUCCESS;
}

void CV64_M64P_Static_SetSaveSlot(int slot) {
    CoreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
}

/*===========================================================================
 * Speed Control
 *===========================================================================*/

static int s_staticSpeedFactor = 100;

void CV64_M64P_Static_SetSpeedFactor(int factor) {
    s_staticSpeedFactor = factor;
    
    m64p_static_handle configHandle;
    if (ConfigOpenSection("Core", &configHandle) == M64ERR_SUCCESS) {
        ConfigSetParameter(configHandle, "SpeedFactor", M64TYPE_INT, &factor);
    }
}

int CV64_M64P_Static_GetSpeedFactor() {
    return s_staticSpeedFactor;
}

void CV64_M64P_Static_SetSpeedLimiter(bool enabled) {
    int value = enabled ? 1 : 0;
    
    m64p_static_handle configHandle;
    if (ConfigOpenSection("Core", &configHandle) == M64ERR_SUCCESS) {
        ConfigSetParameter(configHandle, "SpeedLimiter", M64TYPE_BOOL, &value);
    }
}

/*===========================================================================
 * Frame Callback
 *===========================================================================*/

void CV64_M64P_Static_SetFrameCallback(CV64_FrameCallback callback, void* context) {
    s_staticFrameCallback = callback;
    s_staticFrameCallbackContext = context;
    
    /* Note: Frame callback integration requires hooking into the core's
       frame processing. This would typically be done via CoreDoCommand
       with M64CMD_SET_FRAME_CALLBACK if supported, or by modifying
       the static library build to call our callback. */
}

/*===========================================================================
 * Controller Override
 *===========================================================================*/

static CV64_M64P_ControllerState s_staticControllerOverrides[4];
static bool s_staticControllerOverrideActive[4] = {false, false, false, false};

void CV64_M64P_Static_SetControllerOverride(int controller, const CV64_M64P_ControllerState* state) {
    if (controller >= 0 && controller < 4 && state) {
        s_staticControllerOverrides[controller] = *state;
        s_staticControllerOverrideActive[controller] = true;
    }
}

void CV64_M64P_Static_ClearControllerOverride(int controller) {
    if (controller >= 0 && controller < 4) {
        s_staticControllerOverrideActive[controller] = false;
    }
}

/*===========================================================================
 * Wrapper Functions (to match dynamic API)
 * These allow seamless switching between static and dynamic linking
 *===========================================================================*/

/* These functions implement the standard CV64_M64P_* API using static linking */

bool CV64_M64P_Init(void* hwnd) {
    if (!CV64_M64P_Static_Initialize(hwnd)) return false;
    return CV64_M64P_Static_LoadPlugins();
}

void CV64_M64P_Shutdown() {
    CV64_M64P_Static_Shutdown();
}

bool CV64_M64P_LoadPlugins() {
    return CV64_M64P_Static_LoadPlugins();
}

void CV64_M64P_ListPlugins() {
    std::filesystem::path coreDir = StaticGetCoreDirectory();
    std::filesystem::path pluginDir = coreDir / "plugins";
    
    StaticLogDebug("Listing plugins in: " + pluginDir.string());
    
    if (std::filesystem::exists(pluginDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
            if (entry.path().extension() == ".dll") {
                StaticLogDebug("  Found: " + entry.path().filename().string());
            }
        }
    }
}

CV64_IntegrationState CV64_M64P_GetState() {
    return CV64_M64P_Static_GetState();
}

const char* CV64_M64P_GetLastError() {
    return CV64_M64P_Static_GetLastError();
}

bool CV64_M64P_LoadEmbeddedROM() {
    return CV64_M64P_Static_LoadEmbeddedROM();
}

bool CV64_M64P_LoadROM(const char* rom_path) {
    return CV64_M64P_Static_LoadROM(rom_path);
}

void CV64_M64P_CloseROM() {
    CV64_M64P_Static_CloseROM();
}

bool CV64_M64P_GetROMHeader(CV64_RomHeader* header) {
    return CV64_M64P_Static_GetROMHeader(header);
}

bool CV64_M64P_IsCV64ROM() {
    return CV64_M64P_Static_IsCV64ROM();
}

bool CV64_M64P_Start() {
    return CV64_M64P_Static_Start();
}

void CV64_M64P_Stop() {
    CV64_M64P_Static_Stop();
}

void CV64_M64P_Pause() {
    CV64_M64P_Static_Pause();
}

void CV64_M64P_Resume() {
    CV64_M64P_Static_Resume();
}

void CV64_M64P_Reset(bool hard) {
    CV64_M64P_Static_Reset(hard);
}

void CV64_M64P_AdvanceFrame() {
    CV64_M64P_Static_AdvanceFrame();
}

bool CV64_M64P_IsRunning() {
    return CV64_M64P_Static_IsRunning();
}

bool CV64_M64P_IsPaused() {
    return CV64_M64P_Static_IsPaused();
}

u8 CV64_M64P_ReadMemory8(u32 address) {
    return CV64_M64P_Static_ReadMemory8(address);
}

u16 CV64_M64P_ReadMemory16(u32 address) {
    return CV64_M64P_Static_ReadMemory16(address);
}

u32 CV64_M64P_ReadMemory32(u32 address) {
    return CV64_M64P_Static_ReadMemory32(address);
}

void CV64_M64P_WriteMemory8(u32 address, u8 value) {
    CV64_M64P_Static_WriteMemory8(address, value);
}

void CV64_M64P_WriteMemory16(u32 address, u16 value) {
    CV64_M64P_Static_WriteMemory16(address, value);
}

void CV64_M64P_WriteMemory32(u32 address, u32 value) {
    CV64_M64P_Static_WriteMemory32(address, value);
}

void* CV64_M64P_GetRDRAMPointer() {
    return CV64_M64P_Static_GetRDRAMPointer();
}

bool CV64_M64P_SaveState(int slot) {
    return CV64_M64P_Static_SaveState(slot);
}

bool CV64_M64P_LoadState(int slot) {
    return CV64_M64P_Static_LoadState(slot);
}

void CV64_M64P_SetSaveSlot(int slot) {
    CV64_M64P_Static_SetSaveSlot(slot);
}

void CV64_M64P_SetSpeedFactor(int factor) {
    CV64_M64P_Static_SetSpeedFactor(factor);
}

int CV64_M64P_GetSpeedFactor() {
    return CV64_M64P_Static_GetSpeedFactor();
}

void CV64_M64P_SetSpeedLimiter(bool enabled) {
    CV64_M64P_Static_SetSpeedLimiter(enabled);
}

void CV64_M64P_SetFrameCallback(CV64_FrameCallback callback, void* context) {
    CV64_M64P_Static_SetFrameCallback(callback, context);
}

void CV64_M64P_SetControllerOverride(int controller, const CV64_M64P_ControllerState* state) {
    CV64_M64P_Static_SetControllerOverride(controller, state);
}

void CV64_M64P_ClearControllerOverride(int controller) {
    CV64_M64P_Static_ClearControllerOverride(controller);
}

#endif /* CV64_STATIC_MUPEN64PLUS */

