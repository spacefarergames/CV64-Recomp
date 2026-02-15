/**
 * @file cv64_gfx_plugin.h
 * @brief Castlevania 64 PC Recomp - Graphics Plugin Integration
 * 
 * Handles integration with mupen64plus graphics plugins (GLideN64, etc.)
 * and provides enhanced rendering features.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_GFX_PLUGIN_H
#define CV64_GFX_PLUGIN_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Supported Graphics Plugins
 *===========================================================================*/

typedef enum CV64_GfxPluginType {
    CV64_GFX_PLUGIN_AUTO = 0,
    CV64_GFX_PLUGIN_GLIDEN64,       /* GLideN64 - Recommended */
    CV64_GFX_PLUGIN_ANGRYLION,      /* Angrylion Plus - Most accurate */
    CV64_GFX_PLUGIN_PARALLEL,       /* Parallel RDP - Vulkan */
    CV64_GFX_PLUGIN_RICE,           /* Rice Video - Legacy */
    CV64_GFX_PLUGIN_COUNT
} CV64_GfxPluginType;

/*===========================================================================
 * Plugin Configuration
 *===========================================================================*/

typedef struct CV64_GfxPluginConfig {
    /* Plugin selection */
    CV64_GfxPluginType plugin_type;
    char plugin_path[260];
    
    /* Window settings */
    void* hwnd;                     /* HWND window handle */
    u32 window_width;
    u32 window_height;
    bool fullscreen;
    bool vsync;
    
    /* Resolution */
    u32 render_width;               /* Internal render width */
    u32 render_height;              /* Internal render height */
    u32 native_width;               /* N64 native (320) */
    u32 native_height;              /* N64 native (240) */
    
    /* Aspect ratio */
    f32 aspect_ratio;               /* Custom aspect ratio */
    bool widescreen;                /* 16:9 mode */
    bool widescreen_hack;           /* Apply widescreen hack to 3D */
    
    /* Quality settings */
    u32 msaa;                       /* MSAA samples (0, 2, 4, 8) */
    u32 anisotropy;                 /* Anisotropic filtering (1-16) */
    bool bilinear;                  /* Bilinear filtering */
    bool dithering;                 /* N64 dithering */
    
    /* Fog */
    bool fog_enabled;
    f32 fog_multiplier;             /* Extend fog distance */
    
    /* GLideN64 specific */
    bool n64_depth_compare;
    bool fb_emulation;              /* Frame buffer emulation */
    bool copy_depth_to_rdram;
    bool copy_color_to_rdram;
    
    /* HD textures */
    bool hd_textures;
    char hd_texture_path[260];
    
} CV64_GfxPluginConfig;

/*===========================================================================
 * Plugin Function Pointers (from mupen64plus API)
 *===========================================================================*/

/* Plugin startup info */
typedef struct {
    u8* rdram;
    u8* dmem;
    u8* imem;
    u32* mi_intr_reg;
    u32* dpc_start_reg;
    u32* dpc_end_reg;
    u32* dpc_current_reg;
    u32* dpc_status_reg;
    u32* dpc_clock_reg;
    u32* dpc_bufbusy_reg;
    u32* dpc_pipebusy_reg;
    u32* dpc_tmem_reg;
    u32* vi_status_reg;
    u32* vi_origin_reg;
    u32* vi_width_reg;
    u32* vi_intr_reg;
    u32* vi_current_reg;
    u32* vi_burst_reg;
    u32* vi_v_sync_reg;
    u32* vi_h_sync_reg;
    u32* vi_leap_reg;
    u32* vi_h_start_reg;
    u32* vi_v_start_reg;
    u32* vi_v_burst_reg;
    u32* vi_x_scale_reg;
    u32* vi_y_scale_reg;
    void (*CheckInterrupts)(void);
} CV64_GfxInfo;

/* Plugin function types */
typedef void (*GfxChangeWindow)(void);
typedef int  (*GfxInitiateGFX)(CV64_GfxInfo gfx_info);
typedef void (*GfxMoveScreen)(int x, int y);
typedef void (*GfxProcessDList)(void);
typedef void (*GfxProcessRDPList)(void);
typedef void (*GfxRomClosed)(void);
typedef int  (*GfxRomOpen)(void);
typedef void (*GfxShowCFB)(void);
typedef void (*GfxUpdateScreen)(void);
typedef void (*GfxViStatusChanged)(void);
typedef void (*GfxViWidthChanged)(void);
typedef void (*GfxReadScreen2)(void* dest, int* width, int* height, int front);
typedef void (*GfxSetRenderingCallback)(void (*callback)(int));
typedef void (*GfxResizeVideoOutput)(int width, int height);
typedef void (*GfxFBRead)(u32 addr);
typedef void (*GfxFBWrite)(u32 addr, u32 size);
typedef void (*GfxFBGetFrameBufferInfo)(void* info);

/*===========================================================================
 * Graphics Plugin State
 *===========================================================================*/

typedef struct CV64_GfxPluginState {
    /* Plugin loaded flag */
    bool loaded;
    bool initialized;
    
    /* Plugin type */
    CV64_GfxPluginType type;
    
    /* Library handle */
    void* library;  /* HMODULE */
    
    /* Function pointers */
    GfxChangeWindow     change_window;
    GfxInitiateGFX      initiate_gfx;
    GfxMoveScreen       move_screen;
    GfxProcessDList     process_dlist;
    GfxProcessRDPList   process_rdplist;
    GfxRomClosed        rom_closed;
    GfxRomOpen          rom_open;
    GfxShowCFB          show_cfb;
    GfxUpdateScreen     update_screen;
    GfxViStatusChanged  vi_status_changed;
    GfxViWidthChanged   vi_width_changed;
    GfxReadScreen2      read_screen2;
    GfxSetRenderingCallback set_rendering_callback;
    GfxResizeVideoOutput resize_video;
    GfxFBRead           fb_read;
    GfxFBWrite          fb_write;
    GfxFBGetFrameBufferInfo fb_get_info;
    
    /* Current configuration */
    CV64_GfxPluginConfig config;
    
    /* Stats */
    u64 frame_count;
    f64 fps;
    f64 last_frame_time;
    
} CV64_GfxPluginState;

/*===========================================================================
 * API Functions
 *===========================================================================*/

/**
 * @brief Initialize graphics plugin system
 * @param config Plugin configuration
 * @return true on success
 */
CV64_API bool CV64_GfxPlugin_Init(const CV64_GfxPluginConfig* config);

/**
 * @brief Shutdown graphics plugin
 */
CV64_API void CV64_GfxPlugin_Shutdown(void);

/**
 * @brief Load a graphics plugin DLL
 * @param plugin_path Path to plugin DLL
 * @param type Plugin type hint
 * @return true on success
 */
CV64_API bool CV64_GfxPlugin_Load(const char* plugin_path, CV64_GfxPluginType type);

/**
 * @brief Unload current graphics plugin
 */
CV64_API void CV64_GfxPlugin_Unload(void);

/**
 * @brief Start graphics plugin for a ROM
 * @param gfx_info Graphics info from mupen64plus core
 * @return true on success
 */
CV64_API bool CV64_GfxPlugin_StartROM(CV64_GfxInfo* gfx_info);

/**
 * @brief Stop graphics plugin (ROM closed)
 */
CV64_API void CV64_GfxPlugin_StopROM(void);

/**
 * @brief Process display list (render frame)
 */
CV64_API void CV64_GfxPlugin_ProcessDList(void);

/**
 * @brief Update screen (called by VI interrupt)
 */
CV64_API void CV64_GfxPlugin_UpdateScreen(void);

/**
 * @brief Handle window resize
 * @param width New width
 * @param height New height
 */
CV64_API void CV64_GfxPlugin_Resize(u32 width, u32 height);

/**
 * @brief Toggle fullscreen mode
 */
CV64_API void CV64_GfxPlugin_ToggleFullscreen(void);

/**
 * @brief Get current plugin state
 */
CV64_API CV64_GfxPluginState* CV64_GfxPlugin_GetState(void);

/**
 * @brief Apply configuration changes
 * @param config New configuration
 */
CV64_API void CV64_GfxPlugin_ApplyConfig(const CV64_GfxPluginConfig* config);

/**
 * @brief Set widescreen mode
 * @param enable Enable widescreen
 * @param aspect Aspect ratio (e.g., 16.0f/9.0f)
 */
CV64_API void CV64_GfxPlugin_SetWidescreen(bool enable, f32 aspect);

/**
 * @brief Set render callback (called after each frame)
 * @param callback Callback function
 */
CV64_API void CV64_GfxPlugin_SetRenderCallback(void (*callback)(int));

/**
 * @brief Take a screenshot
 * @param path Output path (NULL for auto)
 * @return true on success
 */
CV64_API bool CV64_GfxPlugin_Screenshot(const char* path);

/**
 * @brief Get current FPS
 */
CV64_API f64 CV64_GfxPlugin_GetFPS(void);

/*===========================================================================
 * GLideN64 Specific Functions
 *===========================================================================*/

/**
 * @brief Configure GLideN64 specific settings
 */
CV64_API void CV64_GfxPlugin_ConfigureGLideN64(
    bool fb_emulation,
    bool n64_depth_compare,
    bool native_res_2d
);

/**
 * @brief Set GLideN64 HD texture path
 */
CV64_API void CV64_GfxPlugin_SetHDTexturePath(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* CV64_GFX_PLUGIN_H */
