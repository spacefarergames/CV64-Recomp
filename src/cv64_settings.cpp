/**
 * @file cv64_settings.cpp
 * @brief Castlevania 64 PC - Settings Management Implementation
 * 
 * Handles loading/saving settings from INI files and providing
 * a user-friendly settings dialog.
 */

#include "../include/cv64_settings.h"
#include "../include/cv64_config_bridge.h"
#include "../include/cv64_memory_hook.h"
#include "../include/cv64_anim_interp.h"
#include "../include/cv64_anim_bridge.h"
#include "../CV64_RMG.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

// External reference to main application instance
extern HINSTANCE g_hInst;

//=============================================================================
// Internal State
//=============================================================================

static CV64_AllSettings g_settings;
static std::string g_patchesPath;
static bool g_initialized = false;

//=============================================================================
// INI Helper Functions
//=============================================================================

static std::string GetExeDirectory() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

static std::string TrimString(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::map<std::string, std::map<std::string, std::string>> ParseINI(const std::string& filepath) {
    std::map<std::string, std::map<std::string, std::string>> result;
    std::ifstream file(filepath);
    if (!file.is_open()) return result;
    
    std::string currentSection;
    std::string line;
    
    while (std::getline(file, line)) {
        line = TrimString(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = TrimString(line.substr(0, eqPos));
            std::string value = TrimString(line.substr(eqPos + 1));
            result[currentSection][key] = value;
        }
    }
    
    return result;
}

static bool WriteINI(const std::string& filepath, 
                     const std::map<std::string, std::map<std::string, std::string>>& data,
                     const std::string& header = "") {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    if (!header.empty()) {
        file << header << "\n\n";
    }
    
    for (const auto& section : data) {
        file << "[" << section.first << "]\n";
        for (const auto& kv : section.second) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }
    
    return true;
}

static std::string GetString(const std::map<std::string, std::map<std::string, std::string>>& ini,
                             const std::string& section, const std::string& key, const std::string& def) {
    auto sit = ini.find(section);
    if (sit == ini.end()) return def;
    auto kit = sit->second.find(key);
    if (kit == sit->second.end()) return def;
    return kit->second;
}

static int GetInt(const std::map<std::string, std::map<std::string, std::string>>& ini,
                  const std::string& section, const std::string& key, int def) {
    std::string val = GetString(ini, section, key, "");
    if (val.empty()) return def;
    try { return std::stoi(val); } catch (...) { return def; }
}

static float GetFloat(const std::map<std::string, std::map<std::string, std::string>>& ini,
                      const std::string& section, const std::string& key, float def) {
    std::string val = GetString(ini, section, key, "");
    if (val.empty()) return def;
    try { return std::stof(val); } catch (...) { return def; }
}

static bool GetBool(const std::map<std::string, std::map<std::string, std::string>>& ini,
                    const std::string& section, const std::string& key, bool def) {
    std::string val = GetString(ini, section, key, "");
    if (val.empty()) return def;
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return (val == "true" || val == "1" || val == "yes");
}

//=============================================================================
// Settings API Implementation
//=============================================================================

bool CV64_Settings_Init(void) {
    if (g_initialized) return true;
    
    g_patchesPath = GetExeDirectory() + "patches\\";
    
    // Create patches folder if it doesn't exist
    std::filesystem::create_directories(g_patchesPath);
    
    // Load settings
    CV64_Settings_LoadAll();
    
    // Cheats are disabled
    OutputDebugStringA("[CV64] Settings loaded (cheats disabled)\n");
    
    g_initialized = true;
    return true;
}

void CV64_Settings_Shutdown(void) {
    g_initialized = false;
}

void CV64_Settings_ResetToDefaults(void) {
    // Graphics defaults
    g_settings.graphics.resolution = "720P";
    g_settings.graphics.customWidth = 1280;
    g_settings.graphics.customHeight = 720;
    g_settings.graphics.fullscreen = false;
    g_settings.graphics.borderless = false;
    g_settings.graphics.internalScale = 2;
    g_settings.graphics.textureFilter = "LINEAR";
    g_settings.graphics.antiAliasing = "FXAA";
    g_settings.graphics.vsync = true;
    g_settings.graphics.maxFps = 60;
    g_settings.graphics.aspectRatio = "16_9";
    g_settings.graphics.stretchToFill = true;
    g_settings.graphics.enableHDTextures = true;

    // Audio defaults
    g_settings.audio.sampleRate = 48000;
    g_settings.audio.latencyMs = 50;
    g_settings.audio.masterVolume = 100;
    g_settings.audio.musicVolume = 100;
    g_settings.audio.sfxVolume = 100;
    g_settings.audio.enableReverb = false;
    g_settings.audio.enableSurround = false;
    
    // Camera defaults
    g_settings.camera.enableDpad = true;
    g_settings.camera.enableRightStick = true;
    g_settings.camera.enableMouse = true;
    g_settings.camera.dpadSpeed = 2.0f;
    g_settings.camera.stickSensitivityX = 1.5f;
    g_settings.camera.stickSensitivityY = 1.5f;
    g_settings.camera.mouseSensitivityX = 0.15f;
    g_settings.camera.mouseSensitivityY = 0.15f;
    g_settings.camera.invertX = false;
    g_settings.camera.invertY = false;
    g_settings.camera.enableSmoothing = true;
    
    // Controls defaults
    g_settings.controls.leftStickDeadzone = 0.15f;
    g_settings.controls.rightStickDeadzone = 0.15f;
    g_settings.controls.mouseSensX = 0.15f;
    g_settings.controls.mouseSensY = 0.15f;
    g_settings.controls.mouseInvertY = false;
    
    // Patches defaults
    g_settings.patches.dpadCamera = true;
    g_settings.patches.smoothFirstPerson = true;
    g_settings.patches.modernControls = true;
    g_settings.patches.keyboardMouse = true;
    g_settings.patches.widescreen = true;
    g_settings.patches.framerateUnlock = true;
    g_settings.patches.skipIntro = false;
    g_settings.patches.fastText = false;
    g_settings.patches.instantMap = true;
    g_settings.patches.fixSoftlocks = true;
    g_settings.patches.fixCollision = true;
    g_settings.patches.fixCameraClip = true;
    
    // Threading defaults (ON by default for better performance)
    g_settings.threading.enableAsyncGraphics = true;
    g_settings.threading.enableAsyncAudio = true;
    g_settings.threading.enableWorkerThreads = true;
    g_settings.threading.workerThreadCount = 0;  // Auto-detect
    g_settings.threading.graphicsQueueDepth = 2; // Double buffering
    g_settings.threading.enableParallelRSP = false; // EXPERIMENTAL - keep off
    g_settings.threading.enablePerfOverlay = false; // OFF by default
    g_settings.threading.perfOverlayMode = 0; // OFF
    
    // Post Processing defaults (ALL ON by default for enhanced graphics!)
    // These map to ReShade FX effects in postprocessing_preset.ini
    g_settings.postProcessing.enableReflections = true;        // Barbatos_SSR_Lite.fx
    g_settings.postProcessing.enableReflectiveMaterials = true;// Chromaticity.fx
    g_settings.postProcessing.enableColorGrading = true;       // DPX.fx
    g_settings.postProcessing.enableFilmGrain = true;          // FilmGrain.fx
    g_settings.postProcessing.enableDynamicShadows = true;     // MiAO.fx
    g_settings.postProcessing.enableEnhancedLighting = true;   // TrackingRays.fx
    g_settings.postProcessing.enableVignette = true;           // Vignette.fx
    
    // Cheats defaults - Camera patch ON, performance cheats ON, gameplay cheats OFF
    g_settings.cheats.enableCameraPatch = true;       // THE MAIN FEATURE!
    g_settings.cheats.enableLagReduction = true;      // Fix slowdowns with many enemies
    g_settings.cheats.enableForestFix = false;        // DISABLED — overrides fog in Forest of Silence
    g_settings.cheats.enableInfiniteHealth = false;
    g_settings.cheats.enableInfiniteSubweapon = false;
    g_settings.cheats.enableInfiniteGold = false;
    g_settings.cheats.enableMoonJump = false;
    g_settings.cheats.enableNoClip = false;
    g_settings.cheats.enableSpeedBoost = false;
}

bool CV64_Settings_LoadAll(void) {
    CV64_Settings_ResetToDefaults();
    
    // Load Graphics
    auto gfxIni = ParseINI(g_patchesPath + "cv64_graphics.ini");
    g_settings.graphics.resolution = GetString(gfxIni, "Display", "resolution", "720P");
    g_settings.graphics.customWidth = GetInt(gfxIni, "Display", "custom_width", 1280);
    g_settings.graphics.customHeight = GetInt(gfxIni, "Display", "custom_height", 720);
    g_settings.graphics.fullscreen = GetBool(gfxIni, "Display", "fullscreen", false);
    g_settings.graphics.borderless = GetBool(gfxIni, "Display", "borderless", false);
    g_settings.graphics.internalScale = GetInt(gfxIni, "Quality", "internal_scale", 2);
    g_settings.graphics.textureFilter = GetString(gfxIni, "Quality", "texture_filter", "LINEAR");
    g_settings.graphics.vsync = GetBool(gfxIni, "Quality", "vsync", true);
    g_settings.graphics.maxFps = GetInt(gfxIni, "Quality", "max_fps", 60);
    g_settings.graphics.aspectRatio = GetString(gfxIni, "AspectRatio", "ratio", "16_9");
    g_settings.graphics.stretchToFill = GetBool(gfxIni, "AspectRatio", "stretch", true);
    g_settings.graphics.enableHDTextures = GetBool(gfxIni, "Enhancements", "enable_hd_textures", true);
    std::string aa = GetString(gfxIni, "Quality", "anti_aliasing", "FXAA");
    bool fxaa = GetBool(gfxIni, "Quality", "fxaa", true);
    g_settings.graphics.antiAliasing = fxaa ? "FXAA" : "NONE";
    
    // Load Audio
    auto audioIni = ParseINI(g_patchesPath + "cv64_audio.ini");
    g_settings.audio.sampleRate = GetInt(audioIni, "Audio", "sample_rate", 48000);
    g_settings.audio.latencyMs = GetInt(audioIni, "Audio", "latency_ms", 50);
    g_settings.audio.masterVolume = (int)(GetFloat(audioIni, "Volume", "master", 1.0f) * 100);
    g_settings.audio.musicVolume = (int)(GetFloat(audioIni, "Volume", "music", 1.0f) * 100);
    g_settings.audio.sfxVolume = (int)(GetFloat(audioIni, "Volume", "sfx", 1.0f) * 100);
    g_settings.audio.enableReverb = GetBool(audioIni, "Enhancements", "enable_reverb", false);
    g_settings.audio.enableSurround = GetBool(audioIni, "Enhancements", "enable_surround", false);
    
    // Load Camera
    auto camIni = ParseINI(g_patchesPath + "cv64_camera.ini");
    g_settings.camera.enableDpad = GetBool(camIni, "Camera", "enable_dpad", true);
    g_settings.camera.enableRightStick = GetBool(camIni, "Camera", "enable_right_stick", true);
    g_settings.camera.enableMouse = GetBool(camIni, "Camera", "enable_mouse", true);
    g_settings.camera.dpadSpeed = GetFloat(camIni, "Sensitivity", "dpad_rotation_speed", 2.0f);
    g_settings.camera.stickSensitivityX = GetFloat(camIni, "Sensitivity", "stick_sensitivity_x", 1.5f);
    g_settings.camera.stickSensitivityY = GetFloat(camIni, "Sensitivity", "stick_sensitivity_y", 1.5f);
    g_settings.camera.mouseSensitivityX = GetFloat(camIni, "Sensitivity", "mouse_sensitivity_x", 0.15f);
    g_settings.camera.mouseSensitivityY = GetFloat(camIni, "Sensitivity", "mouse_sensitivity_y", 0.15f);
    g_settings.camera.invertX = GetBool(camIni, "Behavior", "invert_x", false);
    g_settings.camera.invertY = GetBool(camIni, "Behavior", "invert_y", false);
    g_settings.camera.enableSmoothing = GetBool(camIni, "Behavior", "enable_smoothing", true);
    
    // Load Controls
    auto ctrlIni = ParseINI(g_patchesPath + "cv64_controls.ini");
    g_settings.controls.leftStickDeadzone = GetFloat(ctrlIni, "Gamepad", "left_stick_deadzone", 0.15f);
    g_settings.controls.rightStickDeadzone = GetFloat(ctrlIni, "Gamepad", "right_stick_deadzone", 0.15f);
    g_settings.controls.mouseSensX = GetFloat(ctrlIni, "Mouse", "sensitivity_x", 0.15f);
    g_settings.controls.mouseSensY = GetFloat(ctrlIni, "Mouse", "sensitivity_y", 0.15f);
    g_settings.controls.mouseInvertY = GetBool(ctrlIni, "Mouse", "invert_y", false);
    
    // Load Patches
    auto patchIni = ParseINI(g_patchesPath + "cv64_patches.ini");
    g_settings.patches.dpadCamera = GetBool(patchIni, "CameraPatches", "DPAD_CAMERA", true);
    g_settings.patches.smoothFirstPerson = GetBool(patchIni, "CameraPatches", "FIRST_PERSON_SMOOTH", true);
    g_settings.patches.modernControls = GetBool(patchIni, "ControlPatches", "MODERN_CONTROLS", true);
    g_settings.patches.keyboardMouse = GetBool(patchIni, "ControlPatches", "KEYBOARD_MOUSE", true);
    g_settings.patches.widescreen = GetBool(patchIni, "GraphicsPatches", "WIDESCREEN", true);
    g_settings.patches.framerateUnlock = GetBool(patchIni, "GraphicsPatches", "FRAMERATE_UNLOCK", true);
    g_settings.patches.animInterpolation = GetBool(patchIni, "GraphicsPatches", "ANIM_INTERPOLATION", true);
    g_settings.patches.skipIntro = GetBool(patchIni, "GameplayPatches", "SKIP_INTRO", false);
    g_settings.patches.fastText = GetBool(patchIni, "GameplayPatches", "FAST_TEXT", false);
    g_settings.patches.instantMap = GetBool(patchIni, "GameplayPatches", "INSTANT_MAP", true);
    g_settings.patches.fixSoftlocks = GetBool(patchIni, "BugFixes", "FIX_SOFTLOCKS", true);
    g_settings.patches.fixCollision = GetBool(patchIni, "BugFixes", "FIX_COLLISION", true);
    g_settings.patches.fixCameraClip = GetBool(patchIni, "BugFixes", "FIX_CAMERA_CLIP", true);
    
    // Load Threading settings
    auto threadIni = ParseINI(g_patchesPath + "cv64_threading.ini");
    g_settings.threading.enableAsyncGraphics = GetBool(threadIni, "Threading", "enable_async_graphics", true);
    g_settings.threading.enableAsyncAudio = GetBool(threadIni, "Threading", "enable_async_audio", true);
    g_settings.threading.enableWorkerThreads = GetBool(threadIni, "Threading", "enable_worker_threads", true);
    g_settings.threading.workerThreadCount = GetInt(threadIni, "Threading", "worker_thread_count", 0);
    g_settings.threading.graphicsQueueDepth = GetInt(threadIni, "Threading", "graphics_queue_depth", 2);
    g_settings.threading.enableParallelRSP = GetBool(threadIni, "Threading", "enable_parallel_rsp", false);
    g_settings.threading.enablePerfOverlay = GetBool(threadIni, "Performance", "enable_overlay", false);
    g_settings.threading.perfOverlayMode = GetInt(threadIni, "Performance", "overlay_mode", 0);
    
    // Load Post Processing settings from postprocessing_preset.ini in patches folder
    // Parse the Techniques line to determine which effects are enabled
    {
        std::string presetPath = g_patchesPath + "postprocessing_preset.ini";
        std::ifstream file(presetPath);
        
        // Default all to true (will be set based on Techniques line)
        g_settings.postProcessing.enableReflections = true;
        g_settings.postProcessing.enableReflectiveMaterials = true;
        g_settings.postProcessing.enableColorGrading = true;
        g_settings.postProcessing.enableFilmGrain = true;
        g_settings.postProcessing.enableDynamicShadows = true;
        g_settings.postProcessing.enableEnhancedLighting = true;
        g_settings.postProcessing.enableVignette = true;
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                // Look for the Techniques= line
                if (line.find("Techniques=") == 0) {
                    std::string techniques = line.substr(11); // Skip "Techniques="
                    
                    // Check which effects are in the Techniques line
                    g_settings.postProcessing.enableReflections = (techniques.find("Barbatos_SSR_Lite@") != std::string::npos);
                    g_settings.postProcessing.enableReflectiveMaterials = (techniques.find("Chromaticity@") != std::string::npos);
                    g_settings.postProcessing.enableColorGrading = (techniques.find("DPX@") != std::string::npos);
                    g_settings.postProcessing.enableFilmGrain = (techniques.find("FilmGrain@") != std::string::npos);
                    g_settings.postProcessing.enableDynamicShadows = (techniques.find("MiAO@") != std::string::npos);
                    g_settings.postProcessing.enableEnhancedLighting = (techniques.find("TrackingRays@") != std::string::npos);
                    g_settings.postProcessing.enableVignette = (techniques.find("Vignette@") != std::string::npos);
                    break;
                }
            }
            file.close();
        }
        // If file doesn't exist or can't be read, defaults (all true) will be used
        // and SaveAll will create the file with the default configuration
    }
    
    // Load Cheats settings
    auto cheatIni = ParseINI(g_patchesPath + "cv64_cheats.ini");
    g_settings.cheats.enableCameraPatch = GetBool(cheatIni, "Features", "camera_patch", true);
    g_settings.cheats.enableLagReduction = GetBool(cheatIni, "Performance", "lag_reduction", true);
    g_settings.cheats.enableForestFix = GetBool(cheatIni, "Performance", "forest_fps_fix", false);
    
    g_settings.cheats.enableInfiniteHealth = GetBool(cheatIni, "Gameplay", "infinite_health", false);
    g_settings.cheats.enableInfiniteSubweapon = GetBool(cheatIni, "Gameplay", "infinite_subweapon", false);
    g_settings.cheats.enableInfiniteGold = GetBool(cheatIni, "Gameplay", "infinite_gold", false);
    g_settings.cheats.enableMoonJump = GetBool(cheatIni, "Gameplay", "moon_jump", false);
    
    g_settings.cheats.enableNoClip = GetBool(cheatIni, "Debug", "no_clip", false);
    g_settings.cheats.enableSpeedBoost = GetBool(cheatIni, "Debug", "speed_boost", false);
    
    return true;
}

bool CV64_Settings_SaveAll(void) {
    // Save Graphics
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        ini["Display"]["resolution"] = g_settings.graphics.resolution;
        ini["Display"]["custom_width"] = std::to_string(g_settings.graphics.customWidth);
        ini["Display"]["custom_height"] = std::to_string(g_settings.graphics.customHeight);
        ini["Display"]["fullscreen"] = g_settings.graphics.fullscreen ? "true" : "false";
        ini["Display"]["borderless"] = g_settings.graphics.borderless ? "true" : "false";
        ini["Display"]["backend"] = "AUTO";
        ini["Display"]["refresh_rate"] = "60";
        
        ini["AspectRatio"]["ratio"] = g_settings.graphics.aspectRatio;
        ini["AspectRatio"]["stretch"] = g_settings.graphics.stretchToFill ? "true" : "false";
        /* Always use stretch mode (0) to prevent black bars - CASTLEVANIA should fill the window */
        ini["AspectRatio"]["aspect_mode"] = "0";
        
        ini["Quality"]["internal_scale"] = std::to_string(g_settings.graphics.internalScale);
        ini["Quality"]["native_res_factor"] = std::to_string(g_settings.graphics.internalScale);
        ini["Quality"]["texture_filter"] = g_settings.graphics.textureFilter;
        ini["Quality"]["bilinear_mode"] = (g_settings.graphics.textureFilter == "LINEAR") ? "1" : "0";
        ini["Quality"]["anisotropy"] = "0";
        ini["Quality"]["anti_aliasing"] = g_settings.graphics.antiAliasing;
        ini["Quality"]["multisampling"] = "0";
        ini["Quality"]["fxaa"] = (g_settings.graphics.antiAliasing == "FXAA") ? "true" : "false";
        ini["Quality"]["vsync"] = g_settings.graphics.vsync ? "true" : "false";
        ini["Quality"]["max_fps"] = std::to_string(g_settings.graphics.maxFps);
        
        ini["FrameBuffer"]["enable_fb_emulation"] = "true";
        ini["FrameBuffer"]["buffer_swap_mode"] = "0";
        ini["FrameBuffer"]["copy_color_to_rdram"] = "2";   // Double buffer (async) for performance
        ini["FrameBuffer"]["copy_depth_to_rdram"] = "0";   // Disabled for performance
        ini["FrameBuffer"]["copy_from_rdram"] = "false";
        ini["FrameBuffer"]["copy_aux_to_rdram"] = "false";
        ini["FrameBuffer"]["n64_depth_compare"] = "0";     // Disabled (CV64 doesn't use HW depth compare)
        ini["FrameBuffer"]["force_depth_clear"] = "false";
        ini["FrameBuffer"]["disable_fbinfo"] = "false";
        
        // Performance settings saved to graphics INI
        ini["Performance"]["threaded_video"] = g_settings.threading.enableAsyncGraphics ? "true" : "false";
        ini["Performance"]["shader_storage"] = "true";
        ini["Performance"]["texture_cache_mb"] = "256";

        ini["Enhancements"]["enable_hd_textures"] = g_settings.graphics.enableHDTextures ? "true" : "false";
        ini["Enhancements"]["tx_hires_enable"] = g_settings.graphics.enableHDTextures ? "true" : "false";
        ini["Enhancements"]["tx_hires_full_alpha"] = "true";
        
        WriteINI(g_patchesPath + "cv64_graphics.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Graphics Configuration\n"
            "; ===========================================================================");
    }
    
    // Save Audio
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        ini["Audio"]["backend"] = "AUTO";
        ini["Audio"]["sample_rate"] = std::to_string(g_settings.audio.sampleRate);
        ini["Audio"]["buffer_size"] = "1024";
        ini["Audio"]["latency_ms"] = std::to_string(g_settings.audio.latencyMs);
        
        char volBuf[32];
        snprintf(volBuf, sizeof(volBuf), "%.2f", g_settings.audio.masterVolume / 100.0f);
        ini["Volume"]["master"] = volBuf;
        snprintf(volBuf, sizeof(volBuf), "%.2f", g_settings.audio.musicVolume / 100.0f);
        ini["Volume"]["music"] = volBuf;
        snprintf(volBuf, sizeof(volBuf), "%.2f", g_settings.audio.sfxVolume / 100.0f);
        ini["Volume"]["sfx"] = volBuf;
        ini["Volume"]["voice"] = "1.0";
        ini["Volume"]["ambient"] = "1.0";
        
        ini["Enhancements"]["enable_reverb"] = g_settings.audio.enableReverb ? "true" : "false";
        ini["Enhancements"]["reverb_amount"] = "0.3";
        ini["Enhancements"]["enable_surround"] = g_settings.audio.enableSurround ? "true" : "false";
        ini["Enhancements"]["enable_bass_boost"] = "false";
        ini["Enhancements"]["bass_boost_amount"] = "0.2";
        
        ini["HDMusic"]["enable_hd_music"] = "false";
        ini["HDMusic"]["hd_music_path"] = "";
        
        ini["Accuracy"]["accurate_n64_audio"] = "true";
        ini["Accuracy"]["enable_hle_audio"] = "true";
        
        WriteINI(g_patchesPath + "cv64_audio.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Audio Configuration\n"
            "; ===========================================================================");
    }
    
    // Save Camera
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        ini["Camera"]["enabled"] = "true";
        ini["Camera"]["enable_dpad"] = g_settings.camera.enableDpad ? "true" : "false";
        ini["Camera"]["enable_right_stick"] = g_settings.camera.enableRightStick ? "true" : "false";
        ini["Camera"]["enable_mouse"] = g_settings.camera.enableMouse ? "true" : "false";
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", g_settings.camera.dpadSpeed);
        ini["Sensitivity"]["dpad_rotation_speed"] = buf;
        snprintf(buf, sizeof(buf), "%.1f", g_settings.camera.stickSensitivityX);
        ini["Sensitivity"]["stick_sensitivity_x"] = buf;
        snprintf(buf, sizeof(buf), "%.1f", g_settings.camera.stickSensitivityY);
        ini["Sensitivity"]["stick_sensitivity_y"] = buf;
        snprintf(buf, sizeof(buf), "%.2f", g_settings.camera.mouseSensitivityX);
        ini["Sensitivity"]["mouse_sensitivity_x"] = buf;
        snprintf(buf, sizeof(buf), "%.2f", g_settings.camera.mouseSensitivityY);
        ini["Sensitivity"]["mouse_sensitivity_y"] = buf;
        
        ini["Limits"]["pitch_min"] = "-60.0";
        ini["Limits"]["pitch_max"] = "80.0";
        
        ini["Behavior"]["enable_smoothing"] = g_settings.camera.enableSmoothing ? "true" : "false";
        ini["Behavior"]["smoothing_factor"] = "0.15";
        ini["Behavior"]["invert_x"] = g_settings.camera.invertX ? "true" : "false";
        ini["Behavior"]["invert_y"] = g_settings.camera.invertY ? "true" : "false";
        
        ini["AutoCenter"]["enable_auto_center"] = "false";
        ini["AutoCenter"]["auto_center_delay"] = "3.0";
        ini["AutoCenter"]["auto_center_speed"] = "2.0";
        
        ini["Modes"]["allow_in_first_person"] = "false";
        ini["Modes"]["allow_in_cutscenes"] = "false";
        ini["Modes"]["allow_in_menus"] = "false";
        
        WriteINI(g_patchesPath + "cv64_camera.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Camera Configuration\n"
            "; ===========================================================================");
    }
    
    // Save Controls
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        
        // Keep keyboard defaults
        ini["Keyboard"]["stick_up"] = "W";
        ini["Keyboard"]["stick_down"] = "S";
        ini["Keyboard"]["stick_left"] = "A";
        ini["Keyboard"]["stick_right"] = "D";
        ini["Keyboard"]["button_a"] = "Z";
        ini["Keyboard"]["button_b"] = "X";
        ini["Keyboard"]["button_z"] = "C";
        ini["Keyboard"]["button_start"] = "RETURN";
        ini["Keyboard"]["button_l"] = "Q";
        ini["Keyboard"]["button_r"] = "E";
        ini["Keyboard"]["button_c_up"] = "I";
        ini["Keyboard"]["button_c_down"] = "K";
        ini["Keyboard"]["button_c_left"] = "J";
        ini["Keyboard"]["button_c_right"] = "L";
        ini["Keyboard"]["button_dpad_up"] = "UP";
        ini["Keyboard"]["button_dpad_down"] = "DOWN";
        ini["Keyboard"]["button_dpad_left"] = "LEFT";
        ini["Keyboard"]["button_dpad_right"] = "RIGHT";
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", g_settings.controls.mouseSensX);
        ini["Mouse"]["sensitivity_x"] = buf;
        snprintf(buf, sizeof(buf), "%.2f", g_settings.controls.mouseSensY);
        ini["Mouse"]["sensitivity_y"] = buf;
        ini["Mouse"]["invert_y"] = g_settings.controls.mouseInvertY ? "true" : "false";
        ini["Mouse"]["capture_toggle_key"] = "F4";
        
        snprintf(buf, sizeof(buf), "%.2f", g_settings.controls.leftStickDeadzone);
        ini["Gamepad"]["left_stick_deadzone"] = buf;
        snprintf(buf, sizeof(buf), "%.2f", g_settings.controls.rightStickDeadzone);
        ini["Gamepad"]["right_stick_deadzone"] = buf;
        ini["Gamepad"]["left_stick_sensitivity"] = "1.0";
        ini["Gamepad"]["right_stick_sensitivity"] = "1.0";
        ini["Gamepad"]["trigger_threshold"] = "0.5";
        
        // Keep XInput defaults
        ini["XInputMappings"]["A_to"] = "A";
        ini["XInputMappings"]["B_to"] = "B";
        ini["XInputMappings"]["X_to"] = "C_DOWN";
        ini["XInputMappings"]["Y_to"] = "C_UP";
        ini["XInputMappings"]["LB_to"] = "Z";
        ini["XInputMappings"]["RB_to"] = "R";
        ini["XInputMappings"]["LT_to"] = "L";
        ini["XInputMappings"]["RT_to"] = "R";
        ini["XInputMappings"]["BACK_to"] = "none";
        ini["XInputMappings"]["START_to"] = "START";
        ini["XInputMappings"]["LSTICK_to"] = "C_LEFT";
        ini["XInputMappings"]["RSTICK_to"] = "C_RIGHT";
        
        ini["CameraControls"]["enable_dpad_camera"] = g_settings.camera.enableDpad ? "true" : "false";
        ini["CameraControls"]["enable_right_stick_camera"] = g_settings.camera.enableRightStick ? "true" : "false";
        ini["CameraControls"]["enable_mouse_camera"] = g_settings.camera.enableMouse ? "true" : "false";
        ini["CameraControls"]["camera_rotation_speed"] = "2.0";
        ini["CameraControls"]["camera_pitch_limit_up"] = "80.0";
        ini["CameraControls"]["camera_pitch_limit_down"] = "-60.0";
        
        WriteINI(g_patchesPath + "cv64_controls.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Controller Configuration\n"
            "; ===========================================================================");
    }
    
    // Save Patches
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        ini["CameraPatches"]["DPAD_CAMERA"] = g_settings.patches.dpadCamera ? "true" : "false";
        ini["CameraPatches"]["FREE_CAMERA"] = "false";
        ini["CameraPatches"]["FIRST_PERSON_SMOOTH"] = g_settings.patches.smoothFirstPerson ? "true" : "false";
        
        ini["ControlPatches"]["MODERN_CONTROLS"] = g_settings.patches.modernControls ? "true" : "false";
        ini["ControlPatches"]["KEYBOARD_MOUSE"] = g_settings.patches.keyboardMouse ? "true" : "false";
        ini["ControlPatches"]["ANALOG_MOVEMENT"] = "true";
        
        ini["GraphicsPatches"]["WIDESCREEN"] = g_settings.patches.widescreen ? "true" : "false";
        ini["GraphicsPatches"]["HD_UI"] = "false";
        ini["GraphicsPatches"]["ENHANCED_FOG"] = "false";
        ini["GraphicsPatches"]["DRAW_DISTANCE"] = "false";
        ini["GraphicsPatches"]["FRAMERATE_UNLOCK"] = g_settings.patches.framerateUnlock ? "true" : "false";
        ini["GraphicsPatches"]["ANIM_INTERPOLATION"] = g_settings.patches.animInterpolation ? "true" : "false";
        
        ini["GameplayPatches"]["SKIP_INTRO"] = g_settings.patches.skipIntro ? "true" : "false";
        ini["GameplayPatches"]["FAST_TEXT"] = g_settings.patches.fastText ? "true" : "false";
        ini["GameplayPatches"]["INSTANT_MAP"] = g_settings.patches.instantMap ? "true" : "false";
        ini["GameplayPatches"]["SAVE_ANYWHERE"] = "false";
        
        ini["AudioPatches"]["CD_QUALITY_MUSIC"] = "false";
        ini["AudioPatches"]["ENHANCED_SFX"] = "false";
        
        ini["BugFixes"]["FIX_SOFTLOCKS"] = g_settings.patches.fixSoftlocks ? "true" : "false";
        ini["BugFixes"]["FIX_COLLISION"] = g_settings.patches.fixCollision ? "true" : "false";
        ini["BugFixes"]["FIX_CAMERA_CLIP"] = g_settings.patches.fixCameraClip ? "true" : "false";
        
        WriteINI(g_patchesPath + "cv64_patches.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Game Patches Configuration\n"
            "; ===========================================================================");
    }
    
    // Save Threading settings
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        ini["Threading"]["enable_async_graphics"] = g_settings.threading.enableAsyncGraphics ? "true" : "false";
        ini["Threading"]["enable_async_audio"] = g_settings.threading.enableAsyncAudio ? "true" : "false";
        ini["Threading"]["enable_worker_threads"] = g_settings.threading.enableWorkerThreads ? "true" : "false";
        ini["Threading"]["worker_thread_count"] = std::to_string(g_settings.threading.workerThreadCount);
        ini["Threading"]["graphics_queue_depth"] = std::to_string(g_settings.threading.graphicsQueueDepth);
        ini["Threading"]["enable_parallel_rsp"] = g_settings.threading.enableParallelRSP ? "true" : "false";
        
        ini["Performance"]["enable_overlay"] = g_settings.threading.enablePerfOverlay ? "true" : "false";
        ini["Performance"]["overlay_mode"] = std::to_string(g_settings.threading.perfOverlayMode);
        
        ini["Info"]["Description"] = "Threading improves performance on multi-core CPUs";
        ini["Info"]["AsyncGraphics"] = "Allows GPU to present frames while CPU continues";
        ini["Info"]["AsyncAudio"] = "Reduces audio stuttering with ring buffer";
        ini["Info"]["WorkerThreads"] = "Background tasks (texture loading, etc)";
        ini["Info"]["WorkerThreadCount"] = "0 = auto-detect based on CPU cores";
        ini["Info"]["GraphicsQueueDepth"] = "1=single, 2=double, 3=triple buffering";
        ini["Info"]["ParallelRSP"] = "EXPERIMENTAL - keep false unless testing";
        
        WriteINI(g_patchesPath + "cv64_threading.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Threading & Performance Configuration\n"
            "; ===========================================================================\n"
            "; Threading is ON by default for better performance on multi-core CPUs.\n"
            "; Disable if you experience issues or are on a low-end system.\n"
            "; ===========================================================================");
    }
    
    // Save Post Processing settings to postprocessing_preset.ini in patches folder
    {
        // Build the Techniques line based on enabled effects
        std::string techniques;
        std::string techniqueSorting;
        
        // Order: Chromaticity, FilmGrain, TrackingRays, Barbatos_SSR_Lite, MiAO, DPX, Vignette
        if (g_settings.postProcessing.enableReflectiveMaterials) {
            if (!techniques.empty()) techniques += ",";
            techniques += "Chromaticity@Chromaticity.fx";
        }
        if (g_settings.postProcessing.enableFilmGrain) {
            if (!techniques.empty()) techniques += ",";
            techniques += "FilmGrain@FilmGrain.fx";
        }
        if (g_settings.postProcessing.enableEnhancedLighting) {
            if (!techniques.empty()) techniques += ",";
            techniques += "TrackingRays@TrackingRays.fx";
        }
        if (g_settings.postProcessing.enableReflections) {
            if (!techniques.empty()) techniques += ",";
            techniques += "Barbatos_SSR_Lite@Barbatos_SSR_Lite.fx";
        }
        if (g_settings.postProcessing.enableDynamicShadows) {
            if (!techniques.empty()) techniques += ",";
            techniques += "MiAO@MiAO.fx";
        }
        if (g_settings.postProcessing.enableColorGrading) {
            if (!techniques.empty()) techniques += ",";
            techniques += "DPX@DPX.fx";
        }
        if (g_settings.postProcessing.enableVignette) {
            if (!techniques.empty()) techniques += ",";
            techniques += "Vignette@Vignette.fx";
        }
        
        // TechniqueSorting matches Techniques
        techniqueSorting = techniques;
        
        // Build the full INI content with ReShade format
        std::string iniContent = "PreprocessorDefinitions=\n";
        iniContent += "Techniques=" + techniques + "\n";
        iniContent += "TechniqueSorting=" + techniqueSorting + "\n";
        iniContent += "\n";
        
        // Add effect parameters (always include all for when they're enabled)
        iniContent += "[Barbatos_SSR_Lite.fx]\n";
        iniContent += "DebugView=0\n";
        iniContent += "FadeDistance=4.999000\n";
        iniContent += "g_BlendMode=16\n";
        iniContent += "g_Contrast=1.000000\n";
        iniContent += "g_Saturation=0.150000\n";
        iniContent += "Intensity=0.300000\n";
        iniContent += "Metallic=0.200000\n";
        iniContent += "ReflectionMode=0\n";
        iniContent += "RenderScale=0.500000\n";
        iniContent += "STEPS=20.000000\n";
        iniContent += "THICKNESS_THRESHOLD=0.500000\n";
        iniContent += "VERTICAL_FOV=37.000000\n";
        iniContent += "\n";
        
        iniContent += "[Chromaticity.fx]\n";
        iniContent += "CHROMA_A_WEIGHT=0.299000\n";
        iniContent += "CHROMA_A_X=0.670000\n";
        iniContent += "CHROMA_A_Y=0.330000\n";
        iniContent += "CHROMA_B_WEIGHT=0.587000\n";
        iniContent += "CHROMA_B_X=0.210000\n";
        iniContent += "CHROMA_B_Y=0.710000\n";
        iniContent += "CHROMA_C_WEIGHT=0.114000\n";
        iniContent += "CHROMA_C_X=0.140000\n";
        iniContent += "CHROMA_C_Y=0.080000\n";
        iniContent += "CRT_TR=0.099000\n";
        iniContent += "CRT_TR0=0.018000\n";
        iniContent += "CRT_TR2=4.500000\n";
        iniContent += "GAMMAIN=2.400000\n";
        iniContent += "GAMMAOUT=2.200000\n";
        iniContent += "SCALE_W=1\n";
        iniContent += "\n";
        
        iniContent += "[DPX.fx]\n";
        iniContent += "Colorfulness=2.500000\n";
        iniContent += "Contrast=0.100000\n";
        iniContent += "RGB_C=0.360000,0.360000,0.340000\n";
        iniContent += "RGB_Curve=8.000000,8.000000,8.000000\n";
        iniContent += "Saturation=3.000000\n";
        iniContent += "Strength=0.200000\n";
        iniContent += "\n";
        
        iniContent += "[FilmGrain.fx]\n";
        iniContent += "Intensity=0.037000\n";
        iniContent += "Mean=1.000000\n";
        iniContent += "SignalToNoiseRatio=0\n";
        iniContent += "Variance=1.000000\n";
        iniContent += "\n";
        
        iniContent += "[MiAO.fx]\n";
        iniContent += "DebugView=0\n";
        iniContent += "EffectHorizonAngleThreshold=0.040000\n";
        iniContent += "EffectShadowClamp=0.980000\n";
        iniContent += "EnableDistantRadius=1\n";
        iniContent += "EnableTemporal=1\n";
        iniContent += "FadeOutFrom=50.000000\n";
        iniContent += "FadeOutTo=1.000000\n";
        iniContent += "FOV=75.000000\n";
        iniContent += "QualityLevel=0\n";
        iniContent += "Radius=5.000000\n";
        iniContent += "RenderScale=0.500000\n";
        iniContent += "ShadowPow=1.320000\n";
        iniContent += "\n";
        
        iniContent += "[TrackingRays.fx]\n";
        iniContent += "Curve=2.400000\n";
        iniContent += "Delay=4.500000\n";
        iniContent += "Intensity=0.080000\n";
        iniContent += "MergeTolerance=0.100000\n";
        iniContent += "Scale=2.700000\n";
        iniContent += "_WipWarn=0\n";
        iniContent += "\n";
        
        iniContent += "[Vignette.fx]\n";
        iniContent += "Amount=-1.000000\n";
        iniContent += "Center=0.500000,0.500000\n";
        iniContent += "Radius=2.000000\n";
        iniContent += "Ratio=1.000000\n";
        iniContent += "Slope=2\n";
        iniContent += "Type=0\n";
        
        // Write to patches folder (where ReShade.ini expects it)
        std::string presetPath = g_patchesPath + "postprocessing_preset.ini";
        std::ofstream file(presetPath);
        if (file.is_open()) {
            file << iniContent;
            file.close();
        }
    }
    
    // Save Cheats settings
    {
        std::map<std::string, std::map<std::string, std::string>> ini;
        
        // Performance cheats
        ini["Performance"]["camera_patch"] = g_settings.cheats.enableCameraPatch ? "true" : "false";
        ini["Performance"]["lag_reduction"] = g_settings.cheats.enableLagReduction ? "true" : "false";
        ini["Performance"]["forest_fps_fix"] = g_settings.cheats.enableForestFix ? "true" : "false";
        
        // Gameplay cheats
        ini["Gameplay"]["infinite_health"] = g_settings.cheats.enableInfiniteHealth ? "true" : "false";
        ini["Gameplay"]["infinite_subweapon"] = g_settings.cheats.enableInfiniteSubweapon ? "true" : "false";
        ini["Gameplay"]["infinite_gold"] = g_settings.cheats.enableInfiniteGold ? "true" : "false";
        ini["Gameplay"]["moon_jump"] = g_settings.cheats.enableMoonJump ? "true" : "false";
        
        // Debug cheats
        ini["Debug"]["no_clip"] = g_settings.cheats.enableNoClip ? "true" : "false";
        ini["Debug"]["speed_boost"] = g_settings.cheats.enableSpeedBoost ? "true" : "false";
        
        // Info section
        ini["Info"]["Description"] = "Gameshark-style cheats for Castlevania 64";
        ini["Info"]["Performance_Cheats"] = "Improve FPS and reduce lag - safe to leave enabled";
        ini["Info"]["Gameplay_Cheats"] = "Make the game easier - disabled by default";
        ini["Info"]["Debug_Cheats"] = "For testing only - may cause issues";
        
        WriteINI(g_patchesPath + "cv64_cheats.ini", ini,
            "; ===========================================================================\n"
            "; Castlevania 64 PC - Cheats Configuration\n"
            "; ===========================================================================\n"
            "; Gameshark-style memory modifications applied every frame.\n"
            ";\n"
            "; PERFORMANCE CHEATS (recommended - enabled by default):\n"
            ";   60fps_menu       - Makes menus run at 60fps instead of 30fps\n"
            ";   60fps_gameplay   - Makes gameplay run at 60fps instead of 30fps\n"
            ";   lag_reduction    - Reduces slowdowns when many enemies on screen\n"
            ";   forest_fps_fix   - Fixes FPS drops in Forest of Silence area\n"
            ";\n"
            "; GAMEPLAY CHEATS (disabled by default):\n"
            ";   infinite_health  - Locks HP at 100\n"
            ";   infinite_subweapon - Locks sub-weapon ammo at 99\n"
            ";   infinite_gold    - Locks gold at 9999\n"
            ";   moon_jump        - Hold L button to jump higher\n"
            ";\n"
            "; ===========================================================================");
    }
    
    return true;
}

const CV64_AllSettings& CV64_Settings_Get(void) {
    return g_settings;
}

CV64_AllSettings& CV64_Settings_GetMutable(void) {
    return g_settings;
}

const std::string& CV64_Settings_GetPatchesPath(void) {
    return g_patchesPath;
}

//=============================================================================
// Settings Dialog Implementation
//=============================================================================

// Dialog control IDs - grouped by tab with offset
#define TAB_OFFSET_DISPLAY   0
#define TAB_OFFSET_AUDIO     100
#define TAB_OFFSET_CAMERA    200
#define TAB_OFFSET_CONTROLS  300
#define TAB_OFFSET_GAMEPLAY  400

#define IDC_TAB_CONTROL         1001

// Graphics tab (1010-1099)
#define IDC_COMBO_RESOLUTION    1010
#define IDC_CHECK_FULLSCREEN    1011
#define IDC_CHECK_BORDERLESS    1012
#define IDC_CHECK_VSYNC         1013
#define IDC_COMBO_TEXFILTER     1014
#define IDC_COMBO_AA            1015
#define IDC_SLIDER_SCALE        1016
#define IDC_STATIC_SCALE        1017
#define IDC_CHECK_HDTEX         1018
#define IDC_COMBO_ASPECT        1019
// Dynamic Shadows controls (on Display tab)
#define IDC_CHECK_DYNAMIC_SHADOWS   1020
#define IDC_COMBO_SHADOW_TYPE       1021
#define IDC_SLIDER_SHADOW_INTENSITY 1022
#define IDC_STATIC_SHADOW_INTENSITY 1023
#define IDC_SLIDER_SHADOW_SOFTNESS  1024
#define IDC_STATIC_SHADOW_SOFTNESS  1025
// Bloom controls (on Display tab)
#define IDC_CHECK_BLOOM             1026
#define IDC_SLIDER_BLOOM_INTENSITY  1027
#define IDC_STATIC_BLOOM_INTENSITY  1028
#define IDC_SLIDER_BLOOM_THRESHOLD  1029
#define IDC_STATIC_BLOOM_THRESHOLD  1030

// Audio tab (1110-1199)
#define IDC_SLIDER_MASTER       1110
#define IDC_SLIDER_MUSIC        1111
#define IDC_SLIDER_SFX          1112
#define IDC_STATIC_MASTER       1113
#define IDC_STATIC_MUSIC        1114
#define IDC_STATIC_SFX          1115
#define IDC_COMBO_SAMPLERATE    1116
#define IDC_CHECK_REVERB        1117
#define IDC_CHECK_SURROUND      1118

// Camera tab (1210-1299)
#define IDC_CHECK_DPAD          1210
#define IDC_CHECK_RSTICK        1211
#define IDC_CHECK_MOUSE         1212
#define IDC_SLIDER_DPADSPEED    1213
#define IDC_SLIDER_STICKX       1214
#define IDC_SLIDER_STICKY       1215
#define IDC_CHECK_INVERTX       1216
#define IDC_CHECK_INVERTY       1217
#define IDC_CHECK_SMOOTH        1218
#define IDC_STATIC_DPADSPEED    1219
#define IDC_STATIC_STICKX       1220
#define IDC_STATIC_STICKY       1221

// Controls tab (1310-1399)
#define IDC_SLIDER_DEADZONE_L   1310
#define IDC_SLIDER_DEADZONE_R   1311
#define IDC_STATIC_DEADZONE_L   1312
#define IDC_STATIC_DEADZONE_R   1313

// Gameplay tab (1410-1499)
#define IDC_CHECK_DPAD_CAM      1410
#define IDC_CHECK_MODERN_CTRL   1411
#define IDC_CHECK_WIDESCREEN    1412
#define IDC_CHECK_UNLOCK_FPS    1413
#define IDC_CHECK_SKIP_INTRO    1414
#define IDC_CHECK_FAST_TEXT     1415
#define IDC_CHECK_INSTANT_MAP   1416
#define IDC_CHECK_FIX_SOFTLOCK  1417
#define IDC_CHECK_FIX_COLLISION 1418
#define IDC_CHECK_FIX_CAMERA    1419
#define IDC_CHECK_ANIM_INTERP   1420

// Performance tab (1610-1699) - Now tab index 5
#define IDC_CHECK_PERF_THREADED 1610
#define IDC_CHECK_PERF_SHADERS  1611
#define IDC_SLIDER_PERF_TEXCACHE 1612
#define IDC_STATIC_PERF_TEXCACHE 1613
#define IDC_CHECK_PERF_ASYNC_GFX 1614
#define IDC_CHECK_PERF_ASYNC_AUDIO 1615
#define IDC_COMBO_PERF_DEPTH    1616
#define IDC_COMBO_PERF_FBCOPY   1617
#define IDC_CHECK_PERF_OVERLAY  1618
#define IDC_COMBO_PERF_OVERLAY_MODE 1619

// Cheats tab (1710-1799)
#define IDC_CHECK_CHEAT_CAMERA_PATCH    1710
#define IDC_CHECK_CHEAT_LAG_REDUCTION   1712
#define IDC_CHECK_CHEAT_FOREST_FIX      1713
#define IDC_CHECK_CHEAT_INF_HEALTH      1714
#define IDC_CHECK_CHEAT_INF_SUBWEAPON   1715
#define IDC_CHECK_CHEAT_INF_GOLD        1716
#define IDC_CHECK_CHEAT_MOON_JUMP       1717
#define IDC_CHECK_CHEAT_NO_CLIP         1718
#define IDC_CHECK_CHEAT_SPEED_BOOST     1719

// Buttons
#define IDC_BTN_OK              1500
#define IDC_BTN_CANCEL          1501
#define IDC_BTN_APPLY           1502
#define IDC_BTN_DEFAULTS        1503

static HWND g_hSettingsDlg = NULL;
static HWND g_hTabCtrl = NULL;
static CV64_AllSettings g_tempSettings;
static int g_currentTab = 0;
static HFONT g_hFont = NULL;

// Store all control HWNDs by tab
#define MAX_CONTROLS_PER_TAB 40
static HWND g_tabControls[8][MAX_CONTROLS_PER_TAB];
static int g_tabControlCount[8] = {0};

// Forward declarations
static void CreateAllControls(HWND hDlg);
static void ShowTabPage(int index);
static void LoadSettingsToUI(HWND hDlg);
static void SaveUIToSettings(HWND hDlg);
static void UpdateSliderLabels(HWND hDlg);
static void ApplySettingsToEmulation(void);

static HWND AddControl(HWND hDlg, int tab, const wchar_t* className, const wchar_t* text, 
                       DWORD style, int x, int y, int w, int h, int id) {
    // Offset y by tab content area (tab headers ~28px)
    int offsetY = 38;
    HWND hwnd = CreateWindowW(className, text, WS_CHILD | style,
        15 + x, offsetY + y, w, h, hDlg, (HMENU)(INT_PTR)id, g_hInst, NULL);
    if (hwnd && g_hFont) {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }
    if (tab >= 0 && tab < 8 && g_tabControlCount[tab] < MAX_CONTROLS_PER_TAB) {
        g_tabControls[tab][g_tabControlCount[tab]++] = hwnd;
    }
    return hwnd;
}

static void CreateDisplayTab(HWND hDlg) {
    int tab = 0;
    int y = 5;

    AddControl(hDlg, tab, L"STATIC", L"Resolution:", 0, 0, y + 3, 80, 20, -1);
    HWND combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 90, y, 150, 200, IDC_COMBO_RESOLUTION);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"480p (640x480)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"720p (1280x720)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"1080p (1920x1080)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"1440p (2560x1440)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"4K (3840x2160)");
    y += 28;

    AddControl(hDlg, tab, L"STATIC", L"Aspect Ratio:", 0, 0, y + 3, 80, 20, -1);
    combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 90, y, 150, 200, IDC_COMBO_ASPECT);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"4:3 (Original)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"16:9 (Widescreen)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"16:10");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"21:9 (Ultrawide)");
    y += 28;

    AddControl(hDlg, tab, L"BUTTON", L"Fullscreen", BS_AUTOCHECKBOX, 0, y, 100, 20, IDC_CHECK_FULLSCREEN);
    AddControl(hDlg, tab, L"BUTTON", L"Borderless", BS_AUTOCHECKBOX, 110, y, 100, 20, IDC_CHECK_BORDERLESS);
    y += 25;

    AddControl(hDlg, tab, L"BUTTON", L"V-Sync", BS_AUTOCHECKBOX, 0, y, 200, 20, IDC_CHECK_VSYNC);
    y += 30;

    AddControl(hDlg, tab, L"STATIC", L"Texture Filter:", 0, 0, y + 3, 90, 20, -1);
    combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 100, y, 130, 200, IDC_COMBO_TEXFILTER);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Sharp (Nearest)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Smooth (Linear)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Best (Trilinear)");
    y += 28;

    AddControl(hDlg, tab, L"STATIC", L"Anti-Aliasing:", 0, 0, y + 3, 90, 20, -1);
    combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 100, y, 130, 200, IDC_COMBO_AA);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Off");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"FXAA");
    y += 28;

    AddControl(hDlg, tab, L"STATIC", L"Render Scale:", 0, 0, y + 3, 85, 20, -1);
    HWND slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS, 90, y, 120, 22, IDC_SLIDER_SCALE);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(1, 4));
    AddControl(hDlg, tab, L"STATIC", L"2x", 0, 215, y + 3, 40, 20, IDC_STATIC_SCALE);
    y += 28;

    AddControl(hDlg, tab, L"BUTTON", L"HD Texture Packs", BS_AUTOCHECKBOX, 0, y, 180, 20, IDC_CHECK_HDTEX);
}

static void CreateAudioTab(HWND hDlg) {
    int tab = 1;
    int y = 5;
    
    AddControl(hDlg, tab, L"STATIC", L"Master Volume:", 0, 0, y + 3, 90, 20, -1);
    HWND slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 95, y, 130, 22, IDC_SLIDER_MASTER);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    AddControl(hDlg, tab, L"STATIC", L"100%", 0, 230, y + 3, 45, 20, IDC_STATIC_MASTER);
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Music Volume:", 0, 0, y + 3, 90, 20, -1);
    slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 95, y, 130, 22, IDC_SLIDER_MUSIC);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    AddControl(hDlg, tab, L"STATIC", L"100%", 0, 230, y + 3, 45, 20, IDC_STATIC_MUSIC);
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Effects Volume:", 0, 0, y + 3, 90, 20, -1);
    slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 95, y, 130, 22, IDC_SLIDER_SFX);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    AddControl(hDlg, tab, L"STATIC", L"100%", 0, 230, y + 3, 45, 20, IDC_STATIC_SFX);
    y += 35;
    
    AddControl(hDlg, tab, L"STATIC", L"Sample Rate:", 0, 0, y + 3, 80, 20, -1);
    HWND combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 90, y, 140, 200, IDC_COMBO_SAMPLERATE);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"22050 Hz");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"44100 Hz (CD)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"48000 Hz (Best)");
    y += 35;
    
    AddControl(hDlg, tab, L"BUTTON", L"Enable Reverb", BS_AUTOCHECKBOX, 0, y, 150, 20, IDC_CHECK_REVERB);
    y += 25;
    AddControl(hDlg, tab, L"BUTTON", L"Surround Sound", BS_AUTOCHECKBOX, 0, y, 150, 20, IDC_CHECK_SURROUND);
}

static void CreateCameraTab(HWND hDlg) {
    int tab = 2;
    int y = 5;
    
    AddControl(hDlg, tab, L"STATIC", L"Camera Control Input:", 0, 0, y, 200, 20, -1);
    y += 22;
    
    AddControl(hDlg, tab, L"BUTTON", L"D-Pad / Arrow Keys", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_DPAD);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Right Analog Stick", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_RSTICK);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Mouse (click to capture)", BS_AUTOCHECKBOX, 10, y, 180, 20, IDC_CHECK_MOUSE);
    y += 30;
    
    AddControl(hDlg, tab, L"STATIC", L"D-Pad Speed:", 0, 0, y + 3, 85, 20, -1);
    HWND slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 90, y, 120, 22, IDC_SLIDER_DPADSPEED);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 50));
    AddControl(hDlg, tab, L"STATIC", L"2.0", 0, 215, y + 3, 40, 20, IDC_STATIC_DPADSPEED);
    y += 26;
    
    AddControl(hDlg, tab, L"STATIC", L"Stick Sens X:", 0, 0, y + 3, 85, 20, -1);
    slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 90, y, 120, 22, IDC_SLIDER_STICKX);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 50));
    AddControl(hDlg, tab, L"STATIC", L"1.5", 0, 215, y + 3, 40, 20, IDC_STATIC_STICKX);
    y += 26;
    
    AddControl(hDlg, tab, L"STATIC", L"Stick Sens Y:", 0, 0, y + 3, 85, 20, -1);
    slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 90, y, 120, 22, IDC_SLIDER_STICKY);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 50));
    AddControl(hDlg, tab, L"STATIC", L"1.5", 0, 215, y + 3, 40, 20, IDC_STATIC_STICKY);
    y += 30;
    
    AddControl(hDlg, tab, L"BUTTON", L"Invert X", BS_AUTOCHECKBOX, 0, y, 80, 20, IDC_CHECK_INVERTX);
    AddControl(hDlg, tab, L"BUTTON", L"Invert Y", BS_AUTOCHECKBOX, 90, y, 80, 20, IDC_CHECK_INVERTY);
    y += 25;
    AddControl(hDlg, tab, L"BUTTON", L"Smooth Camera", BS_AUTOCHECKBOX, 0, y, 150, 20, IDC_CHECK_SMOOTH);
}

static void CreateControlsTab(HWND hDlg) {
    int tab = 3;
    int y = 5;
    
    AddControl(hDlg, tab, L"STATIC", L"Left Stick Deadzone:", 0, 0, y + 3, 120, 20, -1);
    HWND slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 125, y, 100, 22, IDC_SLIDER_DEADZONE_L);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 50));
    AddControl(hDlg, tab, L"STATIC", L"15%", 0, 230, y + 3, 40, 20, IDC_STATIC_DEADZONE_L);
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Right Stick Deadzone:", 0, 0, y + 3, 120, 20, -1);
    slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 125, y, 100, 22, IDC_SLIDER_DEADZONE_R);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 50));
    AddControl(hDlg, tab, L"STATIC", L"15%", 0, 230, y + 3, 40, 20, IDC_STATIC_DEADZONE_R);
    y += 40;
    
    AddControl(hDlg, tab, L"STATIC", L"Keyboard Controls:", 0, 0, y, 200, 18, -1);
    y += 20;
    AddControl(hDlg, tab, L"STATIC", L"  WASD = Move", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  Arrow Keys = Camera", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  Z = Attack / Confirm", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  X = Use Item / Cancel", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  C = Lock-On", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  Enter = Start/Pause", 0, 0, y, 200, 16, -1); y += 18;
    AddControl(hDlg, tab, L"STATIC", L"  Q/E = Shoulder Buttons", 0, 0, y, 200, 16, -1);
}

static void CreateGameplayTab(HWND hDlg) {
    int tab = 4;
    int y = 5;
    
    AddControl(hDlg, tab, L"STATIC", L"Enhancements:", 0, 0, y, 200, 18, -1);
    y += 20;
    
    AddControl(hDlg, tab, L"BUTTON", L"Free Camera Control", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_DPAD_CAM);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Modern Controller Layout", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_MODERN_CTRL);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Widescreen Support", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_WIDESCREEN);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Unlock Frame Rate", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_UNLOCK_FPS);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Animation Interpolation", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_ANIM_INTERP);
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Quality of Life:", 0, 0, y, 200, 18, -1);
    y += 20;
    
    AddControl(hDlg, tab, L"BUTTON", L"Skip Intro", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_SKIP_INTRO);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Fast Text", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_FAST_TEXT);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Instant Map", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_INSTANT_MAP);
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Bug Fixes:", 0, 0, y, 200, 18, -1);
    y += 20;
    
    AddControl(hDlg, tab, L"BUTTON", L"Fix Softlocks", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_FIX_SOFTLOCK);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Fix Collision", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_FIX_COLLISION);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Fix Camera Clip", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_FIX_CAMERA);
}

// CreatePostProcessingTab removed - shadows now on Display tab

static void CreatePerformanceTab(HWND hDlg) {
    int tab = 5;  // Now tab 5 (Post FX removed)
    int y = 5;
    
    // Section: Threading
    AddControl(hDlg, tab, L"STATIC", L"Threading & Parallelization:", 0, 0, y, 280, 20, -1);
    y += 24;
    
    AddControl(hDlg, tab, L"BUTTON", L"Threaded Video (GPU parallelization)", BS_AUTOCHECKBOX, 10, y, 300, 20, IDC_CHECK_PERF_THREADED);
    y += 24;
    AddControl(hDlg, tab, L"BUTTON", L"Shader Storage (reduce stutter)", BS_AUTOCHECKBOX, 10, y, 300, 20, IDC_CHECK_PERF_SHADERS);
    y += 24;
    AddControl(hDlg, tab, L"BUTTON", L"Async Graphics", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_PERF_ASYNC_GFX);
    AddControl(hDlg, tab, L"BUTTON", L"Async Audio", BS_AUTOCHECKBOX, 170, y, 150, 20, IDC_CHECK_PERF_ASYNC_AUDIO);
    y += 32;
    
    // Section: Texture Cache
    AddControl(hDlg, tab, L"STATIC", L"Texture Cache (MB):", 0, 0, y + 3, 130, 20, -1);
    HWND slider = AddControl(hDlg, tab, TRACKBAR_CLASSW, L"", TBS_HORZ, 140, y, 180, 24, IDC_SLIDER_PERF_TEXCACHE);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(64, 512));
    SendMessage(slider, TBM_SETTICFREQ, 64, 0);
    AddControl(hDlg, tab, L"STATIC", L"256 MB", 0, 330, y + 3, 70, 20, IDC_STATIC_PERF_TEXCACHE);
    y += 36;
    
    // Section: Framebuffer
    AddControl(hDlg, tab, L"STATIC", L"Framebuffer Settings:", 0, 0, y, 200, 20, -1);
    y += 24;
    
    AddControl(hDlg, tab, L"STATIC", L"Framebuffer Quality:", 0, 10, y + 3, 130, 20, -1);
    HWND combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 145, y, 180, 200, IDC_COMBO_PERF_DEPTH);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Disabled (Fastest)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Fast");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Compatible (Slowest)");
    y += 28;
    
    AddControl(hDlg, tab, L"STATIC", L"Buffering:", 0, 10, y + 3, 130, 20, -1);
    combo = AddControl(hDlg, tab, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 145, y, 180, 200, IDC_COMBO_PERF_FBCOPY);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Disabled");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Synchronous");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Double Buffer (Async)");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Triple Buffer");
    y += 36;
    
    // Info text
    AddControl(hDlg, tab, L"STATIC", L"Note: Some performance settings may require", 0, 0, y, 400, 18, -1);
    y += 18;
    AddControl(hDlg, tab, L"STATIC", L"a restart to take full effect.", 0, 0, y, 400, 18, -1);
}

static void CreateCheatsTab(HWND hDlg) {
int tab = 7;
int y = 5;
    
// Section: Performance Cheats (Recommended)
AddControl(hDlg, tab, L"STATIC", L"Performance Cheats (Recommended):", 0, 0, y, 280, 20, -1);
y += 22;
    
AddControl(hDlg, tab, L"BUTTON", L"Enable Camera Patch", BS_AUTOCHECKBOX, 10, y, 200, 20, IDC_CHECK_CHEAT_CAMERA_PATCH);
y += 22;
AddControl(hDlg, tab, L"BUTTON", L"Reduce Lag (Many Enemies)", BS_AUTOCHECKBOX, 10, y, 220, 20, IDC_CHECK_CHEAT_LAG_REDUCTION);
y += 22;
AddControl(hDlg, tab, L"BUTTON", L"Forest of Silence FPS Fix", BS_AUTOCHECKBOX, 10, y, 220, 20, IDC_CHECK_CHEAT_FOREST_FIX);
y += 30;
    
    // Section: Gameplay Cheats
    AddControl(hDlg, tab, L"STATIC", L"Gameplay Cheats:", 0, 0, y, 200, 20, -1);
    y += 22;
    
    AddControl(hDlg, tab, L"BUTTON", L"Infinite Health", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_CHEAT_INF_HEALTH);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Infinite Sub-weapon", BS_AUTOCHECKBOX, 10, y, 170, 20, IDC_CHECK_CHEAT_INF_SUBWEAPON);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Infinite Gold (9999)", BS_AUTOCHECKBOX, 10, y, 170, 20, IDC_CHECK_CHEAT_INF_GOLD);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Moon Jump (Hold L)", BS_AUTOCHECKBOX, 10, y, 170, 20, IDC_CHECK_CHEAT_MOON_JUMP);
    y += 30;
    
    // Section: Debug Cheats
    AddControl(hDlg, tab, L"STATIC", L"Debug Cheats:", 0, 0, y, 200, 20, -1);
    y += 22;
    
    AddControl(hDlg, tab, L"BUTTON", L"No Clip (Walk Through Walls)", BS_AUTOCHECKBOX, 10, y, 220, 20, IDC_CHECK_CHEAT_NO_CLIP);
    y += 22;
    AddControl(hDlg, tab, L"BUTTON", L"Speed Boost", BS_AUTOCHECKBOX, 10, y, 150, 20, IDC_CHECK_CHEAT_SPEED_BOOST);
    y += 30;
    
    // Info text
    AddControl(hDlg, tab, L"STATIC", L"Performance cheats improve FPS and are safe", 0, 0, y, 350, 18, -1);
    y += 18;
    AddControl(hDlg, tab, L"STATIC", L"to leave enabled. Gameplay cheats make the", 0, 0, y, 350, 18, -1);
    y += 18;
    AddControl(hDlg, tab, L"STATIC", L"game easier. All cheats apply instantly.", 0, 0, y, 350, 18, -1);
}

static void CreateAllControls(HWND hDlg) {
    // Reset control tracking
    for (int i = 0; i < 8; i++) {
        g_tabControlCount[i] = 0;
        for (int j = 0; j < MAX_CONTROLS_PER_TAB; j++) {
            g_tabControls[i][j] = NULL;
        }
    }

    CreateDisplayTab(hDlg);
    CreateAudioTab(hDlg);
    CreateCameraTab(hDlg);
    CreateControlsTab(hDlg);
    CreateGameplayTab(hDlg);
    CreatePerformanceTab(hDlg);
    // Post FX tab removed - shadows now on Display tab
    // Cheats tab disabled
}

static void ShowTabPage(int index) {
for (int tab = 0; tab < 6; tab++) {  // Only 6 tabs now (Post FX removed)
    int showCmd = (tab == index) ? SW_SHOW : SW_HIDE;
    for (int i = 0; i < g_tabControlCount[tab]; i++) {
        if (g_tabControls[tab][i]) {
            ShowWindow(g_tabControls[tab][i], showCmd);
        }
        }
    }
    g_currentTab = index;
}

static void LoadSettingsToUI(HWND hDlg) {
    // Graphics
    int resIndex = 1;
    if (g_tempSettings.graphics.resolution == "480P") resIndex = 0;
    else if (g_tempSettings.graphics.resolution == "720P") resIndex = 1;
    else if (g_tempSettings.graphics.resolution == "1080P") resIndex = 2;
    else if (g_tempSettings.graphics.resolution == "1440P") resIndex = 3;
    else if (g_tempSettings.graphics.resolution == "4K") resIndex = 4;
    SendDlgItemMessage(hDlg, IDC_COMBO_RESOLUTION, CB_SETCURSEL, resIndex, 0);
    
    int aspectIndex = 1;
    if (g_tempSettings.graphics.aspectRatio == "4_3") aspectIndex = 0;
    else if (g_tempSettings.graphics.aspectRatio == "16_9") aspectIndex = 1;
    else if (g_tempSettings.graphics.aspectRatio == "16_10") aspectIndex = 2;
    else if (g_tempSettings.graphics.aspectRatio == "21_9") aspectIndex = 3;
    SendDlgItemMessage(hDlg, IDC_COMBO_ASPECT, CB_SETCURSEL, aspectIndex, 0);
    
    CheckDlgButton(hDlg, IDC_CHECK_FULLSCREEN, g_tempSettings.graphics.fullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_BORDERLESS, g_tempSettings.graphics.borderless ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_VSYNC, g_tempSettings.graphics.vsync ? BST_CHECKED : BST_UNCHECKED);
    
    int texIndex = 1;
    if (g_tempSettings.graphics.textureFilter == "NEAREST") texIndex = 0;
    else if (g_tempSettings.graphics.textureFilter == "LINEAR") texIndex = 1;
    else if (g_tempSettings.graphics.textureFilter == "TRILINEAR") texIndex = 2;
    SendDlgItemMessage(hDlg, IDC_COMBO_TEXFILTER, CB_SETCURSEL, texIndex, 0);
    
    SendDlgItemMessage(hDlg, IDC_COMBO_AA, CB_SETCURSEL, (g_tempSettings.graphics.antiAliasing == "FXAA") ? 1 : 0, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SCALE, TBM_SETPOS, TRUE, g_tempSettings.graphics.internalScale);
    CheckDlgButton(hDlg, IDC_CHECK_HDTEX, g_tempSettings.graphics.enableHDTextures ? BST_CHECKED : BST_UNCHECKED);

    // Audio
    SendDlgItemMessage(hDlg, IDC_SLIDER_MASTER, TBM_SETPOS, TRUE, g_tempSettings.audio.masterVolume);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MUSIC, TBM_SETPOS, TRUE, g_tempSettings.audio.musicVolume);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SFX, TBM_SETPOS, TRUE, g_tempSettings.audio.sfxVolume);
    
    int srIndex = 2;
    if (g_tempSettings.audio.sampleRate == 22050) srIndex = 0;
    else if (g_tempSettings.audio.sampleRate == 44100) srIndex = 1;
    SendDlgItemMessage(hDlg, IDC_COMBO_SAMPLERATE, CB_SETCURSEL, srIndex, 0);
    
    CheckDlgButton(hDlg, IDC_CHECK_REVERB, g_tempSettings.audio.enableReverb ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_SURROUND, g_tempSettings.audio.enableSurround ? BST_CHECKED : BST_UNCHECKED);
    
    // Camera
    CheckDlgButton(hDlg, IDC_CHECK_DPAD, g_tempSettings.camera.enableDpad ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_RSTICK, g_tempSettings.camera.enableRightStick ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_MOUSE, g_tempSettings.camera.enableMouse ? BST_CHECKED : BST_UNCHECKED);
    
    SendDlgItemMessage(hDlg, IDC_SLIDER_DPADSPEED, TBM_SETPOS, TRUE, (int)(g_tempSettings.camera.dpadSpeed * 10));
    SendDlgItemMessage(hDlg, IDC_SLIDER_STICKX, TBM_SETPOS, TRUE, (int)(g_tempSettings.camera.stickSensitivityX * 10));
    SendDlgItemMessage(hDlg, IDC_SLIDER_STICKY, TBM_SETPOS, TRUE, (int)(g_tempSettings.camera.stickSensitivityY * 10));
    
    CheckDlgButton(hDlg, IDC_CHECK_INVERTX, g_tempSettings.camera.invertX ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_INVERTY, g_tempSettings.camera.invertY ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_SMOOTH, g_tempSettings.camera.enableSmoothing ? BST_CHECKED : BST_UNCHECKED);
    
    // Controls
    SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_L, TBM_SETPOS, TRUE, (int)(g_tempSettings.controls.leftStickDeadzone * 100));
    SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_R, TBM_SETPOS, TRUE, (int)(g_tempSettings.controls.rightStickDeadzone * 100));
    
    // Gameplay
    CheckDlgButton(hDlg, IDC_CHECK_DPAD_CAM, g_tempSettings.patches.dpadCamera ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_MODERN_CTRL, g_tempSettings.patches.modernControls ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_WIDESCREEN, g_tempSettings.patches.widescreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_UNLOCK_FPS, g_tempSettings.patches.framerateUnlock ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_ANIM_INTERP, g_tempSettings.patches.animInterpolation ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_SKIP_INTRO, g_tempSettings.patches.skipIntro ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_FAST_TEXT, g_tempSettings.patches.fastText ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_INSTANT_MAP, g_tempSettings.patches.instantMap ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_FIX_SOFTLOCK, g_tempSettings.patches.fixSoftlocks ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_FIX_COLLISION, g_tempSettings.patches.fixCollision ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_FIX_CAMERA, g_tempSettings.patches.fixCameraClip ? BST_CHECKED : BST_UNCHECKED);

    // Performance tab
    CheckDlgButton(hDlg, IDC_CHECK_PERF_THREADED, g_tempSettings.threading.enableAsyncGraphics ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_PERF_SHADERS, true); // Shader storage always on
    CheckDlgButton(hDlg, IDC_CHECK_PERF_ASYNC_GFX, g_tempSettings.threading.enableAsyncGraphics ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_PERF_ASYNC_AUDIO, g_tempSettings.threading.enableAsyncAudio ? BST_CHECKED : BST_UNCHECKED);
    
    // Texture cache slider (64-512 MB)
    SendDlgItemMessage(hDlg, IDC_SLIDER_PERF_TEXCACHE, TBM_SETPOS, TRUE, 256); // Default 256MB
    
    // Depth compare and FB copy modes (loaded from threading struct for now)
    SendDlgItemMessage(hDlg, IDC_COMBO_PERF_DEPTH, CB_SETCURSEL, 0, 0);  // Disabled (CV64 doesn't use HW depth compare)
    SendDlgItemMessage(hDlg, IDC_COMBO_PERF_FBCOPY, CB_SETCURSEL, 2, 0); // Double buffer default
    
    // Performance overlay
    CheckDlgButton(hDlg, IDC_CHECK_PERF_OVERLAY, g_tempSettings.threading.enablePerfOverlay ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(hDlg, IDC_COMBO_PERF_OVERLAY_MODE, CB_SETCURSEL, g_tempSettings.threading.perfOverlayMode, 0);
    
    // Cheats tab
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_CAMERA_PATCH, g_tempSettings.cheats.enableCameraPatch ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_LAG_REDUCTION, g_tempSettings.cheats.enableLagReduction ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_FOREST_FIX, g_tempSettings.cheats.enableForestFix ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_INF_HEALTH, g_tempSettings.cheats.enableInfiniteHealth ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_INF_SUBWEAPON, g_tempSettings.cheats.enableInfiniteSubweapon ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_INF_GOLD, g_tempSettings.cheats.enableInfiniteGold ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_MOON_JUMP, g_tempSettings.cheats.enableMoonJump ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_NO_CLIP, g_tempSettings.cheats.enableNoClip ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_CHEAT_SPEED_BOOST, g_tempSettings.cheats.enableSpeedBoost ? BST_CHECKED : BST_UNCHECKED);
    
    UpdateSliderLabels(hDlg);
}

static void SaveUIToSettings(HWND hDlg) {
    // Graphics
    int resIndex = (int)SendDlgItemMessage(hDlg, IDC_COMBO_RESOLUTION, CB_GETCURSEL, 0, 0);
    const char* resolutions[] = { "480P", "720P", "1080P", "1440P", "4K" };
    if (resIndex >= 0 && resIndex < 5) g_tempSettings.graphics.resolution = resolutions[resIndex];
    
    int aspectIndex = (int)SendDlgItemMessage(hDlg, IDC_COMBO_ASPECT, CB_GETCURSEL, 0, 0);
    const char* aspects[] = { "4_3", "16_9", "16_10", "21_9" };
    if (aspectIndex >= 0 && aspectIndex < 4) g_tempSettings.graphics.aspectRatio = aspects[aspectIndex];
    
    g_tempSettings.graphics.fullscreen = IsDlgButtonChecked(hDlg, IDC_CHECK_FULLSCREEN) == BST_CHECKED;
    g_tempSettings.graphics.borderless = IsDlgButtonChecked(hDlg, IDC_CHECK_BORDERLESS) == BST_CHECKED;
    g_tempSettings.graphics.vsync = IsDlgButtonChecked(hDlg, IDC_CHECK_VSYNC) == BST_CHECKED;
    
    int texIndex = (int)SendDlgItemMessage(hDlg, IDC_COMBO_TEXFILTER, CB_GETCURSEL, 0, 0);
    const char* texFilters[] = { "NEAREST", "LINEAR", "TRILINEAR" };
    if (texIndex >= 0 && texIndex < 3) g_tempSettings.graphics.textureFilter = texFilters[texIndex];
    
    g_tempSettings.graphics.antiAliasing = (SendDlgItemMessage(hDlg, IDC_COMBO_AA, CB_GETCURSEL, 0, 0) == 1) ? "FXAA" : "NONE";
    g_tempSettings.graphics.internalScale = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_SCALE, TBM_GETPOS, 0, 0);
    g_tempSettings.graphics.enableHDTextures = IsDlgButtonChecked(hDlg, IDC_CHECK_HDTEX) == BST_CHECKED;

    // Audio
    g_tempSettings.audio.masterVolume = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_MASTER, TBM_GETPOS, 0, 0);
    g_tempSettings.audio.musicVolume = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_MUSIC, TBM_GETPOS, 0, 0);
    g_tempSettings.audio.sfxVolume = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_SFX, TBM_GETPOS, 0, 0);
    
    int srIndex = (int)SendDlgItemMessage(hDlg, IDC_COMBO_SAMPLERATE, CB_GETCURSEL, 0, 0);
    const int sampleRates[] = { 22050, 44100, 48000 };
    if (srIndex >= 0 && srIndex < 3) g_tempSettings.audio.sampleRate = sampleRates[srIndex];
    
    g_tempSettings.audio.enableReverb = IsDlgButtonChecked(hDlg, IDC_CHECK_REVERB) == BST_CHECKED;
    g_tempSettings.audio.enableSurround = IsDlgButtonChecked(hDlg, IDC_CHECK_SURROUND) == BST_CHECKED;
    
    // Camera
    g_tempSettings.camera.enableDpad = IsDlgButtonChecked(hDlg, IDC_CHECK_DPAD) == BST_CHECKED;
    g_tempSettings.camera.enableRightStick = IsDlgButtonChecked(hDlg, IDC_CHECK_RSTICK) == BST_CHECKED;
    g_tempSettings.camera.enableMouse = IsDlgButtonChecked(hDlg, IDC_CHECK_MOUSE) == BST_CHECKED;
    
    g_tempSettings.camera.dpadSpeed = SendDlgItemMessage(hDlg, IDC_SLIDER_DPADSPEED, TBM_GETPOS, 0, 0) / 10.0f;
    g_tempSettings.camera.stickSensitivityX = SendDlgItemMessage(hDlg, IDC_SLIDER_STICKX, TBM_GETPOS, 0, 0) / 10.0f;
    g_tempSettings.camera.stickSensitivityY = SendDlgItemMessage(hDlg, IDC_SLIDER_STICKY, TBM_GETPOS, 0, 0) / 10.0f;
    
    g_tempSettings.camera.invertX = IsDlgButtonChecked(hDlg, IDC_CHECK_INVERTX) == BST_CHECKED;
    g_tempSettings.camera.invertY = IsDlgButtonChecked(hDlg, IDC_CHECK_INVERTY) == BST_CHECKED;
    g_tempSettings.camera.enableSmoothing = IsDlgButtonChecked(hDlg, IDC_CHECK_SMOOTH) == BST_CHECKED;
    
    // Controls
    g_tempSettings.controls.leftStickDeadzone = SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_L, TBM_GETPOS, 0, 0) / 100.0f;
    g_tempSettings.controls.rightStickDeadzone = SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_R, TBM_GETPOS, 0, 0) / 100.0f;
    
    // Gameplay
    g_tempSettings.patches.dpadCamera = IsDlgButtonChecked(hDlg, IDC_CHECK_DPAD_CAM) == BST_CHECKED;
    g_tempSettings.patches.modernControls = IsDlgButtonChecked(hDlg, IDC_CHECK_MODERN_CTRL) == BST_CHECKED;
    g_tempSettings.patches.widescreen = IsDlgButtonChecked(hDlg, IDC_CHECK_WIDESCREEN) == BST_CHECKED;
    g_tempSettings.patches.framerateUnlock = IsDlgButtonChecked(hDlg, IDC_CHECK_UNLOCK_FPS) == BST_CHECKED;
    g_tempSettings.patches.animInterpolation = IsDlgButtonChecked(hDlg, IDC_CHECK_ANIM_INTERP) == BST_CHECKED;
    g_tempSettings.patches.skipIntro = IsDlgButtonChecked(hDlg, IDC_CHECK_SKIP_INTRO) == BST_CHECKED;
    g_tempSettings.patches.fastText = IsDlgButtonChecked(hDlg, IDC_CHECK_FAST_TEXT) == BST_CHECKED;
    g_tempSettings.patches.instantMap = IsDlgButtonChecked(hDlg, IDC_CHECK_INSTANT_MAP) == BST_CHECKED;
    g_tempSettings.patches.fixSoftlocks = IsDlgButtonChecked(hDlg, IDC_CHECK_FIX_SOFTLOCK) == BST_CHECKED;
    g_tempSettings.patches.fixCollision = IsDlgButtonChecked(hDlg, IDC_CHECK_FIX_COLLISION) == BST_CHECKED;
    g_tempSettings.patches.fixCameraClip = IsDlgButtonChecked(hDlg, IDC_CHECK_FIX_CAMERA) == BST_CHECKED;

    // Performance tab
    g_tempSettings.threading.enableAsyncGraphics = IsDlgButtonChecked(hDlg, IDC_CHECK_PERF_THREADED) == BST_CHECKED;
    g_tempSettings.threading.enableAsyncAudio = IsDlgButtonChecked(hDlg, IDC_CHECK_PERF_ASYNC_AUDIO) == BST_CHECKED;
    g_tempSettings.threading.enablePerfOverlay = IsDlgButtonChecked(hDlg, IDC_CHECK_PERF_OVERLAY) == BST_CHECKED;
    g_tempSettings.threading.perfOverlayMode = (int)SendDlgItemMessage(hDlg, IDC_COMBO_PERF_OVERLAY_MODE, CB_GETCURSEL, 0, 0);
    
    // Cheats tab
    g_tempSettings.cheats.enableCameraPatch = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_CAMERA_PATCH) == BST_CHECKED;
    g_tempSettings.cheats.enableLagReduction = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_LAG_REDUCTION) == BST_CHECKED;
    g_tempSettings.cheats.enableForestFix = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_FOREST_FIX) == BST_CHECKED;
    g_tempSettings.cheats.enableInfiniteHealth = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_INF_HEALTH) == BST_CHECKED;
    g_tempSettings.cheats.enableInfiniteSubweapon = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_INF_SUBWEAPON) == BST_CHECKED;
    g_tempSettings.cheats.enableInfiniteGold = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_INF_GOLD) == BST_CHECKED;
    g_tempSettings.cheats.enableMoonJump = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_MOON_JUMP) == BST_CHECKED;
    g_tempSettings.cheats.enableNoClip = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_NO_CLIP) == BST_CHECKED;
    g_tempSettings.cheats.enableSpeedBoost = IsDlgButtonChecked(hDlg, IDC_CHECK_CHEAT_SPEED_BOOST) == BST_CHECKED;
}

static void UpdateSliderLabels(HWND hDlg) {
    wchar_t buf[32];
    
    int scale = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_SCALE, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%dx", scale);
    SetDlgItemTextW(hDlg, IDC_STATIC_SCALE, buf);
    
    int vol = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_MASTER, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d%%", vol);
    SetDlgItemTextW(hDlg, IDC_STATIC_MASTER, buf);
    
    vol = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_MUSIC, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d%%", vol);
    SetDlgItemTextW(hDlg, IDC_STATIC_MUSIC, buf);
    
    vol = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_SFX, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d%%", vol);
    SetDlgItemTextW(hDlg, IDC_STATIC_SFX, buf);
    
    float sens = SendDlgItemMessage(hDlg, IDC_SLIDER_DPADSPEED, TBM_GETPOS, 0, 0) / 10.0f;
    swprintf_s(buf, L"%.1f", sens);
    SetDlgItemTextW(hDlg, IDC_STATIC_DPADSPEED, buf);
    
    sens = SendDlgItemMessage(hDlg, IDC_SLIDER_STICKX, TBM_GETPOS, 0, 0) / 10.0f;
    swprintf_s(buf, L"%.1f", sens);
    SetDlgItemTextW(hDlg, IDC_STATIC_STICKX, buf);
    
    sens = SendDlgItemMessage(hDlg, IDC_SLIDER_STICKY, TBM_GETPOS, 0, 0) / 10.0f;
    swprintf_s(buf, L"%.1f", sens);
    SetDlgItemTextW(hDlg, IDC_STATIC_STICKY, buf);
    
    int dz = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_L, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d%%", dz);
    SetDlgItemTextW(hDlg, IDC_STATIC_DEADZONE_L, buf);
    
    dz = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_DEADZONE_R, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d%%", dz);
    SetDlgItemTextW(hDlg, IDC_STATIC_DEADZONE_R, buf);

    // Performance tab - texture cache
    int texCache = (int)SendDlgItemMessage(hDlg, IDC_SLIDER_PERF_TEXCACHE, TBM_GETPOS, 0, 0);
    swprintf_s(buf, L"%d MB", texCache);
    SetDlgItemTextW(hDlg, IDC_STATIC_PERF_TEXCACHE, buf);
}

// Apply settings to running emulation
static void ApplySettingsToEmulation(bool showMessage) {
    // This function applies settings that can be changed at runtime
    // Note: Some settings like resolution require restart
    
    OutputDebugStringA("[CV64] Applying settings to emulation...\n");
    
    // Reload configuration from INI files into mupen64plus/GLideN64
    if (CV64_Config_ApplyAll()) {
        OutputDebugStringA("[CV64] Configuration reloaded from INI files successfully\n");
    } else {
        OutputDebugStringA("[CV64] WARNING: Failed to reload configuration\n");
    }
    
    // Cheats are disabled
    OutputDebugStringA("[CV64] Settings applied (cheats disabled)\n");
    
    // Camera settings can be applied immediately
    // These would call into the camera patch system
    // CV64_CameraPatch_SetEnabled(g_settings.patches.dpadCamera);
    // CV64_CameraPatch_SetSensitivity(g_settings.camera.dpadSpeed, ...);

    // Animation interpolation toggle
    {
        bool interpEnabled = g_settings.patches.animInterpolation;
        CV64_AnimInterpConfig* cfg = CV64_AnimInterp_GetConfig();
        if (cfg) {
            cfg->enabled = interpEnabled;
        }
        CV64_AnimBridge_SetEnabled(interpEnabled);
        OutputDebugStringA(interpEnabled
            ? "[CV64] Animation interpolation enabled\n"
            : "[CV64] Animation interpolation disabled\n");
    }
    
    // Audio volume could be applied immediately if we have access to the audio system
    // CV64_Audio_SetVolume(g_settings.audio.masterVolume / 100.0f);
    
    OutputDebugStringA("[CV64] Settings applied to CASTLEVANIA.\n");
    
    if (showMessage) {
        MessageBoxA(NULL, 
            "Settings have been applied to CASTLEVANIA.\n\n"
            "Most changes take effect immediately.\n"
            "Some settings (like resolution) may require\n"
            "restarting the game to take full effect.\n\n"
            "Press F4 to hard reset if needed.",
            "Settings Applied", MB_OK | MB_ICONINFORMATION);
    }
}

static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
switch (message) {
case WM_INITDIALOG:
{
    g_hSettingsDlg = hDlg;
        
    // Enable dark mode for title bar only (full dark mode looks bad with many controls)
    EnableDarkModeTitleBarOnly(hDlg);
        
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
        
        g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        
        // Create tab control - LARGER SIZE for better layout
        g_hTabCtrl = CreateWindowW(WC_TABCONTROLW, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            10, 10, 500, 440, hDlg, (HMENU)IDC_TAB_CONTROL, g_hInst, NULL);
        SendMessage(g_hTabCtrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        
        // Add tabs - include Performance tab (Post FX removed)
        TCITEMW tie = { 0 };
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"Display"; TabCtrl_InsertItem(g_hTabCtrl, 0, &tie);
        tie.pszText = (LPWSTR)L"Audio"; TabCtrl_InsertItem(g_hTabCtrl, 1, &tie);
        tie.pszText = (LPWSTR)L"Camera"; TabCtrl_InsertItem(g_hTabCtrl, 2, &tie);
        tie.pszText = (LPWSTR)L"Controls"; TabCtrl_InsertItem(g_hTabCtrl, 3, &tie);
        tie.pszText = (LPWSTR)L"Gameplay"; TabCtrl_InsertItem(g_hTabCtrl, 4, &tie);
        tie.pszText = (LPWSTR)L"Performance"; TabCtrl_InsertItem(g_hTabCtrl, 5, &tie);
        // Cheats tab disabled
        
        // Create all controls
        CreateAllControls(hDlg);
        
        // Buttons - repositioned for larger dialog with Apply button
        HWND btn = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            140, 460, 80, 28, hDlg, (HMENU)IDC_BTN_OK, g_hInst, NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        btn = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE,
            230, 460, 80, 28, hDlg, (HMENU)IDC_BTN_APPLY, g_hInst, NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        btn = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
            320, 460, 80, 28, hDlg, (HMENU)IDC_BTN_CANCEL, g_hInst, NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        btn = CreateWindowW(L"BUTTON", L"Defaults", WS_CHILD | WS_VISIBLE,
            410, 460, 80, 28, hDlg, (HMENU)IDC_BTN_DEFAULTS, g_hInst, NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        
        // Load settings and show first tab
        g_tempSettings = g_settings;
        LoadSettingsToUI(hDlg);
        ShowTabPage(0);
        
        // Center dialog on parent
        HWND hParent = GetParent(hDlg);
        if (hParent) {
            RECT rcParent, rcDlg;
            GetWindowRect(hParent, &rcParent);
            GetWindowRect(hDlg, &rcDlg);
            int x = rcParent.left + ((rcParent.right - rcParent.left) - (rcDlg.right - rcDlg.left)) / 2;
            int y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcDlg.bottom - rcDlg.top)) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        
        return TRUE;
    }
    
    case WM_NOTIFY:
    {
        NMHDR* pnmh = (NMHDR*)lParam;
        if (pnmh->hwndFrom == g_hTabCtrl && pnmh->code == TCN_SELCHANGE) {
            ShowTabPage(TabCtrl_GetCurSel(g_hTabCtrl));
        }
        break;
    }
    
    case WM_HSCROLL:
        UpdateSliderLabels(hDlg);
        break;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_OK:
            SaveUIToSettings(hDlg);
            g_settings = g_tempSettings;
            CV64_Settings_SaveAll();
            ApplySettingsToEmulation(true);
            EndDialog(hDlg, IDOK);
            return TRUE;
            
        case IDC_BTN_APPLY:
            // Apply without closing dialog - for live preview
            SaveUIToSettings(hDlg);
            g_settings = g_tempSettings;
            CV64_Settings_SaveAll();
            ApplySettingsToEmulation(false);
            return TRUE;
            
        case IDC_BTN_CANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
            
        case IDC_BTN_DEFAULTS:
            if (MessageBoxW(hDlg, L"Reset all settings to defaults?", L"Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                CV64_Settings_ResetToDefaults();
                g_tempSettings = g_settings;
                LoadSettingsToUI(hDlg);
            }
            return TRUE;
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    
    return FALSE;
}

bool CV64_Settings_ShowDialog(HWND hParent) {
    // Create dialog template in memory
    struct {
        DLGTEMPLATE dlg;
        WORD menu;
        WORD windowClass;
        WCHAR title[32];
    } dlgTemplate = { 0 };
    
    dlgTemplate.dlg.style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlgTemplate.dlg.cdit = 0;
    dlgTemplate.dlg.x = 0;
    dlgTemplate.dlg.y = 0;
    dlgTemplate.dlg.cx = 270; // Dialog units (~540 pixels width)
    dlgTemplate.dlg.cy = 270; // Dialog units (~540 pixels height)
    wcscpy_s(dlgTemplate.title, L"CASTLEVANIA 64 - Settings");
    
    return DialogBoxIndirectW(g_hInst, &dlgTemplate.dlg, hParent, SettingsDlgProc) == IDOK;
}
