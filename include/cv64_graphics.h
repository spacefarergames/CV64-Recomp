/**
 * @file cv64_graphics.h
 * @brief Castlevania 64 PC Recomp - Enhanced Graphics System
 * 
 * Provides modern graphics capabilities while maintaining
 * compatibility with original N64 RDP/RSP display lists.
 * 
 * Features:
 * - OpenGL/Vulkan/Direct3D backend support
 * - High resolution rendering
 * - Widescreen support
 * - Enhanced textures
 * - Modern shader effects
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_GRAPHICS_H
#define CV64_GRAPHICS_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Graphics Backend Types
 *===========================================================================*/

typedef enum CV64_GraphicsBackend {
    CV64_GFX_BACKEND_AUTO = 0,      /**< Auto-detect best backend */
    CV64_GFX_BACKEND_OPENGL,        /**< OpenGL 3.3+ */
    CV64_GFX_BACKEND_VULKAN,        /**< Vulkan 1.0+ */
    CV64_GFX_BACKEND_D3D11,         /**< Direct3D 11 */
    CV64_GFX_BACKEND_D3D12,         /**< Direct3D 12 */
    CV64_GFX_BACKEND_COUNT
} CV64_GraphicsBackend;

/*===========================================================================
 * Resolution Presets
 *===========================================================================*/

typedef enum CV64_ResolutionPreset {
    CV64_RES_NATIVE = 0,            /**< Original N64 resolution (320x240) */
    CV64_RES_480P,                  /**< 640x480 */
    CV64_RES_720P,                  /**< 1280x720 */
    CV64_RES_1080P,                 /**< 1920x1080 */
    CV64_RES_1440P,                 /**< 2560x1440 */
    CV64_RES_4K,                    /**< 3840x2160 */
    CV64_RES_CUSTOM,                /**< Custom resolution */
    CV64_RES_COUNT
} CV64_ResolutionPreset;

/*===========================================================================
 * Aspect Ratio Types
 *===========================================================================*/

typedef enum CV64_AspectRatio {
    CV64_ASPECT_4_3 = 0,            /**< Original 4:3 */
    CV64_ASPECT_16_9,               /**< Widescreen 16:9 */
    CV64_ASPECT_16_10,              /**< 16:10 */
    CV64_ASPECT_21_9,               /**< Ultrawide 21:9 */
    CV64_ASPECT_AUTO,               /**< Match window aspect */
    CV64_ASPECT_COUNT
} CV64_AspectRatio;

/*===========================================================================
 * Texture Filtering Modes
 *===========================================================================*/

typedef enum CV64_TextureFilter {
    CV64_FILTER_NEAREST = 0,        /**< Original N64 point sampling */
    CV64_FILTER_LINEAR,             /**< Bilinear filtering */
    CV64_FILTER_TRILINEAR,          /**< Trilinear with mipmaps */
    CV64_FILTER_ANISOTROPIC_2X,     /**< Anisotropic 2x */
    CV64_FILTER_ANISOTROPIC_4X,     /**< Anisotropic 4x */
    CV64_FILTER_ANISOTROPIC_8X,     /**< Anisotropic 8x */
    CV64_FILTER_ANISOTROPIC_16X,    /**< Anisotropic 16x */
    CV64_FILTER_COUNT
} CV64_TextureFilter;

/*===========================================================================
 * Anti-Aliasing Modes
 *===========================================================================*/

typedef enum CV64_AntiAliasing {
    CV64_AA_NONE = 0,               /**< No anti-aliasing */
    CV64_AA_MSAA_2X,                /**< MSAA 2x */
    CV64_AA_MSAA_4X,                /**< MSAA 4x */
    CV64_AA_MSAA_8X,                /**< MSAA 8x */
    CV64_AA_FXAA,                   /**< Fast approximate AA */
    CV64_AA_SMAA,                   /**< Subpixel morphological AA */
    CV64_AA_TAA,                    /**< Temporal AA */
    CV64_AA_COUNT
} CV64_AntiAliasing;

/*===========================================================================
 * Graphics Configuration Structure
 *===========================================================================*/

typedef struct CV64_GraphicsConfig {
    /* Backend selection */
    CV64_GraphicsBackend backend;
    
    /* Resolution settings */
    CV64_ResolutionPreset resolution_preset;
    u32 custom_width;
    u32 custom_height;
    bool fullscreen;
    bool borderless;
    u32 refresh_rate;
    
    /* Aspect ratio */
    CV64_AspectRatio aspect_ratio;
    
    /* Rendering quality */
    u32 internal_resolution_scale;  /**< 1-8x internal resolution */
    CV64_TextureFilter texture_filter;
    CV64_AntiAliasing anti_aliasing;
    bool vsync;
    u32 max_fps;                    /**< 0 = unlimited */
    
    /* Enhanced graphics */
    bool enable_hd_textures;        /**< Load HD texture packs */
    bool enable_enhanced_lighting;
    bool enable_shadows;
    bool enable_fog;
    bool enable_bloom;
    bool enable_ambient_occlusion;
    f32 gamma;
    f32 brightness;
    f32 contrast;
    
    /* N64 accuracy options */
    bool accurate_n64_blending;
    bool accurate_n64_coverage;
    bool enable_n64_dither;
    bool force_3point_filter;       /**< N64 3-point texture filtering */
    
} CV64_GraphicsConfig;

/*===========================================================================
 * Screen Parameters (matching cv64 decomp)
 *===========================================================================*/

typedef struct CV64_ScreenParams {
    s16 screen_width;
    s16 screen_height;
    s16 field2_0x4;
    s16 field3_0x6;
    s16 screen_offset_x;
    s16 screen_offset_y;
    s16 field6_0xc;
    s16 field7_0xe;
    s16 field8_0x10;
    s16 field9_0x12;
    s16 field10_0x14;
    s16 field11_0x16;
    s16 field12_0x18;
    s16 field13_0x1a;
    s16 field14_0x1c;
    s16 field15_0x1e;
} CV64_ScreenParams;

/*===========================================================================
 * Projection Matrix Parameters (matching cv64 decomp)
 *===========================================================================*/

typedef struct CV64_ProjectionParams {
    f32 fovy;
    f32 aspect;
    f32 near_plane;
    f32 far_plane;
    f32 scale;
} CV64_ProjectionParams;

/*===========================================================================
 * Camera Structure (PC enhanced version)
 *===========================================================================*/

typedef struct CV64_Camera {
    /* Original N64 camera fields */
    s16 type;
    u16 flags;
    struct CV64_Camera* prev;
    struct CV64_Camera* sibling;
    struct CV64_Camera* next;
    struct CV64_Camera* parent;
    u16 perspNorm;
    CV64_ScreenParams* screen_params;
    CV64_ProjectionParams* projection_params;
    Gfx* clip_ratio_dl;
    Vec3f position;
    Vec3 position_int;
    Angle angle;
    Vec3f look_at_direction;
    Mat4f matrix;
    
    /* PC Enhanced fields */
    f32 fov_override;               /**< FOV override for widescreen */
    f32 near_clip_override;         /**< Near clip override */
    f32 far_clip_override;          /**< Far clip override */
    bool use_overrides;
    
} CV64_Camera;

/*===========================================================================
 * Graphics API Functions
 *===========================================================================*/

/**
 * @brief Initialize graphics system
 * @param hwnd Window handle
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_Init(void* hwnd);

/**
 * @brief Shutdown graphics system
 */
CV64_API void CV64_Graphics_Shutdown(void);

/**
 * @brief Begin rendering a new frame
 */
CV64_API void CV64_Graphics_BeginFrame(void);

/**
 * @brief End frame and present
 */
CV64_API void CV64_Graphics_EndFrame(void);

/**
 * @brief Process N64 display list
 * @param dl Pointer to display list
 */
CV64_API void CV64_Graphics_ProcessDisplayList(Gfx* dl);

/**
 * @brief Set viewport
 * @param x Viewport X
 * @param y Viewport Y
 * @param width Viewport width
 * @param height Viewport height
 */
CV64_API void CV64_Graphics_SetViewport(u32 x, u32 y, u32 width, u32 height);

/**
 * @brief Set scissor rectangle
 * @param x Scissor X
 * @param y Scissor Y
 * @param width Scissor width
 * @param height Scissor height
 */
CV64_API void CV64_Graphics_SetScissor(u32 x, u32 y, u32 width, u32 height);

/**
 * @brief Clear screen
 * @param color Clear color (RGBA)
 * @param clear_depth TRUE to clear depth buffer
 */
CV64_API void CV64_Graphics_Clear(u32 color, bool clear_depth);

/*===========================================================================
 * Configuration Functions
 *===========================================================================*/

/**
 * @brief Get current graphics configuration
 * @return Pointer to configuration
 */
CV64_API CV64_GraphicsConfig* CV64_Graphics_GetConfig(void);

/**
 * @brief Apply configuration changes
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_ApplyConfig(void);

/**
 * @brief Load graphics configuration from file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_LoadConfig(const char* filepath);

/**
 * @brief Save graphics configuration to file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_SaveConfig(const char* filepath);

/*===========================================================================
 * Resolution Functions
 *===========================================================================*/

/**
 * @brief Set resolution
 * @param width Screen width
 * @param height Screen height
 * @param fullscreen Fullscreen mode
 */
CV64_API void CV64_Graphics_SetResolution(u32 width, u32 height, bool fullscreen);

/**
 * @brief Get current resolution
 * @param out_width Output width
 * @param out_height Output height
 */
CV64_API void CV64_Graphics_GetResolution(u32* out_width, u32* out_height);

/**
 * @brief Set aspect ratio correction
 * @param aspect Aspect ratio type
 */
CV64_API void CV64_Graphics_SetAspectRatio(CV64_AspectRatio aspect);

/**
 * @brief Get widescreen correction factor
 * @return Correction multiplier for FOV
 */
CV64_API f32 CV64_Graphics_GetWidescreenFactor(void);

/*===========================================================================
 * HD Texture Functions
 *===========================================================================*/

/**
 * @brief Load HD texture pack
 * @param pack_path Path to texture pack
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_LoadHDTexturePack(const char* pack_path);

/**
 * @brief Enable/disable HD textures
 * @param enabled TRUE to enable
 */
CV64_API void CV64_Graphics_SetHDTexturesEnabled(bool enabled);

/*===========================================================================
 * Shader Functions
 *===========================================================================*/

/**
 * @brief Reload shaders
 * @return TRUE on success
 */
CV64_API bool CV64_Graphics_ReloadShaders(void);

/**
 * @brief Set shader parameter
 * @param name Parameter name
 * @param value Parameter value
 */
CV64_API void CV64_Graphics_SetShaderParam(const char* name, f32 value);

#ifdef __cplusplus
}
#endif

#endif /* CV64_GRAPHICS_H */
