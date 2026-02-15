/**
 * @file cv64_config_bridge.cpp
 * @brief CV64 INI to mupen64plus/GLideN64 Configuration Bridge
 * 
 * This module bridges CV64's INI configuration files to the mupen64plus core
 * and GLideN64 plugin configuration systems. It ensures that our custom INI
 * settings are applied instead of the default mupen64plus.cfg or GLideN64.ini.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef CV64_STATIC_MUPEN64PLUS

#include "../include/cv64_ini_parser.h"
#include <Windows.h>
#include <string>
#include <filesystem>
#include <cstring>

/*===========================================================================
 * External Core Config API (linked from mupen64plus-core-static.lib)
 *===========================================================================*/

extern "C" {
    typedef void* m64p_handle;
    typedef int m64p_error;
    typedef int m64p_type;
    
    #define M64TYPE_INT    1
    #define M64TYPE_FLOAT  2
    #define M64TYPE_BOOL   3
    #define M64TYPE_STRING 4
    
    m64p_error __cdecl ConfigOpenSection(const char*, m64p_handle*);
    m64p_error __cdecl ConfigSetParameter(m64p_handle, const char*, m64p_type, const void*);
    m64p_error __cdecl ConfigSetDefaultInt(m64p_handle, const char*, int, const char*);
    m64p_error __cdecl ConfigSetDefaultFloat(m64p_handle, const char*, float, const char*);
    m64p_error __cdecl ConfigSetDefaultBool(m64p_handle, const char*, int, const char*);
    m64p_error __cdecl ConfigSetDefaultString(m64p_handle, const char*, const char*, const char*);
    m64p_error __cdecl ConfigSaveFile(void);
    m64p_error __cdecl ConfigSaveSection(const char*);
    
    /* Video extension function */
    void CV64_VidExt_GetSize(int* width, int* height);
}

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static CV64_IniParser s_graphicsIni;
static CV64_IniParser s_controlsIni;
static CV64_IniParser s_audioIni;
static CV64_IniParser s_patchesIni;
static bool s_configLoaded = false;
static std::filesystem::path s_exeDir;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void ConfigLog(const char* msg) {
    OutputDebugStringA("[CV64_CONFIG] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

static std::filesystem::path GetExeDirectory() {
    if (s_exeDir.empty()) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        s_exeDir = std::filesystem::path(path).parent_path();
    }
    return s_exeDir;
}

static std::string GetAbsolutePath(const std::string& relativePath) {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        /* Convert to native path format (backslashes on Windows) */
        return path.make_preferred().string();
    }
    std::filesystem::path fullPath = GetExeDirectory() / path;
    /* Convert to native path format (backslashes on Windows) */
    return fullPath.make_preferred().string();
}

static bool EnsureDirectoryExists(const std::filesystem::path& dir) {
    if (std::filesystem::exists(dir)) {
        return std::filesystem::is_directory(dir);
    }
    
    std::error_code ec;
    if (std::filesystem::create_directories(dir, ec)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Created directory: %s", dir.string().c_str());
        ConfigLog(msg);
        return true;
    } else {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to create directory: %s (error: %s)", 
                 dir.string().c_str(), ec.message().c_str());
        ConfigLog(msg);
        return false;
    }
}

/*===========================================================================
 * CV64 INI Loading
 *===========================================================================*/

static bool CreateDefaultGraphicsIni(const std::string& path) {
    ConfigLog("Creating default cv64_graphics.ini...");
    
    CV64_IniParser defaultIni;
    
    /* Display */
    defaultIni.SetString("Display", "backend", "AUTO");
    defaultIni.SetString("Display", "resolution", "720P");
    defaultIni.SetInt("Display", "custom_width", 1280);
    defaultIni.SetInt("Display", "custom_height", 720);
    defaultIni.SetBool("Display", "fullscreen", false);
    defaultIni.SetBool("Display", "borderless", false);
    defaultIni.SetInt("Display", "refresh_rate", 60);
    
    /* Aspect Ratio - Use stretch mode (0) by default so game fills window when resized */
    defaultIni.SetString("AspectRatio", "ratio", "16_9");
    defaultIni.SetInt("AspectRatio", "aspect_mode", 0);  /* 0=stretch to fill window */
    defaultIni.SetBool("AspectRatio", "stretch", true);
    
    /* Quality */
    defaultIni.SetInt("Quality", "internal_scale", 3);
    defaultIni.SetInt("Quality", "native_res_factor", 3);
    defaultIni.SetString("Quality", "texture_filter", "LINEAR");
    defaultIni.SetInt("Quality", "bilinear_mode", 1);
    defaultIni.SetInt("Quality", "anisotropy", 16);
    defaultIni.SetString("Quality", "anti_aliasing", "FXAA");
    defaultIni.SetInt("Quality", "multisampling", 0);
    defaultIni.SetBool("Quality", "fxaa", true);
    defaultIni.SetBool("Quality", "vsync", true);
    defaultIni.SetInt("Quality", "max_fps", 60);
    
     /* Frame Buffer */
    defaultIni.SetBool("FrameBuffer", "enable_fb_emulation", true);
    defaultIni.SetInt("FrameBuffer", "buffer_swap_mode", 0);
    defaultIni.SetInt("FrameBuffer", "copy_color_to_rdram", 2);   // 2 = Double buffer (async)
    defaultIni.SetInt("FrameBuffer", "copy_depth_to_rdram", 0);   // 0 = Off (performance — CV64 doesn't read depth back)
    defaultIni.SetBool("FrameBuffer", "copy_from_rdram", false);
    defaultIni.SetBool("FrameBuffer", "copy_aux_to_rdram", false);
    defaultIni.SetInt("FrameBuffer", "n64_depth_compare", 0);     // 0 = Off (CV64 doesn't use HW depth compare)
    defaultIni.SetBool("FrameBuffer", "force_depth_clear", false);
    defaultIni.SetBool("FrameBuffer", "disable_fbinfo", false);

     /* Performance - RDP Optimizations */
    defaultIni.SetBool("Performance", "threaded_video", true);
    defaultIni.SetBool("Performance", "shader_storage", true);
    defaultIni.SetInt("Performance", "texture_cache_mb", 512);
    defaultIni.SetBool("Performance", "fragment_depth_write", false);  // OFF for performance
    defaultIni.SetBool("Performance", "dithering_pattern", false);
    defaultIni.SetBool("Performance", "hires_noise_dithering", false);
    defaultIni.SetInt("Performance", "rdram_dithering_mode", 0);
    defaultIni.SetBool("Performance", "coverage", false);
    defaultIni.SetBool("Performance", "hw_lighting", false);
    defaultIni.SetBool("Performance", "inaccurate_texcoords", true);

     /* Enhancements */
    defaultIni.SetBool("Enhancements", "enable_hd_textures", true);
    defaultIni.SetBool("Enhancements", "tx_hires_enable", true);
    defaultIni.SetBool("Enhancements", "tx_hires_full_alpha", true);
    defaultIni.SetBool("Enhancements", "enable_enhanced_lighting", false);
    defaultIni.SetBool("Enhancements", "enable_fog", true);
    defaultIni.SetBool("Enhancements", "enable_ambient_occlusion", false);

    /* Texture Filter */
    defaultIni.SetInt("TextureFilter", "tx_filter_mode", 0);
    defaultIni.SetInt("TextureFilter", "tx_enhancement_mode", 3);  // 3 = xBRZ 2x
    defaultIni.SetBool("TextureFilter", "tx_deposterize", true);   // Remove color banding
    defaultIni.SetBool("TextureFilter", "tx_filter_ignore_bg", false);
    defaultIni.SetInt("TextureFilter", "tx_cache_size", 512);  // Large cache for performance
    defaultIni.SetBool("TextureFilter", "tx_cache_compression", true);
    defaultIni.SetBool("TextureFilter", "tx_save_cache", true);
    defaultIni.SetBool("TextureFilter", "tx_enhanced_file_storage", false);
    defaultIni.SetBool("TextureFilter", "tx_hires_file_storage", true);
    defaultIni.SetInt("TextureFilter", "tx_hires_vram_limit", 0);
    /* Enable alternate CRC by default - required for Rice format HD texture packs */
    defaultIni.SetBool("TextureFilter", "tx_hires_alt_crc", true);
    /* Enable strong CRC for better texture matching */
    defaultIni.SetBool("TextureFilter", "tx_strong_crc", true);
    
    /* Texture Paths - use backslashes for Windows compatibility */
    defaultIni.SetString("TexturePaths", "tx_path", "assets\\hires_texture");
    defaultIni.SetString("TexturePaths", "tx_cache_path", "assets\\cache");
    defaultIni.SetString("TexturePaths", "tx_dump_path", "assets\\texture_dump");
    
    /* Color Correction */
    defaultIni.SetFloat("ColorCorrection", "gamma", 1.0f);
    defaultIni.SetFloat("ColorCorrection", "brightness", 0.0f);
    defaultIni.SetFloat("ColorCorrection", "contrast", 1.0f);
    defaultIni.SetBool("ColorCorrection", "force_gamma", false);
    defaultIni.SetFloat("ColorCorrection", "gamma_level", 1.0f);
    
    /* N64 Accuracy */
    defaultIni.SetBool("N64Accuracy", "accurate_n64_blending", false);
    defaultIni.SetBool("N64Accuracy", "accurate_n64_coverage", false);
    defaultIni.SetBool("N64Accuracy", "enable_coverage", false);
    defaultIni.SetBool("N64Accuracy", "enable_n64_dither", false);
    defaultIni.SetBool("N64Accuracy", "enable_dithering_pattern", false);
    defaultIni.SetBool("N64Accuracy", "enable_dithering_quantization", true);
    defaultIni.SetBool("N64Accuracy", "force_3point_filter", false);
    defaultIni.SetBool("N64Accuracy", "enable_lod", true);
    defaultIni.SetBool("N64Accuracy", "enable_hw_lighting", false);
    defaultIni.SetBool("N64Accuracy", "enable_clipping", true);
    defaultIni.SetBool("N64Accuracy", "enable_legacy_blending", false);
    defaultIni.SetBool("N64Accuracy", "enable_hybrid_filter", true);
    defaultIni.SetBool("N64Accuracy", "enable_inaccurate_texcoords", true);
    
    /* 2D Graphics */
    defaultIni.SetInt("2DGraphics", "correct_texrect_coords", 1);
    defaultIni.SetInt("2DGraphics", "native_res_texrects", 0);
    defaultIni.SetInt("2DGraphics", "backgrounds_mode", 1);
    defaultIni.SetBool("2DGraphics", "enable_texcoord_bounds", false);
    
    /* OSD */
    defaultIni.SetBool("OSD", "show_fps", false);
    defaultIni.SetBool("OSD", "show_vis", false);
    defaultIni.SetBool("OSD", "show_percent", false);
    defaultIni.SetBool("OSD", "show_internal_resolution", false);
    defaultIni.SetBool("OSD", "show_rendering_resolution", false);
    defaultIni.SetBool("OSD", "show_statistics", false);
    defaultIni.SetInt("OSD", "counters_position", 1);
    
    /* Overscan */
    defaultIni.SetBool("Overscan", "enable_overscan", false);
    defaultIni.SetInt("Overscan", "overscan_ntsc_left", 0);
    defaultIni.SetInt("Overscan", "overscan_ntsc_right", 0);
    defaultIni.SetInt("Overscan", "overscan_ntsc_top", 0);
    defaultIni.SetInt("Overscan", "overscan_ntsc_bottom", 0);
    defaultIni.SetInt("Overscan", "overscan_pal_left", 0);
    defaultIni.SetInt("Overscan", "overscan_pal_right", 0);
    defaultIni.SetInt("Overscan", "overscan_pal_top", 0);
    defaultIni.SetInt("Overscan", "overscan_pal_bottom", 0);
    
    return defaultIni.Save(path.c_str());
}

static bool CreateDefaultAudioIni(const std::string& path) {
    ConfigLog("Creating default cv64_audio.ini...");
    
    CV64_IniParser defaultIni;
    
    /* Audio */
    defaultIni.SetInt("Audio", "volume", 100);
    defaultIni.SetBool("Audio", "mute", false);
    defaultIni.SetInt("Audio", "buffer_size", 1024);
    defaultIni.SetInt("Audio", "sample_rate", 44100);
    
    return defaultIni.Save(path.c_str());
}

static bool CreateDefaultControlsIni(const std::string& path) {
    ConfigLog("Creating default cv64_controls.ini...");
    
    CV64_IniParser defaultIni;
    
    /* Controls */
    defaultIni.SetString("Controls", "device", "keyboard");
    defaultIni.SetBool("Controls", "rumble", false);
    
    return defaultIni.Save(path.c_str());
}

static bool CreateDefaultPatchesIni(const std::string& path) {
    ConfigLog("Creating default cv64_patches.ini...");
    
    CV64_IniParser defaultIni;
    
    /* Patches */
    defaultIni.SetBool("Patches", "camera_patch", true);
    defaultIni.SetBool("Patches", "widescreen", false);
    
    return defaultIni.Save(path.c_str());
}

bool CV64_Config_LoadAllInis() {
    if (s_configLoaded) {
        return true;
    }
    
    ConfigLog("Loading CV64 configuration files...");
    
    std::filesystem::path exeDir = GetExeDirectory();
    std::filesystem::path patchesDir = exeDir / "patches";
    
    /* Ensure patches directory exists */
    if (!EnsureDirectoryExists(patchesDir)) {
        ConfigLog("Error: Could not create patches directory");
        return false;
    }
    
    /* Ensure asset directories exist */
    std::filesystem::path assetsDir = exeDir / "assets";
    EnsureDirectoryExists(assetsDir);
    EnsureDirectoryExists(assetsDir / "hires_texture");
    EnsureDirectoryExists(assetsDir / "cache");
    EnsureDirectoryExists(assetsDir / "texture_dump");
    
    /* Load graphics configuration */
    std::string graphicsPath = (patchesDir / "cv64_graphics.ini").string();
    if (s_graphicsIni.Load(graphicsPath.c_str())) {
        ConfigLog("Loaded cv64_graphics.ini");
    } else {
        ConfigLog("cv64_graphics.ini not found, creating default...");
        if (CreateDefaultGraphicsIni(graphicsPath)) {
            ConfigLog("Created default cv64_graphics.ini");
            s_graphicsIni.Load(graphicsPath.c_str());
        } else {
            ConfigLog("Warning: Could not create cv64_graphics.ini, using hardcoded defaults");
        }
    }
    
    /* Load controls configuration */
    std::string controlsPath = (patchesDir / "cv64_controls.ini").string();
    if (s_controlsIni.Load(controlsPath.c_str())) {
        ConfigLog("Loaded cv64_controls.ini");
    } else {
        ConfigLog("cv64_controls.ini not found, creating default...");
        if (CreateDefaultControlsIni(controlsPath)) {
            ConfigLog("Created default cv64_controls.ini");
            s_controlsIni.Load(controlsPath.c_str());
        } else {
            ConfigLog("Warning: Could not create cv64_controls.ini, using hardcoded defaults");
        }
    }
    
    /* Load audio configuration */
    std::string audioPath = (patchesDir / "cv64_audio.ini").string();
    if (s_audioIni.Load(audioPath.c_str())) {
        ConfigLog("Loaded cv64_audio.ini");
    } else {
        ConfigLog("cv64_audio.ini not found, creating default...");
        if (CreateDefaultAudioIni(audioPath)) {
            ConfigLog("Created default cv64_audio.ini");
            s_audioIni.Load(audioPath.c_str());
        } else {
            ConfigLog("Warning: Could not create cv64_audio.ini, using hardcoded defaults");
        }
    }
    
    /* Load patches configuration */
    std::string patchesPath = (patchesDir / "cv64_patches.ini").string();
    if (s_patchesIni.Load(patchesPath.c_str())) {
        ConfigLog("Loaded cv64_patches.ini");
    } else {
        ConfigLog("cv64_patches.ini not found, creating default...");
        if (CreateDefaultPatchesIni(patchesPath)) {
            ConfigLog("Created default cv64_patches.ini");
            s_patchesIni.Load(patchesPath.c_str());
        } else {
            ConfigLog("Warning: Could not create cv64_patches.ini, using hardcoded defaults");
        }
    }
    
    s_configLoaded = true;
    return true;
}

/*===========================================================================
 * Apply Configuration to mupen64plus Core
 *===========================================================================*/

bool CV64_Config_ApplyToCore() {
    ConfigLog("Applying CV64 config to mupen64plus core...");
    
    m64p_handle coreSection = nullptr;
    if (ConfigOpenSection("Core", &coreSection) != 0 || coreSection == nullptr) {
        ConfigLog("Error: Failed to open Core config section");
        return false;
    }
    
    /* Apply core settings */
    int r4300Emulator = 2; /* 0=pure interp, 1=cached interp, 2=dynarec */
    ConfigSetParameter(coreSection, "R4300Emulator", M64TYPE_INT, &r4300Emulator);
    
    /* CRITICAL: Enable 8MB Expansion Pak - required for CV64 hooks!
     * DisableExtraMem=0 means USE extra memory (8MB total)
     * DisableExtraMem=1 means only 4MB (our hooks will partially fail)
     * CV64's Gameshark cheats use addresses like 0x80387ACE which need 8MB */
    int disableExtraMem = 0; /* 0 = Enable 8MB Expansion Pak */
    ConfigSetParameter(coreSection, "DisableExtraMem", M64TYPE_BOOL, &disableExtraMem);
    ConfigLog("DisableExtraMem=0 (8MB Expansion Pak ENABLED for CV64 hooks)");
    
    /* Overclock the N64 CPU by factor of 2
     * CountPerOp controls how many cycles the CPU executes per operation
     * 0 = auto, 1 = normal speed, 2 = 2x overclock, 3 = 3x overclock, etc.
     * Higher values make the emulated CPU run faster, reducing lag */
    int countPerOp = 2; /* 2x overclock for smoother gameplay */
    ConfigSetParameter(coreSection, "CountPerOp", M64TYPE_INT, &countPerOp);
    ConfigLog("CountPerOp=2 (CPU overclocked by 2x)");
    
    int siDmaDuration = -1; /* Auto SI DMA duration */
    ConfigSetParameter(coreSection, "SiDmaDuration", M64TYPE_INT, &siDmaDuration);
    
    ConfigLog("Core configuration applied successfully");
    return true;
}

/*===========================================================================
 * Apply Configuration to GLideN64
 *===========================================================================*/

bool CV64_Config_ApplyToGLideN64() {
    ConfigLog("Applying CV64 config to GLideN64...");
    
    /* Open Video-General section */
    m64p_handle videoGeneral = nullptr;
    if (ConfigOpenSection("Video-General", &videoGeneral) != 0 || videoGeneral == nullptr) {
        ConfigLog("Error: Failed to open Video-General config section");
        return false;
    }
    
    /* Open Video-GLideN64 section */
    m64p_handle videoGliden64 = nullptr;
    if (ConfigOpenSection("Video-GLideN64", &videoGliden64) != 0 || videoGliden64 == nullptr) {
        ConfigLog("Error: Failed to open Video-GLideN64 config section");
        return false;
    }
    
    /* Apply Video-General settings from cv64_graphics.ini */
    bool fullscreen = s_graphicsIni.GetBool("Display", "fullscreen", false);
    
    /* Get actual window size if available, otherwise use INI defaults */
    int screenWidth = s_graphicsIni.GetInt("Display", "custom_width", 1280);
    int screenHeight = s_graphicsIni.GetInt("Display", "custom_height", 720);
    
    /* Try to get actual window dimensions from video extension */
    int vidExtWidth = 0, vidExtHeight = 0;
    CV64_VidExt_GetSize(&vidExtWidth, &vidExtHeight);
    if (vidExtWidth > 0 && vidExtHeight > 0) {
        screenWidth = vidExtWidth;
        screenHeight = vidExtHeight;
        char sizeMsg[128];
        snprintf(sizeMsg, sizeof(sizeMsg), "Using actual window size for GLideN64: %dx%d", screenWidth, screenHeight);
        ConfigLog(sizeMsg);
    }
    
    bool vsync = s_graphicsIni.GetBool("Quality", "vsync", true);
    
    ConfigSetParameter(videoGeneral, "Fullscreen", M64TYPE_BOOL, &fullscreen);
    ConfigSetParameter(videoGeneral, "ScreenWidth", M64TYPE_INT, &screenWidth);
    ConfigSetParameter(videoGeneral, "ScreenHeight", M64TYPE_INT, &screenHeight);
    ConfigSetParameter(videoGeneral, "VerticalSync", M64TYPE_BOOL, &vsync);
    
    /* Apply GLideN64 settings from cv64_graphics.ini */
    
    /* Aspect ratio and resolution */
    /* Force aspect_mode to 0 (stretch to fill window) to prevent black bars
     * 0 = Stretch to fill window (no black bars)
     * 1 = Force 4:3 (pillarbox on widescreen)
     * 2 = Force 16:9 (letterbox on 4:3)
     * 3 = Adjust dynamically */
    int aspectMode = 0;  // Always use stretch mode
    int nativeResFactor = s_graphicsIni.GetInt("Quality", "native_res_factor", 3);
    int bufferSwapMode = s_graphicsIni.GetInt("FrameBuffer", "buffer_swap_mode", 0);
    
    char aspectMsg[128];
    snprintf(aspectMsg, sizeof(aspectMsg), "AspectRatio set to %d (0=stretch, 1=4:3, 2=16:9, 3=adjust)", aspectMode);
    ConfigLog(aspectMsg);
    
    ConfigSetParameter(videoGliden64, "AspectRatio", M64TYPE_INT, &aspectMode);
    ConfigSetParameter(videoGliden64, "UseNativeResolutionFactor", M64TYPE_INT, &nativeResFactor);
    ConfigSetParameter(videoGliden64, "BufferSwapMode", M64TYPE_INT, &bufferSwapMode);
    
    /* Anti-aliasing */
    int multisampling = s_graphicsIni.GetInt("Quality", "multisampling", 0);
    bool fxaa = s_graphicsIni.GetBool("Quality", "fxaa", true);
    ConfigSetParameter(videoGliden64, "MultiSampling", M64TYPE_INT, &multisampling);
    ConfigSetParameter(videoGliden64, "FXAA", M64TYPE_BOOL, &fxaa);
    
    /* Texture settings */
    int bilinearMode = s_graphicsIni.GetInt("Quality", "bilinear_mode", 1);
    int anisotropy = s_graphicsIni.GetInt("Quality", "anisotropy", 16);
    ConfigSetParameter(videoGliden64, "bilinearMode", M64TYPE_INT, &bilinearMode);
    ConfigSetParameter(videoGliden64, "anisotropy", M64TYPE_INT, &anisotropy);
    
    /* Frame buffer emulation */
    bool enableFB = s_graphicsIni.GetBool("FrameBuffer", "enable_fb_emulation", true);
    int copyColorToRdram = s_graphicsIni.GetInt("FrameBuffer", "copy_color_to_rdram", 2);
    int copyDepthToRdram = s_graphicsIni.GetInt("FrameBuffer", "copy_depth_to_rdram", 0);
    bool copyFromRdram = s_graphicsIni.GetBool("FrameBuffer", "copy_from_rdram", false);
    bool copyAuxToRdram = s_graphicsIni.GetBool("FrameBuffer", "copy_aux_to_rdram", false);
    int n64DepthCompare = s_graphicsIni.GetInt("FrameBuffer", "n64_depth_compare", 0);
    bool forceDepthClear = s_graphicsIni.GetBool("FrameBuffer", "force_depth_clear", false);
    bool disableFbInfo = s_graphicsIni.GetBool("FrameBuffer", "disable_fbinfo", false);
    
    ConfigSetParameter(videoGliden64, "EnableFBEmulation", M64TYPE_BOOL, &enableFB);
    ConfigSetParameter(videoGliden64, "EnableCopyColorToRDRAM", M64TYPE_INT, &copyColorToRdram);
    ConfigSetParameter(videoGliden64, "EnableCopyDepthToRDRAM", M64TYPE_INT, &copyDepthToRdram);
    ConfigSetParameter(videoGliden64, "EnableCopyColorFromRDRAM", M64TYPE_BOOL, &copyFromRdram);
    ConfigSetParameter(videoGliden64, "EnableCopyAuxiliaryToRDRAM", M64TYPE_BOOL, &copyAuxToRdram);
    ConfigSetParameter(videoGliden64, "EnableN64DepthCompare", M64TYPE_INT, &n64DepthCompare);
    ConfigSetParameter(videoGliden64, "ForceDepthBufferClear", M64TYPE_BOOL, &forceDepthClear);
    ConfigSetParameter(videoGliden64, "DisableFBInfo", M64TYPE_BOOL, &disableFbInfo);
    
    /* N64 Accuracy settings */
    bool enableLod = s_graphicsIni.GetBool("N64Accuracy", "enable_lod", true);
    bool enableCoverage = s_graphicsIni.GetBool("N64Accuracy", "enable_coverage", false);
    bool enableClipping = s_graphicsIni.GetBool("N64Accuracy", "enable_clipping", true);
    bool enableDitheringPattern = s_graphicsIni.GetBool("N64Accuracy", "enable_dithering_pattern", false);
    bool enableDitheringQuant = s_graphicsIni.GetBool("N64Accuracy", "enable_dithering_quantization", true);
    bool enableHwLighting = s_graphicsIni.GetBool("N64Accuracy", "enable_hw_lighting", false);
    bool enableLegacyBlending = s_graphicsIni.GetBool("N64Accuracy", "enable_legacy_blending", false);
    bool enableHybridFilter = s_graphicsIni.GetBool("N64Accuracy", "enable_hybrid_filter", true);
    bool enableInaccurateTexcoords = s_graphicsIni.GetBool("N64Accuracy", "enable_inaccurate_texcoords", true);
    
    ConfigSetParameter(videoGliden64, "EnableLOD", M64TYPE_BOOL, &enableLod);
    ConfigSetParameter(videoGliden64, "EnableCoverage", M64TYPE_BOOL, &enableCoverage);
    ConfigSetParameter(videoGliden64, "EnableClipping", M64TYPE_BOOL, &enableClipping);
    ConfigSetParameter(videoGliden64, "EnableDitheringPattern", M64TYPE_BOOL, &enableDitheringPattern);
    ConfigSetParameter(videoGliden64, "DitheringQuantization", M64TYPE_BOOL, &enableDitheringQuant);
    ConfigSetParameter(videoGliden64, "EnableHWLighting", M64TYPE_BOOL, &enableHwLighting);
    ConfigSetParameter(videoGliden64, "EnableLegacyBlending", M64TYPE_BOOL, &enableLegacyBlending);
    ConfigSetParameter(videoGliden64, "EnableHybridFilter", M64TYPE_BOOL, &enableHybridFilter);
    ConfigSetParameter(videoGliden64, "EnableInaccurateTextureCoordinates", M64TYPE_BOOL, &enableInaccurateTexcoords);
    
    /* 2D Graphics settings */
    int correctTexrectCoords = s_graphicsIni.GetInt("2DGraphics", "correct_texrect_coords", 1);
    int nativeResTexrects = s_graphicsIni.GetInt("2DGraphics", "native_res_texrects", 0);
    int bgMode = s_graphicsIni.GetInt("2DGraphics", "backgrounds_mode", 1);
    bool enableTexcoordBounds = s_graphicsIni.GetBool("2DGraphics", "enable_texcoord_bounds", false);
    
    ConfigSetParameter(videoGliden64, "CorrectTexrectCoords", M64TYPE_INT, &correctTexrectCoords);
    ConfigSetParameter(videoGliden64, "EnableNativeResTexrects", M64TYPE_INT, &nativeResTexrects);
    ConfigSetParameter(videoGliden64, "BackgroundsMode", M64TYPE_INT, &bgMode);
    ConfigSetParameter(videoGliden64, "EnableTexCoordBounds", M64TYPE_BOOL, &enableTexcoordBounds);
    
    /* HD Texture settings - CRITICAL FOR HD TEXTURES */
    bool txHiresEnable = s_graphicsIni.GetBool("Enhancements", "tx_hires_enable", true);
    bool txHiresFullAlpha = s_graphicsIni.GetBool("Enhancements", "tx_hires_full_alpha", true);
    /* Enable alternate CRC by default - required for many HD texture packs including Rice format */
    bool txHiresAltCrc = s_graphicsIni.GetBool("TextureFilter", "tx_hires_alt_crc", true);
    int txFilterMode = s_graphicsIni.GetInt("TextureFilter", "tx_filter_mode", 0);
    int txEnhancementMode = s_graphicsIni.GetInt("TextureFilter", "tx_enhancement_mode", 3);
    bool txDeposterize = s_graphicsIni.GetBool("TextureFilter", "tx_deposterize", true);
    bool txFilterIgnoreBg = s_graphicsIni.GetBool("TextureFilter", "tx_filter_ignore_bg", false);
    int txCacheSize = s_graphicsIni.GetInt("TextureFilter", "tx_cache_size", 512);
    bool txCacheCompression = s_graphicsIni.GetBool("TextureFilter", "tx_cache_compression", true);
    bool txSaveCache = s_graphicsIni.GetBool("TextureFilter", "tx_save_cache", true);
    bool txEnhancedFileStorage = s_graphicsIni.GetBool("TextureFilter", "tx_enhanced_file_storage", false);
    bool txHiresFileStorage = s_graphicsIni.GetBool("TextureFilter", "tx_hires_file_storage", true);
    int txHiresVramLimit = s_graphicsIni.GetInt("TextureFilter", "tx_hires_vram_limit", 0);
    
    ConfigSetParameter(videoGliden64, "txHiresEnable", M64TYPE_BOOL, &txHiresEnable);
    ConfigSetParameter(videoGliden64, "txHiresFullAlphaChannel", M64TYPE_BOOL, &txHiresFullAlpha);
    ConfigSetParameter(videoGliden64, "txHresAltCRC", M64TYPE_BOOL, &txHiresAltCrc);
    ConfigSetParameter(videoGliden64, "txFilterMode", M64TYPE_INT, &txFilterMode);
    ConfigSetParameter(videoGliden64, "txEnhancementMode", M64TYPE_INT, &txEnhancementMode);
    ConfigSetParameter(videoGliden64, "txDeposterize", M64TYPE_BOOL, &txDeposterize);
    ConfigSetParameter(videoGliden64, "txFilterIgnoreBG", M64TYPE_BOOL, &txFilterIgnoreBg);
    ConfigSetParameter(videoGliden64, "txCacheSize", M64TYPE_INT, &txCacheSize);
    ConfigSetParameter(videoGliden64, "txCacheCompression", M64TYPE_BOOL, &txCacheCompression);
    ConfigSetParameter(videoGliden64, "txSaveCache", M64TYPE_BOOL, &txSaveCache);
    ConfigSetParameter(videoGliden64, "txEnhancedTextureFileStorage", M64TYPE_BOOL, &txEnhancedFileStorage);
    ConfigSetParameter(videoGliden64, "txHiresTextureFileStorage", M64TYPE_BOOL, &txHiresFileStorage);
    ConfigSetParameter(videoGliden64, "txHiresVramLimit", M64TYPE_INT, &txHiresVramLimit);
    
    /* Force 16bpp OFF to allow full quality HD textures */
    bool txForce16bpp = false;
    ConfigSetParameter(videoGliden64, "txForce16bpp", M64TYPE_BOOL, &txForce16bpp);
    
    /* Enable strong CRC for better texture matching */
    bool txStrongCRC = s_graphicsIni.GetBool("TextureFilter", "tx_strong_crc", true);
    ConfigSetParameter(videoGliden64, "txStrongCRC", M64TYPE_BOOL, &txStrongCRC);
    
    /* HD Texture paths - CONVERT TO ABSOLUTE PATHS */
    std::string txPath = s_graphicsIni.GetString("TexturePaths", "tx_path", "assets/hires_texture");
    std::string txCachePath = s_graphicsIni.GetString("TexturePaths", "tx_cache_path", "assets/cache");
    std::string txDumpPath = s_graphicsIni.GetString("TexturePaths", "tx_dump_path", "assets/texture_dump");
    
    std::string txPathAbs = GetAbsolutePath(txPath);
    std::string txCachePathAbs = GetAbsolutePath(txCachePath);
    std::string txDumpPathAbs = GetAbsolutePath(txDumpPath);
    
    ConfigSetParameter(videoGliden64, "txPath", M64TYPE_STRING, txPathAbs.c_str());
    ConfigSetParameter(videoGliden64, "txCachePath", M64TYPE_STRING, txCachePathAbs.c_str());
    ConfigSetParameter(videoGliden64, "txDumpPath", M64TYPE_STRING, txDumpPathAbs.c_str());
    
    char logMsg[512];
    snprintf(logMsg, sizeof(logMsg), "HD Texture path: %s", txPathAbs.c_str());
    ConfigLog(logMsg);
    snprintf(logMsg, sizeof(logMsg), "Cache path: %s", txCachePathAbs.c_str());
    ConfigLog(logMsg);
    snprintf(logMsg, sizeof(logMsg), "HD Textures enabled: %s", txHiresEnable ? "YES" : "NO");
    ConfigLog(logMsg);
    snprintf(logMsg, sizeof(logMsg), "HD Texture Alt CRC: %s", txHiresAltCrc ? "YES" : "NO");
    ConfigLog(logMsg);
    snprintf(logMsg, sizeof(logMsg), "HD Texture Full Alpha: %s", txHiresFullAlpha ? "YES" : "NO");
    ConfigLog(logMsg);
    snprintf(logMsg, sizeof(logMsg), "HD Texture File Storage: %s", txHiresFileStorage ? "YES" : "NO");
    ConfigLog(logMsg);
    
    /* Log expected texture pack locations for Castlevania */
    snprintf(logMsg, sizeof(logMsg), "GLideN64 will look for textures in: %s\\CASTLEVANIA or %s\\CASTLEVANIA_HIRESTEXTURES", 
             txPathAbs.c_str(), txPathAbs.c_str());
    ConfigLog(logMsg);
    
    /* Gamma correction - Use 1.0 for neutral (no gamma boost) */
    bool forceGamma = s_graphicsIni.GetBool("ColorCorrection", "force_gamma", false);
    float gammaLevel = s_graphicsIni.GetFloat("ColorCorrection", "gamma_level", 1.0f);  // 1.0 = neutral, 2.0 = too bright
    ConfigSetParameter(videoGliden64, "ForceGammaCorrection", M64TYPE_BOOL, &forceGamma);
    ConfigSetParameter(videoGliden64, "GammaCorrectionLevel", M64TYPE_FLOAT, &gammaLevel);
    
    char gammaMsg[128];
    snprintf(gammaMsg, sizeof(gammaMsg), "Gamma level: %.2f (1.0=neutral, 2.0=bright)", gammaLevel);
    ConfigLog(gammaMsg);
    
    /* OSD settings */
    bool showFps = s_graphicsIni.GetBool("OSD", "show_fps", false);
    bool showVis = s_graphicsIni.GetBool("OSD", "show_vis", false);
    bool showPercent = s_graphicsIni.GetBool("OSD", "show_percent", false);
    bool showIntRes = s_graphicsIni.GetBool("OSD", "show_internal_resolution", false);
    bool showRenderRes = s_graphicsIni.GetBool("OSD", "show_rendering_resolution", false);
    int countersPos = s_graphicsIni.GetInt("OSD", "counters_position", 1);
    
    ConfigSetParameter(videoGliden64, "ShowFPS", M64TYPE_BOOL, &showFps);
    ConfigSetParameter(videoGliden64, "ShowVIS", M64TYPE_BOOL, &showVis);
    ConfigSetParameter(videoGliden64, "ShowPercent", M64TYPE_BOOL, &showPercent);
    ConfigSetParameter(videoGliden64, "ShowInternalResolution", M64TYPE_BOOL, &showIntRes);
    ConfigSetParameter(videoGliden64, "ShowRenderingResolution", M64TYPE_BOOL, &showRenderRes);
    ConfigSetParameter(videoGliden64, "CountersPos", M64TYPE_INT, &countersPos);
    
    /* Overscan settings */
    bool enableOverscan = s_graphicsIni.GetBool("Overscan", "enable_overscan", false);
    int overscanNtscLeft = s_graphicsIni.GetInt("Overscan", "overscan_ntsc_left", 0);
    int overscanNtscRight = s_graphicsIni.GetInt("Overscan", "overscan_ntsc_right", 0);
    int overscanNtscTop = s_graphicsIni.GetInt("Overscan", "overscan_ntsc_top", 0);
    int overscanNtscBottom = s_graphicsIni.GetInt("Overscan", "overscan_ntsc_bottom", 0);
    int overscanPalLeft = s_graphicsIni.GetInt("Overscan", "overscan_pal_left", 0);
    int overscanPalRight = s_graphicsIni.GetInt("Overscan", "overscan_pal_right", 0);
    int overscanPalTop = s_graphicsIni.GetInt("Overscan", "overscan_pal_top", 0);
    int overscanPalBottom = s_graphicsIni.GetInt("Overscan", "overscan_pal_bottom", 0);
    
    ConfigSetParameter(videoGliden64, "EnableOverscan", M64TYPE_BOOL, &enableOverscan);
    ConfigSetParameter(videoGliden64, "OverscanNtscLeft", M64TYPE_INT, &overscanNtscLeft);
    ConfigSetParameter(videoGliden64, "OverscanNtscRight", M64TYPE_INT, &overscanNtscRight);
    ConfigSetParameter(videoGliden64, "OverscanNtscTop", M64TYPE_INT, &overscanNtscTop);
    ConfigSetParameter(videoGliden64, "OverscanNtscBottom", M64TYPE_INT, &overscanNtscBottom);
    ConfigSetParameter(videoGliden64, "OverscanPalLeft", M64TYPE_INT, &overscanPalLeft);
    ConfigSetParameter(videoGliden64, "OverscanPalRight", M64TYPE_INT, &overscanPalRight);
    ConfigSetParameter(videoGliden64, "OverscanPalTop", M64TYPE_INT, &overscanPalTop);
    ConfigSetParameter(videoGliden64, "OverscanPalBottom", M64TYPE_INT, &overscanPalBottom);
    
    /* =========================================================================
     * CV64-SPECIFIC RDP PERFORMANCE OPTIMIZATIONS
     * These settings are tuned specifically for Castlevania 64's rendering patterns
     * to maximize performance without sacrificing visual quality.
     * =========================================================================*/

    ConfigLog("=== Applying CV64 RDP Performance Optimizations ===");

    /* -------------------------------------------------------------------------
     * OPTIMIZATION #1: THREADED VIDEO PROCESSING (HUGE IMPACT)
     * Offloads GL commands to a separate thread, nearly doubling performance
     * on multi-core systems by parallelizing CPU and GPU work.
     * -------------------------------------------------------------------------*/
    bool threadedVideo = true;
    ConfigSetParameter(videoGliden64, "ThreadedVideo", M64TYPE_BOOL, &threadedVideo);
    ConfigSetParameter(videoGeneral, "ThreadedVideo", M64TYPE_BOOL, &threadedVideo);
    ConfigLog("  [PERF] Threaded Video: ENABLED (major CPU/GPU parallelization)");
    
     /* -------------------------------------------------------------------------
      * OPTIMIZATION #2: FRAMEBUFFER COPY REDUCTION
      * RDRAM <-> GPU copies are expensive. Optimize while keeping FB effects.
      * - Color copy: Use double-buffer (async) instead of sync
      * - Depth copy: Disable (CV64 doesn't need depth readback)
      * - From RDRAM: Keep disabled (not used by CV64)
      * - Aux buffer: Disable (not needed)
      * This preserves fog/lighting effects while reducing copy overhead.
      * -------------------------------------------------------------------------*/
    int copyColorOptimized = 2;   // 2 = Double buffer (async, preserves effects)
    int copyDepthOptimized = 0;   // 0 = Disabled (performance)
    bool copyFromRdramOpt = false;
    bool copyAuxOpt = false;
    ConfigSetParameter(videoGliden64, "EnableCopyColorToRDRAM", M64TYPE_INT, &copyColorOptimized);
    ConfigSetParameter(videoGliden64, "EnableCopyDepthToRDRAM", M64TYPE_INT, &copyDepthOptimized);
    ConfigSetParameter(videoGliden64, "EnableCopyColorFromRDRAM", M64TYPE_BOOL, &copyFromRdramOpt);
    ConfigSetParameter(videoGliden64, "EnableCopyAuxiliaryToRDRAM", M64TYPE_BOOL, &copyAuxOpt);
    ConfigLog("  [PERF] FB Color Copy: DOUBLE-BUFFER (async, preserves effects)");
    ConfigLog("  [PERF] FB Depth Copy: DISABLED (major performance gain)");
    
    /* -------------------------------------------------------------------------
     * OPTIMIZATION #3: TEXTURE CACHE OPTIMIZATION
     * CV64 has many unique textures (castle walls, floors, enemies).
     * Large cache = fewer texture reloads = smoother gameplay.
     * 256MB is aggressive but prevents stuttering in texture-heavy areas.
     * -------------------------------------------------------------------------*/
    int txCacheSizeCV64 = 256;  // 256MB cache for CV64's texture variety
    ConfigSetParameter(videoGliden64, "txCacheSize", M64TYPE_INT, &txCacheSizeCV64);
    ConfigLog("  [PERF] Texture Cache: 256MB (prevents texture reload stutter)");
    
    /* Enable shader storage to eliminate shader compilation stutter */
    bool enableShadersStorage = true;
    ConfigSetParameter(videoGliden64, "EnableShadersStorage", M64TYPE_BOOL, &enableShadersStorage);
    ConfigLog("  [PERF] Shader Storage: ENABLED (eliminates shader stutter)");
    
      /* -------------------------------------------------------------------------
       * OPTIMIZATION #4: DISABLE EXPENSIVE RENDERING FEATURES
       * These features add visual accuracy but are costly. CV64 doesn't need them.
       * -------------------------------------------------------------------------*/

     /* Fragment Depth Write: Expensive per-pixel operation */
     bool fragmentDepthWrite = false;
     ConfigSetParameter(videoGliden64, "EnableFragmentDepthWrite", M64TYPE_BOOL, &fragmentDepthWrite);
     ConfigLog("  [PERF] Fragment Depth Write: DISABLED");

     /* Dithering Pattern: Visual nicety, costs GPU cycles */
     bool ditheringPattern = false;
     ConfigSetParameter(videoGliden64, "EnableDitheringPattern", M64TYPE_BOOL, &ditheringPattern);
     ConfigLog("  [PERF] Dithering Pattern: DISABLED");
    
    /* RDRAM Image Dithering: Skip blue noise dithering for RDRAM images */
    int rdramDitheringMode = 0;  // 0 = disabled
    ConfigSetParameter(videoGliden64, "RDRAMImageDitheringMode", M64TYPE_INT, &rdramDitheringMode);
    ConfigLog("  [PERF] RDRAM Dithering: DISABLED");
    
    /* Hi-res noise dithering: Disable for performance */
    bool hiresNoiseDithering = false;
    ConfigSetParameter(videoGliden64, "EnableHiresNoiseDithering", M64TYPE_BOOL, &hiresNoiseDithering);
    ConfigLog("  [PERF] Hi-res Noise Dithering: DISABLED");
    
    /* Coverage: AA coverage calculation is expensive */
    bool enableCoverageOpt = false;
    ConfigSetParameter(videoGliden64, "EnableCoverage", M64TYPE_BOOL, &enableCoverageOpt);
    ConfigLog("  [PERF] Coverage Calculation: DISABLED");
    
    /* Hardware Lighting: Software is faster for N64-style lighting */
    bool hwLightingOpt = false;
    ConfigSetParameter(videoGliden64, "EnableHWLighting", M64TYPE_BOOL, &hwLightingOpt);
    ConfigLog("  [PERF] HW Lighting: DISABLED (SW lighting faster for N64)");
    
    /* Legacy Blending: More compatible and often faster */
    bool legacyBlendingOpt = false;
    ConfigSetParameter(videoGliden64, "EnableLegacyBlending", M64TYPE_BOOL, &legacyBlendingOpt);
    
     /* -------------------------------------------------------------------------
      * OPTIMIZATION #5: N64 DEPTH COMPARE MODE
       * CV64 does NOT use N64 depth compare. Enabling it causes fog/lighting
       * washout artifacts. Keep it disabled (0).
       * 0 = Disabled, 1 = Fast, 2 = Compatible (slowest)
       * -------------------------------------------------------------------------*/
    int n64DepthCompareOpt = 0;  // 0 = Disabled (CV64 doesn't use HW depth compare)
    ConfigSetParameter(videoGliden64, "EnableN64DepthCompare", M64TYPE_INT, &n64DepthCompareOpt);
    ConfigLog("  [PERF] N64 Depth Compare: DISABLED (CV64 doesn't need it, prevents fog washout)");
    
    /* -------------------------------------------------------------------------
     * ADDITIONAL CV64-SPECIFIC OPTIMIZATIONS
     * -------------------------------------------------------------------------*/
    
    /* Halos Removal: Cleaner texture edges, minimal perf impact */
    bool enableHalosRemoval = true;
    ConfigSetParameter(videoGliden64, "enableHalosRemoval", M64TYPE_BOOL, &enableHalosRemoval);
    ConfigLog("  Halos Removal: ENABLED (cleaner texture edges)");
    
    /* Hacks field: No game-specific hacks needed for CV64 */
    int hacks = 0;
    ConfigSetParameter(videoGliden64, "hacks", M64TYPE_INT, &hacks);
    
    /* Native Resolution Texrects: Optimized mode for HUD */
    int nativeResTexrectsCV64 = 1;  // Optimized mode
    ConfigSetParameter(videoGliden64, "EnableNativeResTexrects", M64TYPE_INT, &nativeResTexrectsCV64);
    ConfigLog("  Native Res Texrects: OPTIMIZED (better HUD rendering)");
    
    /* Backgrounds Mode: Stripped mode is more efficient */
    int bgModeCV64 = 1;  // Stripped mode
    ConfigSetParameter(videoGliden64, "BackgroundsMode", M64TYPE_INT, &bgModeCV64);
    ConfigLog("  Backgrounds Mode: STRIPPED (more efficient)");
    
    /* Inaccurate Texture Coordinates: Faster, minimal visual impact */
    bool inaccurateTexCoords = true;
    ConfigSetParameter(videoGliden64, "EnableInaccurateTextureCoordinates", M64TYPE_BOOL, &inaccurateTexCoords);
    ConfigLog("  [PERF] Inaccurate Tex Coords: ENABLED (faster sampling)");

    ConfigLog("=== CV64 RDP Performance Optimizations Complete ===");

    /* =========================================================================
     * CV64 PER-PIXEL LIGHTING SETTINGS
     * Luminance-based fake depth lighting (no depth buffer needed)
     * =========================================================================*/

    ConfigLog("=== Applying CV64 Per-Pixel Lighting Settings ===");

    bool enablePerPixelLighting = s_graphicsIni.GetBool("Enhancements", "enable_enhanced_lighting", false);
    float lightIntensity = s_graphicsIni.GetFloat("Enhancements", "light_intensity", 0.8f);
    float ambientIntensity = s_graphicsIni.GetFloat("Enhancements", "ambient_intensity", 0.3f);
    float normalStrength = s_graphicsIni.GetFloat("Enhancements", "normal_strength", 2.0f);

    ConfigSetParameter(videoGliden64, "EnablePerPixelLighting", M64TYPE_BOOL, &enablePerPixelLighting);
    ConfigSetParameter(videoGliden64, "LightIntensity", M64TYPE_FLOAT, &lightIntensity);
    ConfigSetParameter(videoGliden64, "AmbientIntensity", M64TYPE_FLOAT, &ambientIntensity);
    ConfigSetParameter(videoGliden64, "NormalStrength", M64TYPE_FLOAT, &normalStrength);

    char lightMsg[256];
    snprintf(lightMsg, sizeof(lightMsg), "Per-Pixel Lighting: %s (Intensity=%.2f, Ambient=%.2f, Normal=%.1f)",
             enablePerPixelLighting ? "ENABLED" : "DISABLED", lightIntensity, ambientIntensity, normalStrength);
    ConfigLog(lightMsg);

    ConfigLog("=== CV64 Per-Pixel Lighting Settings Complete ===");

    ConfigLog("GLideN64 configuration applied successfully");
    return true;
}

/*===========================================================================
 * Master Configuration Function
 *===========================================================================*/

extern "C" bool CV64_Config_ApplyAll() {
    /* Load all INI files first (will create default files if missing) */
    if (!CV64_Config_LoadAllInis()) {
        ConfigLog("Warning: Some INI files could not be loaded or created");
    }
    
    /* Apply to mupen64plus core */
    if (!CV64_Config_ApplyToCore()) {
        ConfigLog("Warning: Failed to apply core configuration");
    }
    
    /* Apply to GLideN64 */
    if (!CV64_Config_ApplyToGLideN64()) {
        ConfigLog("Warning: Failed to apply GLideN64 configuration");
    }
    
    /* NOTE: We do NOT call ConfigSaveFile() - our CV64 INI files are the single source of truth.
     * All settings are applied to mupen64plus in-memory config via ConfigSetParameter().
     * The plugins read from this in-memory config during initialization.
     * This avoids creating/maintaining a separate mupen64plus.cfg file. */
    
    ConfigLog("CV64 configuration applied (in-memory, INI files are authoritative)");
    return true;
}

/*===========================================================================
 * Getters for Other Modules
 *===========================================================================*/

extern "C" const char* CV64_Config_GetHDTexturePath() {
    static std::string s_txPath;
    if (!s_configLoaded) {
        CV64_Config_LoadAllInis();
    }
    std::string txPath = s_graphicsIni.GetString("TexturePaths", "tx_path", "assets/hires_texture");
    s_txPath = GetAbsolutePath(txPath);
    return s_txPath.c_str();
}

extern "C" const char* CV64_Config_GetCachePath() {
    static std::string s_cachePath;
    if (!s_configLoaded) {
        CV64_Config_LoadAllInis();
    }
    std::string cachePath = s_graphicsIni.GetString("TexturePaths", "tx_cache_path", "assets/cache");
    s_cachePath = GetAbsolutePath(cachePath);
    return s_cachePath.c_str();
}

extern "C" bool CV64_Config_IsHDTexturesEnabled() {
    if (!s_configLoaded) {
        CV64_Config_LoadAllInis();
    }
    return s_graphicsIni.GetBool("Enhancements", "tx_hires_enable", true);
}

extern "C" bool CV64_Config_IsPatchEnabled(const char* section, const char* key) {
    if (!s_configLoaded) {
        CV64_Config_LoadAllInis();
    }
    return s_patchesIni.GetBool(section, key, false);
}

#endif /* CV64_STATIC_MUPEN64PLUS */
