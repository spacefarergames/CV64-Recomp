/**
 * @file cv64_model_viewer.cpp
 * @brief CV64 3D Model Viewer Implementation
 */

#include "../include/cv64_model_viewer.h"
#include "../include/cv64_model_database.h"
#include "../include/cv64_rom_reader.h"
#include "../include/cv64_n64_parser.h"
#include "../include/cv64_vidext.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <gl/GL.h>
#include <CommCtrl.h>
#include <windowsx.h>

#pragma comment(lib, "opengl32.lib")

// Constants
#define VIEWER_WINDOW_WIDTH  1280
#define VIEWER_WINDOW_HEIGHT 768
#define VIEWER_WINDOW_CLASS  L"CV64ModelViewerClass"

// UI Layout
#define MODEL_LIST_WIDTH 250
#define INFO_PANEL_HEIGHT 120

// Internal state
static bool g_initialized = false;
static HWND g_viewerHwnd = NULL;
static HWND g_viewportHwnd = NULL;
static HWND g_modelListHwnd = NULL;
static HWND g_infoPanelHwnd = NULL;
static HDC g_viewerHdc = NULL;
static HGLRC g_viewerGlrc = NULL;
static CV64_ViewerCamera g_camera = { 0 };
static CV64_ViewerState g_state = CV64_VIEWER_STATE_IDLE;
static std::vector<CV64_ModelInfo> g_models;
static CV64_ModelInfo g_currentModel = { 0 };
static bool g_wireframeMode = false;
static bool g_texturesEnabled = true;
static bool g_lightingEnabled = true;
static bool g_animationPlaying = false;
static uint32_t g_currentFrame = 0;
static float g_fps = 0.0f;
static char g_romPath[512] = { 0 };
static CV64_ROMFile g_romFile = NULL;
static CV64_ParsedGeometry g_currentGeometry = { 0 };
static bool g_geometryLoaded = false;

// Dialog controls
#define IDC_MODEL_LIST          3000
#define IDC_MODEL_LOAD          3001
#define IDC_MODEL_EXPORT        3002
#define IDC_MODEL_INFO          3003
#define IDC_WIREFRAME_CHECK     3004
#define IDC_TEXTURES_CHECK      3005
#define IDC_LIGHTING_CHECK      3006
#define IDC_ANIMATION_PLAY      3007
#define IDC_ANIMATION_SLIDER    3008
#define IDC_RESET_CAMERA        3009
#define IDC_INFO_TEXT           3010

/**
 * @brief Initialize OpenGL for model viewer
 */
static bool InitializeOpenGL(HWND hwnd) {
    g_viewerHdc = GetDC(hwnd);
    if (!g_viewerHdc) {
        return false;
    }
    
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int pixelFormat = ChoosePixelFormat(g_viewerHdc, &pfd);
    if (!pixelFormat) {
        return false;
    }
    
    if (!SetPixelFormat(g_viewerHdc, pixelFormat, &pfd)) {
        return false;
    }
    
    g_viewerGlrc = wglCreateContext(g_viewerHdc);
    if (!g_viewerGlrc) {
        return false;
    }
    
    if (!wglMakeCurrent(g_viewerHdc, g_viewerGlrc)) {
        return false;
    }
    
    // Setup OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    
    // Setup lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    float lightPos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    float lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    float lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    
    // Set initial viewport
    RECT rect;
    GetClientRect(hwnd, &rect);
    glViewport(0, 0, rect.right, rect.bottom);
    
    wglMakeCurrent(NULL, NULL);
    
    return true;
}

/**
 * @brief Cleanup OpenGL
 */
static void CleanupOpenGL(void) {
    if (g_viewerGlrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_viewerGlrc);
        g_viewerGlrc = NULL;
    }
    
    if (g_viewerHdc && g_viewportHwnd) {
        ReleaseDC(g_viewportHwnd, g_viewerHdc);
        g_viewerHdc = NULL;
    }
}

/**
 * @brief Render parsed geometry
 */
static void RenderParsedGeometry(const CV64_ParsedGeometry* geom) {
    if (!geom || !geom->vertices || !geom->indices) {
        return;
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, 0, geom->vertices);
    glNormalPointer(GL_FLOAT, 0, geom->normals);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, geom->colors);
    
    glDrawElements(GL_TRIANGLES, geom->indexCount, GL_UNSIGNED_SHORT, geom->indices);
    
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

/**
 * @brief Render a simple test cube
 */
static void RenderTestCube(void) {
    glBegin(GL_QUADS);
    
    // Front face (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    
    // Back face (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    
    // Top face (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    
    // Bottom face (yellow)
    glColor3f(1.0f, 1.0f, 0.0f);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    
    // Right face (cyan)
    glColor3f(0.0f, 1.0f, 1.0f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    
    // Left face (magenta)
    glColor3f(1.0f, 0.0f, 1.0f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    
    glEnd();
}

/**
 * @brief Update model info panel
 */
static void UpdateInfoPanel(void) {
    if (!g_infoPanelHwnd) return;
    
    char infoText[1024];
    if (g_currentModel.loaded) {
        sprintf_s(infoText, 
            "Model: %s\r\n"
            "Type: %s\r\n"
            "Vertices: %u | Triangles: %u\r\n"
            "Textures: %u\r\n"
            "ROM Offset: 0x%08X | Size: 0x%X bytes",
            g_currentModel.name,
            g_currentModel.type == CV64_MODEL_TYPE_CHARACTER ? "Character" :
            g_currentModel.type == CV64_MODEL_TYPE_ENEMY ? "Enemy" :
            g_currentModel.type == CV64_MODEL_TYPE_BOSS ? "Boss" :
            g_currentModel.type == CV64_MODEL_TYPE_ITEM ? "Item" :
            g_currentModel.type == CV64_MODEL_TYPE_WEAPON ? "Weapon" :
            g_currentModel.type == CV64_MODEL_TYPE_PROP ? "Prop" : "Unknown",
            g_currentModel.vertexCount,
            g_currentModel.triangleCount,
            g_currentModel.textureCount,
            g_currentModel.romOffset,
            g_currentModel.dataSize
        );
    } else {
        sprintf_s(infoText, 
            "No model loaded.\r\n"
            "Select a model from the list and double-click to load.\r\n\r\n"
            "Controls:\r\n"
            "Left Mouse: Orbit | Wheel: Zoom\r\n"
            "W: Wireframe | T: Textures | L: Lighting | R: Reset Camera"
        );
    }
    
    SetDlgItemTextA(g_viewerHwnd, IDC_INFO_TEXT, infoText);
}

/**
 * @brief Populate model list from database
 */
static void PopulateModelList(void) {
    if (!g_modelListHwnd) return;
    
    // Clear existing items
    SendMessageA(g_modelListHwnd, LB_RESETCONTENT, 0, 0);
    
    // Get model database
    uint32_t modelCount = 0;
    const CV64_ModelDatabaseEntry* database = CV64_GetModelDatabase(&modelCount);
    
    // Add models to list
    for (uint32_t i = 0; i < modelCount; i++) {
        char itemText[256];
        const char* typeStr = 
            database[i].modelType == CV64_MODEL_TYPE_CHARACTER ? "CHAR" :
            database[i].modelType == CV64_MODEL_TYPE_ENEMY ? "ENEMY" :
            database[i].modelType == CV64_MODEL_TYPE_BOSS ? "BOSS" :
            database[i].modelType == CV64_MODEL_TYPE_ITEM ? "ITEM" :
            database[i].modelType == CV64_MODEL_TYPE_WEAPON ? "WEAPON" :
            database[i].modelType == CV64_MODEL_TYPE_PROP ? "PROP" : "???";
        
        sprintf_s(itemText, "[%s] %s", typeStr, database[i].name);
        int index = SendMessageA(g_modelListHwnd, LB_ADDSTRING, 0, (LPARAM)itemText);
        SendMessageA(g_modelListHwnd, LB_SETITEMDATA, index, (LPARAM)database[i].modelID);
        
        // Store in internal list
        CV64_ModelInfo modelInfo = { 0 };
        modelInfo.modelID = database[i].modelID;
        strcpy_s(modelInfo.name, database[i].name);
        modelInfo.type = (CV64_ModelType)database[i].modelType;
        modelInfo.romOffset = database[i].romOffset;
        modelInfo.dataSize = database[i].dataSize;
        modelInfo.textureCount = database[i].textureCount;
        modelInfo.loaded = false;
        
        // Add to vector if not duplicate
        bool found = false;
        for (const auto& m : g_models) {
            if (m.modelID == modelInfo.modelID) {
                found = true;
                break;
            }
        }
        if (!found) {
            g_models.push_back(modelInfo);
        }
    }
    
    char statusText[256];
    sprintf_s(statusText, "Loaded %u models from database", modelCount);
    OutputDebugStringA("[CV64] ");
    OutputDebugStringA(statusText);
    OutputDebugStringA("\n");
}

/**
 * @brief Render the current model or test scene
 */
static void RenderScene(void) {
    if (!g_viewerHdc || !g_viewerGlrc) {
        return;
    }
    
    wglMakeCurrent(g_viewerHdc, g_viewerGlrc);
    
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Setup camera
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    RECT rect;
    GetClientRect(g_viewportHwnd, &rect);
    float aspect = (float)(rect.right - rect.left) / (float)(rect.bottom - rect.top);
    
    // Perspective projection
    float fov = g_camera.fov > 0.0f ? g_camera.fov : 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float top = nearPlane * tanf(fov * 3.14159f / 360.0f);
    float right = top * aspect;
    
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Position camera
    float distance = g_camera.distance > 0.0f ? g_camera.distance : 5.0f;
    float radX = g_camera.orbitAngleX * 3.14159f / 180.0f;
    float radY = g_camera.orbitAngleY * 3.14159f / 180.0f;
    
    float eyeX = distance * cosf(radY) * sinf(radX);
    float eyeY = distance * sinf(radY);
    float eyeZ = distance * cosf(radY) * cosf(radX);
    
    glTranslatef(-g_camera.panX, -g_camera.panY, 0.0f);
    
    // Look at model center
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    if (g_currentModel.loaded) {
        cx = (g_currentModel.minX + g_currentModel.maxX) * 0.5f;
        cy = (g_currentModel.minY + g_currentModel.maxY) * 0.5f;
        cz = (g_currentModel.minZ + g_currentModel.maxZ) * 0.5f;
    }
    
    glTranslatef(0.0f, 0.0f, -distance);
    glRotatef(g_camera.orbitAngleY, 1.0f, 0.0f, 0.0f);
    glRotatef(g_camera.orbitAngleX, 0.0f, 1.0f, 0.0f);
    glTranslatef(-cx, -cy, -cz);
    
    // Apply render modes
    if (g_wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    if (g_lightingEnabled) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
    
    // Render model or test cube
    if (g_currentModel.loaded && g_geometryLoaded) {
        // Render actual model from ROM
        RenderParsedGeometry(&g_currentGeometry);
    } else if (g_currentModel.loaded) {
        // Model loaded but no geometry - show placeholder
        RenderTestCube();
    } else {
        // No model loaded - show test cube
        RenderTestCube();
    }
    
    // Draw grid
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i++) {
        glVertex3f((float)i, 0.0f, -10.0f);
        glVertex3f((float)i, 0.0f,  10.0f);
        glVertex3f(-10.0f, 0.0f, (float)i);
        glVertex3f( 10.0f, 0.0f, (float)i);
    }
    glEnd();
    
    // Swap buffers
    SwapBuffers(g_viewerHdc);
    
    wglMakeCurrent(NULL, NULL);
}

/**
 * @brief Viewport child window procedure (for OpenGL rendering)
 */
static LRESULT CALLBACK ViewportWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE:
        if (g_viewerHdc && g_viewerGlrc) {
            wglMakeCurrent(g_viewerHdc, g_viewerGlrc);
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            wglMakeCurrent(NULL, NULL);
        }
        return 0;
    
    case WM_LBUTTONDOWN:
        CV64_ModelViewer_HandleMouseButton(0, true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        SetFocus(hwnd);
        return 0;
    
    case WM_LBUTTONUP:
        CV64_ModelViewer_HandleMouseButton(0, false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    
    case WM_RBUTTONDOWN:
        CV64_ModelViewer_HandleMouseButton(1, true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    
    case WM_RBUTTONUP:
        CV64_ModelViewer_HandleMouseButton(1, false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    
    case WM_MOUSEMOVE:
        CV64_ModelViewer_HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    
    case WM_MOUSEWHEEL:
        CV64_ModelViewer_HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    
    case WM_KEYDOWN:
        switch (wParam) {
        case 'W':
            CV64_ModelViewer_SetWireframe(!g_wireframeMode);
            break;
        case 'T':
            CV64_ModelViewer_SetTexturesEnabled(!g_texturesEnabled);
            break;
        case 'L':
            CV64_ModelViewer_SetLightingEnabled(!g_lightingEnabled);
            break;
        case 'R':
            CV64_ModelViewer_ResetCamera();
            break;
        }
        return 0;
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        RenderScene();
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief Viewer window procedure
 */
static LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
switch (uMsg) {
case WM_CREATE:
{
    RECT rect;
    GetClientRect(hwnd, &rect);
        
        // Create 3D viewport window (child window for OpenGL)
        g_viewportHwnd = CreateWindowExW(
            0,
            L"CV64ViewportClass",
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            MODEL_LIST_WIDTH, 0,
            rect.right - MODEL_LIST_WIDTH, rect.bottom - INFO_PANEL_HEIGHT,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
    if (!g_viewportHwnd) {
        MessageBoxW(hwnd, L"Failed to create viewport!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
        
    // Initialize OpenGL on the viewport window
    if (!InitializeOpenGL(g_viewportHwnd)) {
        MessageBoxW(hwnd, L"Failed to initialize OpenGL!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
        
    // Create model list (left panel)
    g_modelListHwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "LISTBOX",
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
            0, 0, MODEL_LIST_WIDTH, VIEWER_WINDOW_HEIGHT - INFO_PANEL_HEIGHT,
            hwnd,
            (HMENU)IDC_MODEL_LIST,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Create info panel (bottom)
        g_infoPanelHwnd = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, VIEWER_WINDOW_HEIGHT - INFO_PANEL_HEIGHT,
            VIEWER_WINDOW_WIDTH, INFO_PANEL_HEIGHT,
            hwnd,
            (HMENU)IDC_INFO_TEXT,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Set fonts
        HFONT hFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        
        SendMessage(g_modelListHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_infoPanelHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Populate model list
        PopulateModelList();
        UpdateInfoPanel();
        
        // Setup timer for rendering
        SetTimer(hwnd, 1, 16, NULL); // ~60 FPS
        return 0;
    }
    
    case WM_COMMAND:
        if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == IDC_MODEL_LIST) {
            // Double-click on model list - load selected model
            int selectedIndex = SendMessageA(g_modelListHwnd, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR) {
                uint32_t modelID = (uint32_t)SendMessageA(g_modelListHwnd, LB_GETITEMDATA, selectedIndex, 0);
                CV64_ModelViewer_LoadModel(modelID);
                UpdateInfoPanel();
            }
        }
        return 0;
    
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        CleanupOpenGL();
        g_viewerHwnd = NULL;
        g_modelListHwnd = NULL;
        g_infoPanelHwnd = NULL;
        return 0;
    
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        // Resize viewport window
        if (g_viewportHwnd) {
            SetWindowPos(g_viewportHwnd, NULL,
                MODEL_LIST_WIDTH, 0,
                width - MODEL_LIST_WIDTH, height - INFO_PANEL_HEIGHT,
                SWP_NOZORDER);
        }
        
        // Resize model list
        if (g_modelListHwnd) {
            SetWindowPos(g_modelListHwnd, NULL,
                0, 0,
                MODEL_LIST_WIDTH, height - INFO_PANEL_HEIGHT,
                SWP_NOZORDER);
        }
        
        // Resize info panel
        if (g_infoPanelHwnd) {
            SetWindowPos(g_infoPanelHwnd, NULL,
                0, height - INFO_PANEL_HEIGHT,
                width, INFO_PANEL_HEIGHT,
                SWP_NOZORDER);
        }
        return 0;
    }
    
    case WM_TIMER:
        if (wParam == 1) {
            RenderScene();
        }
        return 0;
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        // Main window doesn't need to render anything, viewport handles it
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// Public API Implementation

bool CV64_ModelViewer_Init(void) {
if (g_initialized) {
    return true;
}
    
// Register main viewer window class
WNDCLASSEXW wcex = { 0 };
wcex.cbSize = sizeof(WNDCLASSEXW);
wcex.style = CS_HREDRAW | CS_VREDRAW;
wcex.lpfnWndProc = ViewerWindowProc;
wcex.hInstance = GetModuleHandle(NULL);
wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
wcex.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
wcex.lpszClassName = VIEWER_WINDOW_CLASS;
    
if (!RegisterClassExW(&wcex)) {
    return false;
}
    
// Register viewport window class (for OpenGL rendering)
WNDCLASSEXW vcex = { 0 };
vcex.cbSize = sizeof(WNDCLASSEXW);
vcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
vcex.lpfnWndProc = ViewportWindowProc;
vcex.hInstance = GetModuleHandle(NULL);
vcex.hCursor = LoadCursor(NULL, IDC_ARROW);
vcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
vcex.lpszClassName = L"CV64ViewportClass";
    
if (!RegisterClassExW(&vcex)) {
    return false;
}
    
    // Initialize camera
    CV64_ModelViewer_ResetCamera();
    
    // Scan models from database
    CV64_ModelViewer_ScanROM(NULL);
    
    g_initialized = true;
    
    OutputDebugStringA("[CV64] Model viewer initialized\n");
    return true;
}

void CV64_ModelViewer_Shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    CV64_ModelViewer_HideWindow();
    CV64_ModelViewer_UnloadModel();
    
    // Close ROM file
    if (g_romFile) {
        CV64_ROM_Close(g_romFile);
        g_romFile = NULL;
    }
    
    g_models.clear();
    
    UnregisterClassW(L"CV64ViewportClass", GetModuleHandle(NULL));
    UnregisterClassW(VIEWER_WINDOW_CLASS, GetModuleHandle(NULL));
    
    g_initialized = false;
    
    OutputDebugStringA("[CV64] Model viewer shut down\n");
}

void CV64_ModelViewer_ShowWindow(HWND hWndParent) {
    if (!g_initialized) {
        return;
    }
    
    if (g_viewerHwnd && IsWindow(g_viewerHwnd)) {
        ShowWindow(g_viewerHwnd, SW_SHOW);
        SetForegroundWindow(g_viewerHwnd);
        return;
    }
    
    // Create viewer window
    g_viewerHwnd = CreateWindowExW(
        0,
        VIEWER_WINDOW_CLASS,
        L"CV64 3D Model Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        VIEWER_WINDOW_WIDTH, VIEWER_WINDOW_HEIGHT,
        hWndParent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (g_viewerHwnd) {
        ShowWindow(g_viewerHwnd, SW_SHOW);
        UpdateWindow(g_viewerHwnd);
    }
}

void CV64_ModelViewer_HideWindow(void) {
    if (g_viewerHwnd && IsWindow(g_viewerHwnd)) {
        DestroyWindow(g_viewerHwnd);
        g_viewerHwnd = NULL;
    }
}

bool CV64_ModelViewer_IsVisible(void) {
    return g_viewerHwnd && IsWindow(g_viewerHwnd) && IsWindowVisible(g_viewerHwnd);
}

uint32_t CV64_ModelViewer_ScanROM(const char* romPath) {
    // Close previous ROM if open
    if (g_romFile) {
        CV64_ROM_Close(g_romFile);
        g_romFile = NULL;
    }
    
    // Try to open ROM file
    if (romPath && romPath[0]) {
        strcpy_s(g_romPath, romPath);
        g_romFile = CV64_ROM_Open(romPath);
        
        if (!g_romFile) {
            // Try default ROM path
            char defaultPath[512];
            GetCurrentDirectoryA(sizeof(defaultPath), defaultPath);
            strcat_s(defaultPath, "\\assets\\Castlevania (U) (V1.2) [!].z64");
            
            g_romFile = CV64_ROM_Open(defaultPath);
            if (g_romFile) {
                strcpy_s(g_romPath, defaultPath);
            }
        }
    } else {
        // Try to find ROM in default location
        char defaultPath[512];
        GetCurrentDirectoryA(sizeof(defaultPath), defaultPath);
        strcat_s(defaultPath, "\\assets\\Castlevania (U) (V1.2) [!].z64");
        
        g_romFile = CV64_ROM_Open(defaultPath);
        if (g_romFile) {
            strcpy_s(g_romPath, defaultPath);
        }
    }
    
    if (g_romFile) {
        OutputDebugStringA("[CV64] ROM file opened successfully\n");
    } else {
        OutputDebugStringA("[CV64] WARNING: Could not open ROM file. Models will use placeholder geometry.\n");
    }
    
    // Clear existing models
    g_models.clear();
    
    // Get model database
    uint32_t modelCount = 0;
    const CV64_ModelDatabaseEntry* database = CV64_GetModelDatabase(&modelCount);
    
    // Convert database entries to CV64_ModelInfo
    for (uint32_t i = 0; i < modelCount; i++) {
        CV64_ModelInfo modelInfo = { 0 };
        modelInfo.modelID = database[i].modelID;
        strcpy_s(modelInfo.name, database[i].name);
        modelInfo.type = (CV64_ModelType)database[i].modelType;
        modelInfo.romOffset = database[i].romOffset;
        modelInfo.dataSize = database[i].dataSize;
        modelInfo.textureCount = database[i].textureCount;
        modelInfo.loaded = false;
        
        // Set placeholder vertex/triangle counts
        modelInfo.vertexCount = 0;
        modelInfo.triangleCount = 0;
        
        g_models.push_back(modelInfo);
    }
    
    char logMsg[256];
    sprintf_s(logMsg, "[CV64] Model viewer scanned %u models from database\n", modelCount);
    OutputDebugStringA(logMsg);
    
    return (uint32_t)g_models.size();
}

uint32_t CV64_ModelViewer_GetModels(CV64_ModelInfo* outModels, uint32_t maxModels) {
    if (!outModels) {
        return 0;
    }
    
    size_t modelSize = g_models.size();
    size_t maxSize = (size_t)maxModels;
    uint32_t count = (uint32_t)(modelSize < maxSize ? modelSize : maxSize);
    for (uint32_t i = 0; i < count; i++) {
        outModels[i] = g_models[i];
    }
    
    return count;
}

bool CV64_ModelViewer_LoadModel(uint32_t modelID) {
    // Clear previous geometry
    if (g_geometryLoaded) {
        CV64_FreeGeometry(&g_currentGeometry);
        g_geometryLoaded = false;
    }
    
    // Find model in our list
    for (auto& model : g_models) {
        if (model.modelID == modelID) {
            g_currentModel = model;
            g_currentModel.loaded = true;
            g_state = CV64_VIEWER_STATE_LOADING;
            
            // Get database entry for ROM addresses
            const CV64_ModelDatabaseEntry* dbEntry = CV64_FindModelByID(modelID);
            if (dbEntry) {
                // Set model info
                g_currentModel.vertexCount = 0;
                g_currentModel.triangleCount = 0;
                
                char logMsg[256];
                sprintf_s(logMsg, "[CV64] Loading model: %s (ID: 0x%04X, ROM: 0x%08X)\n",
                    g_currentModel.name, modelID, g_currentModel.romOffset);
                OutputDebugStringA(logMsg);
                
                // Try to load from ROM if available
                if (g_romFile && CV64_ROM_IsValid(g_romFile)) {
                    // Read vertex data from ROM
                    uint32_t dataSize = dbEntry->dataSize;
                    if (dataSize > 0x10000) dataSize = 0x10000; // Safety limit: 64KB
                    
                    uint8_t* romData = (uint8_t*)malloc(dataSize);
                    if (romData) {
                        size_t bytesRead = CV64_ROM_Read(g_romFile, dbEntry->vertexOffset, romData, dataSize);
                        
                        if (bytesRead > 0) {
                            // Try to parse as vertex data
                            if (CV64_ParseN64Vertices(romData, bytesRead, &g_currentGeometry)) {
                                g_geometryLoaded = true;
                                
                                // Update model info
                                g_currentModel.vertexCount = g_currentGeometry.vertexCount;
                                g_currentModel.triangleCount = g_currentGeometry.triangleCount;
                                g_currentModel.minX = g_currentGeometry.minX;
                                g_currentModel.minY = g_currentGeometry.minY;
                                g_currentModel.minZ = g_currentGeometry.minZ;
                                g_currentModel.maxX = g_currentGeometry.maxX;
                                g_currentModel.maxY = g_currentGeometry.maxY;
                                g_currentModel.maxZ = g_currentGeometry.maxZ;
                                
                                sprintf_s(logMsg, "[CV64] Successfully loaded geometry: %u vertices, %u triangles\n",
                                    g_currentGeometry.vertexCount, g_currentGeometry.triangleCount);
                                OutputDebugStringA(logMsg);
                            } else {
                                OutputDebugStringA("[CV64] Failed to parse geometry data\n");
                            }
                        } else {
                            OutputDebugStringA("[CV64] Failed to read from ROM\n");
                        }
                        
                        free(romData);
                    }
                } else {
                    OutputDebugStringA("[CV64] ROM not loaded, using placeholder geometry\n");
                    // Create test geometry as placeholder
                    if (CV64_CreateTestGeometry(&g_currentGeometry)) {
                        g_geometryLoaded = true;
                        g_currentModel.vertexCount = g_currentGeometry.vertexCount;
                        g_currentModel.triangleCount = g_currentGeometry.triangleCount;
                    }
                }
                
                g_state = CV64_VIEWER_STATE_RENDERING;
            }
            
            return true;
        }
    }
    
    OutputDebugStringA("[CV64] Model not found in database\n");
    return false;
}

void CV64_ModelViewer_UnloadModel(void) {
    if (g_currentModel.loaded) {
        // Cleanup model resources
        if (g_geometryLoaded) {
            CV64_FreeGeometry(&g_currentGeometry);
            g_geometryLoaded = false;
        }
        
        memset(&g_currentModel, 0, sizeof(CV64_ModelInfo));
        g_state = CV64_VIEWER_STATE_IDLE;
    }
}

bool CV64_ModelViewer_GetCurrentModel(CV64_ModelInfo* outInfo) {
    if (!outInfo || !g_currentModel.loaded) {
        return false;
    }
    
    *outInfo = g_currentModel;
    return true;
}

void CV64_ModelViewer_UpdateCamera(float deltaTime) {
    // Camera update handled in mouse events
}

void CV64_ModelViewer_Render(void) {
    RenderScene();
}

void CV64_ModelViewer_ResetCamera(void) {
    g_camera.orbitAngleX = 45.0f;
    g_camera.orbitAngleY = 30.0f;
    g_camera.distance = 5.0f;
    g_camera.panX = 0.0f;
    g_camera.panY = 0.0f;
    g_camera.fov = 45.0f;
    g_camera.isDragging = false;
}

void CV64_ModelViewer_SetCameraAngles(float angleX, float angleY) {
    g_camera.orbitAngleX = angleX;
    g_camera.orbitAngleY = angleY;
}

void CV64_ModelViewer_SetCameraDistance(float distance) {
    g_camera.distance = distance;
}

bool CV64_ModelViewer_ExportModel(const char* outputPath, const char* format) {
    // TODO: Implement model export
    return false;
}

void CV64_ModelViewer_SetWireframe(bool enabled) {
    g_wireframeMode = enabled;
}

void CV64_ModelViewer_SetTexturesEnabled(bool enabled) {
    g_texturesEnabled = enabled;
}

void CV64_ModelViewer_SetLightingEnabled(bool enabled) {
    g_lightingEnabled = enabled;
}

void CV64_ModelViewer_SetAnimationPlaying(bool playing) {
    g_animationPlaying = playing;
}

void CV64_ModelViewer_SetAnimationFrame(uint32_t frame) {
    g_currentFrame = frame;
}

CV64_ViewerState CV64_ModelViewer_GetState(void) {
    return g_state;
}

void CV64_ModelViewer_GetStats(uint32_t* outTotalModels, uint32_t* outLoadedModel, float* outFPS) {
    if (outTotalModels) *outTotalModels = (uint32_t)g_models.size();
    if (outLoadedModel) *outLoadedModel = g_currentModel.loaded ? g_currentModel.modelID : 0;
    if (outFPS) *outFPS = g_fps;
}

void CV64_ModelViewer_HandleMouseButton(int button, bool pressed, int x, int y) {
    if (button == 0) { // Left button - orbit
        g_camera.isDragging = pressed;
        g_camera.lastMouseX = x;
        g_camera.lastMouseY = y;
        
        if (pressed) {
            SetCapture(g_viewerHwnd);
        } else {
            ReleaseCapture();
        }
    }
}

void CV64_ModelViewer_HandleMouseMove(int x, int y) {
    if (g_camera.isDragging) {
        int deltaX = x - g_camera.lastMouseX;
        int deltaY = y - g_camera.lastMouseY;
        
        g_camera.orbitAngleX += deltaX * 0.5f;
        g_camera.orbitAngleY += deltaY * 0.5f;
        
        // Clamp vertical rotation
        if (g_camera.orbitAngleY > 89.0f) g_camera.orbitAngleY = 89.0f;
        if (g_camera.orbitAngleY < -89.0f) g_camera.orbitAngleY = -89.0f;
        
        g_camera.lastMouseX = x;
        g_camera.lastMouseY = y;
    }
}

void CV64_ModelViewer_HandleMouseWheel(int delta) {
    g_camera.distance -= delta / 120.0f * 0.5f;
    
    // Clamp distance
    if (g_camera.distance < 1.0f) g_camera.distance = 1.0f;
    if (g_camera.distance > 50.0f) g_camera.distance = 50.0f;
}
