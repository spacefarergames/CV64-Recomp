/**
 * @file cv64_vidext.cpp
 * @brief Castlevania 64 PC Recomp - Video Extension Implementation
 * 
 * Custom video extension that embeds mupen64plus rendering directly into
 * the main CV64 window, eliminating the separate bordered window.
 * 
 * This is also where our frame callback is invoked - VidExt_GLSwapBuf is
 * called every frame by GLideN64, making it the ideal hook point.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_vidext.h"
#include "../include/cv64_threading.h"
#include "../include/cv64_memory_hook.h"
#include "../include/cv64_m64p_integration.h"
#include "../include/cv64_reshade.h"
#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>

#pragma comment(lib, "opengl32.lib")

/*===========================================================================
 * External GLideN64 API (linked from mupen64plus-video-gliden64-static.lib)
 *===========================================================================*/

extern "C" {
    /* GLideN64 resize function - notifies plugin of window size change
     * This is a static-link friendly wrapper defined in MupenPlusPluginAPI.cpp */
    void gliden64_ResizeVideoOutput(int width, int height);
}

/*===========================================================================
 * Frame Callback for Native Hooks
 *===========================================================================*/

static void (*s_frameCallback)(void*) = nullptr;
static void* s_frameCallbackContext = nullptr;

void CV64_VidExt_SetFrameCallback(void (*callback)(void*), void* context) {
    s_frameCallback = callback;
    s_frameCallbackContext = context;
    
    char msg[128];
    sprintf_s(msg, sizeof(msg), "Frame callback registered: %p", callback);
    OutputDebugStringA("[CV64_VidExt] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static HWND s_mainWindow = NULL;
static HDC s_hdc = NULL;
static HGLRC s_glContext = NULL;
static bool s_initialized = false;
static bool s_fullscreen = false;

/* Fullscreen state change callback */
static void (*s_fullscreenCallback)(bool) = NULL;

/* Video dimensions */
static int s_width = 1280;
static int s_height = 720;

/* Saved windowed mode state for fullscreen toggle */
static RECT s_windowedRect = { 0, 0, 1280, 720 };
static LONG s_windowedStyle = WS_OVERLAPPEDWINDOW;
static LONG s_windowedExStyle = 0;

/* OpenGL attributes */
static int s_glDoubleBuffer = 1;
static int s_glDepthSize = 24;
static int s_glRedSize = 8;
static int s_glGreenSize = 8;
static int s_glBlueSize = 8;
static int s_glAlphaSize = 8;
static int s_glSwapControl = 1;
static int s_glMultisampleBuffers = 0;
static int s_glMultisampleSamples = 0;
static int s_glContextMajor = 3;
static int s_glContextMinor = 3;
static int s_glContextProfile = M64P_GL_CONTEXT_PROFILE_COMPATIBILITY;

/* Video extension functions structure */
static m64p_video_extension_functions s_vidExtFunctions;

/*===========================================================================
 * Debug Logging
 *===========================================================================*/

static void VidExtLog(const char* msg) {
    OutputDebugStringA("[CV64_VidExt] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/*===========================================================================
 * Video Extension Function Implementations
 *===========================================================================*/

static m64p_error VidExt_Init(void) {
    VidExtLog("VidExt_Init called");
    
    if (!s_mainWindow) {
        VidExtLog("ERROR: Main window not set!");
        return M64ERR_NOT_INIT;
    }
    
    /* Get device context for the main window */
    s_hdc = GetDC(s_mainWindow);
    if (!s_hdc) {
        VidExtLog("ERROR: Failed to get device context");
        return M64ERR_SYSTEM_FAIL;
    }
    
    s_initialized = true;
    VidExtLog("VidExt_Init successful");
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_InitWithRenderMode(m64p_render_mode mode) {
    VidExtLog("VidExt_InitWithRenderMode called");
    
    if (mode == M64P_RENDER_VULKAN) {
        VidExtLog("WARNING: Vulkan not supported, using OpenGL");
    }
    
    return VidExt_Init();
}

static m64p_error VidExt_Quit(void) {
    VidExtLog("VidExt_Quit called");
    
    if (s_glContext) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(s_glContext);
        s_glContext = NULL;
    }
    
    if (s_hdc && s_mainWindow) {
        ReleaseDC(s_mainWindow, s_hdc);
        s_hdc = NULL;
    }
    
    s_initialized = false;
    VidExtLog("VidExt_Quit successful");
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ListModes(m64p_2d_size* sizes, int* numSizes) {
    VidExtLog("VidExt_ListModes called");
    
    if (!sizes || !numSizes) {
        return M64ERR_INPUT_INVALID;
    }
    
    /* Return common resolutions */
    int maxModes = *numSizes;
    int count = 0;
    
    static const m64p_2d_size modes[] = {
        { 640, 480 },
        { 800, 600 },
        { 1024, 768 },
        { 1280, 720 },
        { 1280, 960 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 },
        { 3840, 2160 }
    };
    
    int numModes = sizeof(modes) / sizeof(modes[0]);
    count = (maxModes < numModes) ? maxModes : numModes;
    
    for (int i = 0; i < count; i++) {
        sizes[i] = modes[i];
    }
    
    *numSizes = count;
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ListRates(m64p_2d_size size, int* rates, int* numRates) {
    VidExtLog("VidExt_ListRates called");
    
    if (!rates || !numRates) {
        return M64ERR_INPUT_INVALID;
    }
    
    /* Return common refresh rates */
    int maxRates = *numRates;
    int count = 0;
    
    static const int commonRates[] = { 60, 75, 120, 144, 165, 240 };
    int numCommonRates = sizeof(commonRates) / sizeof(commonRates[0]);
    
    count = (maxRates < numCommonRates) ? maxRates : numCommonRates;
    
    for (int i = 0; i < count; i++) {
        rates[i] = commonRates[i];
    }
    
    *numRates = count;
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_SetMode(int width, int height, int bpp, int screenMode, int flags) {
char msg[256];
sprintf_s(msg, sizeof(msg), "VidExt_SetMode: %dx%d, bpp=%d, mode=%d, flags=%d", width, height, bpp, screenMode, flags);
    VidExtLog(msg);
    
    if (!s_mainWindow || !s_hdc) {
        VidExtLog("ERROR: Not initialized");
        return M64ERR_NOT_INIT;
    }
    
    /* Get actual window client area size - use window size, not requested resolution */
    RECT clientRect;
    GetClientRect(s_mainWindow, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    /* Use window size if it's valid, otherwise use requested size */
    s_width = (windowWidth > 0) ? windowWidth : width;
    s_height = (windowHeight > 0) ? windowHeight : height;
    s_fullscreen = (screenMode == 2); /* M64VIDEO_FULLSCREEN = 2 */
    
    sprintf_s(msg, sizeof(msg), "Using actual window size: %dx%d", s_width, s_height);
    VidExtLog(msg);
    
    /* Set pixel format for OpenGL */
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,                     /* Color depth */
        0, 0, 0, 0, 0, 0,
        0, 0,
        0, 0, 0, 0, 0,
        (BYTE)s_glDepthSize,    /* Depth buffer */
        0,                      /* Stencil buffer */
        0,                      /* Aux buffers */
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };
    
    int pixelFormat = ChoosePixelFormat(s_hdc, &pfd);
    if (pixelFormat == 0) {
        VidExtLog("ERROR: ChoosePixelFormat failed");
        return M64ERR_SYSTEM_FAIL;
    }
    
    if (!SetPixelFormat(s_hdc, pixelFormat, &pfd)) {
        VidExtLog("ERROR: SetPixelFormat failed");
        return M64ERR_SYSTEM_FAIL;
    }
    
    /* Create OpenGL context */
    s_glContext = wglCreateContext(s_hdc);
    if (!s_glContext) {
        VidExtLog("ERROR: wglCreateContext failed");
        return M64ERR_SYSTEM_FAIL;
    }
    
    if (!wglMakeCurrent(s_hdc, s_glContext)) {
        VidExtLog("ERROR: wglMakeCurrent failed");
        wglDeleteContext(s_glContext);
        s_glContext = NULL;
        return M64ERR_SYSTEM_FAIL;
    }
    
    /* Set viewport to cover entire window using actual window size */
    glViewport(0, 0, s_width, s_height);
    
    VidExtLog("OpenGL context created and activated successfully");
    
    /* NOTE: ReShade initialization is DISABLED for now.
     * GLideN64's framebuffer management is complex and external post-processing
     * causes visual glitches. For proper post-processing, effects should be
     * integrated into GLideN64's PostProcessor class directly. */
    // CV64_ReShade_Init(s_mainWindow, s_glContext);
    
    /* Force window redraw */
    InvalidateRect(s_mainWindow, NULL, FALSE);
    
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_SetModeWithRate(int width, int height, int rate, int bpp, int screenMode, int flags) {
char msg[256];
sprintf_s(msg, sizeof(msg), "VidExt_SetModeWithRate: %dx%d @ %dHz", width, height, rate);
    VidExtLog(msg);
    
    /* Just call SetMode - we ignore the rate for now */
    return VidExt_SetMode(width, height, bpp, screenMode, flags);
}

static m64p_function VidExt_GLGetProc(const char* name) {
    /* Get OpenGL function pointer */
    m64p_function proc = (m64p_function)wglGetProcAddress(name);
    
    /* If wglGetProcAddress fails, try getting it from opengl32.dll */
    if (!proc) {
        HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
        if (hOpenGL) {
            proc = (m64p_function)GetProcAddress(hOpenGL, name);
        }
    }
    
    return proc;
}

static m64p_error VidExt_GLSetAttr(m64p_GLattr attr, int value) {
    switch (attr) {
        case M64P_GL_DOUBLEBUFFER:
            s_glDoubleBuffer = value;
            break;
        case M64P_GL_DEPTH_SIZE:
            s_glDepthSize = value;
            break;
        case M64P_GL_RED_SIZE:
            s_glRedSize = value;
            break;
        case M64P_GL_GREEN_SIZE:
            s_glGreenSize = value;
            break;
        case M64P_GL_BLUE_SIZE:
            s_glBlueSize = value;
            break;
        case M64P_GL_ALPHA_SIZE:
            s_glAlphaSize = value;
            break;
        case M64P_GL_SWAP_CONTROL:
            s_glSwapControl = value;
            break;
        case M64P_GL_MULTISAMPLEBUFFERS:
            s_glMultisampleBuffers = value;
            break;
        case M64P_GL_MULTISAMPLESAMPLES:
            s_glMultisampleSamples = value;
            break;
        case M64P_GL_CONTEXT_MAJOR_VERSION:
            s_glContextMajor = value;
            break;
        case M64P_GL_CONTEXT_MINOR_VERSION:
            s_glContextMinor = value;
            break;
        case M64P_GL_CONTEXT_PROFILE_MASK:
            s_glContextProfile = value;
            break;
        default:
            return M64ERR_INPUT_INVALID;
    }
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_GLGetAttr(m64p_GLattr attr, int* value) {
    if (!value) {
        return M64ERR_INPUT_INVALID;
    }
    
    switch (attr) {
        case M64P_GL_DOUBLEBUFFER:
            *value = s_glDoubleBuffer;
            break;
        case M64P_GL_DEPTH_SIZE:
            *value = s_glDepthSize;
            break;
        case M64P_GL_RED_SIZE:
            *value = s_glRedSize;
            break;
        case M64P_GL_GREEN_SIZE:
            *value = s_glGreenSize;
            break;
        case M64P_GL_BLUE_SIZE:
            *value = s_glBlueSize;
            break;
        case M64P_GL_ALPHA_SIZE:
            *value = s_glAlphaSize;
            break;
        case M64P_GL_SWAP_CONTROL:
            *value = s_glSwapControl;
            break;
        case M64P_GL_MULTISAMPLEBUFFERS:
            *value = s_glMultisampleBuffers;
            break;
        case M64P_GL_MULTISAMPLESAMPLES:
            *value = s_glMultisampleSamples;
            break;
        case M64P_GL_CONTEXT_MAJOR_VERSION:
            *value = s_glContextMajor;
            break;
        case M64P_GL_CONTEXT_MINOR_VERSION:
            *value = s_glContextMinor;
            break;
        case M64P_GL_CONTEXT_PROFILE_MASK:
            *value = s_glContextProfile;
            break;
        default:
            return M64ERR_INPUT_INVALID;
    }
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_GLSwapBuf(void) {
if (!s_hdc) {
    return M64ERR_NOT_INIT;
}
    
/* === CRITICAL: Call our frame callback for native hooks/patches ===
 * This is invoked every frame by GLideN64, making it the perfect hook point
 * for our memory hooks, Gameshark cheats, and camera patches. */
if (s_frameCallback) {
    s_frameCallback(s_frameCallbackContext);
}

/* NOTE: ReShade post-processing is DISABLED for now.
 * GLideN64's framebuffer management is complex and intercepting it externally
 * causes visual glitches. Proper integration requires modifying GLideN64's
 * PostProcessor class directly. The cv64_reshade module is available for
 * future integration work. */
// CV64_ReShade_ProcessFrame();
    
/* If async graphics is enabled, queue the frame instead of immediate swap */
if (CV64_Threading_IsAsyncGraphicsEnabled()) {
    /* Note: In a full implementation, we'd capture the framebuffer here
     * and queue it for presentation. For now, do synchronous swap.
     * 
     * Future TODO:
     * - glReadPixels() to capture framebuffer
     * - CV64_Graphics_QueueFrame() with captured data
     * - Graphics thread presents via SwapBuffers
     */
        
    // For now, just notify the graphics thread that a frame boundary occurred
    CV64_Graphics_OnVIInterrupt();
    }
    
    SwapBuffers(s_hdc);
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_SetCaption(const char* caption) {
    if (s_mainWindow && caption && caption[0] != '\0') {
        SetWindowTextA(s_mainWindow, caption);
    }
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ToggleFS(void) {
    VidExtLog("VidExt_ToggleFS called (from plugin)");
    
    /* Use our toggle fullscreen implementation */
    CV64_VidExt_ToggleFullscreen();
    
    return M64ERR_SUCCESS;
}

static m64p_error VidExt_ResizeWindow(int width, int height) {
    char msg[128];
    sprintf_s(msg, sizeof(msg), "VidExt_ResizeWindow: %dx%d", width, height);
    VidExtLog(msg);
    
    s_width = width;
    s_height = height;
    
    /* Update OpenGL viewport */
    if (s_glContext && s_hdc) {
        wglMakeCurrent(s_hdc, s_glContext);
        glViewport(0, 0, width, height);
    }
    
    /* Resize the window if not in fullscreen */
    if (!s_fullscreen && s_mainWindow) {
        RECT windowRect = { 0, 0, width, height };
        AdjustWindowRect(&windowRect, GetWindowLongPtr(s_mainWindow, GWL_STYLE), FALSE);
        SetWindowPos(s_mainWindow, NULL, 0, 0, 
                     windowRect.right - windowRect.left, 
                     windowRect.bottom - windowRect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }
    
    return M64ERR_SUCCESS;
}

static uint32_t VidExt_GLGetDefaultFramebuffer(void) {
    return 0; /* Default framebuffer is always 0 */
}

static m64p_error VidExt_VKGetSurface(void** surface, void* instance) {
    /* Vulkan not supported */
    return M64ERR_UNSUPPORTED;
}

static m64p_error VidExt_VKGetInstanceExtensions(const char** extensions[], uint32_t* count) {
    /* Vulkan not supported */
    if (count) *count = 0;
    return M64ERR_UNSUPPORTED;
}

/*===========================================================================
 * Public API Implementation
 *===========================================================================*/

bool CV64_VidExt_Init(HWND hwnd) {
    VidExtLog("CV64_VidExt_Init called");
    
    if (!hwnd) {
        VidExtLog("ERROR: NULL window handle");
        return false;
    }
    
    s_mainWindow = hwnd;
    
    /* Get initial window size */
    RECT rect;
    GetClientRect(hwnd, &rect);
    s_width = rect.right - rect.left;
    s_height = rect.bottom - rect.top;
    
    /* Initialize the function structure */
    memset(&s_vidExtFunctions, 0, sizeof(s_vidExtFunctions));
    
    /* 17 functions (count includes InitWithRenderMode and VK functions) */
    s_vidExtFunctions.Functions = 17;
    
    s_vidExtFunctions.VidExtFuncInit = VidExt_Init;
    s_vidExtFunctions.VidExtFuncQuit = VidExt_Quit;
    s_vidExtFunctions.VidExtFuncListModes = VidExt_ListModes;
    s_vidExtFunctions.VidExtFuncListRates = VidExt_ListRates;
    s_vidExtFunctions.VidExtFuncSetMode = VidExt_SetMode;
    s_vidExtFunctions.VidExtFuncSetModeWithRate = VidExt_SetModeWithRate;
    s_vidExtFunctions.VidExtFuncGLGetProc = VidExt_GLGetProc;
    s_vidExtFunctions.VidExtFuncGLSetAttr = VidExt_GLSetAttr;
    s_vidExtFunctions.VidExtFuncGLGetAttr = VidExt_GLGetAttr;
    s_vidExtFunctions.VidExtFuncGLSwapBuf = VidExt_GLSwapBuf;
    s_vidExtFunctions.VidExtFuncSetCaption = VidExt_SetCaption;
    s_vidExtFunctions.VidExtFuncToggleFS = VidExt_ToggleFS;
    s_vidExtFunctions.VidExtFuncResizeWindow = VidExt_ResizeWindow;
    s_vidExtFunctions.VidExtFuncGLGetDefaultFramebuffer = VidExt_GLGetDefaultFramebuffer;
    s_vidExtFunctions.VidExtFuncInitWithRenderMode = VidExt_InitWithRenderMode;
    s_vidExtFunctions.VidExtFuncVKGetSurface = VidExt_VKGetSurface;
    s_vidExtFunctions.VidExtFuncVKGetInstanceExtensions = VidExt_VKGetInstanceExtensions;
    
    char msg[128];
    sprintf_s(msg, sizeof(msg), "Video extension initialized for window %p (%dx%d)", hwnd, s_width, s_height);
    VidExtLog(msg);
    
    return true;
}

void CV64_VidExt_Shutdown(void) {
    VidExtLog("CV64_VidExt_Shutdown called");
    
    // ReShade shutdown disabled - not initialized
    // CV64_ReShade_Shutdown();
    
    VidExt_Quit();
    
    s_mainWindow = NULL;
    s_width = 640;
    s_height = 480;
    s_fullscreen = false;
}

m64p_video_extension_functions* CV64_VidExt_GetFunctions(void) {
    return &s_vidExtFunctions;
}

HWND CV64_VidExt_GetRenderWindow(void) {
    return s_mainWindow;
}

bool CV64_VidExt_IsInitialized(void) {
    return s_initialized;
}

void CV64_VidExt_GetSize(int* width, int* height) {
    if (width) *width = s_width;
    if (height) *height = s_height;
}

void CV64_VidExt_NotifyResize(int width, int height) {
    if (!s_mainWindow || width <= 0 || height <= 0) {
        return;
    }
    
    s_width = width;
    s_height = height;
    
    /* Update OpenGL viewport if context is active */
    if (s_glContext && s_hdc) {
        wglMakeCurrent(s_hdc, s_glContext);
        
        /* Set viewport to fill the entire window */
        glViewport(0, 0, width, height);
        
        /* Force a full redraw */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    /* CRITICAL: Notify GLideN64 of the new window size so it can update its internal
     * framebuffer dimensions. Without this, GLideN64 continues rendering at the old
     * size and the image doesn't fill the resized window. */
    gliden64_ResizeVideoOutput(width, height);
    
    // ReShade resize disabled - not initialized
    // CV64_ReShade_Resize(width, height);
    
    char msg[128];
    sprintf_s(msg, sizeof(msg), "Window resized to %dx%d - viewport and GLideN64 updated", width, height);
    VidExtLog(msg);
}

void CV64_VidExt_ToggleFullscreen(void) {
    if (!s_mainWindow) {
        return;
    }
    
    s_fullscreen = !s_fullscreen;
    
    if (s_fullscreen) {
        /* Save current windowed state */
        GetWindowRect(s_mainWindow, &s_windowedRect);
        s_windowedStyle = GetWindowLongPtr(s_mainWindow, GWL_STYLE);
        s_windowedExStyle = GetWindowLongPtr(s_mainWindow, GWL_EXSTYLE);
        
        /* Get monitor info for the monitor containing the window */
        HMONITOR hMonitor = MonitorFromWindow(s_mainWindow, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(hMonitor, &monitorInfo);
        
        /* Go borderless fullscreen */
        SetWindowLongPtr(s_mainWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLongPtr(s_mainWindow, GWL_EXSTYLE, WS_EX_APPWINDOW);
        
        int screenWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        int screenHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        
        SetWindowPos(s_mainWindow, HWND_TOP, 
                     monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                     screenWidth, screenHeight, 
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        
        /* Update video dimensions */
        s_width = screenWidth;
        s_height = screenHeight;
        
        /* Update OpenGL viewport */
        if (s_glContext && s_hdc) {
            wglMakeCurrent(s_hdc, s_glContext);
            glViewport(0, 0, screenWidth, screenHeight);
        }
        
        /* Notify GLideN64 of fullscreen size */
        gliden64_ResizeVideoOutput(screenWidth, screenHeight);
        
        VidExtLog("Entered borderless fullscreen");
    } else {
        /* Restore windowed mode */
        SetWindowLongPtr(s_mainWindow, GWL_STYLE, s_windowedStyle);
        SetWindowLongPtr(s_mainWindow, GWL_EXSTYLE, s_windowedExStyle);
        
        int windowWidth = s_windowedRect.right - s_windowedRect.left;
        int windowHeight = s_windowedRect.bottom - s_windowedRect.top;
        
        SetWindowPos(s_mainWindow, HWND_NOTOPMOST,
                     s_windowedRect.left, s_windowedRect.top,
                     windowWidth, windowHeight,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        
        /* Get client area size */
        RECT clientRect;
        GetClientRect(s_mainWindow, &clientRect);
        s_width = clientRect.right - clientRect.left;
        s_height = clientRect.bottom - clientRect.top;
        
        /* Update OpenGL viewport */
        if (s_glContext && s_hdc) {
            wglMakeCurrent(s_hdc, s_glContext);
            glViewport(0, 0, s_width, s_height);
        }
        
        /* Notify GLideN64 of windowed size */
        gliden64_ResizeVideoOutput(s_width, s_height);
        
        VidExtLog("Exited fullscreen, restored windowed mode");
    }
    
    /* Notify callback of fullscreen state change */
    if (s_fullscreenCallback) {
        s_fullscreenCallback(s_fullscreen);
    }
}

bool CV64_VidExt_IsFullscreen(void) {
    return s_fullscreen;
}

void CV64_VidExt_SetFullscreenCallback(void (*callback)(bool fullscreen)) {
    s_fullscreenCallback = callback;
}
