/**
 * @file cv64_vidext.h
 * @brief Castlevania 64 PC Recomp - Video Extension Implementation
 * 
 * Custom video extension that embeds mupen64plus rendering into the main window.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_VIDEXT_H
#define CV64_VIDEXT_H

#include "cv64_types.h"
#include "cv64_m64p_integration.h"
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Video Extension Types (from m64p_types.h)
 *===========================================================================*/

typedef struct {
    unsigned int uiWidth;
    unsigned int uiHeight;
} m64p_2d_size;

typedef enum {
    M64P_GL_DOUBLEBUFFER = 1,
    M64P_GL_BUFFER_SIZE,
    M64P_GL_DEPTH_SIZE,
    M64P_GL_RED_SIZE,
    M64P_GL_GREEN_SIZE,
    M64P_GL_BLUE_SIZE,
    M64P_GL_ALPHA_SIZE,
    M64P_GL_SWAP_CONTROL,
    M64P_GL_MULTISAMPLEBUFFERS,
    M64P_GL_MULTISAMPLESAMPLES,
    M64P_GL_CONTEXT_MAJOR_VERSION,
    M64P_GL_CONTEXT_MINOR_VERSION,
    M64P_GL_CONTEXT_PROFILE_MASK
} m64p_GLattr;

typedef enum {
    M64P_GL_CONTEXT_PROFILE_CORE,
    M64P_GL_CONTEXT_PROFILE_COMPATIBILITY,
    M64P_GL_CONTEXT_PROFILE_ES
} m64p_GLContextType;

typedef enum {
    M64P_RENDER_OPENGL = 0,
    M64P_RENDER_VULKAN
} m64p_render_mode;

typedef void (*m64p_function)(void);

/*===========================================================================
 * Video Extension Function Pointer Structure
 *===========================================================================*/

typedef struct {
    unsigned int Functions;
    m64p_error    (*VidExtFuncInit)(void);
    m64p_error    (*VidExtFuncQuit)(void);
    m64p_error    (*VidExtFuncListModes)(m64p_2d_size *, int *);
    m64p_error    (*VidExtFuncListRates)(m64p_2d_size, int *, int *);
    m64p_error    (*VidExtFuncSetMode)(int, int, int, int, int);
    m64p_error    (*VidExtFuncSetModeWithRate)(int, int, int, int, int, int);
    m64p_function (*VidExtFuncGLGetProc)(const char*);
    m64p_error    (*VidExtFuncGLSetAttr)(m64p_GLattr, int);
    m64p_error    (*VidExtFuncGLGetAttr)(m64p_GLattr, int *);
    m64p_error    (*VidExtFuncGLSwapBuf)(void);
    m64p_error    (*VidExtFuncSetCaption)(const char *);
    m64p_error    (*VidExtFuncToggleFS)(void);
    m64p_error    (*VidExtFuncResizeWindow)(int, int);
    uint32_t      (*VidExtFuncGLGetDefaultFramebuffer)(void);
    m64p_error    (*VidExtFuncInitWithRenderMode)(m64p_render_mode);
    m64p_error    (*VidExtFuncVKGetSurface)(void**, void*);
    m64p_error    (*VidExtFuncVKGetInstanceExtensions)(const char**[], uint32_t*);
} m64p_video_extension_functions;

/*===========================================================================
 * CV64 Video Extension API
 *===========================================================================*/

/**
 * @brief Initialize the video extension with the main window handle
 * @param hwnd The main application window handle
 * @return true on success
 */
bool CV64_VidExt_Init(HWND hwnd);

/**
 * @brief Shutdown the video extension
 */
void CV64_VidExt_Shutdown(void);

/**
 * @brief Get the video extension function structure to pass to mupen64plus
 * @return Pointer to the video extension functions structure
 */
m64p_video_extension_functions* CV64_VidExt_GetFunctions(void);

/**
 * @brief Get the current render window handle
 * @return The HWND being used for rendering
 */
HWND CV64_VidExt_GetRenderWindow(void);

/**
 * @brief Check if video extension is initialized
 */
bool CV64_VidExt_IsInitialized(void);

/**
 * @brief Get current video dimensions
 */
void CV64_VidExt_GetSize(int* width, int* height);

/**
 * @brief Notify video extension of window resize
 * @param width New window width
 * @param height New window height
 */
void CV64_VidExt_NotifyResize(int width, int height);

/**
 * @brief Toggle borderless fullscreen mode (ALT+ENTER)
 */
void CV64_VidExt_ToggleFullscreen(void);

/**
 * @brief Check if currently in fullscreen mode
 */
bool CV64_VidExt_IsFullscreen(void);

/**
 * @brief Set a callback to be called when fullscreen state changes
 * @param callback Function to call with fullscreen state (true = fullscreen, false = windowed)
 */
void CV64_VidExt_SetFullscreenCallback(void (*callback)(bool fullscreen));

/**
 * @brief Set frame callback invoked every frame in VidExt_GLSwapBuf
 * 
 * This is the main hook point for native patches - GLideN64 calls SwapBuf
 * every frame, so any callback registered here runs at 60fps during gameplay.
 * 
 * @param callback Function to call each frame
 * @param context User context passed to callback
 */
void CV64_VidExt_SetFrameCallback(void (*callback)(void*), void* context);

#ifdef __cplusplus
}
#endif

#endif /* CV64_VIDEXT_H */
