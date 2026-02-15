/**
 * @file CV64_RMG.cpp
 * @brief Castlevania 64 PC Recomp - Main Application Entry Point
 * 
 * CASTLEVANIA 64 PC RECOMPILATION PORT
 * ====================================
 * Based on the cv64 decomp project and using RMG as reference.
 * Integrates mupen64plus for N64 emulation with our custom patches.
 * 
 * Features:
 * - Full N64 emulation via mupen64plus (from RMG)
 * - D-PAD camera control (THE MOST REQUESTED FEATURE!)
 * - Modern controller support (XInput, keyboard/mouse)
 * - High resolution support
 * - Widescreen patches
 * - Audio enhancements
 * 
 * @copyright 2024 CV64 Recomp Team
 */




#include "framework.h"
#include "CV64_RMG.h"
#include "include/cv64_recomp.h"
#include "include/cv64_m64p_integration.h"
#include "include/cv64_vidext.h"
#include "include/cv64_settings.h"
#include "include/cv64_performance_overlay.h"
#include "include/cv64_input_remapping.h"
#include "include/cv64_memory_hook.h"
#include "include/cv64_threading.h"
#include "include/cv64_gliden64_optimize.h"
#include "include/cv64_mempak_editor.h"
#include "include/cv64_savestate_manager.h"
#include "include/cv64_mod_loader.h"
#include "include/cv64_model_viewer.h"
#include "include/cv64_anim_interp.h"
#include "include/cv64_patches.h"
#include "include/cv64_anim_bridge.h"


#include <stdio.h>
#include <chrono>
#include <dwmapi.h>
#include <wingdi.h>
#include <CommCtrl.h>
#include <Xinput.h>
#include <Uxtheme.h>


#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "UxTheme.lib")

#define MAX_LOADSTRING 100

// Window dimensions - FIXED at 1280x720 for OpenGL compatibility
#define DEFAULT_WINDOW_WIDTH  1280
#define DEFAULT_WINDOW_HEIGHT 720

// Target frame rate
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

// Default ROM path
#define DEFAULT_ROM_PATH "assets\\Castlevania (U) (V1.2) [!].z64"

// Splash screen bitmap filename (loaded from assets folder)
#define SPLASH_BITMAP_FILENAME "Retro_CASTLE.bmp"

// Global Variables:
HINSTANCE g_hInst = NULL;                           // Current instance
HWND g_hWnd = NULL;                                 // Main window handle
HMENU g_hMenu = NULL;                               // Main menu handle
WCHAR szTitle[MAX_LOADSTRING];                      // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];                // The main window class name

// Splash screen bitmap
static HBITMAP g_splashBitmap = NULL;
static int g_splashWidth = 0;
static int g_splashHeight = 0;

// Recomp state
static bool g_isRunning = false;
static bool g_isPaused = false;
static bool g_isInitialized = false;
static bool g_emulationStarted = false;

// Gamepad state tracking for edge detection
static bool g_gamepadStartWasPressed = false;

// Timing
static std::chrono::high_resolution_clock::time_point g_lastFrameTime;
static double g_deltaTime = 0.0;
static double g_frameAccumulator = 0.0;
static int g_fps = 0;
static int g_frameCount = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    GameInfoDlg(HWND, UINT, WPARAM, LPARAM);

// Recomp functions
static bool InitializeRecomp(HWND hWnd);
static void ShutdownRecomp(void);
static void RunGameFrame(void);
static void UpdateTiming(void);
static void RenderFrame(HDC hdc);
static bool LoadDefaultROM(void);
static void OnFrameCallback(void* context);
static bool CheckGamepadStartButton(void);

// Window theming functions
static void EnableDarkModeForWindow(HWND hwnd);
static void UpdateMenuBarVisibility(bool fullscreen);

// Dark mode constants
enum PreferredAppMode {
    Default = 0,
    AllowDark = 1,
    ForceDark = 2,
    ForceLight = 3,
    Max = 4
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

// DWM backdrop types for Windows 11
// Values for DWMWA_SYSTEMBACKDROP_TYPE attribute
#define CV64_DWMSBT_AUTO 0
#define CV64_DWMSBT_NONE 1
#define CV64_DWMSBT_MAINWINDOW 2      // Mica
#define CV64_DWMSBT_TRANSIENTWINDOW 3 // Acrylic
#define CV64_DWMSBT_TABBEDWINDOW 4    // Mica Alt (tabbed)

// Undocumented DWM attributes
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif

// Undocumented messages for dark mode menu bar rendering
#define WM_UAHDRAWMENU         0x0091
#define WM_UAHDRAWMENUITEM     0x0092

// UAH menu structures for dark mode menu bar
typedef struct {
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags;
} UAHMENU;

typedef struct {
    DRAWITEMSTRUCT dis;
    UAHMENU um;
    int iPosition;
} UAHDRAWMENUITEM;

// Dark mode colors for menu bar
#define DARK_MENU_BAR_BG       RGB(32, 32, 32)
#define DARK_MENU_BAR_TEXT     RGB(255, 255, 255)
#define DARK_MENU_BAR_HOT_BG   RGB(65, 65, 65)
#define DARK_MENU_BAR_DISABLED RGB(128, 128, 128)

// Function pointers for undocumented APIs
typedef BOOL (WINAPI *fnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
typedef BOOL (WINAPI *fnShouldAppsUseDarkMode)(void);
typedef void (WINAPI *fnAllowDarkModeForWindow)(HWND, BOOL);
typedef void (WINAPI *fnSetPreferredAppMode)(PreferredAppMode);
typedef void (WINAPI *fnFlushMenuThemes)(void);

// Global dark mode function pointers (loaded once)
static fnSetWindowCompositionAttribute g_SetWindowCompositionAttribute = nullptr;
static fnShouldAppsUseDarkMode g_ShouldAppsUseDarkMode = nullptr;
static fnAllowDarkModeForWindow g_AllowDarkModeForWindow = nullptr;
static fnSetPreferredAppMode g_SetPreferredAppMode = nullptr;
static fnFlushMenuThemes g_FlushMenuThemes = nullptr;
static bool g_darkModeAPIsLoaded = false;
static bool g_darkModeSupported = false;  // True if dark mode APIs are available

// Forward declarations for dark mode functions
static void InitDarkModeBrushes();

/**
 * @brief Load undocumented dark mode APIs from uxtheme.dll
 */
static void LoadDarkModeAPIs()
{
    if (g_darkModeAPIsLoaded) return;
    g_darkModeAPIsLoaded = true;
    
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        g_SetWindowCompositionAttribute = (fnSetWindowCompositionAttribute)
            GetProcAddress(hUser32, "SetWindowCompositionAttribute");
    }
    
    HMODULE hUxTheme = LoadLibraryW(L"uxtheme.dll");
    if (hUxTheme) {
        // These are undocumented ordinal exports
        g_ShouldAppsUseDarkMode = (fnShouldAppsUseDarkMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(132));
        g_AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133));
        g_SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));
        g_FlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136));
    }
    
    // Dark mode is supported if we have at least SetPreferredAppMode
    g_darkModeSupported = (g_SetPreferredAppMode != nullptr);
    
    OutputDebugStringA("[CV64] Dark mode APIs loaded:\n");
    char msg[256];
    sprintf_s(msg, "[CV64]   SetWindowCompositionAttribute: %s\n", g_SetWindowCompositionAttribute ? "YES" : "NO");
    OutputDebugStringA(msg);
    sprintf_s(msg, "[CV64]   ShouldAppsUseDarkMode: %s\n", g_ShouldAppsUseDarkMode ? "YES" : "NO");
    OutputDebugStringA(msg);
    sprintf_s(msg, "[CV64]   AllowDarkModeForWindow: %s\n", g_AllowDarkModeForWindow ? "YES" : "NO");
    OutputDebugStringA(msg);
    sprintf_s(msg, "[CV64]   SetPreferredAppMode: %s\n", g_SetPreferredAppMode ? "YES" : "NO");
    OutputDebugStringA(msg);
    sprintf_s(msg, "[CV64]   FlushMenuThemes: %s\n", g_FlushMenuThemes ? "YES" : "NO");
    OutputDebugStringA(msg);
    sprintf_s(msg, "[CV64]   Dark mode supported: %s\n", g_darkModeSupported ? "YES" : "NO");
    OutputDebugStringA(msg);
}

/**
 * @brief Check if Windows dark mode is enabled in system settings
 */
static bool IsSystemDarkModeEnabled()
{
    // Check registry for AppsUseLightTheme
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_DWORD, nullptr, &value, &size) == ERROR_SUCCESS) {
        return value == 0;
    }
    
    // Fallback to undocumented API
    if (g_ShouldAppsUseDarkMode) {
        return g_ShouldAppsUseDarkMode();
    }
    
    return true; // Default to dark mode for this game
}

/**
 * @brief Initialize application-wide dark mode support
 * Call this early in wWinMain before creating windows
 */
static void InitializeAppDarkMode()
{
    LoadDarkModeAPIs();
    InitDarkModeBrushes();
    
    // Set app to allow dark mode
    if (g_SetPreferredAppMode) {
        g_SetPreferredAppMode(AllowDark);
        OutputDebugStringA("[CV64] App dark mode preference set to AllowDark\n");
    }
}

/**
 * @brief Handle WM_UAHDRAWMENU - draws the menu bar background in dark mode
 */
static void UAHDrawMenuBar(HWND hwnd, UAHMENU* pUDM)
{
    if (!pUDM || !pUDM->hdc) return;
    
    MENUBARINFO mbi = { sizeof(mbi) };
    if (!GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) {
        return;
    }
    
    RECT rcWindow;
    if (!GetWindowRect(hwnd, &rcWindow)) {
        return;
    }
    
    // Convert menu bar rect from screen to window coordinates
    RECT rcMenuBar = mbi.rcBar;
    OffsetRect(&rcMenuBar, -rcWindow.left, -rcWindow.top);
    
    // Fill menu bar with dark background
    HBRUSH hBrush = CreateSolidBrush(DARK_MENU_BAR_BG);
    FillRect(pUDM->hdc, &rcMenuBar, hBrush);
    DeleteObject(hBrush);
}

/**
 * @brief Handle WM_UAHDRAWMENUITEM - draws individual menu items in dark mode
 */
static void UAHDrawMenuItem(HWND hwnd, UAHDRAWMENUITEM* pUDMI)
{
    if (!pUDMI || !pUDMI->dis.hDC) return;
    
    // Get menu item info
    WCHAR szText[256] = { 0 };
    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_STRING | MIIM_STATE;
    mii.dwTypeData = szText;
    mii.cch = _countof(szText);
    
    if (!GetMenuItemInfoW(pUDMI->um.hmenu, pUDMI->iPosition, TRUE, &mii)) {
        return;
    }
    
    // Determine colors based on state
    COLORREF bgColor = DARK_MENU_BAR_BG;
    COLORREF textColor = DARK_MENU_BAR_TEXT;
    
    DWORD state = pUDMI->dis.itemState;
    if (state & ODS_HOTLIGHT || state & ODS_SELECTED) {
        bgColor = DARK_MENU_BAR_HOT_BG;
    }
    if (state & (ODS_GRAYED | ODS_DISABLED | ODS_INACTIVE)) {
        textColor = DARK_MENU_BAR_DISABLED;
    }
    
    // Fill background
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(pUDMI->dis.hDC, &pUDMI->dis.rcItem, hBrush);
    DeleteObject(hBrush);
    
    // Draw text centered in the item rect
    SetBkMode(pUDMI->dis.hDC, TRANSPARENT);
    SetTextColor(pUDMI->dis.hDC, textColor);
    
    RECT rcText = pUDMI->dis.rcItem;
    DrawTextW(pUDMI->dis.hDC, szText, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

/**
 * @brief Enable Windows Dark Mode for the application window
 * Supports Windows 10 1809+ and Windows 11 dark mode with Mica backdrop
 */
static void EnableDarkModeForWindow(HWND hwnd)
{
    // Ensure APIs are loaded
    LoadDarkModeAPIs();
    
    // Allow dark mode for this specific window (must be done before other attributes)
    if (g_AllowDarkModeForWindow) {
        g_AllowDarkModeForWindow(hwnd, TRUE);
    }
    
    // Windows 10 1809 and later support for dark mode title bar
    BOOL useDarkMode = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    
    // If that failed, try the older undocumented attribute (Windows 10 1809-1903)
    if (FAILED(hr)) {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &useDarkMode, sizeof(useDarkMode));
    }
    
    // Enable Mica backdrop (Windows 11 22H2+) for title bar only
    // Note: Mica requires extending frame - if it causes issues, we skip it
    int backdropType = CV64_DWMSBT_MAINWINDOW; // Mica
    hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));
    
    if (SUCCEEDED(hr)) {
        OutputDebugStringA("[CV64] Mica backdrop enabled via DWMWA_SYSTEMBACKDROP_TYPE\n");
    }
    // Don't use DWMWA_MICA_EFFECT fallback - it requires frame extension that can cause issues
    
    // Set dark mode colors for window composition (affects menus)
    if (g_SetWindowCompositionAttribute) {
        BOOL darkMode = TRUE;
        WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &darkMode, sizeof(darkMode) };
        g_SetWindowCompositionAttribute(hwnd, &data);
    }
    
    // Flush menu themes to apply dark mode to menus immediately
    if (g_FlushMenuThemes) {
        g_FlushMenuThemes();
    }
    
    // Update the non-client area to apply dark mode
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    
    // Force redraw of menu bar
    DrawMenuBar(hwnd);
    
    OutputDebugStringA("[CV64] Dark mode enabled for window\n");
}

// Dark mode colors for dialogs and controls
#define DARK_DLG_BG_COLOR       RGB(32, 32, 32)
#define DARK_DLG_TEXT_COLOR     RGB(255, 255, 255)
#define DARK_DLG_EDIT_BG        RGB(45, 45, 45)
#define DARK_DLG_DISABLED_TEXT  RGB(128, 128, 128)
#define DARK_DLG_BORDER_COLOR   RGB(60, 60, 60)
#define DARK_DLG_HIGHLIGHT      RGB(0, 120, 215)

// Static brushes for dialog theming (created once)
static HBRUSH g_hDarkBgBrush = NULL;
static HBRUSH g_hDarkEditBrush = NULL;

/**
 * @brief Initialize dark mode brushes (call once at startup)
 */
static void InitDarkModeBrushes()
{
    if (!g_hDarkBgBrush) {
        g_hDarkBgBrush = CreateSolidBrush(DARK_DLG_BG_COLOR);
    }
    if (!g_hDarkEditBrush) {
        g_hDarkEditBrush = CreateSolidBrush(DARK_DLG_EDIT_BG);
    }
}

/**
 * @brief Apply dark mode theme to all child controls of a dialog
 */
static void ApplyDarkModeToChildren(HWND hDlg)
{
    // Set dark mode for all child windows using uxtheme
    EnumChildWindows(hDlg, [](HWND hwndChild, LPARAM lParam) -> BOOL {
        // Allow dark mode for child window
        if (g_AllowDarkModeForWindow) {
            g_AllowDarkModeForWindow(hwndChild, TRUE);
        }
        
        // Set dark mode explorer theme for common controls
        wchar_t className[64];
        GetClassNameW(hwndChild, className, 64);
        
        // Apply dark theme to specific control types
        if (wcscmp(className, L"SysListView32") == 0 ||
            wcscmp(className, L"SysTreeView32") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcscmp(className, L"Button") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcscmp(className, L"ComboBox") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_CFD", NULL);
        }
        else if (wcscmp(className, L"Edit") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcscmp(className, L"SysTabControl32") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcsncmp(className, L"msctls_trackbar", 15) == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcscmp(className, L"ScrollBar") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        else if (wcscmp(className, L"Static") == 0) {
            SetWindowTheme(hwndChild, L"DarkMode_Explorer", NULL);
        }
        
        return TRUE;
    }, 0);
}

/**
 * @brief Handle WM_CTLCOLOR* messages for dark mode dialogs
 * Returns brush if handled, NULL if not
 */
HBRUSH HandleDarkModeCtlColor(HWND hDlg, UINT message, HDC hdc, HWND hCtl)
{
    if (!g_darkModeSupported) {
        return NULL;
    }
    
    InitDarkModeBrushes();
    
    switch (message) {
    case WM_CTLCOLORDLG:
        return g_hDarkBgBrush;
        
    case WM_CTLCOLORSTATIC:
        SetTextColor(hdc, DARK_DLG_TEXT_COLOR);
        SetBkColor(hdc, DARK_DLG_BG_COLOR);
        SetBkMode(hdc, OPAQUE);  // Use OPAQUE so SetBkColor takes effect for checkbox/radio labels
        return g_hDarkBgBrush;
        
    case WM_CTLCOLOREDIT:
        SetTextColor(hdc, DARK_DLG_TEXT_COLOR);
        SetBkColor(hdc, DARK_DLG_EDIT_BG);
        return g_hDarkEditBrush;
        
    case WM_CTLCOLORLISTBOX:
        SetTextColor(hdc, DARK_DLG_TEXT_COLOR);
        SetBkColor(hdc, DARK_DLG_EDIT_BG);
        return g_hDarkEditBrush;
        
    case WM_CTLCOLORBTN:
        SetTextColor(hdc, DARK_DLG_TEXT_COLOR);
        SetBkColor(hdc, DARK_DLG_BG_COLOR);
        return g_hDarkBgBrush;
    }
    
    return NULL;
}

/**
 * @brief Enable dark mode for dialogs (helper exposed globally)
 * Call this in WM_INITDIALOG of any dialog procedure
 */
void EnableDarkModeForDialog(HWND hDlg)
{
    InitDarkModeBrushes();
    EnableDarkModeForWindow(hDlg);
    ApplyDarkModeToChildren(hDlg);
    
    // Set dark background brush for dialog
    // Note: The actual coloring is done via WM_CTLCOLOR* messages
}

/**
 * @brief Enable dark mode for title bar/borders only
 * Use this for dialogs where full dark mode theming looks bad
 */
void EnableDarkModeTitleBarOnly(HWND hDlg)
{
    // Only apply dark mode to the window frame/title bar, not to child controls
    LoadDarkModeAPIs();
    
    // Allow dark mode for this specific window
    if (g_AllowDarkModeForWindow) {
        g_AllowDarkModeForWindow(hDlg, TRUE);
    }
    
    // Windows 10 1809+ dark mode title bar
    BOOL useDarkMode = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hDlg, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    
    // Fallback for older Windows 10
    if (FAILED(hr)) {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hDlg, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &useDarkMode, sizeof(useDarkMode));
    }
    
    // Update non-client area
    SetWindowPos(hDlg, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

//=============================================================================
// Menu Bar Visibility
//=============================================================================

/**
 * @brief Update menu bar visibility based on fullscreen state
 * @param fullscreen true to hide menu, false to show menu
 */
static void UpdateMenuBarVisibility(bool fullscreen)
{
    if (!g_hWnd || !g_hMenu) {
        return;
    }
    
    if (fullscreen) {
        // Hide menu bar in fullscreen
        SetMenu(g_hWnd, NULL);
        OutputDebugStringA("[CV64] Menu bar hidden (fullscreen mode)\n");
    } else {
        // Show menu bar in windowed mode
        SetMenu(g_hWnd, g_hMenu);
        OutputDebugStringA("[CV64] Menu bar shown (windowed mode)\n");
    }
    
    // Force window to redraw with new menu state
    DrawMenuBar(g_hWnd);
}

/*===========================================================================
 * Animation Bridge Callbacks
 * These bridge CV64AnimBridge (GLideN64) → cv64_anim_interp (main app)
 *===========================================================================*/

static void AnimBridge_OnLogicTick(void)
{
    CV64_AnimInterp_OnLogicTick();
}

static void AnimBridge_OnCapture(
    u32 actor_addr, u8 num_limbs,
    const s16* limb_rx, const s16* limb_ry, const s16* limb_rz,
    f32 pos_x, f32 pos_y, f32 pos_z,
    s16 pitch, s16 yaw, s16 roll)
{
    /* Convert per-limb s16 rotation arrays into CV64_BoneTransform array */
    CV64_BoneTransform bones[CV64_ANIM_MAX_BONES];
    u32 count = (num_limbs > CV64_ANIM_MAX_BONES) ? CV64_ANIM_MAX_BONES : num_limbs;

    for (u32 i = 0; i < count; i++) {
        bones[i].position = { 0.0f, 0.0f, 0.0f };
        bones[i].rot_x = limb_rx[i];
        bones[i].rot_y = limb_ry[i];
        bones[i].rot_z = limb_rz[i];
        bones[i].pad   = 0;
        bones[i].scale = { 1.0f, 1.0f, 1.0f };
    }

    Vec3f root_pos = { pos_x, pos_y, pos_z };
    CV64_AnimInterp_Capture(actor_addr, count, bones, &root_pos, pitch, yaw, roll);
}

static void AnimBridge_OnUpdate(f32 alpha)
{
    CV64_AnimInterp_Update(alpha);
}

static void AnimBridge_OnMapChange(void)
{
    CV64_AnimInterp_RemoveAll();
}

static bool AnimBridge_OnGetPose(
    u32 actor_addr, u8 num_limbs,
    s16* out_limb_rx, s16* out_limb_ry, s16* out_limb_rz,
    f32* out_pos_x, f32* out_pos_y, f32* out_pos_z,
    s16* out_pitch, s16* out_yaw, s16* out_roll)
{
    const CV64_SkeletonSnapshot* pose = CV64_AnimInterp_GetPose(actor_addr);
    if (!pose || pose->bone_count == 0)
        return false;

    u32 count = (num_limbs < pose->bone_count) ? num_limbs : pose->bone_count;
    for (u32 i = 0; i < count; i++) {
        out_limb_rx[i] = pose->bones[i].rot_x;
        out_limb_ry[i] = pose->bones[i].rot_y;
        out_limb_rz[i] = pose->bones[i].rot_z;
    }

    *out_pos_x = pose->root_position.x;
    *out_pos_y = pose->root_position.y;
    *out_pos_z = pose->root_position.z;
    *out_pitch = pose->root_rot_x;
    *out_yaw   = pose->root_rot_y;
    *out_roll  = pose->root_rot_z;
    return true;
}

/**
* @brief Frame callback - called every emulated frame
* NOTE: Do NOT call CV64_Controller_Update here!
* The mupen64plus input-sdl plugin handles all controller input.
 * Calling our controller system conflicts with SDL's input handling.
 */
static void OnFrameCallback(void* context)
{
    // Lazy initialization of memory hook system
    // The RDRAM pointer might not be available until emulation is actually running
    static bool s_hookInitAttempted = false;
    static int s_framesSinceInit = 0;
    static int s_rdramRetryCount = 0;
    const int MAX_RDRAM_RETRIES = 5;  // Only log warning a few times
     
    if (!CV64_Memory_IsInitialized()) {
        void* rdram = CV64_M64P_GetRDRAMPointer();
        if (rdram != NULL) {
            // Use 8MB RDRAM size - CV64's Gameshark addresses (0x80387xxx) require it
            // DisableExtraMem=0 in config should give us 8MB from mupen64plus
            CV64_Memory_SetRDRAM((u8*)rdram, 0x800000);  // 8MB RDRAM (Expansion Pak)
            OutputDebugStringA("[CV64] Memory hook system initialized via frame callback (8MB RDRAM)\n");
            s_hookInitAttempted = true;
            s_framesSinceInit = 0;
            s_rdramRetryCount = 0;
        } else if (s_rdramRetryCount < MAX_RDRAM_RETRIES) {
            if (s_rdramRetryCount == 0) {
                OutputDebugStringA("[CV64] WARNING: RDRAM pointer not available yet, will retry...\n");
            }
            s_rdramRetryCount++;
            s_hookInitAttempted = true;
            // Don't call CV64_Memory_FrameUpdate yet - RDRAM not ready
            return;
        } else {
            // Still no RDRAM after retries - skip memory operations silently
            return;
        }
    } else {
        // Log success after a few frames to confirm it's working
        s_framesSinceInit++;
        if (s_framesSinceInit == 60) {
            char msg[256];
            snprintf(msg, sizeof(msg), "[CV64] Memory hook confirmed working after 60 frames. RDRAM=%p, Size=0x%X\n",
                     CV64_Memory_GetRDRAM(), CV64_Memory_GetRDRAMSize());
            OutputDebugStringA(msg);
        }
    }
    
    // Update memory hooks every frame (only if RDRAM is initialized)
    // This handles:
    // - Gameshark cheats (60fps patches)
    // - Game state reading (map name, player stats for window title)
    // - Camera patch updates (if enabled)
    CV64_Memory_FrameUpdate();
     
    // Update GLideN64 optimization system frame hooks
    // This tracks texture usage, draw calls, and applies area-specific optimizations
    static bool s_optFrameStarted = false;
    if (!s_optFrameStarted) {
        CV64_Optimize_FrameBegin();
        s_optFrameStarted = true;
    }
    // Note: FrameEnd is called in the main loop after frame completion
}

/**
 * @brief Load the default Castlevania 64 ROM
 * First tries test ROM (baserom_camera.z64), then embedded ROM, then external file
 */
static bool LoadDefaultROM(void)
{
    // Construct base path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    std::string basePath(exePath);
    size_t lastSlash = basePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    }
    
    // FOR TESTING: Check for baserom_camera.z64 first (camera patch test ROM)
    std::string testRomPath = basePath + "assets\\baserom_camera.z64";
    if (!std::filesystem::exists(testRomPath)) {
        // Also try project directory
        testRomPath = "C:\\Users\\patte\\source\\repos\\CV64_RMG\\assets\\baserom_camera.z64";
    }
    
    if (std::filesystem::exists(testRomPath)) {
        OutputDebugStringA("[CV64] Found test ROM: baserom_camera.z64 - loading for camera patch testing\n");
        if (CV64_M64P_LoadROM(testRomPath.c_str())) {
            OutputDebugStringA("[CV64] Loaded baserom_camera.z64 successfully\n");
            return true;
        }
        OutputDebugStringA("[CV64] Failed to load test ROM, trying other options...\n");
    }
    
    // Try embedded ROM (preferred - no external files needed)
    if (CV64_M64P_LoadEmbeddedROM()) {
        OutputDebugStringA("[CV64] Loaded embedded ROM successfully\n");
        return true;
    }
    
    OutputDebugStringA("[CV64] No embedded ROM, trying external file...\n");
    
    std::string romPath = basePath + DEFAULT_ROM_PATH;
    
    // Also try absolute path from project
    if (!std::filesystem::exists(romPath)) {
        romPath = "C:\\Users\\patte\\source\\repos\\CV64_RMG\\assets\\Castlevania (U) (V1.2) [!].z64";
    }
    
    if (!std::filesystem::exists(romPath)) {
        MessageBoxA(g_hWnd, 
            "Castlevania 64 ROM not found!\n\n"
            "No embedded ROM and external file not found at:\n"
            "assets\\Castlevania (U) (V1.2) [!].z64",
            "CV64 Recomp - ROM Not Found", MB_ICONWARNING);
        return false;
    }
    
    // Load the ROM
    if (!CV64_M64P_LoadROM(romPath.c_str())) {
        MessageBoxA(g_hWnd, 
            CV64_M64P_GetLastError(),
            "CV64 Recomp - ROM Load Error", MB_ICONERROR);
        return false;
    }
    
    // Verify it's Castlevania 64
    if (!CV64_M64P_IsCV64ROM()) {
        MessageBoxA(g_hWnd,
            "The loaded ROM doesn't appear to be Castlevania 64.\n"
            "Some features may not work correctly.",
            "CV64 Recomp - ROM Warning", MB_ICONWARNING);
    }
    
    return true;
}

/**
 * @brief Initialize all recomp subsystems
 */
static bool InitializeRecomp(HWND hWnd)
{
    if (g_isInitialized) {
        return true;
    }

    OutputDebugStringA("[CV64] Initializing CV64 Recomp...\n");

    // Initialize settings system first
    if (!CV64_Settings_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize settings system\n");
    } else {
        OutputDebugStringA("[CV64] Settings system initialized\n");
    }

    // Register fullscreen callback for menu bar auto-hide
    CV64_VidExt_SetFullscreenCallback(UpdateMenuBarVisibility);
    OutputDebugStringA("[CV64] Fullscreen callback registered\n");

    // Initialize mupen64plus integration
    if (!CV64_M64P_Init(hWnd)) {
        MessageBoxA(hWnd, 
            CV64_M64P_GetLastError(),
            "CV64 Recomp - Initialization Error", MB_ICONERROR);
        
        // List plugins for debugging
        CV64_M64P_ListPlugins();
        
        // Fall back to our basic systems
        OutputDebugStringA("[CV64] mupen64plus init failed, running in standalone mode\n");
    } else {
        OutputDebugStringA("[CV64] mupen64plus core initialized successfully\n");
        // List plugins for debugging
        CV64_M64P_ListPlugins();
    }

    // Initialize controller system
    if (!CV64_Controller_Init()) {
        MessageBoxW(hWnd, L"Failed to initialize controller system!", L"CV64 Recomp Error", MB_ICONERROR);
        return false;
    }

    // Initialize input remapping system (loads saved profiles from disk)
    if (!CV64_InputRemapping_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize input remapping system\n");
    } else {
        OutputDebugStringA("[CV64] Input remapping system initialized\n");
    }

    // Initialize memory pak editor system
    if (!CV64_MempakEditor_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize memory pak editor\n");
    } else {
        OutputDebugStringA("[CV64] Memory pak editor initialized\n");
    }

    // Initialize save state manager
    if (!CV64_SaveState_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize save state manager\n");
    } else {
        OutputDebugStringA("[CV64] Save state manager initialized\n");
    }

    // Initialize mod loader system
    if (!CV64_ModLoader_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize mod loader\n");
    } else {
        OutputDebugStringA("[CV64] Mod loader initialized\n");
        // Scan and load mods
        uint32_t modCount = CV64_ModLoader_ScanMods();
        char logMsg[256];
        sprintf_s(logMsg, "[CV64] Found %u mods\n", modCount);
        OutputDebugStringA(logMsg);
        CV64_ModLoader_LoadEnabledMods();
    }

    // Initialize 3D model viewer
    if (!CV64_ModelViewer_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize model viewer\n");
    } else {
        OutputDebugStringA("[CV64] 3D model viewer initialized\n");
    }

    // Initialize patch system
    if (!CV64_Patches_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize patch system\n");
    } else {
        OutputDebugStringA("[CV64] Patch system initialized\n");
    }

    // Initialize animation interpolation system
    if (!CV64_AnimInterp_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize animation interpolation\n");
    } else {
        OutputDebugStringA("[CV64] Animation interpolation system initialized\n");

        // Register callbacks so CV64AnimBridge feeds data into cv64_anim_interp
        CV64_AnimBridge_SetCallbacks(
            AnimBridge_OnCapture,
            AnimBridge_OnLogicTick,
            AnimBridge_OnUpdate,
            AnimBridge_OnMapChange,
            AnimBridge_OnGetPose);
        OutputDebugStringA("[CV64] Animation bridge callbacks registered\n");
    }

    // Initialize camera patch (THE MOST IMPORTANT FEATURE!)
    if (!CV64_CameraPatch_Init()) {
        MessageBoxW(hWnd, L"Failed to initialize camera patch!", L"CV64 Recomp Error", MB_ICONERROR);
        return false;
    }

    // Enable D-PAD camera control by default
    CV64_Controller_SetDPadCameraMode(true);
    CV64_CameraPatch_SetEnabled(true);

    // Enable performance cheats by default
    CV64_Memory_SetLagReductionCheat(true);
    CV64_Memory_SetForestFpsFixCheat(false);  // DISABLED — overrides fog in Forest of Silence
    CV64_Memory_SetExtendedDrawDistance(false);  // DISABLED — causes fog washout artifacts
    OutputDebugStringA("[CV64] Performance cheats enabled: lag reduction, forest FPS fix, extended draw distance\n");

    // Set our frame callback for patches - hooks into VidExt_GLSwapBuf which
    // is called every frame by GLideN64, making it the ideal hook point
    CV64_VidExt_SetFrameCallback(OnFrameCallback, NULL);

    // Initialize performance overlay (OFF by default, toggle with F3)
    if (!CV64_PerfOverlay_Init()) {
        OutputDebugStringA("[CV64] WARNING: Failed to initialize performance overlay\n");
    } else {
        OutputDebugStringA("[CV64] Performance overlay initialized (press F3 to toggle)\n");
        // Set initial mode from settings
        const CV64_AllSettings& settings = CV64_Settings_Get();
        if (settings.threading.enablePerfOverlay) {
            CV64_PerfOverlay_SetMode((CV64_PerfOverlayMode)settings.threading.perfOverlayMode);
        }
    }

    // Initialize threading system for async graphics, audio, and worker tasks
    {
        CV64_ThreadConfig threadConfig = {
            .enableAsyncGraphics = true,      // Async frame presentation with triple buffering
            .enableAsyncAudio = true,         // Audio ring buffer for smooth playback
            .enableWorkerThreads = true,      // Worker thread pool for async tasks
            .workerThreadCount = 0,           // Auto-detect (hardware_concurrency - 2)
            .graphicsQueueDepth = 2,          // Triple buffering (2 queued + 1 presenting)
            .enableParallelRSP = false        // Experimental RSP threading - disabled
        };
        
        if (CV64_Threading_Init(&threadConfig)) {
            OutputDebugStringA("[CV64] Threading system initialized:\n");
            OutputDebugStringA("[CV64]   - Graphics Thread: Async frame presentation with triple buffering\n");
            OutputDebugStringA("[CV64]   - Audio Ring Buffer: Smooth audio with reduced underruns\n");
            OutputDebugStringA("[CV64]   - Worker Thread Pool: For async tasks (texture loading, etc.)\n");
            OutputDebugStringA("[CV64]   - Frame Pacing: VSync-aware frame timing\n");
            OutputDebugStringA("[CV64]   - Thread Statistics: Performance monitoring enabled\n");
        } else {
            OutputDebugStringA("[CV64] WARNING: Failed to initialize threading system (fallback to sync mode)\n");
        }
    }

    // Initialize GLideN64 optimization system for CV64-specific performance gains
    {
        CV64_OptConfig optConfig = {};
        optConfig.profile = CV64_OPT_PROFILE_BALANCED;
        optConfig.textureCacheSizeMB = 100;
        optConfig.enableTexturePreload = true;
        optConfig.enableTextureBatching = true;
        optConfig.textureHashCacheSize = 4096;
        optConfig.enableDrawCallBatching = true;
        optConfig.enableStateChangeReduction = true;
        optConfig.maxBatchedTriangles = 1024;
        optConfig.enableFBOptimization = true;
        optConfig.enableAreaOptimization = true;
        optConfig.optimizeFogRendering = false;
        optConfig.optimizeShadows = false;
        optConfig.enableMipmapping = true;
        optConfig.shadowMapResolution = 1024;
        
        if (CV64_Optimize_Init(&optConfig)) {
            OutputDebugStringA("[CV64] GLideN64 optimization system initialized:\n");
            OutputDebugStringA("[CV64]   - Texture Cache: Optimized for CV64 texture patterns\n");
            OutputDebugStringA("[CV64]   - Draw Call Batching: Reduced GPU overhead\n");
            OutputDebugStringA("[CV64]   - Area Optimization: Per-area rendering hints\n");
            OutputDebugStringA("[CV64]   - Texture Preloading: Known CV64 textures prioritized\n");
        } else {
            OutputDebugStringA("[CV64] WARNING: GLideN64 optimization system failed to initialize\n");
        }
    }

    // Load splash screen bitmap from assets folder
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string basePath(exePath);
    size_t lastSlash = basePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    }
    std::string splashPath = basePath + "assets\\" + SPLASH_BITMAP_FILENAME;
    
    g_splashBitmap = (HBITMAP)LoadImageA(NULL, splashPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (g_splashBitmap) {
        BITMAP bm;
        GetObject(g_splashBitmap, sizeof(bm), &bm);
        g_splashWidth = bm.bmWidth;
        g_splashHeight = bm.bmHeight;
        OutputDebugStringA("[CV64] Splash bitmap loaded from: ");
        OutputDebugStringA(splashPath.c_str());
        OutputDebugStringA("\n");
    } else {
        OutputDebugStringA("[CV64] WARNING: Failed to load splash bitmap from: ");
        OutputDebugStringA(splashPath.c_str());
        OutputDebugStringA("\n");
    }

    g_isInitialized = true;
    g_lastFrameTime = std::chrono::high_resolution_clock::now();

    OutputDebugStringA("[CV64] Initialization complete\n");

    return true;
}

/**
 * @brief Shutdown all recomp subsystems
 */
static void ShutdownRecomp(void)
{
    if (!g_isInitialized) {
        return;
    }

    // Stop emulation if running
    if (CV64_M64P_IsRunning()) {
        CV64_M64P_Stop();
    }

    // Shutdown optimization system
    CV64_Optimize_Shutdown();

    // Shutdown threading system (waits for all threads to complete)
    CV64_Threading_Shutdown();

    CV64_AnimInterp_Shutdown();
    CV64_Patches_Shutdown();
    CV64_CameraPatch_Shutdown();
    CV64_InputRemapping_Shutdown();
    CV64_MempakEditor_Shutdown();
    CV64_SaveState_Shutdown();
    CV64_ModLoader_Shutdown();
    CV64_ModelViewer_Shutdown();
    CV64_Controller_Shutdown();
    CV64_M64P_Shutdown();
    CV64_Settings_Shutdown();
    CV64_PerfOverlay_Shutdown();

    // Clean up splash bitmap
    if (g_splashBitmap) {
        DeleteObject(g_splashBitmap);
        g_splashBitmap = NULL;
    }

    g_isInitialized = false;
}

/**
 * @brief Check if gamepad Start button was just pressed (edge detection)
 * @return TRUE if Start was just pressed this frame
 */
static bool CheckGamepadStartButton(void)
{
    bool startPressed = false;
    
    // Check all connected XInput controllers
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
                startPressed = true;
                break;
            }
        }
    }
    
    // Edge detection - only return true on the frame Start was first pressed
    bool justPressed = startPressed && !g_gamepadStartWasPressed;
    g_gamepadStartWasPressed = startPressed;
    
    return justPressed;
}

/**
 * @brief Update timing for frame limiting
 */
static void UpdateTiming(void)
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(currentTime - g_lastFrameTime).count();
    g_lastFrameTime = currentTime;
    
    g_deltaTime = elapsed / 1000.0; // Convert to seconds
    g_frameAccumulator += elapsed;
    g_frameCount++;

    // Update performance overlay
    CV64_PerfOverlay_Update(g_deltaTime);
    CV64_PerfOverlay_RecordFrame(elapsed);

    // Update FPS counter every second
    if (g_frameAccumulator >= 1000.0) {
        g_fps = g_frameCount;
        g_frameCount = 0;
        g_frameAccumulator = 0.0;

        // Update window title with game info
        WCHAR titleWithInfo[512];
        
        // Debug: Check emulation state
        bool isRunning = CV64_M64P_IsRunning();
        bool memInitialized = CV64_Memory_IsInitialized();
        
        if (g_emulationStarted && isRunning && memInitialized) {
            // Get game info from memory
            CV64_GameInfo gameInfo;
            if (CV64_Memory_GetGameInfo(&gameInfo) && gameInfo.mapId >= 0 &&
                gameInfo.gameState != CV64_STATE_NOT_RUNNING &&
                gameInfo.gameState != CV64_STATE_TITLE_SCREEN &&
                strcmp(gameInfo.characterName, "Unknown") != 0) {

                // Convert strings to wide
                WCHAR mapNameW[64], charNameW[32];
                MultiByteToWideChar(CP_UTF8, 0, gameInfo.mapName, -1, mapNameW, 64);
                MultiByteToWideChar(CP_UTF8, 0, gameInfo.characterName, -1, charNameW, 32);

                // Get time of day and subweapon names
                const char* timeOfDay = CV64_Memory_GetTimeOfDayString();
                const char* subweaponName = CV64_Memory_GetSubweaponName(gameInfo.subweapon);
                WCHAR timeW[16], subwpnW[16];
                MultiByteToWideChar(CP_UTF8, 0, timeOfDay, -1, timeW, 16);
                MultiByteToWideChar(CP_UTF8, 0, subweaponName, -1, subwpnW, 16);

                // Build full title with all game info
                if (gameInfo.health > 0 && gameInfo.maxHealth > 0) {
                    // Base: "Castlevania 64 - Reinhardt | Forest of Silence | HP: 95/100"
                    WCHAR extra[128] = L"";
                    int extraLen = 0;

                    // Add subweapon if equipped
                    if (gameInfo.subweapon > 0 && gameInfo.subweapon <= 4) {
                        extraLen += swprintf_s(extra + extraLen, 128 - extraLen,
                            L" | %s x%d", subwpnW, gameInfo.subweaponAmmo);
                    }

                    // Add gold if non-zero
                    if (gameInfo.gold > 0) {
                        extraLen += swprintf_s(extra + extraLen, 128 - extraLen,
                            L" | %uG", gameInfo.gold);
                    }

                    // Boss fight indicator
                    if (gameInfo.bossBarVisible) {
                        extraLen += swprintf_s(extra + extraLen, 128 - extraLen, L" | BOSS");
                    }

                    swprintf_s(titleWithInfo,
                        L"Castlevania 64 - %s | %s | HP: %d/%d | %s%s | FPS: %d",
                        charNameW, mapNameW, gameInfo.health, gameInfo.maxHealth,
                        timeW, extra, g_fps);
                } else {
                    // No player stats (menu, cutscene, etc.)
                    swprintf_s(titleWithInfo,
                        L"Castlevania 64 - %s | %s | FPS: %d",
                        charNameW, mapNameW, g_fps);
                }
            } else {
                // Game running but no valid gameplay data yet
                swprintf_s(titleWithInfo, L"Castlevania 64 PC Recomp | FPS: %d", g_fps);
            }
        } else if (g_emulationStarted && isRunning) {
            // Emulation running but memory not initialized yet
            swprintf_s(titleWithInfo, L"Castlevania 64 PC Recomp - Gameplay | FPS: %d", g_fps);
        } else {
            // Emulation not running
            swprintf_s(titleWithInfo, L"Castlevania 64 PC Recomp | FPS: %d", g_fps);
        }
        
        SetWindowTextW(g_hWnd, titleWithInfo);
    }
}

/**
 * @brief Run one frame of game logic
 */
static void RunGameFrame(void)
{
    if (!g_isInitialized || g_isPaused) {
        return;
    }

    // If emulation is running, let mupen64plus handle it
    if (g_emulationStarted && CV64_M64P_IsRunning()) {
        // The frame callback handles our patches
        return;
    }

    // Update controller input
    CV64_Controller_Update(0);

    // Update camera patch
    CV64_CameraPatch_Update((f32)g_deltaTime);

    // Request redraw
    InvalidateRect(g_hWnd, NULL, FALSE);
}

/**
 * @brief Render the current frame
 */
static void RenderFrame(HDC hdc)
{
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);

    // If emulation is running, don't draw our overlay (mupen64plus handles rendering)
    // But DO render the performance overlay on top
    if (g_emulationStarted && CV64_M64P_IsRunning()) {
        // Render performance overlay on top of emulation
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;
        CV64_PerfOverlay_Render(clientWidth, clientHeight);
        return;
    }

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Draw splash bitmap stretched across the entire window
    if (g_splashBitmap && g_splashWidth > 0 && g_splashHeight > 0) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, g_splashBitmap);
        
        // Use StretchBlt to stretch the bitmap to fill the window
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, clientWidth, clientHeight,
                   memDC, 0, 0, g_splashWidth, g_splashHeight, SRCCOPY);
        
        SelectObject(memDC, oldBitmap);
        DeleteDC(memDC);
        
        // Draw "Press SPACE or START to play" text overlay
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        HFONT hFont = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        const wchar_t* prompt = L"Press SPACE or START to play";
        RECT textRect = clientRect;
        textRect.top = clientHeight - 60;
        DrawTextW(hdc, prompt, -1, &textRect, DT_CENTER | DT_SINGLELINE);
        
        DeleteObject(SelectObject(hdc, hOldFont));
    } else {
        // Fallback: black background with text if bitmap not loaded
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &clientRect, blackBrush);
        DeleteObject(blackBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(200, 0, 0));

        HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        const wchar_t* title = L"CASTLEVANIA 64 RECOMP";
        RECT titleRect = clientRect;
        titleRect.top = clientHeight / 3;
        DrawTextW(hdc, title, -1, &titleRect, DT_CENTER | DT_SINGLELINE);

        DeleteObject(SelectObject(hdc, hOldFont));
        
        hFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        SelectObject(hdc, hFont);

        SetTextColor(hdc, RGB(255, 255, 255));
        const wchar_t* prompt = L"Press SPACE or START to play";
        RECT promptRect = clientRect;
        promptRect.top = clientHeight / 2;
        DrawTextW(hdc, prompt, -1, &promptRect, DT_CENTER | DT_SINGLELINE);

        DeleteObject(SelectObject(hdc, hOldFont));
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize dark mode support BEFORE creating any windows
    // This allows menus to use dark mode from the start
    InitializeAppDarkMode();

    // Initialize Common Controls with Visual Styles (ComCtl32 v6)
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | 
                 ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Initialize global strings
    wcscpy_s(szTitle, L"Castlevania 64 PC Recomp");
    LoadStringW(hInstance, IDC_CV64RMG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // Initialize recomp subsystems
    if (!InitializeRecomp(g_hWnd))
    {
        return FALSE;
    }

    g_isRunning = true;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CV64RMG));

    MSG msg;

    // Main game loop (non-blocking message processing)
    while (g_isRunning)
    {
        // Process all pending Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                g_isRunning = false;
                break;
            }

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (!g_isRunning) break;

        // Check if emulation stopped - restore splash screen and allow restart
        // Only check if we think emulation is running to avoid unnecessary polling
        if (g_emulationStarted) {
            // Safe check - CV64_M64P_IsRunning handles null/uninitialized states
            bool stillRunning = CV64_M64P_IsRunning();
            if (!stillRunning) {
                g_emulationStarted = false;
                // Restore splash screen title and force repaint
                SetWindowTextW(g_hWnd, L"CASTLEVANIA 64 RECOMP - Press SPACE to start");
                InvalidateRect(g_hWnd, NULL, TRUE);
            }
        }
        
        // Check for gamepad Start button to start emulation (when on splash screen)
        if (!g_emulationStarted && !CV64_M64P_IsRunning()) {
            if (CheckGamepadStartButton()) {
                // Start emulation - same logic as spacebar
                if (LoadDefaultROM()) {
                    if (CV64_M64P_Start()) {
                        g_emulationStarted = true;
                        SetWindowTextW(g_hWnd, L"Castlevania 64 PC Recomp - Starting...");
                        InvalidateRect(g_hWnd, NULL, TRUE);
                        OutputDebugStringA("[CV64] Emulation started via gamepad Start button\n");
                    }
                }
            }
        }

        // Update timing
        UpdateTiming();

        // Run game frame (only if not in emulation mode)
        if (!g_emulationStarted) {
            RunGameFrame();
        }

        // Simple frame limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<double, std::milli>(frameEnd - g_lastFrameTime).count();
        if (frameTime < FRAME_TIME_MS)
        {
            Sleep((DWORD)(FRAME_TIME_MS - frameTime));
        }
    }


    // Cleanup
    ShutdownRecomp();

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CV64RMG));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CV64RMG);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hInst = hInstance; // Store instance handle in our global variable

   // Fixed window size 1280x720 for OpenGL compatibility
   int windowWidth = DEFAULT_WINDOW_WIDTH;
   int windowHeight = DEFAULT_WINDOW_HEIGHT;
   
   // Calculate window size to get desired client area
   RECT windowRect = { 0, 0, windowWidth, windowHeight };
   AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);
   windowWidth = windowRect.right - windowRect.left;
   windowHeight = windowRect.bottom - windowRect.top;

   // Center window on screen
   int screenWidth = GetSystemMetrics(SM_CXSCREEN);
   int screenHeight = GetSystemMetrics(SM_CYSCREEN);
   int x = (screenWidth - windowWidth) / 2;
   int y = (screenHeight - windowHeight) / 2;

   g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      x, y, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWnd)
   {
      return FALSE;
   }

   // Store menu handle for fullscreen toggle
   g_hMenu = GetMenu(g_hWnd);

   // Enable Windows Dark Mode for modern theming
   EnableDarkModeForWindow(g_hWnd);

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // Dark mode menu bar rendering (undocumented UAH messages)
    // Only handle these if dark mode APIs are available
    case WM_UAHDRAWMENU:
        if (g_darkModeSupported) {
            UAHMENU* pUDM = (UAHMENU*)lParam;
            if (pUDM && pUDM->hdc) {
                UAHDrawMenuBar(hWnd, pUDM);
            }
            return 0;
        }
        break;
        
    case WM_UAHDRAWMENUITEM:
        if (g_darkModeSupported) {
            UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;
            if (pUDMI && pUDMI->dis.hDC) {
                UAHDrawMenuItem(hWnd, pUDMI);
            }
            return 0;
        }
        break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_SETTINGS:
                // Show settings dialog
                CV64_Settings_ShowDialog(hWnd);
                break;
            case IDM_INPUT_REMAPPING:
                // Show input remapping dialog
                CV64_InputRemapping_ShowDialog(hWnd);
                break;
            case IDM_MEMPAK_EDITOR:
                // Show memory pak editor dialog
                CV64_MempakEditor_ShowDialog(hWnd);
                break;
            case IDM_MOD_LOADER:
                // Show mod loader dialog
                CV64_ModLoader_ShowDialog(hWnd);
                break;
            case IDM_MODEL_VIEWER:
                // Show 3D model viewer window
                CV64_ModelViewer_ShowWindow(hWnd);
                break;
            // case IDM_SAVESTATE_MANAGER:
            //     // Show save state manager dialog
            //     CV64_SaveState_ShowDialog(hWnd);
            //     break;
            case IDM_TOGGLE_FULLSCREEN:
                // Toggle fullscreen mode
                CV64_VidExt_ToggleFullscreen();
                break;
            case IDM_GAME_INFO:
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_GAME_INFO), hWnd, GameInfoDlg);
                break;
            case IDM_ABOUT:
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            // Only render splash screen when emulation is not running
            // GLideN64 handles rendering via OpenGL when emulation is active
            if (!g_emulationStarted || !CV64_M64P_IsRunning()) {
                RenderFrame(hdc);
            }
            
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_KEYDOWN:
        {
            // Handle special keys
            switch (wParam)
            {
            case VK_SPACE:
                // Start emulation (only if not already started)
                if (!g_emulationStarted && !CV64_M64P_IsRunning()) {
                    // Load ROM and start
                    if (LoadDefaultROM()) {
                        if (CV64_M64P_Start()) {
                            g_emulationStarted = true;
                            // Keep window visible - GLideN64 renders into it via video extension
                            // Window title will be updated by UpdateTiming() with game info
                            SetWindowTextW(g_hWnd, L"Castlevania 64 PC Recomp - Starting...");
                            InvalidateRect(g_hWnd, NULL, TRUE);
                        }
                    }
                }
                break;
            case VK_ESCAPE:
                // Stop emulation and return to splash screen
                if (g_emulationStarted || CV64_M64P_IsRunning() || CV64_M64P_IsPaused()) {
                    // Release mouse capture first
                    if (CV64_Controller_IsMouseCaptured()) {
                        CV64_Controller_SetMouseCapture(false);
                    }
                    
                    // Stop the emulation
                    CV64_M64P_Stop();
                    
                    // Reset emulation state
                    g_emulationStarted = false;
                    g_isPaused = false;
                    
                    // Reset window title
                    SetWindowTextW(g_hWnd, L"Castlevania 64 PC Recomp");
                    
                    // Force redraw to show splash screen
                    InvalidateRect(hWnd, NULL, TRUE);
                    
                    OutputDebugStringA("[CV64] Emulation stopped, returning to splash screen\n");
                }
                break;
            case VK_F1:
                // Toggle D-PAD camera mode
                CV64_Controller_SetDPadCameraMode(!CV64_Controller_IsDPadCameraModeEnabled());
                InvalidateRect(hWnd, NULL, TRUE);
                break;
            case VK_F2:
                // Toggle camera patch
                CV64_CameraPatch_SetEnabled(!CV64_CameraPatch_IsEnabled());
                InvalidateRect(hWnd, NULL, TRUE);
                break;
            case VK_F3:
                // Toggle performance overlay (OFF ? MINIMAL ? STANDARD ? DETAILED ? GRAPH ? OFF)
                CV64_PerfOverlay_Toggle();
                InvalidateRect(hWnd, NULL, TRUE);
                break;
            case VK_F5:
                // Quick save
                if (CV64_M64P_IsRunning()) {
                    CV64_SaveState_QuickSave(0);
                }
                break;
            case VK_F6:
                // Toggle difficulty (Easy <-> Normal)
                if (CV64_M64P_IsRunning()) {
                    s32 newDiff = CV64_Memory_ToggleDifficulty();
                    if (newDiff >= 0) {
                        wchar_t msg[128];
                        swprintf_s(msg, L"Difficulty: %hs", CV64_Memory_GetDifficultyName((u8)newDiff));
                        SetWindowTextW(g_hWnd, msg);
                    }
                }
                break;
            case VK_F9:
                // Quick load
                if (CV64_M64P_IsRunning() || g_emulationStarted) {
                    CV64_SaveState_QuickLoad(0);
                }
                break;
            case VK_F10:
                // Open input remapping dialog
                CV64_InputRemapping_ShowDialog(hWnd);
                break;
            case VK_F4:
                // Hard reset
                if (CV64_M64P_IsRunning()) {
                    CV64_M64P_Reset(true);
                }
                break;
            case 'R':
                // Reset camera or soft reset
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    CV64_CameraPatch_Reset();
                } else if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                    if (CV64_M64P_IsRunning()) {
                        CV64_M64P_Reset(false);
                    }
                }
                break;
            case 'M':
                // Toggle mouse capture for camera control
                CV64_Controller_ToggleMouseCapture();
                InvalidateRect(hWnd, NULL, TRUE);
                break;
            case 'V':
                // Toggle vibration/rumble
                CV64_Controller_SetVibrationEnabled(!CV64_Controller_IsVibrationEnabled());
                break;
            }
        }
        break;

    case WM_MOUSEMOVE:
        {
            // Mouse move is handled in controller update when captured
        }
        break;

    case WM_LBUTTONDOWN:
        {
            // Capture mouse for camera control when clicking in window during gameplay
            if (g_emulationStarted && CV64_M64P_IsRunning()) {
                if (!CV64_Controller_IsMouseCaptured()) {
                    CV64_Controller_SetMouseCapture(true);
                }
            }
        }
        break;

    case WM_RBUTTONDOWN:
        {
            // Right click releases mouse capture
            if (CV64_Controller_IsMouseCaptured()) {
                CV64_Controller_SetMouseCapture(false);
            }
        }
        break;

    case WM_MOUSEWHEEL:
        {
            // Forward mouse wheel to controller system for camera zoom
            if (g_emulationStarted && CV64_M64P_IsRunning()) {
                s32 delta = GET_WHEEL_DELTA_WPARAM(wParam);
                CV64_Controller_NotifyMouseWheel(delta);
            }
        }
        break;

    case WM_SYSKEYDOWN:
        {
            // Handle ALT+ENTER for fullscreen toggle
            if (wParam == VK_RETURN && (lParam & (1 << 29))) {
                // ALT is pressed (bit 29 in lParam indicates ALT key)
                CV64_VidExt_ToggleFullscreen();
                return 0; // Prevent default processing
            }
            // Let other system keys pass through
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_MOVE:
        {
            // Handle window move - update mouse capture center
            if (CV64_Controller_IsMouseCaptured()) {
                CV64_Controller_UpdateMouseCapture(hWnd);
            }
        }
        break;

    case WM_SIZE:
        {
            // Handle window resize - update mouse capture center
            if (CV64_Controller_IsMouseCaptured()) {
                CV64_Controller_UpdateMouseCapture(hWnd);
            }
            
            // Notify video extension of resize so GlideN64 updates viewport
            if (wParam != SIZE_MINIMIZED) {
                int newWidth = LOWORD(lParam);
                int newHeight = HIWORD(lParam);
                if (newWidth > 0 && newHeight > 0) {
                    CV64_VidExt_NotifyResize(newWidth, newHeight);
                }
            }
        }
        break;

    case WM_SETFOCUS:
        {
            // Regained focus
            g_isPaused = false;
        }
        break;

    case WM_KILLFOCUS:
        {
            // Lost focus - pause and release mouse
            g_isPaused = true;
            if (CV64_Controller_IsMouseCaptured()) {
                CV64_Controller_SetMouseCapture(false);
            }
        }
        break;

    case WM_CLOSE:
        {
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_DESTROY:
        g_isRunning = false;
        PostQuitMessage(0);
        break;

    case WM_SETTINGCHANGE:
        {
            // Respond to system theme changes (dark mode toggle)
            if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
                // System theme changed - refresh dark mode
                EnableDarkModeForWindow(hWnd);
                if (g_FlushMenuThemes) {
                    g_FlushMenuThemes();
                }
                // Force menu bar redraw
                DrawMenuBar(hWnd);
                OutputDebugStringA("[CV64] System theme changed - refreshed dark mode\n");
            }
        }
        break;

    case WM_THEMECHANGED:
        {
            // Theme changed - refresh dark mode settings
            EnableDarkModeForWindow(hWnd);
            if (g_FlushMenuThemes) {
                g_FlushMenuThemes();
            }
            DrawMenuBar(hWnd);
        }
        break;

    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            // Let Windows draw the non-client area first
            LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);
            
            // Now paint over the white line under the menu bar with our dark color
            if (g_darkModeSupported) {
                MENUBARINFO mbi = { sizeof(mbi) };
                if (GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi)) {
                    RECT rcWindow;
                    if (GetWindowRect(hWnd, &rcWindow)) {
                        // The white line is 1 pixel below the menu bar
                        RECT rcLine;
                        rcLine.left = mbi.rcBar.left - rcWindow.left;
                        rcLine.right = mbi.rcBar.right - rcWindow.left;
                        rcLine.top = mbi.rcBar.bottom - rcWindow.top;
                        rcLine.bottom = rcLine.top + 1;
                        
                        HDC hdc = GetWindowDC(hWnd);
                        if (hdc) {
                            HBRUSH hBrush = CreateSolidBrush(DARK_MENU_BAR_BG);
                            FillRect(hdc, &rcLine, hBrush);
                            DeleteObject(hBrush);
                            ReleaseDC(hWnd, hdc);
                        }
                    }
                }
            }
            return result;
        }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    static HBITMAP hLogoBitmap = NULL;
    static HBITMAP hTransparentBitmap = NULL;
    
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Enable dark mode for the about dialog
        EnableDarkModeForDialog(hDlg);
        
        // Load the logo bitmap from assets folder
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string basePath(exePath);
        size_t lastSlash = basePath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        }
        std::string logoPath = basePath + "assets\\RETRO_LOGO.bmp";
        
        // Load bitmap from file as DIB section for pixel manipulation
        hLogoBitmap = (HBITMAP)LoadImageA(NULL, logoPath.c_str(), IMAGE_BITMAP, 
                                          0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        
        if (hLogoBitmap) {
            // Get bitmap info
            BITMAP bm;
            GetObject(hLogoBitmap, sizeof(BITMAP), &bm);
            
            // Use dark mode background color if dark mode is enabled
            COLORREF bgColor = g_darkModeSupported ? DARK_DLG_BG_COLOR : GetSysColor(COLOR_3DFACE);
            
            // Create a compatible bitmap for the result
            HDC hdcScreen = GetDC(NULL);
            HDC hdcSrc = CreateCompatibleDC(hdcScreen);
            HDC hdcDst = CreateCompatibleDC(hdcScreen);
            
            hTransparentBitmap = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
            
            HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, hLogoBitmap);
            HBITMAP hOldDst = (HBITMAP)SelectObject(hdcDst, hTransparentBitmap);
            
            // Fill destination with dialog background color
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
            FillRect(hdcDst, &rc, hBrush);
            DeleteObject(hBrush);
            
            // Use TransparentBlt to copy bitmap with white as transparent
            TransparentBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight,
                          hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight,
                          RGB(255, 255, 255)); // White is transparent
            
            SelectObject(hdcSrc, hOldSrc);
            SelectObject(hdcDst, hOldDst);
            DeleteDC(hdcSrc);
            DeleteDC(hdcDst);
            ReleaseDC(NULL, hdcScreen);
            
            // Set the transparent bitmap to the static control
            SendDlgItemMessage(hDlg, IDC_ABOUT_LOGO, STM_SETIMAGE, 
                             IMAGE_BITMAP, (LPARAM)hTransparentBitmap);
            
            
            OutputDebugStringA("[CV64] About dialog logo loaded successfully\n");
        } else {
            OutputDebugStringA("[CV64] WARNING: Failed to load about dialog logo from: ");
            OutputDebugStringA(logoPath.c_str());
            OutputDebugStringA("\n");
        }
        
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            // Clean up bitmaps
            if (hLogoBitmap) {
                DeleteObject(hLogoBitmap);
                hLogoBitmap = NULL;
            }
            if (hTransparentBitmap) {
                DeleteObject(hTransparentBitmap);
                hTransparentBitmap = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        
    case WM_DESTROY:
        // Clean up bitmaps if dialog is destroyed
        if (hLogoBitmap) {
            DeleteObject(hLogoBitmap);
            hLogoBitmap = NULL;
        }
        if (hTransparentBitmap) {
            DeleteObject(hTransparentBitmap);
            hTransparentBitmap = NULL;
        }
        break;
        
    // Dark mode color handling
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    {
        HBRUSH hBrush = HandleDarkModeCtlColor(hDlg, message, (HDC)wParam, (HWND)lParam);
        if (hBrush) {
            return (INT_PTR)hBrush;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

// ============================================================================
// Game Info Dialog
// ============================================================================

static const char* GameStateToString(CV64_GameState state) {
    switch (state) {
    case CV64_STATE_NOT_RUNNING:      return "Not Running";
    case CV64_STATE_TITLE_SCREEN:     return "Title Screen";
    case CV64_STATE_CHARACTER_SELECT: return "Character Select";
    case CV64_STATE_GAMEPLAY:         return "Gameplay";
    case CV64_STATE_CUTSCENE:         return "Cutscene";
    case CV64_STATE_MENU:             return "Pause Menu";
    case CV64_STATE_READING_TEXT:     return "Reading Text";
    case CV64_STATE_FIRST_PERSON:     return "First Person";
    case CV64_STATE_DOOR_TRANSITION:  return "Door Transition";
    case CV64_STATE_FADING:           return "Fading";
    case CV64_STATE_SOFT_RESET:       return "Soft Reset";
    default:                          return "Unknown";
    }
}

static void PopulateGameInfoFields(HWND hDlg) {
    CV64_GameInfo info;
    bool valid = CV64_Memory_GetGameInfo(&info);

    SetDlgItemTextA(hDlg, IDC_GAMEINFO_CHARNAME, info.characterName);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_MAP, info.mapName);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_STATE, GameStateToString(info.gameState));

    char buf[128];

    if (info.health >= 0 && info.maxHealth > 0)
        sprintf_s(buf, "%d / %d", info.health, info.maxHealth);
    else
        sprintf_s(buf, "--");
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_HP, buf);

    sprintf_s(buf, "%u", info.gold);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_GOLD, buf);

    sprintf_s(buf, "%u", (unsigned)info.jewels);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_JEWELS, buf);

    const char* subName = CV64_Memory_GetSubweaponName(info.subweapon);
    if (info.subweapon > 0 && info.subweapon <= 4)
        sprintf_s(buf, "%s (x%d)", subName, info.subweaponAmmo);
    else
        sprintf_s(buf, "None");
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_SUBWEAPON, buf);

    const char* timeStr = CV64_Memory_GetTimeOfDayString();
    sprintf_s(buf, "%s (raw: %u)", timeStr, info.gameTime);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_TIME, buf);

    sprintf_s(buf, "%s", info.difficulty == 0 ? "Easy" : "Normal");
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_DIFFICULTY, buf);

    sprintf_s(buf, "Slot %d", info.saveFileNumber);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_SAVEFILE, buf);

    SetDlgItemTextA(hDlg, IDC_GAMEINFO_BOSS, info.bossBarVisible ? "Active" : "None");

    sprintf_s(buf, "Start: %u  End: %u  Ambient: %.2f", info.fogDistStart, info.fogDistEnd, info.ambientBrightness);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_FOG, buf);

    sprintf_s(buf, "Mode %u%s%s", info.cameraMode,
        info.isFirstPerson ? " (1st Person)" : "",
        info.isRLocked ? " [R-Lock]" : "");
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_CAMERA, buf);

    sprintf_s(buf, "X:%.1f  Y:%.1f  Z:%.1f", info.playerX, info.playerY, info.playerZ);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_POSITION, buf);

    sprintf_s(buf, "CPU: %.0f%%  GFX: %.0f%%",
        info.processMeterGreen * 100.0f, info.processMeterBlue * 100.0f);
    SetDlgItemTextA(hDlg, IDC_GAMEINFO_PROCMETER, buf);
}

INT_PTR CALLBACK GameInfoDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    static HBITMAP hPortraitBitmap = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        EnableDarkModeForDialog(hDlg);

        // Load character portrait from assets folder
        CV64_GameInfo info;
        CV64_Memory_GetGameInfo(&info);

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string basePath(exePath);
        size_t lastSlash = basePath.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            basePath = basePath.substr(0, lastSlash + 1);

        // Build portrait path: assets\Reinhardt.bmp or assets\Carrie.bmp
        std::string portraitPath = basePath + "assets\\" + info.characterName + ".bmp";

        hPortraitBitmap = (HBITMAP)LoadImageA(NULL, portraitPath.c_str(), IMAGE_BITMAP,
                                              0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (hPortraitBitmap) {
            SendDlgItemMessage(hDlg, IDC_GAMEINFO_PORTRAIT, STM_SETIMAGE,
                             IMAGE_BITMAP, (LPARAM)hPortraitBitmap);
        }

        PopulateGameInfoFields(hDlg);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (hPortraitBitmap) {
                DeleteObject(hPortraitBitmap);
                hPortraitBitmap = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDRETRY)
        {
            // Refresh: reload portrait in case character changed
            CV64_GameInfo info;
            CV64_Memory_GetGameInfo(&info);

            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);
            std::string basePath(exePath);
            size_t lastSlash = basePath.find_last_of("\\/");
            if (lastSlash != std::string::npos)
                basePath = basePath.substr(0, lastSlash + 1);

            std::string portraitPath = basePath + "assets\\" + info.characterName + ".bmp";

            if (hPortraitBitmap) {
                DeleteObject(hPortraitBitmap);
                hPortraitBitmap = NULL;
            }
            hPortraitBitmap = (HBITMAP)LoadImageA(NULL, portraitPath.c_str(), IMAGE_BITMAP,
                                                  0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
            if (hPortraitBitmap) {
                SendDlgItemMessage(hDlg, IDC_GAMEINFO_PORTRAIT, STM_SETIMAGE,
                                 IMAGE_BITMAP, (LPARAM)hPortraitBitmap);
            }

            PopulateGameInfoFields(hDlg);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_DESTROY:
        if (hPortraitBitmap) {
            DeleteObject(hPortraitBitmap);
            hPortraitBitmap = NULL;
        }
        break;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    {
        HBRUSH hBrush = HandleDarkModeCtlColor(hDlg, message, (HDC)wParam, (HWND)lParam);
        if (hBrush) {
            return (INT_PTR)hBrush;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}
