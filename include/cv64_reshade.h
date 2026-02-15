/**
 * @file cv64_reshade.h
 * @brief Castlevania 64 PC Recomp - ReShade Integration Header
 * 
 * Integrates ReShade post-processing framework statically into the executable.
 * UI elements and auto-update are disabled for a seamless experience.
 * Configuration is merged with cv64_postprocessing.ini in the patches folder.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_RESHADE_H
#define CV64_RESHADE_H

#include "cv64_types.h"
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * ReShade Configuration Defines
 *===========================================================================*/

/* Disable ReShade UI overlay - we manage our own UI */
#define CV64_RESHADE_NO_OVERLAY     1

/* Disable auto-update check - we handle updates ourselves */
#define CV64_RESHADE_NO_UPDATE      1

/* Custom config path - merged with cv64_postprocessing.ini */
#define CV64_RESHADE_CONFIG_NAME    "cv64_postprocessing.ini"

/*===========================================================================
 * ReShade Effect Presets
 *===========================================================================*/

typedef enum {
    CV64_RESHADE_PRESET_NONE = 0,       /* No post-processing */
    CV64_RESHADE_PRESET_CLASSIC,        /* CRT-like scanlines, slight blur */
    CV64_RESHADE_PRESET_ENHANCED,       /* Sharpening, color correction */
    CV64_RESHADE_PRESET_CINEMATIC,      /* Film grain, vignette, color grading */
    CV64_RESHADE_PRESET_HORROR,         /* Desaturated, high contrast, film grain */
    CV64_RESHADE_PRESET_CUSTOM,         /* User-defined preset */
    CV64_RESHADE_PRESET_COUNT
} cv64_reshade_preset_t;

/*===========================================================================
 * ReShade Effect Flags (can be combined)
 *===========================================================================*/

typedef enum {
    CV64_EFFECT_NONE            = 0,
    CV64_EFFECT_FXAA            = (1 << 0),     /* Fast approximate anti-aliasing */
    CV64_EFFECT_SMAA            = (1 << 1),     /* Subpixel morphological AA */
    CV64_EFFECT_CAS             = (1 << 2),     /* Contrast Adaptive Sharpening */
    CV64_EFFECT_LUMASHARPEN     = (1 << 3),     /* Luma-based sharpening */
    CV64_EFFECT_VIBRANCE        = (1 << 4),     /* Intelligent saturation */
    CV64_EFFECT_TONEMAP         = (1 << 5),     /* HDR tone mapping */
    CV64_EFFECT_COLORMATRIX     = (1 << 6),     /* Color matrix adjustment */
    CV64_EFFECT_CURVES          = (1 << 7),     /* S-curve contrast */
    CV64_EFFECT_LEVELS          = (1 << 8),     /* Black/white levels */
    CV64_EFFECT_VIGNETTE        = (1 << 9),     /* Screen edge darkening */
    CV64_EFFECT_FILMGRAIN       = (1 << 10),    /* Film grain overlay */
    CV64_EFFECT_SCANLINES       = (1 << 11),    /* CRT scanline effect */
    CV64_EFFECT_CHROMATIC_AB    = (1 << 12),    /* Chromatic aberration */
    CV64_EFFECT_BLOOM           = (1 << 13),    /* Light bloom/glow */
    CV64_EFFECT_AO              = (1 << 14),    /* Ambient occlusion */
    CV64_EFFECT_DOF             = (1 << 15),    /* Depth of field (requires depth) */
} cv64_effect_flags_t;

/*===========================================================================
 * ReShade Configuration Structure
 *===========================================================================*/

typedef struct {
    /* Global enable/disable */
    bool enabled;
    
    /* Current preset */
    cv64_reshade_preset_t preset;
    
    /* Active effects (bitfield) */
    uint32_t active_effects;
    
    /* Effect-specific settings */
    struct {
        /* FXAA settings */
        float fxaa_quality;             /* 0.0-1.0 */
        
        /* Sharpening settings */
        float sharpen_strength;         /* 0.0-2.0 */
        float sharpen_clamp;            /* 0.0-1.0 */
        
        /* Vibrance settings */
        float vibrance_strength;        /* -1.0 to 1.0 */
        
        /* Color settings */
        float brightness;               /* 0.0-2.0 */
        float contrast;                 /* 0.0-2.0 */
        float saturation;               /* 0.0-2.0 */
        float gamma;                    /* 0.5-2.5 */
        
        /* Vignette settings */
        float vignette_strength;        /* 0.0-1.0 */
        float vignette_radius;          /* 0.0-2.5 */
        
        /* Film grain settings */
        float grain_intensity;          /* 0.0-1.0 */
        float grain_size;               /* 1.0-3.0 */
        
        /* Bloom settings */
        float bloom_intensity;          /* 0.0-1.0 */
        float bloom_threshold;          /* 0.0-1.0 */
        
        /* Scanline settings */
        float scanline_intensity;       /* 0.0-1.0 */
        int scanline_count;             /* Lines per screen height */
        
    } params;
    
    /* Paths */
    char shader_path[MAX_PATH];         /* Path to shader files */
    char preset_path[MAX_PATH];         /* Path to preset files */
    
} cv64_reshade_config_t;

/*===========================================================================
 * ReShade Runtime State
 *===========================================================================*/

typedef struct {
    bool initialized;
    bool effects_loaded;
    bool frame_ready;
    
    /* Performance metrics */
    double effect_time_ms;              /* Time spent on effects */
    uint32_t frame_count;
    
    /* OpenGL resources */
    uint32_t framebuffer;
    uint32_t color_texture;
    uint32_t depth_texture;
    
    /* Current resolution */
    int width;
    int height;
    
} cv64_reshade_state_t;

/*===========================================================================
 * ReShade API Functions
 *===========================================================================*/

/**
 * @brief Initialize ReShade integration
 * @param hwnd Main window handle
 * @param hglrc OpenGL context
 * @return true on success
 */
bool CV64_ReShade_Init(HWND hwnd, HGLRC hglrc);

/**
 * @brief Shutdown ReShade and release resources
 */
void CV64_ReShade_Shutdown(void);

/**
 * @brief Load configuration from cv64_postprocessing.ini
 * @return true on success
 */
bool CV64_ReShade_LoadConfig(void);

/**
 * @brief Save current configuration to cv64_postprocessing.ini
 * @return true on success
 */
bool CV64_ReShade_SaveConfig(void);

/**
 * @brief Apply a built-in preset
 * @param preset Preset to apply
 */
void CV64_ReShade_ApplyPreset(cv64_reshade_preset_t preset);

/**
 * @brief Enable or disable a specific effect
 * @param effect Effect flag to toggle
 * @param enabled true to enable, false to disable
 */
void CV64_ReShade_SetEffect(cv64_effect_flags_t effect, bool enabled);

/**
 * @brief Check if an effect is enabled
 * @param effect Effect flag to check
 * @return true if enabled
 */
bool CV64_ReShade_IsEffectEnabled(cv64_effect_flags_t effect);

/**
 * @brief Enable or disable all post-processing
 * @param enabled true to enable, false to disable
 */
void CV64_ReShade_SetEnabled(bool enabled);

/**
 * @brief Check if post-processing is enabled
 * @return true if enabled
 */
bool CV64_ReShade_IsEnabled(void);

/**
 * @brief Process the current frame through ReShade effects
 * Called after GLideN64 renders but before SwapBuffers
 */
void CV64_ReShade_ProcessFrame(void);

/**
 * @brief Notify ReShade of resolution change
 * @param width New width
 * @param height New height
 */
void CV64_ReShade_Resize(int width, int height);

/**
 * @brief Get current ReShade configuration
 * @return Pointer to config structure (read-only)
 */
const cv64_reshade_config_t* CV64_ReShade_GetConfig(void);

/**
 * @brief Get current ReShade state
 * @return Pointer to state structure (read-only)
 */
const cv64_reshade_state_t* CV64_ReShade_GetState(void);

/**
 * @brief Set a configuration parameter
 * @param param Parameter name (e.g., "sharpen_strength")
 * @param value Value as string
 * @return true on success
 */
bool CV64_ReShade_SetParam(const char* param, const char* value);

/**
 * @brief Get a configuration parameter
 * @param param Parameter name
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return true on success
 */
bool CV64_ReShade_GetParam(const char* param, char* buffer, size_t buffer_size);

/**
 * @brief Toggle ReShade on/off (hotkey support)
 */
void CV64_ReShade_Toggle(void);

/**
 * @brief Cycle through presets (hotkey support)
 */
void CV64_ReShade_CyclePreset(void);

/**
 * @brief Reload all shaders from disk
 * @return true on success
 */
bool CV64_ReShade_ReloadShaders(void);

/**
 * @brief Get effect processing time for performance overlay
 * @return Time in milliseconds
 */
double CV64_ReShade_GetProcessingTime(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_RESHADE_H */
