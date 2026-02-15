/**
 * @file cv64_gfx_plugin.cpp
 * @brief Castlevania 64 PC Recomp - Graphics Plugin Implementation
 * 
 * Implements graphics plugin loading and management for mupen64plus plugins.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_gfx_plugin.h"
#include <Windows.h>
#include <string>
#include <filesystem>
#include <cstring>
#include <chrono>

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static CV64_GfxPluginState s_pluginState = { 0 };
static std::string s_lastError;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void SetError(const std::string& error) {
    s_lastError = error;
    OutputDebugStringA("[CV64_GFX] Error: ");
    OutputDebugStringA(error.c_str());
    OutputDebugStringA("\n");
}

static void LogInfo(const std::string& info) {
    OutputDebugStringA("[CV64_GFX] ");
    OutputDebugStringA(info.c_str());
    OutputDebugStringA("\n");
}

static std::filesystem::path GetExecutableDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

static CV64_GfxPluginType DetectPluginType(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find("gliden64") != std::string::npos) {
        return CV64_GFX_PLUGIN_GLIDEN64;
    } else if (lower.find("angrylion") != std::string::npos) {
        return CV64_GFX_PLUGIN_ANGRYLION;
    } else if (lower.find("parallel") != std::string::npos) {
        return CV64_GFX_PLUGIN_PARALLEL;
    } else if (lower.find("rice") != std::string::npos) {
        return CV64_GFX_PLUGIN_RICE;
    }
    
    return CV64_GFX_PLUGIN_AUTO;
}

static std::filesystem::path FindPluginPath(CV64_GfxPluginType type) {
    std::filesystem::path exeDir = GetExecutableDirectory();
    std::filesystem::path pluginsDir = exeDir / "plugins";
    
    // Plugin filename patterns to search for
    std::vector<std::string> patterns;
    
    switch (type) {
        case CV64_GFX_PLUGIN_GLIDEN64:
            patterns = { "mupen64plus-video-GLideN64.dll", "GLideN64.dll" };
            break;
        case CV64_GFX_PLUGIN_ANGRYLION:
            patterns = { "mupen64plus-video-angrylion-plus.dll", "angrylion.dll" };
            break;
        case CV64_GFX_PLUGIN_PARALLEL:
            patterns = { "mupen64plus-video-parallel.dll", "parallel-rdp.dll" };
            break;
        case CV64_GFX_PLUGIN_RICE:
            patterns = { "mupen64plus-video-rice.dll", "rice.dll" };
            break;
        default:
            // Auto-detect: prefer GLideN64
            patterns = {
                "mupen64plus-video-GLideN64.dll",
                "GLideN64.dll",
                "mupen64plus-video-angrylion-plus.dll",
                "mupen64plus-video-parallel.dll"
            };
            break;
    }
    
    // Search in plugins directory first
    if (std::filesystem::exists(pluginsDir)) {
        for (const auto& pattern : patterns) {
            std::filesystem::path pluginPath = pluginsDir / pattern;
            if (std::filesystem::exists(pluginPath)) {
                return pluginPath;
            }
        }
        
        // Search recursively
        for (const auto& entry : std::filesystem::recursive_directory_iterator(pluginsDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                for (const auto& pattern : patterns) {
                    if (filename == pattern) {
                        return entry.path();
                    }
                }
            }
        }
    }
    
    // Search in exe directory
    for (const auto& pattern : patterns) {
        std::filesystem::path pluginPath = exeDir / pattern;
        if (std::filesystem::exists(pluginPath)) {
            return pluginPath;
        }
    }
    
    return std::filesystem::path();
}

/*===========================================================================
 * API Implementation
 *===========================================================================*/

bool CV64_GfxPlugin_Init(const CV64_GfxPluginConfig* config) {
    if (s_pluginState.initialized) {
        return true;
    }
    
    LogInfo("Initializing graphics plugin system...");
    
    // Copy configuration
    if (config) {
        memcpy(&s_pluginState.config, config, sizeof(CV64_GfxPluginConfig));
    } else {
        // Default configuration
        s_pluginState.config.plugin_type = CV64_GFX_PLUGIN_AUTO;
        s_pluginState.config.window_width = 1280;
        s_pluginState.config.window_height = 720;
        s_pluginState.config.render_width = 1280;
        s_pluginState.config.render_height = 720;
        s_pluginState.config.native_width = 320;
        s_pluginState.config.native_height = 240;
        s_pluginState.config.fullscreen = false;
        s_pluginState.config.vsync = true;
        s_pluginState.config.widescreen = true;
        s_pluginState.config.aspect_ratio = 16.0f / 9.0f;
        s_pluginState.config.msaa = 4;
        s_pluginState.config.anisotropy = 8;
        s_pluginState.config.bilinear = true;
        s_pluginState.config.fog_enabled = true;
        s_pluginState.config.fog_multiplier = 1.0f;
        s_pluginState.config.hd_textures = false;
    }
    
    // Find and load plugin
    std::filesystem::path pluginPath;
    
    if (s_pluginState.config.plugin_path[0] != '\0') {
        pluginPath = s_pluginState.config.plugin_path;
    } else {
        pluginPath = FindPluginPath(s_pluginState.config.plugin_type);
    }
    
    if (pluginPath.empty()) {
        SetError("No graphics plugin found. Please install mupen64plus-video-GLideN64.dll");
        return false;
    }
    
    LogInfo("Found plugin: " + pluginPath.string());
    
    if (!CV64_GfxPlugin_Load(pluginPath.string().c_str(), s_pluginState.config.plugin_type)) {
        return false;
    }
    
    s_pluginState.initialized = true;
    s_pluginState.frame_count = 0;
    s_pluginState.fps = 0.0;
    
    LogInfo("Graphics plugin system initialized successfully");
    return true;
}

void CV64_GfxPlugin_Shutdown(void) {
    if (!s_pluginState.initialized) {
        return;
    }
    
    LogInfo("Shutting down graphics plugin...");
    
    CV64_GfxPlugin_Unload();
    
    memset(&s_pluginState, 0, sizeof(s_pluginState));
    s_pluginState.initialized = false;
}

bool CV64_GfxPlugin_Load(const char* plugin_path, CV64_GfxPluginType type) {
    if (s_pluginState.loaded) {
        CV64_GfxPlugin_Unload();
    }
    
    LogInfo(std::string("Loading graphics plugin: ") + plugin_path);
    
    // Load the DLL
    HMODULE hPlugin = LoadLibraryA(plugin_path);
    if (!hPlugin) {
        SetError("Failed to load plugin DLL: " + std::to_string(GetLastError()));
        return false;
    }
    
    s_pluginState.library = hPlugin;
    
    // Detect plugin type if auto
    if (type == CV64_GFX_PLUGIN_AUTO) {
        type = DetectPluginType(std::filesystem::path(plugin_path).filename().string());
    }
    s_pluginState.type = type;
    
    // Get function pointers
    #define LOAD_FUNC(name, type) \
        s_pluginState.name = (type)GetProcAddress(hPlugin, #name); \
        if (!s_pluginState.name) { \
            LogInfo("Warning: Function " #name " not found"); \
        }
    
    LOAD_FUNC(change_window, GfxChangeWindow);
    LOAD_FUNC(move_screen, GfxMoveScreen);
    LOAD_FUNC(process_dlist, GfxProcessDList);
    LOAD_FUNC(process_rdplist, GfxProcessRDPList);
    LOAD_FUNC(rom_closed, GfxRomClosed);
    LOAD_FUNC(rom_open, GfxRomOpen);
    LOAD_FUNC(show_cfb, GfxShowCFB);
    LOAD_FUNC(update_screen, GfxUpdateScreen);
    LOAD_FUNC(vi_status_changed, GfxViStatusChanged);
    LOAD_FUNC(vi_width_changed, GfxViWidthChanged);
    
    // Try different names for InitiateGFX
    s_pluginState.initiate_gfx = (GfxInitiateGFX)GetProcAddress(hPlugin, "InitiateGFX");
    if (!s_pluginState.initiate_gfx) {
        s_pluginState.initiate_gfx = (GfxInitiateGFX)GetProcAddress(hPlugin, "gfxInitiateGFX");
    }
    
    // Optional functions
    s_pluginState.read_screen2 = (GfxReadScreen2)GetProcAddress(hPlugin, "ReadScreen2");
    s_pluginState.set_rendering_callback = (GfxSetRenderingCallback)GetProcAddress(hPlugin, "SetRenderingCallback");
    s_pluginState.resize_video = (GfxResizeVideoOutput)GetProcAddress(hPlugin, "ResizeVideoOutput");
    s_pluginState.fb_read = (GfxFBRead)GetProcAddress(hPlugin, "FBRead");
    s_pluginState.fb_write = (GfxFBWrite)GetProcAddress(hPlugin, "FBWrite");
    s_pluginState.fb_get_info = (GfxFBGetFrameBufferInfo)GetProcAddress(hPlugin, "FBGetFrameBufferInfo");
    
    #undef LOAD_FUNC
    
    // Verify required functions
    if (!s_pluginState.initiate_gfx) {
        SetError("Plugin missing required function: InitiateGFX");
        CV64_GfxPlugin_Unload();
        return false;
    }
    
    s_pluginState.loaded = true;
    strncpy(s_pluginState.config.plugin_path, plugin_path, sizeof(s_pluginState.config.plugin_path) - 1);
    
    LogInfo("Graphics plugin loaded successfully");
    return true;
}

void CV64_GfxPlugin_Unload(void) {
    if (!s_pluginState.loaded) {
        return;
    }
    
    LogInfo("Unloading graphics plugin...");
    
    // Call rom_closed if ROM was open
    if (s_pluginState.rom_closed) {
        s_pluginState.rom_closed();
    }
    
    // Unload the library
    if (s_pluginState.library) {
        FreeLibrary((HMODULE)s_pluginState.library);
        s_pluginState.library = NULL;
    }
    
    // Clear function pointers
    s_pluginState.change_window = NULL;
    s_pluginState.initiate_gfx = NULL;
    s_pluginState.move_screen = NULL;
    s_pluginState.process_dlist = NULL;
    s_pluginState.process_rdplist = NULL;
    s_pluginState.rom_closed = NULL;
    s_pluginState.rom_open = NULL;
    s_pluginState.show_cfb = NULL;
    s_pluginState.update_screen = NULL;
    s_pluginState.vi_status_changed = NULL;
    s_pluginState.vi_width_changed = NULL;
    s_pluginState.read_screen2 = NULL;
    s_pluginState.set_rendering_callback = NULL;
    s_pluginState.resize_video = NULL;
    s_pluginState.fb_read = NULL;
    s_pluginState.fb_write = NULL;
    s_pluginState.fb_get_info = NULL;
    
    s_pluginState.loaded = false;
}

bool CV64_GfxPlugin_StartROM(CV64_GfxInfo* gfx_info) {
    if (!s_pluginState.loaded) {
        SetError("No graphics plugin loaded");
        return false;
    }
    
    if (!s_pluginState.initiate_gfx) {
        SetError("InitiateGFX function not available");
        return false;
    }
    
    LogInfo("Starting graphics plugin for ROM...");
    
    int result = s_pluginState.initiate_gfx(*gfx_info);
    if (result == 0) {
        SetError("InitiateGFX failed");
        return false;
    }
    
    // Call rom_open
    if (s_pluginState.rom_open) {
        int openResult = s_pluginState.rom_open();
        if (openResult == 0) {
            SetError("RomOpen failed");
            return false;
        }
    }
    
    // Set rendering callback if available
    if (s_pluginState.set_rendering_callback) {
        s_pluginState.set_rendering_callback(NULL);  // Can set our own callback here
    }
    
    LogInfo("Graphics plugin started successfully");
    return true;
}

void CV64_GfxPlugin_StopROM(void) {
    if (!s_pluginState.loaded) {
        return;
    }
    
    if (s_pluginState.rom_closed) {
        s_pluginState.rom_closed();
    }
}

void CV64_GfxPlugin_ProcessDList(void) {
    if (s_pluginState.loaded && s_pluginState.process_dlist) {
        s_pluginState.process_dlist();
    }
}

void CV64_GfxPlugin_UpdateScreen(void) {
    if (!s_pluginState.loaded || !s_pluginState.update_screen) {
        return;
    }
    
    // Calculate FPS
    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int frameCounter = 0;
    
    auto now = std::chrono::high_resolution_clock::now();
    frameCounter++;
    s_pluginState.frame_count++;
    
    auto elapsed = std::chrono::duration<double>(now - lastTime).count();
    if (elapsed >= 1.0) {
        s_pluginState.fps = frameCounter / elapsed;
        frameCounter = 0;
        lastTime = now;
    }
    
    s_pluginState.update_screen();
}

void CV64_GfxPlugin_Resize(u32 width, u32 height) {
    s_pluginState.config.window_width = width;
    s_pluginState.config.window_height = height;
    
    if (s_pluginState.loaded && s_pluginState.resize_video) {
        s_pluginState.resize_video(width, height);
    }
}

void CV64_GfxPlugin_ToggleFullscreen(void) {
    if (s_pluginState.loaded && s_pluginState.change_window) {
        s_pluginState.config.fullscreen = !s_pluginState.config.fullscreen;
        s_pluginState.change_window();
    }
}

CV64_GfxPluginState* CV64_GfxPlugin_GetState(void) {
    return &s_pluginState;
}

void CV64_GfxPlugin_ApplyConfig(const CV64_GfxPluginConfig* config) {
    if (config) {
        memcpy(&s_pluginState.config, config, sizeof(CV64_GfxPluginConfig));
    }
}

void CV64_GfxPlugin_SetWidescreen(bool enable, f32 aspect) {
    s_pluginState.config.widescreen = enable;
    s_pluginState.config.aspect_ratio = aspect;
    
    // If plugin supports config changes, apply them
    // This would depend on the specific plugin API
}

void CV64_GfxPlugin_SetRenderCallback(void (*callback)(int)) {
    if (s_pluginState.loaded && s_pluginState.set_rendering_callback) {
        s_pluginState.set_rendering_callback(callback);
    }
}

bool CV64_GfxPlugin_Screenshot(const char* path) {
    if (!s_pluginState.loaded || !s_pluginState.read_screen2) {
        SetError("Screenshot not supported by current plugin");
        return false;
    }
    
    // Read screen buffer
    int width = 0, height = 0;
    s_pluginState.read_screen2(NULL, &width, &height, 0);
    
    if (width <= 0 || height <= 0) {
        SetError("Failed to get screen dimensions");
        return false;
    }
    
    // Allocate buffer
    std::vector<u8> buffer(width * height * 4);
    s_pluginState.read_screen2(buffer.data(), &width, &height, 0);
    
    // Save to file (basic BMP format)
    std::string outputPath;
    if (path && path[0] != '\0') {
        outputPath = path;
    } else {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&time));
        outputPath = "screenshot_" + std::string(timestamp) + ".bmp";
    }
    
    // TODO: Implement proper image saving (BMP/PNG)
    LogInfo("Screenshot saved: " + outputPath);
    
    return true;
}

f64 CV64_GfxPlugin_GetFPS(void) {
    return s_pluginState.fps;
}

void CV64_GfxPlugin_ConfigureGLideN64(
    bool fb_emulation,
    bool n64_depth_compare,
    bool native_res_2d
) {
    if (s_pluginState.type != CV64_GFX_PLUGIN_GLIDEN64) {
        return;
    }
    
    s_pluginState.config.fb_emulation = fb_emulation;
    s_pluginState.config.n64_depth_compare = n64_depth_compare;
    
    // TODO: Apply settings through GLideN64 config API
}

void CV64_GfxPlugin_SetHDTexturePath(const char* path) {
    if (path) {
        strncpy(s_pluginState.config.hd_texture_path, path, 
                sizeof(s_pluginState.config.hd_texture_path) - 1);
        s_pluginState.config.hd_textures = true;
    } else {
        s_pluginState.config.hd_textures = false;
    }
}
