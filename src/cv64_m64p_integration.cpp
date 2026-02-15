/**
 * @file cv64_m64p_integration.cpp
 * @brief Castlevania 64 PC Recomp - mupen64plus Integration Implementation
 * 
 * Integrates the official mupen64plus core for N64 emulation.
 * Source: https://github.com/mupen64plus/mupen64plus-core
 * 
 * @copyright 2024 CV64 Recomp Team
 */

/* Only compile the dynamic DLL loader version when not using static linking */
#ifndef CV64_STATIC_MUPEN64PLUS

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_m64p_integration.h"
#include "../include/cv64_camera_patch.h"
#include "../include/cv64_controller.h"
#include "../include/cv64_vidext.h"
#include "../include/cv64_input_plugin.h"

#include <Windows.h>
#include <string>
#include <filesystem>
#include <cstring>
#include <cstdio>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include "../include/cv64_ini_parser.h"

/* m64p_handle type for config API */
typedef void* m64p_handle;

/* m64p_type enum for config parameter types */
typedef enum {
    M64TYPE_INT = 1,
    M64TYPE_FLOAT,
    M64TYPE_BOOL,
    M64TYPE_STRING
} m64p_type;

/*===========================================================================
 * mupen64plus Library Handles
 *===========================================================================*/

/* Function pointer types from mupen64plus API */
typedef m64p_error (*ptr_CoreStartup)(int, const char*, const char*, void*, void*, void*, void*);
typedef m64p_error (*ptr_CoreShutdown)(void);
typedef m64p_error (*ptr_CoreDoCommand)(m64p_command, int, void*);
typedef m64p_error (*ptr_CoreAttachPlugin)(m64p_plugin_type, m64p_dynlib_handle);
typedef m64p_error (*ptr_CoreDetachPlugin)(m64p_plugin_type);
typedef const char* (*ptr_CoreErrorMessage)(m64p_error);
typedef m64p_error (*ptr_CoreOverrideVidExt)(void*);
typedef m64p_error (*ptr_DebugMemGetPointer)(void**, int);
typedef m64p_error (*ptr_DebugSetCallbacks)(void*, void*, void*);

/* Config API function pointer types */
typedef m64p_error (*ptr_ConfigOpenSection)(const char*, m64p_handle*);
typedef m64p_error (*ptr_ConfigSetParameter)(m64p_handle, const char*, m64p_type, const void*);
typedef m64p_error (*ptr_ConfigSetDefaultInt)(m64p_handle, const char*, int, const char*);
typedef m64p_error (*ptr_ConfigSetDefaultFloat)(m64p_handle, const char*, float, const char*);
typedef m64p_error (*ptr_ConfigSetDefaultBool)(m64p_handle, const char*, int, const char*);
typedef m64p_error (*ptr_ConfigSetDefaultString)(m64p_handle, const char*, const char*, const char*);
typedef m64p_error (*ptr_ConfigSaveFile)(void);
typedef m64p_error (*ptr_ConfigSaveSection)(const char*);

/* Plugin function pointer types */
typedef m64p_error (*ptr_PluginStartup)(m64p_dynlib_handle, void*, void(*)(void*, int, const char*));
typedef m64p_error (*ptr_PluginShutdown)(void);
typedef m64p_error (*ptr_PluginGetVersion)(m64p_plugin_type*, int*, int*, const char**, int*);

/* Library handles */
static HMODULE s_coreLib = NULL;
static HMODULE s_gfxPlugin = NULL;
static HMODULE s_audioPlugin = NULL;
static HMODULE s_inputPlugin = NULL;
static HMODULE s_rspPlugin = NULL;

/* Plugin startup/shutdown function pointers */
static ptr_PluginStartup s_gfxPluginStartup = NULL;
static ptr_PluginShutdown s_gfxPluginShutdown = NULL;
static ptr_PluginStartup s_audioPluginStartup = NULL;
static ptr_PluginShutdown s_audioPluginShutdown = NULL;
static ptr_PluginStartup s_inputPluginStartup = NULL;
static ptr_PluginShutdown s_inputPluginShutdown = NULL;
static ptr_PluginStartup s_rspPluginStartup = NULL;
static ptr_PluginShutdown s_rspPluginShutdown = NULL;

/* Core function pointers */
static ptr_CoreStartup s_coreStartup = NULL;
static ptr_CoreShutdown s_coreShutdown = NULL;
static ptr_CoreDoCommand s_coreDoCommand = NULL;
static ptr_CoreAttachPlugin s_coreAttachPlugin = NULL;
static ptr_CoreDetachPlugin s_coreDetachPlugin = NULL;
static ptr_CoreErrorMessage s_coreErrorMessage = NULL;
static ptr_CoreOverrideVidExt s_coreOverrideVidExt = NULL;

/* Track if plugins are attached */
static bool s_pluginsAttached = false;

/* Additional core function pointers */
static ptr_DebugMemGetPointer s_debugMemGetPointer = NULL;

/* Config API function pointers */
static ptr_ConfigOpenSection s_configOpenSection = NULL;
static ptr_ConfigSetParameter s_configSetParameter = NULL;
static ptr_ConfigSetDefaultInt s_configSetDefaultInt = NULL;
static ptr_ConfigSetDefaultFloat s_configSetDefaultFloat = NULL;
static ptr_ConfigSetDefaultBool s_configSetDefaultBool = NULL;
static ptr_ConfigSetDefaultString s_configSetDefaultString = NULL;
static ptr_ConfigSaveFile s_configSaveFile = NULL;
static ptr_ConfigSaveSection s_configSaveSection = NULL;

/*===========================================================================
 * State Variables
 *===========================================================================*/

static CV64_IntegrationState s_state = CV64_INTEGRATION_UNINITIALIZED;
static std::string s_lastError;
static void* s_hwnd = NULL;
static std::filesystem::path s_romPath;
static CV64_RomHeader s_romHeader;
static bool s_romLoaded = false;

/* Emulation thread */
static std::thread s_emulationThread;
static std::atomic<bool> s_emulationRunning(false);
static std::atomic<bool> s_stopRequested(false);
static std::mutex s_stateMutex;

/* Frame callback for patches */
static CV64_FrameCallback s_frameCallback = NULL;
static void* s_frameCallbackContext = NULL;

/* Controller override */
static bool s_controllerOverride[4] = { false, false, false, false };
static CV64_M64P_ControllerState s_controllerState[4] = { 0 };

/* RDRAM pointer (will be set by memory access) */
static u8* s_rdram = NULL;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void SetError(const std::string& error) {
    s_lastError = error;
    s_state = CV64_INTEGRATION_ERROR;
    OutputDebugStringA("[CV64_M64P] ERROR: ");
    OutputDebugStringA(error.c_str());
    OutputDebugStringA("\n");
}

static void LogDebug(const std::string& msg) {
    OutputDebugStringA("[CV64_M64P] ");
    OutputDebugStringA(msg.c_str());
    OutputDebugStringA("\n");
}

static std::filesystem::path GetCoreDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

static std::filesystem::path FindCoreLib() {
    std::filesystem::path coreDir = GetCoreDirectory();
    LogDebug("Searching for mupen64plus core...");
    LogDebug("EXE directory: " + coreDir.string());
    
    /* Look for mupen64plus core library in exe directory */
    LogDebug("Checking: " + coreDir.string());
    if (std::filesystem::exists(coreDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(coreDir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("mupen64plus") != std::string::npos &&
                filename.find("video") == std::string::npos &&
                filename.find("audio") == std::string::npos &&
                filename.find("input") == std::string::npos &&
                filename.find("rsp") == std::string::npos &&
                entry.path().extension() == ".dll") {
                LogDebug("Found core: " + entry.path().string());
                return entry.path();
            }
        }
    }
    
    /* Check plugins subdirectory */
    std::filesystem::path pluginsDir = coreDir / "plugins";
    LogDebug("Checking: " + pluginsDir.string());
    if (std::filesystem::exists(pluginsDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
            std::string filename = entry.path().filename().string();
            if (filename == "mupen64plus.dll") {
                LogDebug("Found core: " + entry.path().string());
                return entry.path();
            }
        }
    }
    
    /* Check project root plugins folder (2 levels up from exe) */
    std::filesystem::path projectPlugins = coreDir.parent_path().parent_path() / "plugins";
    LogDebug("Checking: " + projectPlugins.string());
    if (std::filesystem::exists(projectPlugins)) {
        std::filesystem::path corePath = projectPlugins / "mupen64plus.dll";
        if (std::filesystem::exists(corePath)) {
            LogDebug("Found core: " + corePath.string());
            return corePath;
        }
        /* Also search for any mupen64plus core dll */
        for (const auto& entry : std::filesystem::directory_iterator(projectPlugins)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("mupen64plus") != std::string::npos &&
                filename.find("video") == std::string::npos &&
                filename.find("audio") == std::string::npos &&
                filename.find("input") == std::string::npos &&
                filename.find("rsp") == std::string::npos &&
                entry.path().extension() == ".dll") {
                LogDebug("Found core: " + entry.path().string());
                return entry.path();
            }
        }
    }
    
    /* Also check RMG subdirectory */
    std::filesystem::path rmgCore = coreDir / "RMG" / "mupen64plus.dll";
    LogDebug("Checking: " + rmgCore.string());
    if (std::filesystem::exists(rmgCore)) {
        LogDebug("Found core: " + rmgCore.string());
        return rmgCore;
    }
    
    LogDebug("Core NOT FOUND in any location!");
    return std::filesystem::path();
}

static std::filesystem::path FindPlugin(const std::string& type) {
    std::filesystem::path coreDir = GetCoreDirectory();
    LogDebug("Searching for plugin: " + type);
    
    /* Search locations in order of priority */
    std::vector<std::filesystem::path> searchPaths = {
        coreDir / "plugins",                              /* x64/Release/plugins */
        coreDir,                                          /* x64/Release */
        coreDir.parent_path().parent_path() / "plugins",  /* project root plugins */
    };
    
    for (const auto& searchDir : searchPaths) {
        LogDebug("Checking: " + searchDir.string());
        if (!std::filesystem::exists(searchDir)) {
            continue;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find(type) != std::string::npos &&
                entry.path().extension() == ".dll") {
                LogDebug("Found plugin: " + entry.path().string());
                return entry.path();
            }
        }
    }
    
    LogDebug("Plugin NOT FOUND: " + type);
    return std::filesystem::path();
}

/*===========================================================================
 * Plugin Loading and Initialization
 *===========================================================================*/

static void PluginDebugCallback(void* context, int level, const char* message) {
    const char* pluginName = (const char*)context;
    OutputDebugStringA("[M64P-");
    OutputDebugStringA(pluginName ? pluginName : "Plugin");
    OutputDebugStringA("] ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

static bool LoadPlugin(const std::string& type, const std::string& searchName, 
                   HMODULE* outHandle, ptr_PluginStartup* outStartup, 
                   ptr_PluginShutdown* outShutdown, m64p_plugin_type expectedType) {
    
std::filesystem::path pluginPath = FindPlugin(searchName);
if (pluginPath.empty()) {
    LogDebug("Could not find " + type + " plugin matching: " + searchName);
    return false;
}
    
LogDebug("Loading " + type + " plugin: " + pluginPath.string());
    
/* Set DLL directory for plugin dependencies (SDL2.dll, etc.) */
std::filesystem::path pluginDir = pluginPath.parent_path();
SetDllDirectoryW(pluginDir.c_str());
LogDebug("  DLL search path set to: " + pluginDir.string());
    
*outHandle = LoadLibraryW(pluginPath.c_str());
    
if (!*outHandle) {
    DWORD err = GetLastError();
    std::string errMsg = "Failed to load " + type + " plugin. Error code: " + std::to_string(err);
    if (err == 126) {
        errMsg += " (DLL not found - missing dependency like SDL2.dll?)";
    } else if (err == 193) {
        errMsg += " (32/64-bit architecture mismatch)";
    } else if (err == 127) {
        errMsg += " (Procedure not found - wrong DLL version?)";
    }
    LogDebug(errMsg);
    SetDllDirectoryW(NULL);
    return false;
}
    
/* Keep DLL directory set for now - plugins may need it during startup */
/* It will be reset after all plugins are loaded */
    
/* Get plugin functions */
*outStartup = (ptr_PluginStartup)GetProcAddress(*outHandle, "PluginStartup");
*outShutdown = (ptr_PluginShutdown)GetProcAddress(*outHandle, "PluginShutdown");
ptr_PluginGetVersion getVersion = (ptr_PluginGetVersion)GetProcAddress(*outHandle, "PluginGetVersion");
    
    if (!*outStartup || !*outShutdown || !getVersion) {
        LogDebug("Failed to get " + type + " plugin functions");
        FreeLibrary(*outHandle);
        *outHandle = NULL;
        return false;
    }
    
    /* Verify plugin type */
    m64p_plugin_type pluginType;
    int pluginVersion, apiVersion, capabilities;
    const char* pluginName = NULL;
    
    m64p_error ret = getVersion(&pluginType, &pluginVersion, &apiVersion, &pluginName, &capabilities);
    if (ret != M64ERR_SUCCESS) {
        LogDebug("Failed to get " + type + " plugin version");
        FreeLibrary(*outHandle);
        *outHandle = NULL;
        return false;
    }
    
    if (pluginType != expectedType) {
        LogDebug("Plugin type mismatch for " + type);
        FreeLibrary(*outHandle);
        *outHandle = NULL;
        return false;
    }
    
    LogDebug(type + " plugin loaded: " + std::string(pluginName ? pluginName : "Unknown") + 
             " v" + std::to_string(pluginVersion >> 16) + "." + 
             std::to_string((pluginVersion >> 8) & 0xFF) + "." +
             std::to_string(pluginVersion & 0xFF));
    
    return true;
}

static bool StartupPlugin(const std::string& type, HMODULE handle, ptr_PluginStartup startup) {
    if (!handle || !startup) {
        return false;
    }
    
    LogDebug("Starting up " + type + " plugin...");
    
    m64p_error ret = startup(s_coreLib, (void*)type.c_str(), PluginDebugCallback);
    if (ret != M64ERR_SUCCESS) {
        LogDebug("Failed to startup " + type + " plugin: " + 
                 std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
        return false;
    }
    
    LogDebug(type + " plugin started successfully");
    return true;
}

static bool LoadAndStartupAllPlugins() {
    LogDebug("Loading all plugins...");
    
    /* Load Video Plugin (try multiple names) */
    bool videoLoaded = LoadPlugin("Video", "video-GLideN64", &s_gfxPlugin, 
                                   &s_gfxPluginStartup, &s_gfxPluginShutdown, M64PLUGIN_GFX);
    if (!videoLoaded) {
        videoLoaded = LoadPlugin("Video", "video-glide64mk2", &s_gfxPlugin,
                                  &s_gfxPluginStartup, &s_gfxPluginShutdown, M64PLUGIN_GFX);
    }
    if (!videoLoaded) {
        videoLoaded = LoadPlugin("Video", "video-rice", &s_gfxPlugin,
                                  &s_gfxPluginStartup, &s_gfxPluginShutdown, M64PLUGIN_GFX);
    }
    if (!videoLoaded) {
        videoLoaded = LoadPlugin("Video", "video", &s_gfxPlugin,
                                  &s_gfxPluginStartup, &s_gfxPluginShutdown, M64PLUGIN_GFX);
    }
    
    /* Load Audio Plugin */
    bool audioLoaded = LoadPlugin("Audio", "audio-sdl", &s_audioPlugin,
                                   &s_audioPluginStartup, &s_audioPluginShutdown, M64PLUGIN_AUDIO);
    if (!audioLoaded) {
        audioLoaded = LoadPlugin("Audio", "audio", &s_audioPlugin,
                                  &s_audioPluginStartup, &s_audioPluginShutdown, M64PLUGIN_AUDIO);
    }
    
    /* Use Built-in Input Plugin */
    /* CRITICAL FIX: The input plugin is built into the main EXE, not an external DLL! */
    /* We MUST use the built-in plugin and NOT load any external input DLL */
    bool inputLoaded = false;
    LogDebug("=== Loading CV64 Built-in Input Plugin (XInput wrapper) ===");
    LogDebug("NOTE: Ignoring any external input plugins (mupen64plus-input-sdl.dll, etc.)");
    
    /* Get handle to our own executable (where the input plugin is built-in) */
    HMODULE ourModule = GetModuleHandle(NULL);
    if (ourModule) {
        /* Try to get the plugin functions from our own EXE */
        s_inputPluginStartup = (ptr_PluginStartup)GetProcAddress(ourModule, "PluginStartup");
        s_inputPluginShutdown = (ptr_PluginShutdown)GetProcAddress(ourModule, "PluginShutdown");
        ptr_PluginGetVersion getVersion = (ptr_PluginGetVersion)GetProcAddress(ourModule, "PluginGetVersion");
        
        if (s_inputPluginStartup && s_inputPluginShutdown && getVersion) {
            /* Verify it's an input plugin */
            m64p_plugin_type pluginType;
            int pluginVersion, apiVersion, capabilities;
            const char* pluginName = NULL;
            
            m64p_error ret = getVersion(&pluginType, &pluginVersion, &apiVersion, &pluginName, &capabilities);
            if (ret == M64ERR_SUCCESS && pluginType == M64PLUGIN_INPUT) {
                s_inputPlugin = ourModule;  /* Use our own module handle */
                inputLoaded = true;
                LogDebug("? Built-in input plugin loaded: " + std::string(pluginName ? pluginName : "CV64 Input"));
                LogDebug("  Version: " + std::to_string(pluginVersion >> 16) + "." + 
                         std::to_string((pluginVersion >> 8) & 0xFF) + "." +
                         std::to_string(pluginVersion & 0xFF));
                LogDebug("  XInput controller support: ENABLED");
                LogDebug("  Memory Pak support: ENABLED");
            } else {
                LogDebug("ERROR: Built-in plugin verification failed");
            }
        } else {
            LogDebug("ERROR: Could not find input plugin exports in main EXE");
            LogDebug("  PluginStartup: " + std::string(s_inputPluginStartup ? "FOUND" : "NOT FOUND"));
            LogDebug("  PluginShutdown: " + std::string(s_inputPluginShutdown ? "FOUND" : "NOT FOUND"));
            LogDebug("  PluginGetVersion: " + std::string(getVersion ? "FOUND" : "NOT FOUND"));
        }
    }
    
    if (!inputLoaded) {
        LogDebug("CRITICAL: Built-in input plugin not found - input will NOT work!");
        LogDebug("Make sure CV64_RMG.def exports PluginStartup, PluginShutdown, etc.");
    }
    
    /* Load RSP Plugin */
    bool rspLoaded = LoadPlugin("RSP", "rsp-hle", &s_rspPlugin,
                                 &s_rspPluginStartup, &s_rspPluginShutdown, M64PLUGIN_RSP);
    if (!rspLoaded) {
        rspLoaded = LoadPlugin("RSP", "rsp", &s_rspPlugin,
                                &s_rspPluginStartup, &s_rspPluginShutdown, M64PLUGIN_RSP);
    }
    
    /* Check required plugins */
    if (!videoLoaded) {
        SetError("Failed to load video plugin.\n\nPlease ensure a video plugin (e.g., mupen64plus-video-GLideN64.dll) is in the plugins folder.");
        return false;
    }
    
    if (!rspLoaded) {
        SetError("Failed to load RSP plugin.\n\nPlease ensure mupen64plus-rsp-hle.dll is in the plugins folder.");
        return false;
    }
    
    /* Warn about missing input plugin - this is important for gameplay! */
    if (!inputLoaded) {
        LogDebug("WARNING: No input plugin found! Controller input will not work.");
        LogDebug("Please ensure mupen64plus-input-sdl.dll is in the plugins folder.");
        /* Don't fail - we can still run without input for testing */
    }
    
    /* Startup all loaded plugins */
    if (!StartupPlugin("Video", s_gfxPlugin, s_gfxPluginStartup)) {
        SetError("Failed to initialize video plugin");
        return false;
    }
    
    if (audioLoaded && !StartupPlugin("Audio", s_audioPlugin, s_audioPluginStartup)) {
        LogDebug("Warning: Audio plugin failed to start, continuing without audio");
    }
    
    if (inputLoaded) {
        if (!StartupPlugin("Input", s_inputPlugin, s_inputPluginStartup)) {
            LogDebug("ERROR: Input plugin failed to start - controllers will not work!");
            /* Clear the plugin handle so we don't try to attach a failed plugin */
            /* NOTE: Don't FreeLibrary the input plugin - it's built into our EXE! */
            s_inputPlugin = NULL;
            s_inputPluginStartup = NULL;
            s_inputPluginShutdown = NULL;
        }
    } else {
        LogDebug("ERROR: No input plugin loaded - controllers will not work!");
    }
    
    if (!StartupPlugin("RSP", s_rspPlugin, s_rspPluginStartup)) {
        SetError("Failed to initialize RSP plugin");
        return false;
    }
    
    /* Reset DLL directory now that all plugins are loaded and started */
    SetDllDirectoryW(NULL);
    
    LogDebug("All plugins loaded and started successfully");
    return true;
}

static bool AttachAllPlugins() {
    if (!s_coreAttachPlugin) {
        SetError("CoreAttachPlugin function not available");
        return false;
    }
    
    LogDebug("Attaching plugins to core...");
    m64p_error ret;
    
    /* Attach Video Plugin */
    if (s_gfxPlugin) {
        ret = s_coreAttachPlugin(M64PLUGIN_GFX, s_gfxPlugin);
        if (ret != M64ERR_SUCCESS) {
            SetError("Failed to attach video plugin: " + 
                     std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
            return false;
        }
        LogDebug("Video plugin attached");
    }
    
    /* Attach Audio Plugin */
    if (s_audioPlugin) {
        ret = s_coreAttachPlugin(M64PLUGIN_AUDIO, s_audioPlugin);
        if (ret != M64ERR_SUCCESS) {
            LogDebug("Warning: Failed to attach audio plugin");
        } else {
            LogDebug("Audio plugin attached");
        }
    }
    
    /* Attach Input Plugin */
    if (s_inputPlugin) {
        LogDebug("Attempting to attach input plugin...");
        LogDebug("  Plugin handle: " + std::to_string((uintptr_t)s_inputPlugin));
        
        /* Verify we can find the required functions */
        auto testGetKeys = GetProcAddress((HMODULE)s_inputPlugin, "GetKeys");
        auto testInitiate = GetProcAddress((HMODULE)s_inputPlugin, "InitiateControllers");
        auto testControllerCommand = GetProcAddress((HMODULE)s_inputPlugin, "ControllerCommand");
        auto testRomOpen = GetProcAddress((HMODULE)s_inputPlugin, "RomOpen");
        
        /* CRITICAL: Check for VRU functions - RMG mupen64plus REQUIRES these specific names! */
        auto testSendVRUWord = GetProcAddress((HMODULE)s_inputPlugin, "SendVRUWord");
        auto testSetMicState = GetProcAddress((HMODULE)s_inputPlugin, "SetMicState");
        auto testReadVRUResults = GetProcAddress((HMODULE)s_inputPlugin, "ReadVRUResults");
        auto testClearVRUWords = GetProcAddress((HMODULE)s_inputPlugin, "ClearVRUWords");
        auto testSetVRUWordMask = GetProcAddress((HMODULE)s_inputPlugin, "SetVRUWordMask");
        
        LogDebug("  GetKeys export: " + std::string(testGetKeys ? "FOUND" : "NOT FOUND"));
        LogDebug("  InitiateControllers export: " + std::string(testInitiate ? "FOUND" : "NOT FOUND"));
        LogDebug("  ControllerCommand export: " + std::string(testControllerCommand ? "FOUND" : "NOT FOUND"));
        LogDebug("  RomOpen export: " + std::string(testRomOpen ? "FOUND" : "NOT FOUND"));
        LogDebug("  === VRU Functions (REQUIRED by RMG mupen64plus core) ===");
        LogDebug("  SendVRUWord export: " + std::string(testSendVRUWord ? "FOUND" : "NOT FOUND"));
        LogDebug("  SetMicState export: " + std::string(testSetMicState ? "FOUND" : "NOT FOUND"));
        LogDebug("  ReadVRUResults export: " + std::string(testReadVRUResults ? "FOUND" : "NOT FOUND"));
        LogDebug("  ClearVRUWords export: " + std::string(testClearVRUWords ? "FOUND" : "NOT FOUND"));
        LogDebug("  SetVRUWordMask export: " + std::string(testSetVRUWordMask ? "FOUND" : "NOT FOUND"));
        
        if (!testSendVRUWord || !testSetMicState || !testReadVRUResults || !testClearVRUWords || !testSetVRUWordMask) {
            LogDebug("  ERROR: VRU functions are MISSING! Core will reject plugin as incompatible!");
            LogDebug("  Make sure CV64_RMG.def exports all VRU functions with correct names!");
        }
        
        ret = s_coreAttachPlugin(M64PLUGIN_INPUT, s_inputPlugin);
        if (ret != M64ERR_SUCCESS) {
            LogDebug("ERROR: Failed to attach input plugin: " + 
                     std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
            LogDebug("Controller input will NOT work!");
        } else {
            LogDebug("Input plugin attached successfully");
        }
    } else {
        LogDebug("WARNING: No input plugin available to attach - controller input disabled");
    }
    
    /* Attach RSP Plugin */
    if (s_rspPlugin) {
        ret = s_coreAttachPlugin(M64PLUGIN_RSP, s_rspPlugin);
        if (ret != M64ERR_SUCCESS) {
            SetError("Failed to attach RSP plugin: " + 
                     std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
            return false;
        }
        LogDebug("RSP plugin attached");
    }
    
    s_pluginsAttached = true;
    LogDebug("All plugins attached successfully");
    return true;
}

static void DetachAllPlugins() {
    if (!s_coreDetachPlugin || !s_pluginsAttached) {
        return;
    }
    
    LogDebug("Detaching plugins from core...");
    
    if (s_gfxPlugin) {
        s_coreDetachPlugin(M64PLUGIN_GFX);
    }
    if (s_audioPlugin) {
        s_coreDetachPlugin(M64PLUGIN_AUDIO);
    }
    if (s_inputPlugin) {
        s_coreDetachPlugin(M64PLUGIN_INPUT);
    }
    if (s_rspPlugin) {
        s_coreDetachPlugin(M64PLUGIN_RSP);
    }
    
    s_pluginsAttached = false;
    LogDebug("All plugins detached");
}

static void ShutdownAllPlugins() {
    LogDebug("Shutting down plugins...");
    
    if (s_gfxPluginShutdown && s_gfxPlugin) {
        s_gfxPluginShutdown();
        LogDebug("Video plugin shutdown");
    }
    if (s_audioPluginShutdown && s_audioPlugin) {
        s_audioPluginShutdown();
        LogDebug("Audio plugin shutdown");
    }
    if (s_inputPluginShutdown && s_inputPlugin) {
        s_inputPluginShutdown();
        LogDebug("Input plugin shutdown");
    }
    if (s_rspPluginShutdown && s_rspPlugin) {
        s_rspPluginShutdown();
        LogDebug("RSP plugin shutdown");
    }
    
    /* Clear function pointers */
    s_gfxPluginStartup = NULL;
    s_gfxPluginShutdown = NULL;
    s_audioPluginStartup = NULL;
    s_audioPluginShutdown = NULL;
    s_inputPluginStartup = NULL;
    s_inputPluginShutdown = NULL;
    s_rspPluginStartup = NULL;
    s_rspPluginShutdown = NULL;
    
    LogDebug("All plugins shutdown");
}

/*===========================================================================
 * Debug and State Callbacks
 *===========================================================================*/

static void DebugCallback(void* context, int level, const char* message) {
    /* Log mupen64plus debug messages */
    OutputDebugStringA("[M64P] ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

static void StateCallback(void* context, m64p_core_param param_type, int new_value) {
    /* Handle state changes from the core */
    if (param_type == M64CORE_EMU_STATE) {
        switch (new_value) {
            case M64EMU_STOPPED:
                s_state = CV64_INTEGRATION_ROM_LOADED;
                break;
            case M64EMU_RUNNING:
                s_state = CV64_INTEGRATION_RUNNING;
                break;
            case M64EMU_PAUSED:
                s_state = CV64_INTEGRATION_PAUSED;
                break;
        }
    }
}

/*===========================================================================
 * Frame Hook (Called every frame)
 *===========================================================================*/

static void OnFrameComplete(unsigned int frameIndex) {
    /* Call our patches */
    if (s_frameCallback) {
        s_frameCallback(s_frameCallbackContext);
    }
    
    /* NOTE: Do NOT call CV64_Controller_Update here!
     * The mupen64plus input-sdl plugin handles all controller input.
     * Our custom input system conflicts with SDL's input handling.
     * Camera patch is also disabled during emulation to avoid input conflicts.
     */
}

/*===========================================================================
 * Memory Pak File Creation
 *===========================================================================*/

/**
 * @brief Create Memory Pak files with pre-initialized Castlevania 64 save data
 * 
 * This MUST be called BEFORE CoreStartup so the input plugin can find the files.
 * The input-sdl plugin looks for mempak files in the config directory passed to CoreStartup.
 */
static void CreateMempakFilesWithCV64Save(const std::filesystem::path& configDir) {
    /* 
     * N64 Controller Pak Format (32KB = 32768 bytes = 256 pages × 128 bytes)
     * =====================================================================
     * Page 0 (0x0000-0x007F): ID Area - Device identification and checksums
     * Pages 1-2 (0x0080-0x017F): Inode table - Page allocation map
     * Pages 3-4 (0x0180-0x027F): Note table - 16 note entries (32 bytes each)
     * Pages 5-127 (0x0280-0x3FFF): Data pages for save data
     * Pages 128-255: Backup/mirror of pages 0-127
     * 
     * We create a mempak with a pre-existing CASTLEVANIA save note (NDOE)
     * so the game recognizes an existing save and can start properly.
     */
    
    auto createMempak = [](const std::filesystem::path& path) -> bool {
        /* Only create if file doesn't exist - don't overwrite existing saves! */
        if (std::filesystem::exists(path)) {
            auto size = std::filesystem::file_size(path);
            if (size == 32768 || size == 131072) {
                LogDebug("  Mempak exists (keeping): " + path.string());
                return true;
            }
        }
        
        FILE* f = fopen(path.string().c_str(), "wb");
        if (!f) {
            LogDebug("  FAILED to create: " + path.string());
            return false;
        }
        
        /* Create formatted 32KB Controller Pak with CV64 save note */
        std::vector<unsigned char> pak(32768, 0x00);
        
        /* === Page 0: ID Area === */
        pak[0x00] = 0x81;
        pak[0x01] = 0x01;
        for (int i = 0x02; i <= 0x19; i++) pak[i] = (unsigned char)i;
        
        unsigned short checksum = 0;
        for (int i = 0; i < 0x1A; i++) checksum += pak[i];
        pak[0x1A] = (checksum >> 8) & 0xFF;
        pak[0x1B] = checksum & 0xFF;
        pak[0x1C] = (~checksum >> 8) & 0xFF;
        pak[0x1D] = ~checksum & 0xFF;
        
        for (int block = 1; block < 4; block++) {
            memcpy(&pak[block * 0x20], &pak[0], 0x20);
        }
        
        /* === Pages 1-2: Inode Table === */
        pak[0x80] = 0x00; pak[0x81] = 0x01;
        pak[0x82] = 0x00; pak[0x83] = 0x01;
        pak[0x84] = 0x00; pak[0x85] = 0x01;
        pak[0x86] = 0x00; pak[0x87] = 0x01;
        pak[0x88] = 0x00; pak[0x89] = 0x01;
        
        /* Pages 5-125: Chain for CV64 save */
        for (int i = 5; i < 125; i++) {
            int offset = 0x80 + (i * 2);
            pak[offset] = 0x00;
            pak[offset + 1] = (i + 1) & 0xFF;
        }
        pak[0x80 + (125 * 2)] = 0x00;
        pak[0x80 + (125 * 2) + 1] = 0x01;  /* End of chain */
        
        /* Pages 126-127: Free */
        pak[0x80 + (126 * 2)] = 0x00; pak[0x80 + (126 * 2) + 1] = 0x03;
        pak[0x80 + (127 * 2)] = 0x00; pak[0x80 + (127 * 2) + 1] = 0x03;
        
        /* === Pages 3-4: Note Table === */
        /* Note 0: Castlevania 64 US (NDOE) */
        int n = 0x180;
        pak[n+0]='N'; pak[n+1]='D'; pak[n+2]='O'; pak[n+3]='E';  /* Game code */
        pak[n+4]='5'; pak[n+5]='2';  /* Konami */
        pak[n+6]=0x00; pak[n+7]=0x05;  /* Start page */
        pak[n+8]=0x00; pak[n+9]=0x02;  /* Status: valid */
        pak[n+10]=0x01;  /* Occupied */
        pak[n+11]=0x00;  /* Extension */
        memcpy(&pak[n+12], "CASTLEVANIA", 11);  /* Note name */
        
        /* Note 1: Castlevania 64 JP (NDUE) */
        n = 0x180 + 32;
        pak[n+0]='N'; pak[n+1]='D'; pak[n+2]='U'; pak[n+3]='E';
        pak[n+4]='5'; pak[n+5]='2';
        pak[n+6]=0x00; pak[n+7]=0x05;
        pak[n+8]=0x00; pak[n+9]=0x02;
        pak[n+10]=0x01;
        pak[n+11]=0x00;
        memcpy(&pak[n+12], "DRACULA", 7);
        
        /* === Pages 5+: Save Data Header === */
        int d = 0x280;
        pak[d]='C'; pak[d+1]='V'; pak[d+2]='6'; pak[d+3]='4';
        pak[d+4]=0x01;  /* Slot 1 */
        pak[d+8]=0x01;  /* New game flag */
        
        /* === Backup copy === */
        memcpy(&pak[0x4000], &pak[0], 0x4000);
        
        fwrite(pak.data(), 1, pak.size(), f);
        fclose(f);
        
        LogDebug("  Created mempak with CV64 save: " + path.string());
        return true;
    };
    
    
    LogDebug("Creating Memory Pak files with Castlevania 64 save...");
    
    /* Create ONLY in save subdirectory - our built-in input plugin looks here */
    std::filesystem::path saveDir = configDir / "save";
    std::filesystem::create_directories(saveDir);
    createMempak(saveDir / "mempak0.mpk");
    
    LogDebug("Memory Pak files ready");
}

/*===========================================================================
 * INI Settings Application
 *===========================================================================*/

/**
 * @brief Find the path to a patches INI file
 */
static std::filesystem::path FindPatchesIni(const char* filename) {
    std::filesystem::path coreDir = GetCoreDirectory();
    
    std::vector<std::filesystem::path> searchPaths = {
        coreDir / "patches" / filename,
        coreDir / filename,
        coreDir.parent_path().parent_path() / "patches" / filename
    };
    
    for (const auto& path : searchPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    return std::filesystem::path();
}

/**
 * @brief Apply graphics settings from cv64_graphics.ini to video plugin
 * Overwrites matching settings in mupen64plus.cfg
 */
static bool ApplyGraphicsSettings(void) {
    if (!s_configOpenSection || !s_configSetParameter) {
        LogDebug("Config API not available, skipping graphics settings");
        return false;
    }
    
    std::filesystem::path iniPath = FindPatchesIni("cv64_graphics.ini");
    if (iniPath.empty()) {
        LogDebug("cv64_graphics.ini not found, using defaults");
        return false;
    }
    
    CV64_IniParser ini;
    if (!ini.Load(iniPath.string().c_str())) {
        LogDebug("Failed to load cv64_graphics.ini");
        return false;
    }
    
    LogDebug("Overwriting mupen64plus.cfg with cv64_graphics.ini settings...");
    
    /* === Video-General Section === */
    m64p_handle videoGeneral = NULL;
    if (s_configOpenSection("Video-General", &videoGeneral) == M64ERR_SUCCESS) {
        bool fullscreen = ini.GetBool("Display", "fullscreen", false);
        int customWidth = ini.GetInt("Display", "custom_width", 1280);
        int customHeight = ini.GetInt("Display", "custom_height", 720);
        bool vsync = ini.GetBool("Quality", "vsync", true);
        
        int fullscreenInt = fullscreen ? 1 : 0;
        int vsyncInt = vsync ? 1 : 0;
        
        s_configSetParameter(videoGeneral, "Fullscreen", M64TYPE_BOOL, &fullscreenInt);
        s_configSetParameter(videoGeneral, "ScreenWidth", M64TYPE_INT, &customWidth);
        s_configSetParameter(videoGeneral, "ScreenHeight", M64TYPE_INT, &customHeight);
        s_configSetParameter(videoGeneral, "VerticalSync", M64TYPE_BOOL, &vsyncInt);
        
        LogDebug("  [Video-General] Resolution: " + std::to_string(customWidth) + "x" + std::to_string(customHeight));
        
        if (s_configSaveSection) {
            s_configSaveSection("Video-General");
        }
    }
    
    /* === Video-GLideN64 Section === */
    m64p_handle glideSection = NULL;
    if (s_configOpenSection("Video-GLideN64", &glideSection) == M64ERR_SUCCESS) {
        int internalScale = ini.GetInt("Quality", "internal_scale", 2);
        bool vsync = ini.GetBool("Quality", "vsync", true);
        std::string aspectRatio = ini.GetString("AspectRatio", "ratio", "16_9");
        std::string texFilter = ini.GetString("Quality", "texture_filter", "LINEAR");
        std::string antiAlias = ini.GetString("Quality", "anti_aliasing", "FXAA");
        int maxFps = ini.GetInt("Quality", "max_fps", 60);
        
        /* Aspect ratio mapping: 0=stretch, 1=4:3, 2=16:9, 3=adjust */
        int aspectValue = 2; /* 16:9 default */
        if (aspectRatio == "4_3") aspectValue = 1;
        else if (aspectRatio == "16_9") aspectValue = 2;
        else if (aspectRatio == "16_10") aspectValue = 2;
        else if (aspectRatio == "21_9") aspectValue = 3;
        
        /* Texture filter mapping */
        int bilinearMode = 1; /* standard bilinear */
        if (texFilter == "NEAREST") bilinearMode = 0;
        else if (texFilter == "LINEAR") bilinearMode = 1;
        else if (texFilter == "TRILINEAR") bilinearMode = 2;
        
        /* MSAA mapping */
        int msaaLevel = 0;
        if (antiAlias == "MSAA_2X") msaaLevel = 2;
        else if (antiAlias == "MSAA_4X") msaaLevel = 4;
        else if (antiAlias == "MSAA_8X") msaaLevel = 8;
        else if (antiAlias == "MSAA_16X") msaaLevel = 16;
        
        /* FXAA enable */
        int fxaaEnabled = (antiAlias == "FXAA") ? 1 : 0;
        
        int vsyncInt = vsync ? 1 : 0;
        
        s_configSetParameter(glideSection, "UseNativeResolutionFactor", M64TYPE_INT, &internalScale);
        s_configSetParameter(glideSection, "AspectRatio", M64TYPE_INT, &aspectValue);
        s_configSetParameter(glideSection, "bilinearMode", M64TYPE_INT, &bilinearMode);
        s_configSetParameter(glideSection, "EnableFXAA", M64TYPE_BOOL, &fxaaEnabled);
        s_configSetParameter(glideSection, "MultiSampling", M64TYPE_INT, &msaaLevel);
        s_configSetParameter(glideSection, "VSync", M64TYPE_BOOL, &vsyncInt);
        s_configSetParameter(glideSection, "FrameBufferEmulation", M64TYPE_BOOL, &vsyncInt);
        
        /* === Hi-Res Texture Pack Settings === */
        /* Always enable loading texture packs from cache */
        int txHiresEnable = 1;           /* Enable hi-res textures */
        int txCacheCompression = 1;      /* Use compressed cache */
        int txHiresFullAlphaChannel = 1; /* Full alpha channel support */
        int txEnhancedTextureFileStorage = 0; /* Use cache, not file storage */
        int txHiresTextureFileStorage = 0;    /* Load from cache */
        int txSaveCache = 1;             /* Save to cache */
        int txDump = 0;                  /* Don't dump textures */
        
        /* Alternative CRC - CRITICAL for texture pack compatibility */
        int txCRC = 1;                   /* 0=Rice CRC, 1=Alternative CRC (more compatible) */
        
        /* Set texture pack path to assets folder in startup directory */
        std::filesystem::path assetsPath = GetCoreDirectory() / "assets";
        std::string txPath = assetsPath.string();
        std::string txCachePath = (assetsPath / "cache").string();
        
        /* Create directories if they don't exist */
        std::filesystem::create_directories(assetsPath);
        std::filesystem::create_directories(assetsPath / "cache");
        std::filesystem::create_directories(assetsPath / "hires_texture");
        
        s_configSetParameter(glideSection, "txHiresEnable", M64TYPE_BOOL, &txHiresEnable);
        s_configSetParameter(glideSection, "txCacheCompression", M64TYPE_BOOL, &txCacheCompression);
        s_configSetParameter(glideSection, "txHiresFullAlphaChannel", M64TYPE_BOOL, &txHiresFullAlphaChannel);
        s_configSetParameter(glideSection, "txEnhancedTextureFileStorage", M64TYPE_BOOL, &txEnhancedTextureFileStorage);
        s_configSetParameter(glideSection, "txHiresTextureFileStorage", M64TYPE_BOOL, &txHiresTextureFileStorage);
        s_configSetParameter(glideSection, "txSaveCache", M64TYPE_BOOL, &txSaveCache);
        s_configSetParameter(glideSection, "txDump", M64TYPE_BOOL, &txDump);
        s_configSetParameter(glideSection, "txCacheSize", M64TYPE_INT, &internalScale); /* Use internal scale as hint */
        
        /* Set Alternative CRC for texture matching */
        s_configSetParameter(glideSection, "txCRC", M64TYPE_INT, &txCRC);
        
        /* Set texture pack paths to assets folder */
        s_configSetParameter(glideSection, "txPath", M64TYPE_STRING, txPath.c_str());
        s_configSetParameter(glideSection, "txCachePath", M64TYPE_STRING, txCachePath.c_str());
        
        /* Set window title to CASTLEVANIA 64 RECOMP */
        const char* windowTitle = "CASTLEVANIA 64 RECOMP";
        s_configSetParameter(glideSection, "title", M64TYPE_STRING, windowTitle);
        
        /* === OSD Settings === */
        /* Set OSD font color to black (0xRRGGBBAA format) */
        /* Yellow = 0xFFFF00FF, Black = 0x000000FF */
        const char* fontColor = "000000FF";  /* Black with full opacity */
        s_configSetParameter(glideSection, "fontColor", M64TYPE_STRING, fontColor);
        
        /* Optionally disable FPS display entirely */
        int showFPS = 0;
        s_configSetParameter(glideSection, "ShowFPS", M64TYPE_BOOL, &showFPS);
        
        LogDebug("  [Video-GLideN64] Scale: " + std::to_string(internalScale) + "x, VSync: " + (vsync ? "ON" : "OFF"));
        LogDebug("  [Video-GLideN64] HiRes Textures: ENABLED, Alternative CRC: ENABLED");
        LogDebug("  [Video-GLideN64] Texture Path: " + txPath);
        LogDebug("  [Video-GLideN64] Window Title: CASTLEVANIA 64 RECOMP");
        LogDebug("  [Video-GLideN64] OSD Font Color: BLACK");
        
        if (s_configSaveSection) {
            s_configSaveSection("Video-GLideN64");
        }
    }
    
    
    return true;
}

/**
 * @brief Apply audio settings from cv64_audio.ini to audio plugin
 * Overwrites matching settings in mupen64plus.cfg
 */
static bool ApplyAudioSettings(void) {
    if (!s_configOpenSection || !s_configSetParameter) {
        return false;
    }
    
    std::filesystem::path iniPath = FindPatchesIni("cv64_audio.ini");
    if (iniPath.empty()) {
        LogDebug("cv64_audio.ini not found, using defaults");
        return false;
    }
    
    CV64_IniParser ini;
    if (!ini.Load(iniPath.string().c_str())) {
        return false;
    }
    
    LogDebug("Overwriting mupen64plus.cfg with cv64_audio.ini settings...");
    
    m64p_handle audioSection = NULL;
    m64p_error ret = s_configOpenSection("Audio-SDL", &audioSection);
    if (ret != M64ERR_SUCCESS) {
        LogDebug("Could not open audio config section");
        return false;
    }
    
    /* [Audio] section */
    int sampleRate = ini.GetInt("Audio", "sample_rate", 48000);
    int bufferSize = ini.GetInt("Audio", "buffer_size", 1024);
    int latencyMs = ini.GetInt("Audio", "latency_ms", 50);
    
    /* [Volume] section - convert 0.0-1.0 to 0-100 */
    float masterVolume = ini.GetFloat("Volume", "master", 1.0f);
    int volume = (int)(masterVolume * 100.0f);
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    /* Apply settings to Audio-SDL section */
    s_configSetParameter(audioSection, "DEFAULT_FREQUENCY", M64TYPE_INT, &sampleRate);
    s_configSetParameter(audioSection, "VOLUME_DEFAULT", M64TYPE_INT, &volume);
    s_configSetParameter(audioSection, "BUFFER_SIZE", M64TYPE_INT, &bufferSize);
    s_configSetParameter(audioSection, "SECONDARY_BUFFER_SIZE", M64TYPE_INT, &latencyMs);
    
    /* Enable swap channels option if needed */
    int swapChannels = 0;
    s_configSetParameter(audioSection, "SWAP_CHANNELS", M64TYPE_BOOL, &swapChannels);
    
    LogDebug("  [Audio-SDL] Sample Rate: " + std::to_string(sampleRate) + "Hz, Volume: " + std::to_string(volume) + "%");
    
    if (s_configSaveSection) {
        s_configSaveSection("Audio-SDL");
    }
    
    return true;
}

/**
 * @brief Apply input settings - ONLY Controller Pak settings
 * 
 * NOTE: We do NOT override input button mappings here!
 * The mupen64plus-input-sdl plugin handles input auto-detection.
 * We ONLY set the controller pak to mempak because Castlevania 64
 * REQUIRES a memory pak to save game progress.
 * 
 * Mempak FILES are created earlier in CreateMempakFilesWithCV64Save() 
 * before CoreStartup so the input plugin can find them.
 */
static bool ApplyInputSettings(void) {
    if (!s_configOpenSection || !s_configSetParameter) {
        LogDebug("WARNING: Config API not available for input settings");
        return false;
    }
    
    LogDebug("Configuring Controller for Castlevania 64...");
    
    /* === Configure controller in Core section === */
    /* The core needs to know controllers are present */
    m64p_handle coreCtrlSection = NULL;
    m64p_error coreRet = s_configOpenSection("Core", &coreCtrlSection);
    if (coreRet == M64ERR_SUCCESS) {
        /* CRITICAL: Ensure ALL controller ports are configured */
        /* Port 1: ENABLED with Memory Pak */
        int controller1Present = 1;
        int controller1Plugin = 2;  /* 2 = Memory Pak */
        s_configSetParameter(coreCtrlSection, "Controller1Present", M64TYPE_BOOL, &controller1Present);
        s_configSetParameter(coreCtrlSection, "Controller1Plugin", M64TYPE_INT, &controller1Plugin);
        LogDebug("  [Core] Controller1Present = TRUE");
        LogDebug("  [Core] Controller1Plugin = 2 (MEMPAK)");
        
        /* Ports 2-4: DISABLED */
        int controllerNotPresent = 0;
        int pluginNone = 1;  /* 1 = None */
        s_configSetParameter(coreCtrlSection, "Controller2Present", M64TYPE_BOOL, &controllerNotPresent);
        s_configSetParameter(coreCtrlSection, "Controller2Plugin", M64TYPE_INT, &pluginNone);
        s_configSetParameter(coreCtrlSection, "Controller3Present", M64TYPE_BOOL, &controllerNotPresent);
        s_configSetParameter(coreCtrlSection, "Controller3Plugin", M64TYPE_INT, &pluginNone);
        s_configSetParameter(coreCtrlSection, "Controller4Present", M64TYPE_BOOL, &controllerNotPresent);
        s_configSetParameter(coreCtrlSection, "Controller4Plugin", M64TYPE_INT, &pluginNone);
        LogDebug("  [Core] Controllers 2-4 = DISABLED");
        
        if (s_configSaveSection) {
            s_configSaveSection("Core");
        }
    } else {
        LogDebug("WARNING: Could not open Core config section for controller settings");
    }
    
    /* === Input-SDL-Control1 Section (for compatibility with external SDL plugin if used) === */
    m64p_handle inputSection = NULL;
    m64p_error ret = s_configOpenSection("Input-SDL-Control1", &inputSection);
    if (ret == M64ERR_SUCCESS) {
        /* 
         * Controller pak plugin values:
         * 1 = None
         * 2 = Mem Pak (REQUIRED for Castlevania 64 saves!)
         * 3 = Rumble Pak
         * 4 = Transfer Pak
         * 5 = Raw
         */
        int mempak = 2;  /* Memory Pak - REQUIRED for CV64! */
        s_configSetParameter(inputSection, "plugin", M64TYPE_INT, &mempak);
        
        /* Make sure controller is plugged in */
        int plugged = 1;
        s_configSetParameter(inputSection, "plugged", M64TYPE_BOOL, &plugged);
        
        /* Set mode to fully automatic so it auto-detects gamepads */
        int mode = 2;  /* 0=manual, 1=auto-named, 2=fully automatic */
        s_configSetParameter(inputSection, "mode", M64TYPE_INT, &mode);
        
        /* Device -1 means keyboard, 0+ means gamepad index */
        int device = -1;  /* Start with keyboard as fallback */
        s_configSetParameter(inputSection, "device", M64TYPE_INT, &device);
        
        LogDebug("  [Input-SDL-Control1] plugin=2 (MemPak), plugged=1, mode=2 (auto)");
        
        if (s_configSaveSection) {
            s_configSaveSection("Input-SDL-Control1");
        }
    } else {
        LogDebug("INFO: Input-SDL-Control1 section not available (using built-in input plugin)");
    }
    
    /* Mempak files are created in CreateMempakFilesWithCV64Save() before CoreStartup */
    
    return true;
}

/**
 * @brief Apply core emulator settings from cv64_patches.ini
 * Overwrites matching settings in mupen64plus.cfg
 */
static bool ApplyCoreSettings(void) {
    if (!s_configOpenSection || !s_configSetParameter) {
        return false;
    }
    
    LogDebug("Applying core emulator settings...");
    
    /* === Core Section === */
    m64p_handle coreSection = NULL;
    m64p_error ret = s_configOpenSection("Core", &coreSection);
    if (ret == M64ERR_SUCCESS) {
        /* R4300 emulator mode (2 = dynamic recompiler for speed) */
        int r4300Mode = 2;
        s_configSetParameter(coreSection, "R4300Emulator", M64TYPE_INT, &r4300Mode);
        
        /* Disable On-Screen Display by default */
        int osdEnabled = 0;
        s_configSetParameter(coreSection, "OnScreenDisplay", M64TYPE_BOOL, &osdEnabled);
        
        /* Set save directory path for mempak/eeprom/sram/flashram saves */
        std::filesystem::path saveDir = GetCoreDirectory() / "save";
        std::filesystem::create_directories(saveDir);
        std::string savePath = saveDir.string();
        s_configSetParameter(coreSection, "SaveSRAMPath", M64TYPE_STRING, savePath.c_str());
        s_configSetParameter(coreSection, "SharedDataPath", M64TYPE_STRING, savePath.c_str());
        
        /* Normal speed (100%) - let vsync handle frame limiting */
        int speedFactor = 100;
        s_configSetParameter(coreSection, "SpeedFactor", M64TYPE_INT, &speedFactor);
        
        LogDebug("  [Core] R4300: DynaRec, SavePath: " + savePath);
        
        if (s_configSaveSection) {
            s_configSaveSection("Core");
        }
    }
    
    /* Try loading from cv64_patches.ini for any overrides */
    std::filesystem::path iniPath = FindPatchesIni("cv64_patches.ini");
    if (!iniPath.empty()) {
        CV64_IniParser ini;
        if (ini.Load(iniPath.string().c_str())) {
            LogDebug("Loaded cv64_patches.ini for additional core settings");
            
            /* Re-open core section for INI overrides */
            if (s_configOpenSection("Core", &coreSection) == M64ERR_SUCCESS) {
                bool framerateUnlock = ini.GetBool("GraphicsPatches", "FRAMERATE_UNLOCK", false);
                if (framerateUnlock) {
                    int speedFactor = 0; /* 0 = unlimited */
                    s_configSetParameter(coreSection, "SpeedFactor", M64TYPE_INT, &speedFactor);
                    LogDebug("  [Core] Framerate unlock enabled (SpeedFactor=0)");
                }
                
                if (s_configSaveSection) {
                    s_configSaveSection("Core");
                }
            }
        }
    }
    
    return true;
}

/**
 * @brief Apply all CV64 INI settings to mupen64plus.cfg
 * This overwrites matching settings and saves before ROM starts
 */
static void ApplyAllINISettings(void) {
    LogDebug("==========================================================");
    LogDebug("  APPLYING CV64 INI SETTINGS -> mupen64plus.cfg");
    LogDebug("==========================================================");
    
    /* Apply settings from each CV64 INI file */
    ApplyCoreSettings();      /* cv64_patches.ini -> Core section */
    ApplyGraphicsSettings();  /* cv64_graphics.ini -> Video sections */
    ApplyAudioSettings();     /* cv64_audio.ini -> Audio-SDL section */
    ApplyInputSettings();     /* cv64_controls.ini -> Input sections */
    
    /* Force save ALL config changes to disk BEFORE plugins load */
    if (s_configSaveFile) {
        m64p_error ret = s_configSaveFile();
        if (ret == M64ERR_SUCCESS) {
            LogDebug("SUCCESS: All CV64 settings written to mupen64plus.cfg");
        } else {
            LogDebug("WARNING: Failed to save mupen64plus.cfg (error code: " + std::to_string(ret) + ")");
        }
    } else {
        LogDebug("WARNING: ConfigSaveFile not available - settings may not persist");
    }
    
    LogDebug("==========================================================");
}

/*===========================================================================
 * API Implementation
 *===========================================================================*/

bool CV64_M64P_Init(void* hwnd) {
    if (s_state != CV64_INTEGRATION_UNINITIALIZED) {
        return true;  /* Already initialized */
    }
    
    s_hwnd = hwnd;
    
    /* Find and load core library */
    std::filesystem::path corePath = FindCoreLib();
    if (corePath.empty()) {
        SetError("Could not find mupen64plus core library.\n\nPlease ensure mupen64plus.dll is in the plugins folder.");
        return false;
    }
    
    LogDebug("Loading core from: " + corePath.string());
    
    /* Add the core's directory to DLL search path so dependencies (SDL2.dll, zlib1.dll) can be found */
    std::filesystem::path coreDirectory = corePath.parent_path();
    SetDllDirectoryW(coreDirectory.c_str());
    LogDebug("Set DLL search directory to: " + coreDirectory.string());
    
    s_coreLib = LoadLibraryW(corePath.c_str());
    
    /* Restore default DLL search path */
    SetDllDirectoryW(NULL);
    
    if (!s_coreLib) {
        DWORD err = GetLastError();
        std::string errMsg = "Failed to load mupen64plus core.\n\nError code: " + std::to_string(err);
        if (err == 126) {
            errMsg += "\n\nThis usually means a dependency DLL is missing.\n";
            errMsg += "Make sure SDL2.dll and zlib1.dll are in:\n" + coreDirectory.string();
        } else if (err == 193) {
            errMsg += "\n\n32/64-bit mismatch. Make sure you're using 64-bit DLLs.";
        }
        SetError(errMsg);
        return false;
    }
    
    LogDebug("Core loaded successfully!");
    
    /* Get core function pointers */
    s_coreStartup = (ptr_CoreStartup)GetProcAddress(s_coreLib, "CoreStartup");
    s_coreShutdown = (ptr_CoreShutdown)GetProcAddress(s_coreLib, "CoreShutdown");
    s_coreDoCommand = (ptr_CoreDoCommand)GetProcAddress(s_coreLib, "CoreDoCommand");
    s_coreAttachPlugin = (ptr_CoreAttachPlugin)GetProcAddress(s_coreLib, "CoreAttachPlugin");
    s_coreDetachPlugin = (ptr_CoreDetachPlugin)GetProcAddress(s_coreLib, "CoreDetachPlugin");
    s_coreErrorMessage = (ptr_CoreErrorMessage)GetProcAddress(s_coreLib, "CoreErrorMessage");
    s_coreOverrideVidExt = (ptr_CoreOverrideVidExt)GetProcAddress(s_coreLib, "CoreOverrideVidExt");
    
    /* Get config API function pointers */
    s_configOpenSection = (ptr_ConfigOpenSection)GetProcAddress(s_coreLib, "ConfigOpenSection");
    s_configSetParameter = (ptr_ConfigSetParameter)GetProcAddress(s_coreLib, "ConfigSetParameter");
    s_configSetDefaultInt = (ptr_ConfigSetDefaultInt)GetProcAddress(s_coreLib, "ConfigSetDefaultInt");
    s_configSetDefaultFloat = (ptr_ConfigSetDefaultFloat)GetProcAddress(s_coreLib, "ConfigSetDefaultFloat");
    s_configSetDefaultBool = (ptr_ConfigSetDefaultBool)GetProcAddress(s_coreLib, "ConfigSetDefaultBool");
    s_configSetDefaultString = (ptr_ConfigSetDefaultString)GetProcAddress(s_coreLib, "ConfigSetDefaultString");
    s_configSaveFile = (ptr_ConfigSaveFile)GetProcAddress(s_coreLib, "ConfigSaveFile");
    s_configSaveSection = (ptr_ConfigSaveSection)GetProcAddress(s_coreLib, "ConfigSaveSection");
    
    if (!s_coreStartup || !s_coreShutdown || !s_coreDoCommand) {
        SetError("Failed to get mupen64plus core functions");
        FreeLibrary(s_coreLib);
        s_coreLib = NULL;
        return false;
    }
    
    /* Initialize the core - config directory is where mempak files need to be! */
    std::filesystem::path configDirPath = GetCoreDirectory();
    std::string configDir = configDirPath.string();
    std::string dataDir = configDir;
    
    /* 
     * IMPORTANT: Create mempak files BEFORE CoreStartup!
     * The input plugin will look for them in the config directory during initialization.
     */
    LogDebug("Pre-creating Memory Pak files in config directory: " + configDir);
    CreateMempakFilesWithCV64Save(configDirPath);
    
    m64p_error ret = s_coreStartup(
        0x020001,           /* Frontend API version */
        configDir.c_str(),  /* Config directory - where mempak files are! */
        dataDir.c_str(),    /* Data directory */
        (void*)"CV64",      /* Debug context */
        DebugCallback,      /* Debug callback */
        NULL,               /* State context */
        StateCallback       /* State callback */
    );
    
    if (ret != M64ERR_SUCCESS) {
        SetError("CoreStartup failed: " + std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
        FreeLibrary(s_coreLib);
        s_coreLib = NULL;
        return false;
    }
    
    LogDebug("Core startup successful");
    
    /* Let GlideN64 create its own separate window for stability */
    /* Video extension embedding was too buggy, so we let the plugin handle it */
    LogDebug("GlideN64 will create its own window");
    
    /* Set the main window title */
    if (s_hwnd) {
        SetWindowTextA((HWND)s_hwnd, "CASTLEVANIA - Press SPACE to start");
    }
    
    /* Apply CV64 INI settings to mupen64plus config BEFORE loading plugins */
    ApplyAllINISettings();
    // Force config to be saved and reloaded before ROM starts
    if (s_configSaveFile) {
        m64p_error ret = s_configSaveFile();
        if (ret == M64ERR_SUCCESS) {
            LogDebug("Config saved before ROM load");
        } else {
            LogDebug("WARNING: Failed to save config before ROM load");
        }
    }
    
    /* Load and initialize all plugins */
    if (!LoadAndStartupAllPlugins()) {
        s_coreShutdown();
        FreeLibrary(s_coreLib);
        s_coreLib = NULL;
        return false;
    }
    
    s_state = CV64_INTEGRATION_CORE_LOADED;
    
    /* Initialize our subsystems */
    CV64_Controller_Init();
    CV64_CameraPatch_Init();
    
    /* Enable camera patch by default for CV64 */
    CV64_Controller_SetDPadCameraMode(true);
    CV64_CameraPatch_SetEnabled(true);
    
    return true;
}

void CV64_M64P_Shutdown(void) {
if (s_state == CV64_INTEGRATION_UNINITIALIZED) {
    return;
}
    
LogDebug("Shutting down CV64 M64P integration...");
    
/* Stop emulation if running */
if (CV64_M64P_IsRunning()) {
    CV64_M64P_Stop();
}
    
/* Make sure emulation thread is finished */
if (s_emulationThread.joinable()) {
    s_emulationThread.join();
}
    
/* Close ROM if loaded */
if (s_romLoaded) {
    CV64_M64P_CloseROM();
}
    
/* Shutdown video extension */
CV64_VidExt_Shutdown();
    
/* Shutdown our subsystems */
CV64_CameraPatch_Shutdown();
CV64_Controller_Shutdown();
    
    /* Detach plugins from core */
    DetachAllPlugins();
    
    /* Shutdown plugins */
    ShutdownAllPlugins();
    
    /* Shutdown core */
    if (s_coreShutdown) {
        s_coreShutdown();
        LogDebug("Core shutdown");
    }
    
    /* Unload plugins */
    if (s_gfxPlugin) { FreeLibrary(s_gfxPlugin); s_gfxPlugin = NULL; }
    if (s_audioPlugin) { FreeLibrary(s_audioPlugin); s_audioPlugin = NULL; }
    /* NOTE: s_inputPlugin points to our own EXE module - do NOT FreeLibrary it! */
    if (s_inputPlugin && s_inputPlugin != GetModuleHandle(NULL)) {
        FreeLibrary(s_inputPlugin);
    }
    s_inputPlugin = NULL;
    if (s_rspPlugin) { FreeLibrary(s_rspPlugin); s_rspPlugin = NULL; }
    
    /* Unload core */
    if (s_coreLib) {
        FreeLibrary(s_coreLib);
        s_coreLib = NULL;
    }
    
    /* Clear function pointers */
    s_coreStartup = NULL;
    s_coreShutdown = NULL;
    s_coreDoCommand = NULL;
    s_coreAttachPlugin = NULL;
    s_coreDetachPlugin = NULL;
    s_coreErrorMessage = NULL;
    
    s_state = CV64_INTEGRATION_UNINITIALIZED;
    LogDebug("CV64 M64P integration shutdown complete");
}

CV64_IntegrationState CV64_M64P_GetState(void) {
    return s_state;
}

const char* CV64_M64P_GetLastError(void) {
    return s_lastError.c_str();
}

void CV64_M64P_ListPlugins(void) {
    std::filesystem::path coreDir = GetCoreDirectory();
    
    LogDebug("=== Plugin Directory Scan ===");
    LogDebug("EXE Directory: " + coreDir.string());
    
    std::vector<std::filesystem::path> searchPaths = {
        coreDir / "plugins",
        coreDir,
        coreDir.parent_path().parent_path() / "plugins",
    };
    
    for (const auto& searchDir : searchPaths) {
        LogDebug("Scanning: " + searchDir.string());
        if (!std::filesystem::exists(searchDir)) {
            LogDebug("  [NOT FOUND]");
            continue;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
            if (entry.path().extension() == ".dll") {
                std::string filename = entry.path().filename().string();
                std::string type = "unknown";
                
                if (filename.find("mupen64plus") != std::string::npos) {
                    if (filename.find("video") != std::string::npos) type = "VIDEO";
                    else if (filename.find("audio") != std::string::npos) type = "AUDIO";
                    else if (filename.find("input") != std::string::npos) type = "INPUT";
                    else if (filename.find("rsp") != std::string::npos) type = "RSP";
                    else type = "CORE";
                } else if (filename.find("SDL") != std::string::npos) {
                    type = "DEPENDENCY";
                } else if (filename.find("zlib") != std::string::npos) {
                    type = "DEPENDENCY";
                }
                
                LogDebug("  [" + type + "] " + filename);
            }
        }
    }
    
    LogDebug("=== Plugin Status ===");
    LogDebug(std::string("  Core: ") + (s_coreLib ? "LOADED" : "NOT LOADED"));
    LogDebug(std::string("  Video Plugin: ") + (s_gfxPlugin ? "LOADED" : "NOT LOADED"));
    LogDebug(std::string("  Audio Plugin: ") + (s_audioPlugin ? "LOADED" : "NOT LOADED"));
    LogDebug(std::string("  Input Plugin: ") + (s_inputPlugin ? "LOADED" : "NOT LOADED"));
    LogDebug(std::string("  RSP Plugin: ") + (s_rspPlugin ? "LOADED" : "NOT LOADED"));
    LogDebug(std::string("  Plugins Attached: ") + (s_pluginsAttached ? "YES" : "NO"));
    LogDebug("=== End Scan ===");
}

bool CV64_M64P_LoadPlugins(void) {

    if (s_state < CV64_INTEGRATION_CORE_LOADED) {
        SetError("Core not initialized - call CV64_M64P_Init first");
        return false;
    }
    
    if (s_gfxPlugin && s_rspPlugin) {
        LogDebug("Plugins already loaded");
        return true;
    }
    
    return LoadAndStartupAllPlugins();
}

/*===========================================================================
 * ROM Loading
 *===========================================================================*/

bool CV64_M64P_LoadROM(const char* rom_path) {
    if (s_state < CV64_INTEGRATION_CORE_LOADED) {
        SetError("Core not initialized");
        return false;
    }
    
    if (!rom_path || !std::filesystem::exists(rom_path)) {
        SetError("ROM file not found: " + std::string(rom_path ? rom_path : "null"));
        return false;
    }
    
    s_romPath = rom_path;
    
    /* Read ROM file */
    FILE* f = fopen(rom_path, "rb");
    if (!f) {
        SetError("Failed to open ROM file");
        return false;
    }
    
    fseek(f, 0, SEEK_END);
    size_t romSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<u8> romData(romSize);
    if (fread(romData.data(), 1, romSize, f) != romSize) {
        fclose(f);
        SetError("Failed to read ROM file");
        return false;
    }
    fclose(f);
    
    /* Open ROM in core */
    m64p_error ret = s_coreDoCommand(M64CMD_ROM_OPEN, (int)romSize, romData.data());
    if (ret != M64ERR_SUCCESS) {
        SetError("Failed to open ROM: " + std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
        return false;
    }
    
    /* Get ROM header */
    ret = s_coreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(s_romHeader), &s_romHeader);
    if (ret != M64ERR_SUCCESS) {
        SetError("Failed to get ROM header");
        s_coreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
        return false;
    }
    
    s_romLoaded = true;
    s_state = CV64_INTEGRATION_ROM_LOADED;
    
    /* Log ROM info */
    char nameBuf[21];
    memcpy(nameBuf, s_romHeader.name, 20);
    nameBuf[20] = '\0';
    
    char msg[256];
    sprintf_s(msg, sizeof(msg), "[CV64] Loaded ROM: %s (CRC: %08X-%08X)\n", nameBuf, s_romHeader.crc1, s_romHeader.crc2);
    OutputDebugStringA(msg);
    
    return true;
}

void CV64_M64P_CloseROM(void) {
    if (!s_romLoaded) {
        return;
    }
    
    s_coreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
    s_romLoaded = false;
    s_state = CV64_INTEGRATION_CORE_LOADED;
}

bool CV64_M64P_GetROMHeader(CV64_RomHeader* header) {
    if (!s_romLoaded || !header) {
        return false;
    }
    *header = s_romHeader;
    return true;
}

bool CV64_M64P_IsCV64ROM(void) {
    if (!s_romLoaded) {
        return false;
    }
    
    /* Check for Castlevania 64 identifiers */
    char name[21];
    memcpy(name, s_romHeader.name, 20);
    name[20] = '\0';
    
    /* Trim trailing spaces */
    for (int i = 19; i >= 0 && name[i] == ' '; i--) {
        name[i] = '\0';
    }
    
    /* Check various versions */
    if (strstr(name, "CASTLEVANIA") != NULL ||
        strstr(name, "AKUMAJOU DRACULA") != NULL ||
        strstr(name, "DRACULA") != NULL) {
        return true;
    }
    
    return false;
}

/*===========================================================================
 * Emulation Control
 *===========================================================================*/

static void EmulationThreadFunc() {
    LogDebug("Emulation thread started");
    s_emulationRunning = true;
    
    m64p_error ret = s_coreDoCommand(M64CMD_EXECUTE, 0, NULL);
    
    s_emulationRunning = false;
    
    if (ret != M64ERR_SUCCESS) {
        LogDebug("Emulation ended with error: " + 
                 std::string(s_coreErrorMessage ? s_coreErrorMessage(ret) : "unknown"));
    } else {
        LogDebug("Emulation ended normally");
    }
    
    /* Update state when emulation ends */
    std::lock_guard<std::mutex> lock(s_stateMutex);
    if (s_state == CV64_INTEGRATION_RUNNING || s_state == CV64_INTEGRATION_PAUSED) {
        s_state = CV64_INTEGRATION_ROM_LOADED;
    }
}

bool CV64_M64P_Start(void) {
    if (s_state < CV64_INTEGRATION_ROM_LOADED) {
        SetError("No ROM loaded");
        return false;
    }
    
    /* Check if emulation thread is already running */
    if (s_emulationRunning) {
        LogDebug("Emulation already running");
        return true;
    }
    
    /* Wait for previous thread to finish if it exists */
    if (s_emulationThread.joinable()) {
        s_emulationThread.join();
    }
    
    /* Attach plugins before starting emulation */
    if (!s_pluginsAttached) {
        if (!AttachAllPlugins()) {
            return false;
        }
    }
    
    LogDebug("Starting emulation on separate thread...");
    
    s_stopRequested = false;
    s_state = CV64_INTEGRATION_RUNNING;
    
    /* Start emulation in a separate thread to keep main window responsive */
    s_emulationThread = std::thread(EmulationThreadFunc);
    
    LogDebug("Emulation thread launched");
    return true;
}

void CV64_M64P_Stop(void) {
    if (!s_emulationRunning && s_state < CV64_INTEGRATION_RUNNING) {
        return;
    }
    
    LogDebug("Stopping emulation...");
    s_stopRequested = true;
    
    /* Send stop command to core */
    s_coreDoCommand(M64CMD_STOP, 0, NULL);
    
    /* Wait for emulation thread to finish (with timeout) */
    if (s_emulationThread.joinable()) {
        s_emulationThread.join();
    }
    
    s_state = CV64_INTEGRATION_ROM_LOADED;
    LogDebug("Emulation stopped");
}

void CV64_M64P_Pause(void) {
    if (s_state != CV64_INTEGRATION_RUNNING) {
        return;
    }
    s_coreDoCommand(M64CMD_PAUSE, 0, NULL);
    s_state = CV64_INTEGRATION_PAUSED;
}

void CV64_M64P_Resume(void) {
    if (s_state != CV64_INTEGRATION_PAUSED) {
        return;
    }
    s_coreDoCommand(M64CMD_RESUME, 0, NULL);
    s_state = CV64_INTEGRATION_RUNNING;
}

void CV64_M64P_Reset(bool hard) {
    s_coreDoCommand(M64CMD_RESET, hard ? 1 : 0, NULL);
}

void CV64_M64P_AdvanceFrame(void) {
    s_coreDoCommand(M64CMD_ADVANCE_FRAME, 0, NULL);
}

bool CV64_M64P_IsRunning(void) {
    return s_emulationRunning || s_state == CV64_INTEGRATION_RUNNING;
}

bool CV64_M64P_IsPaused(void) {
    return s_state == CV64_INTEGRATION_PAUSED;
}

/*===========================================================================
 * Memory Access
 *===========================================================================*/

u8 CV64_M64P_ReadMemory8(u32 address) {
    if (!s_rdram) return 0;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE) {
        return s_rdram[offset ^ 3];  /* XOR for endianness */
    }
    return 0;
}

u16 CV64_M64P_ReadMemory16(u32 address) {
    if (!s_rdram) return 0;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE - 1) {
        return *(u16*)&s_rdram[offset ^ 2];  /* XOR for endianness */
    }
    return 0;
}

u32 CV64_M64P_ReadMemory32(u32 address) {
    if (!s_rdram) return 0;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE - 3) {
        return *(u32*)&s_rdram[offset];
    }
    return 0;
}

void CV64_M64P_WriteMemory8(u32 address, u8 value) {
    if (!s_rdram) return;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE) {
        s_rdram[offset ^ 3] = value;
    }
}

void CV64_M64P_WriteMemory16(u32 address, u16 value) {
    if (!s_rdram) return;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE - 1) {
        *(u16*)&s_rdram[offset ^ 2] = value;
    }
}

void CV64_M64P_WriteMemory32(u32 address, u32 value) {
    if (!s_rdram) return;
    u32 offset = address & 0x003FFFFF;
    if (offset < CV64_ADDR_RDRAM_SIZE - 3) {
        *(u32*)&s_rdram[offset] = value;
    }
}

void* CV64_M64P_GetRDRAMPointer(void) {
    return s_rdram;
}

/*===========================================================================
 * Save States
 *===========================================================================*/

bool CV64_M64P_SaveState(int slot) {
    s_coreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
    m64p_error ret = s_coreDoCommand(M64CMD_STATE_SAVE, 1, NULL);
    return ret == M64ERR_SUCCESS;
}

bool CV64_M64P_LoadState(int slot) {
    s_coreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
    m64p_error ret = s_coreDoCommand(M64CMD_STATE_LOAD, 0, NULL);
    return ret == M64ERR_SUCCESS;
}

void CV64_M64P_SetSaveSlot(int slot) {
    s_coreDoCommand(M64CMD_STATE_SET_SLOT, slot, NULL);
}

/*===========================================================================
 * Speed Control
 *===========================================================================*/

void CV64_M64P_SetSpeedFactor(int factor) {
    int param = M64CORE_SPEED_FACTOR;
    s_coreDoCommand(M64CMD_CORE_STATE_QUERY, param, &factor);
}

int CV64_M64P_GetSpeedFactor(void) {
    int factor = 100;
    int param = M64CORE_SPEED_FACTOR;
    s_coreDoCommand(M64CMD_CORE_STATE_QUERY, param, &factor);
    return factor;
}

void CV64_M64P_SetSpeedLimiter(bool enabled) {
    int param = M64CORE_SPEED_LIMITER;
    int value = enabled ? 1 : 0;
    s_coreDoCommand(M64CMD_CORE_STATE_QUERY, param, &value);
}

/*===========================================================================
 * Frame Callback
 *===========================================================================*/

void CV64_M64P_SetFrameCallback(CV64_FrameCallback callback, void* context) {
    s_frameCallback = callback;
    s_frameCallbackContext = context;
}

/*===========================================================================
 * Controller Override
 *===========================================================================*/

void CV64_M64P_SetControllerOverride(int controller, const CV64_M64P_ControllerState* state) {
    if (controller < 0 || controller >= 4 || !state) {
        return;
    }
    s_controllerOverride[controller] = true;
    s_controllerState[controller] = *state;
}

void CV64_M64P_ClearControllerOverride(int controller) {
    if (controller < 0 || controller >= 4) {
        return;
    }
    s_controllerOverride[controller] = false;
}

#endif /* !CV64_STATIC_MUPEN64PLUS */
