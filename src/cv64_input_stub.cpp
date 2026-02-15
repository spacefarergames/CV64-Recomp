/**
 * @file cv64_input_stub.cpp
 * @brief Stub input plugin DLL that forwards to the main EXE
 * 
 * This DLL exists because mupen64plus requires plugins to be separate DLLs.
 * All it does is forward calls to the main CV64_RMG.exe which has the
 * actual input implementation.
 */

#include <Windows.h>

/* mupen64plus types */
typedef void* m64p_dynlib_handle;
typedef enum { M64ERR_SUCCESS = 0, M64ERR_NOT_INIT, M64ERR_ALREADY_INIT } m64p_error;
typedef enum { M64PLUGIN_INPUT = 4 } m64p_plugin_type;

/* CONTROL structures */
typedef struct { int Present; int RawData; int Plugin; } CONTROL;
typedef struct { void* hMainWindow; void* hinst; int MemoryBswaped; unsigned char* HEADER; CONTROL* Controls; } CONTROL_INFO;
typedef union { unsigned int Value; } BUTTONS;

/* Function pointers to main EXE */
static HMODULE g_mainExe = NULL;
typedef m64p_error (__cdecl *pfn_PluginStartup)(m64p_dynlib_handle, void*, void(*)(void*, int, const char*));
typedef m64p_error (__cdecl *pfn_PluginShutdown)(void);
typedef m64p_error (__cdecl *pfn_PluginGetVersion)(m64p_plugin_type*, int*, int*, const char**, int*);
typedef void (__cdecl *pfn_InitiateControllers)(CONTROL_INFO);
typedef void (__cdecl *pfn_GetKeys)(int, BUTTONS*);
typedef void (__cdecl *pfn_ControllerCommand)(int, unsigned char*);
typedef void (__cdecl *pfn_RomOpen)(void);
typedef void (__cdecl *pfn_RomClosed)(void);
typedef void (__cdecl *pfn_ReadController)(int, unsigned char*);

static pfn_PluginStartup g_PluginStartup = NULL;
static pfn_PluginShutdown g_PluginShutdown = NULL;
static pfn_PluginGetVersion g_PluginGetVersion = NULL;
static pfn_InitiateControllers g_InitiateControllers = NULL;
static pfn_GetKeys g_GetKeys = NULL;
static pfn_ControllerCommand g_ControllerCommand = NULL;
static pfn_RomOpen g_RomOpen = NULL;
static pfn_RomClosed g_RomClosed = NULL;
static pfn_ReadController g_ReadController = NULL;

static bool InitFunctions() {
    if (g_mainExe) return true;
    
    g_mainExe = GetModuleHandle(NULL);
    if (!g_mainExe) return false;
    
    g_PluginStartup = (pfn_PluginStartup)GetProcAddress(g_mainExe, "PluginStartup");
    g_PluginShutdown = (pfn_PluginShutdown)GetProcAddress(g_mainExe, "PluginShutdown");
    g_PluginGetVersion = (pfn_PluginGetVersion)GetProcAddress(g_mainExe, "PluginGetVersion");
    g_InitiateControllers = (pfn_InitiateControllers)GetProcAddress(g_mainExe, "InitiateControllers");
    g_GetKeys = (pfn_GetKeys)GetProcAddress(g_mainExe, "GetKeys");
    g_ControllerCommand = (pfn_ControllerCommand)GetProcAddress(g_mainExe, "ControllerCommand");
    g_RomOpen = (pfn_RomOpen)GetProcAddress(g_mainExe, "RomOpen");
    g_RomClosed = (pfn_RomClosed)GetProcAddress(g_mainExe, "RomClosed");
    g_ReadController = (pfn_ReadController)GetProcAddress(g_mainExe, "ReadController");
    
    return g_PluginStartup && g_PluginShutdown && g_PluginGetVersion && 
           g_InitiateControllers && g_GetKeys;
}

extern "C" {

__declspec(dllexport) m64p_error __cdecl PluginStartup(m64p_dynlib_handle CoreLibHandle, void* Context,
                                                        void (*DebugCallback)(void*, int, const char*)) {
    if (!InitFunctions()) return M64ERR_NOT_INIT;
    return g_PluginStartup(CoreLibHandle, Context, DebugCallback);
}

__declspec(dllexport) m64p_error __cdecl PluginShutdown(void) {
    if (!g_PluginShutdown) return M64ERR_NOT_INIT;
    return g_PluginShutdown();
}

__declspec(dllexport) m64p_error __cdecl PluginGetVersion(m64p_plugin_type* PluginType, int* PluginVersion,
                                                           int* APIVersion, const char** PluginNamePtr, int* Capabilities) {
    if (!InitFunctions()) {
        if (PluginType) *PluginType = M64PLUGIN_INPUT;
        if (PluginVersion) *PluginVersion = 0x020000;
        if (APIVersion) *APIVersion = 0x020001;
        if (PluginNamePtr) *PluginNamePtr = "CV64 Input Stub";
        if (Capabilities) *Capabilities = 0;
        return M64ERR_SUCCESS;
    }
    return g_PluginGetVersion(PluginType, PluginVersion, APIVersion, PluginNamePtr, Capabilities);
}

__declspec(dllexport) void __cdecl InitiateControllers(CONTROL_INFO ControlInfo) {
    if (InitFunctions() && g_InitiateControllers) {
        g_InitiateControllers(ControlInfo);
    }
}

__declspec(dllexport) void __cdecl GetKeys(int Control, BUTTONS* Keys) {
    if (g_GetKeys) {
        g_GetKeys(Control, Keys);
    } else if (Keys) {
        Keys->Value = 0;
    }
}

__declspec(dllexport) void __cdecl ControllerCommand(int Control, unsigned char* Command) {
    if (g_ControllerCommand) {
        g_ControllerCommand(Control, Command);
    }
}

__declspec(dllexport) void __cdecl RomOpen(void) {
    if (g_RomOpen) g_RomOpen();
}

__declspec(dllexport) void __cdecl RomClosed(void) {
    if (g_RomClosed) g_RomClosed();
}

__declspec(dllexport) void __cdecl ReadController(int Control, unsigned char* Command) {
    if (g_ReadController) {
        g_ReadController(Control, Command);
    }
}

__declspec(dllexport) int __cdecl RomConfig(unsigned char* basename) {
    return 1;
}

__declspec(dllexport) void __cdecl SDL_KeyDown(int keymod, int keysym) {}
__declspec(dllexport) void __cdecl SDL_KeyUp(int keymod, int keysym) {}

} /* extern "C" */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    return TRUE;
}
