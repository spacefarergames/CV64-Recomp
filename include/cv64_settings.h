/**
 * @file cv64_settings.h
 * @brief Castlevania 64 PC - Settings Management
 * 
 * Provides a user-friendly settings interface for all game configuration.
 * Reads and writes to INI files in the patches folder.
 */

#ifndef CV64_SETTINGS_H
#define CV64_SETTINGS_H

#include <windows.h>
#include <string>
#include <map>

//=============================================================================
// Settings Structure Definitions
//=============================================================================

/**
 * @brief Graphics settings (from cv64_graphics.ini)
 */
struct CV64_GraphicsSettings {
    // Display
    std::string resolution;      // NATIVE, 480P, 720P, 1080P, 1440P, 4K
    int customWidth;
    int customHeight;
    bool fullscreen;
    bool borderless;

    // Quality
    int internalScale;           // 1-8
    std::string textureFilter;   // NEAREST, LINEAR, TRILINEAR, etc.
    std::string antiAliasing;    // NONE, FXAA, etc.
    bool vsync;
    int maxFps;

    // Aspect Ratio
    std::string aspectRatio;     // 4_3, 16_9, 16_10, 21_9, AUTO
    bool stretchToFill;

    // Enhancements
    bool enableHDTextures;
};

/**
 * @brief Audio settings (from cv64_audio.ini)
 */
struct CV64_AudioSettings {
    // General
    int sampleRate;              // 22050, 44100, 48000
    int latencyMs;
    
    // Volume (0.0 - 1.0 stored as 0-100 int for UI)
    int masterVolume;
    int musicVolume;
    int sfxVolume;
    
    // Enhancements
    bool enableReverb;
    bool enableSurround;
};

/**
 * @brief Camera settings (from cv64_camera.ini)
 */
struct CV64_CameraSettings {
    bool enableDpad;
    bool enableRightStick;
    bool enableMouse;
    
    float dpadSpeed;             // 0.5 - 5.0
    float stickSensitivityX;
    float stickSensitivityY;
    float mouseSensitivityX;
    float mouseSensitivityY;
    
    bool invertX;
    bool invertY;
    bool enableSmoothing;
};

/**
 * @brief Control settings (from cv64_controls.ini)
 */
struct CV64_ControlSettings {
    // Gamepad
    float leftStickDeadzone;     // 0.0 - 0.5
    float rightStickDeadzone;
    
    // Mouse
    float mouseSensX;
    float mouseSensY;
    bool mouseInvertY;
};

/**
 * @brief Game patch settings (from cv64_patches.ini)
 */
struct CV64_PatchSettings {
    // Camera Patches
    bool dpadCamera;
    bool smoothFirstPerson;
    
    // Control Patches
    bool modernControls;
    bool keyboardMouse;
    
    // Graphics Patches
    bool widescreen;
    bool framerateUnlock;
    bool animInterpolation;
    
    // Gameplay Patches
    bool skipIntro;
    bool fastText;
    bool instantMap;
    
    // Bug Fixes
    bool fixSoftlocks;
    bool fixCollision;
    bool fixCameraClip;
};

/**
 * @brief Threading/Performance settings (from cv64_threading.ini)
 */
struct CV64_ThreadingSettings {
    // Threading
    bool enableAsyncGraphics;      // Async frame presentation
    bool enableAsyncAudio;         // Async audio mixing
    bool enableWorkerThreads;      // Background worker pool
    int workerThreadCount;         // 0 = auto-detect
    int graphicsQueueDepth;        // 1-3 (buffering)
    bool enableParallelRSP;        // EXPERIMENTAL - usually false
    
    // Performance Overlay
    bool enablePerfOverlay;        // Show performance stats
    int perfOverlayMode;           // 0-4 (OFF, MINIMAL, STANDARD, DETAILED, GRAPH)
};

/**
 * @brief Post Processing settings (from postprocessing_preset.ini in assets folder)
 * 
 * These control ReShade FX effects via the Techniques line in the preset file.
 * Friendly names:
 *   - Reflections = Barbatos_SSR_Lite.fx (Screen-space reflections)
 *   - Reflective Materials = Chromaticity.fx (Color/gamma correction)
 *   - Color Grading = DPX.fx (Cinematic color processing)
 *   - Film Grain = FilmGrain.fx (Film grain overlay)
 *   - Dynamic Shadows = MiAO.fx (Ambient occlusion)
 *   - Enhanced Lighting = TrackingRays.fx (Light ray effects)
 *   - Vignette = Vignette.fx (Screen edge darkening)
 */
struct CV64_PostProcessingSettings {
    // ReShade FX Effect Toggles
    bool enableReflections;        // Barbatos_SSR_Lite.fx - Screen-space reflections
    bool enableReflectiveMaterials;// Chromaticity.fx - Color/gamma correction
    bool enableColorGrading;       // DPX.fx - Cinematic color processing
    bool enableFilmGrain;          // FilmGrain.fx - Film grain overlay
    bool enableDynamicShadows;     // MiAO.fx - Ambient occlusion
    bool enableEnhancedLighting;   // TrackingRays.fx - Light ray effects
    bool enableVignette;           // Vignette.fx - Screen edge darkening
};

/**
 * @brief Cheat settings (Gameshark-style memory modifications)
 */
struct CV64_CheatSettings {
    // Camera Patch (THE MAIN FEATURE!)
    bool enableCameraPatch;        // D-PAD/RStick camera control (default: true)
    
    // Performance Cheats
    bool enableLagReduction;       // Reduce lag when many enemies on screen (default: true)
    bool enableForestFix;          // Fix FPS drops in Forest of Silence (default: true)
    
    // Gameplay Cheats
    bool enableInfiniteHealth;     // Infinite health (default: false)
    bool enableInfiniteSubweapon;  // Infinite sub-weapon uses (default: false)
    bool enableInfiniteGold;       // 9999 gold (default: false)
    bool enableMoonJump;           // Press L to jump higher (default: false)
    
    // Debug Cheats
    bool enableNoClip;             // Walk through walls (default: false)
    bool enableSpeedBoost;         // Move faster (default: false)
};

/**
 * @brief Master settings container
 */
struct CV64_AllSettings {
    CV64_GraphicsSettings graphics;
    CV64_AudioSettings audio;
    CV64_CameraSettings camera;
    CV64_ControlSettings controls;
    CV64_PatchSettings patches;
    CV64_ThreadingSettings threading;
    CV64_PostProcessingSettings postProcessing;
    CV64_CheatSettings cheats;
};

//=============================================================================
// Settings API
//=============================================================================

/**
 * @brief Initialize the settings system
 * @return true on success
 */
bool CV64_Settings_Init(void);

/**
 * @brief Shutdown the settings system
 */
void CV64_Settings_Shutdown(void);

/**
 * @brief Load all settings from INI files
 * @return true on success
 */
bool CV64_Settings_LoadAll(void);

/**
 * @brief Save all settings to INI files
 * @return true on success
 */
bool CV64_Settings_SaveAll(void);

/**
 * @brief Get current settings (read-only reference)
 */
const CV64_AllSettings& CV64_Settings_Get(void);

/**
 * @brief Get mutable settings for editing
 */
CV64_AllSettings& CV64_Settings_GetMutable(void);

/**
 * @brief Reset all settings to defaults
 */
void CV64_Settings_ResetToDefaults(void);

/**
 * @brief Get the patches folder path
 */
const std::string& CV64_Settings_GetPatchesPath(void);

//=============================================================================
// Settings Dialog
//=============================================================================

/**
 * @brief Show the settings dialog
 * @param hParent Parent window handle
 * @return true if settings were changed and saved
 */
bool CV64_Settings_ShowDialog(HWND hParent);

#endif // CV64_SETTINGS_H
