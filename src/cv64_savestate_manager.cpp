/**
 * @file cv64_savestate_manager.cpp
 * @brief CV64 Save State Manager Implementation
 * 
 * Provides enhanced save state management with thumbnails, metadata, and browser UI.
 */

#include "../include/cv64_savestate_manager.h"
#include "../include/cv64_m64p_integration.h"
#include "../include/cv64_memory_hook.h"
#include "../framework.h"
#include "../Resource.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <direct.h>
#include <CommCtrl.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

// Dialog control IDs
#define IDC_SAVESTATE_LIST          3000
#define IDC_SAVESTATE_THUMBNAIL     3001
#define IDC_SAVESTATE_NAME          3002
#define IDC_SAVESTATE_INFO          3003
#define IDC_SAVESTATE_SAVE_NEW      3004
#define IDC_SAVESTATE_LOAD          3005
#define IDC_SAVESTATE_DELETE        3006
#define IDC_SAVESTATE_RENAME        3007
#define IDC_SAVESTATE_EXPORT        3008
#define IDC_SAVESTATE_IMPORT        3009
#define IDC_SAVESTATE_REFRESH       3010

// Constants
#define MAX_SAVE_STATES 1000
#define THUMBNAIL_WIDTH 320
#define THUMBNAIL_HEIGHT 180
#define SAVE_STATE_DIR "save\\states"
#define METADATA_EXTENSION ".json"
#define THUMBNAIL_EXTENSION ".png"

// Module state
static struct {
    bool initialized;
    CV64_SaveState states[MAX_SAVE_STATES];
    int stateCount;
    int currentSelection;
    int quickSaveSlot;
    uint64_t totalSaves;
    uint64_t totalLoads;
    HBITMAP currentThumbnail;
} g_saveStateMgr = { 0 };

// Forward declarations
extern void EnableDarkModeForDialog(HWND hDlg);
extern HBRUSH HandleDarkModeCtlColor(HWND hDlg, UINT message, HDC hdc, HWND hCtl);

static INT_PTR CALLBACK SaveStateManagerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void UpdateStateList(HWND hDlg);
static void UpdateStateInfo(HWND hDlg, int index);
static void LoadThumbnail(HWND hDlg, const char* thumbnailPath);
static bool ScanSaveStates(void);
static bool SaveMetadata(const CV64_SaveState* state);
static bool LoadMetadata(const char* filename, CV64_SaveState* outState);
static void GenerateSaveStateFilename(char* outPath, size_t size, const char* name);
static void FormatTimestamp(time_t timestamp, char* buffer, size_t size);
static bool CaptureCurrentGameState(CV64_SaveState* outState);

/**
 * @brief Initialize the save state manager
 */
bool CV64_SaveState_Init(void)
{
    if (g_saveStateMgr.initialized) {
        return true;
    }

    // Create save state directory
    _mkdir("save");
    _mkdir(SAVE_STATE_DIR);

    // Initialize state
    memset(&g_saveStateMgr, 0, sizeof(g_saveStateMgr));
    g_saveStateMgr.quickSaveSlot = 0;
    g_saveStateMgr.currentSelection = -1;
    g_saveStateMgr.initialized = true;

    // Scan for existing save states
    ScanSaveStates();

    OutputDebugStringA("[CV64] Save State Manager initialized\n");
    return true;
}

/**
 * @brief Shutdown the save state manager
 */
void CV64_SaveState_Shutdown(void)
{
    if (!g_saveStateMgr.initialized) {
        return;
    }

    // Clean up thumbnail bitmap
    if (g_saveStateMgr.currentThumbnail) {
        DeleteObject(g_saveStateMgr.currentThumbnail);
        g_saveStateMgr.currentThumbnail = NULL;
    }

    g_saveStateMgr.initialized = false;
    OutputDebugStringA("[CV64] Save State Manager shutdown\n");
}

/**
 * @brief Show the save state manager dialog
 */
bool CV64_SaveState_ShowDialog(HWND hParent)
{
    if (!g_saveStateMgr.initialized) {
        CV64_SaveState_Init();
    }

    // Refresh save state list
    ScanSaveStates();

    // Show the dialog
    INT_PTR result = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SAVESTATE_MANAGER), 
                               hParent, SaveStateManagerDlgProc);

    return (result == IDOK);
}

/**
 * @brief Quick save to slot (F5 functionality)
 */
bool CV64_SaveState_QuickSave(int slotIndex)
{
    if (!CV64_M64P_IsRunning()) {
        OutputDebugStringA("[CV64] Quick save failed: emulation not running\n");
        return false;
    }

    // Validate slot
    if (slotIndex < 0 || slotIndex > 9) {
        slotIndex = 0;
    }

    // Create metadata
    CV64_SaveState state = {0};
    CaptureCurrentGameState(&state);
    sprintf_s(state.name, "Quick Save %d", slotIndex + 1);
    sprintf_s(state.filename, "quicksave_%d.st", slotIndex);

    // Save state via mupen64plus (uses internal slot mechanism)
    CV64_M64P_SetSaveSlot(slotIndex);
    if (!CV64_M64P_SaveState(slotIndex)) {
        OutputDebugStringA("[CV64] Quick save failed: mupen64plus error\n");
        return false;
    }

    // Build paths for metadata/thumbnail (mupen64plus saves to its own location)
    char thumbnailPath[MAX_PATH];
    sprintf_s(thumbnailPath, "%s\\quicksave_%d" THUMBNAIL_EXTENSION, SAVE_STATE_DIR, slotIndex);
    
    // Capture screenshot thumbnail (note: not yet implemented)
    CV64_SaveState_CaptureScreenshot(thumbnailPath, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    strcpy_s(state.thumbnailPath, thumbnailPath);
    state.hasScreenshot = false;  // Will be true when screenshot capture is implemented

    // Timestamp
    state.timestamp = time(NULL);

    // Save metadata
    SaveMetadata(&state);

    g_saveStateMgr.totalSaves++;
    g_saveStateMgr.quickSaveSlot = slotIndex;

    char msg[256];
    sprintf_s(msg, "[CV64] Quick save %d successful\n", slotIndex + 1);
    OutputDebugStringA(msg);

    return true;
}

/**
 * @brief Quick load from slot (F9 functionality)
 */
bool CV64_SaveState_QuickLoad(int slotIndex)
{
    // Validate slot
    if (slotIndex < 0 || slotIndex > 9) {
        slotIndex = 0;
    }

    // Check if save state exists (check for metadata file)
    char metaPath[MAX_PATH];
    sprintf_s(metaPath, "%s\\quicksave_%d.st" METADATA_EXTENSION, SAVE_STATE_DIR, slotIndex);
    
    if (GetFileAttributesA(metaPath) == INVALID_FILE_ATTRIBUTES) {
        char msg[256];
        sprintf_s(msg, "[CV64] Quick load failed: slot %d is empty\n", slotIndex + 1);
        OutputDebugStringA(msg);
        return false;
    }

    // Load state via mupen64plus
    CV64_M64P_SetSaveSlot(slotIndex);
    if (!CV64_M64P_LoadState(slotIndex)) {
        OutputDebugStringA("[CV64] Quick load failed: mupen64plus error\n");
        return false;
    }

    g_saveStateMgr.totalLoads++;

    char msg[256];
    sprintf_s(msg, "[CV64] Quick load %d successful\n", slotIndex + 1);
    OutputDebugStringA(msg);

    return true;
}

/**
 * @brief Save state with custom name
 */
bool CV64_SaveState_SaveNamed(const char* name)
{
    if (!CV64_M64P_IsRunning()) {
        return false;
    }

    // Find an available slot (use slots 1-9, reserve 0 for quick save)
    int availableSlot = -1;
    for (int i = 1; i <= 9; i++) {
        char metaPath[MAX_PATH];
        sprintf_s(metaPath, "%s\\named_save_%d.st" METADATA_EXTENSION, SAVE_STATE_DIR, i);
        if (GetFileAttributesA(metaPath) == INVALID_FILE_ATTRIBUTES) {
            availableSlot = i;
            break;
        }
    }

    if (availableSlot == -1) {
        OutputDebugStringA("[CV64] No available save slots\n");
        return false;
    }

    // Generate unique filename
    char filename[MAX_PATH];
    GenerateSaveStateFilename(filename, sizeof(filename), name);

    // Create metadata
    CV64_SaveState state = {0};
    CaptureCurrentGameState(&state);
    strcpy_s(state.name, name);
    strcpy_s(state.filename, filename);

    // Save state to slot
    CV64_M64P_SetSaveSlot(availableSlot);
    if (!CV64_M64P_SaveState(availableSlot)) {
        return false;
    }

    // Capture screenshot
    char thumbnailPath[MAX_PATH];
    sprintf_s(thumbnailPath, "%s\\%s" THUMBNAIL_EXTENSION, SAVE_STATE_DIR, filename);
    CV64_SaveState_CaptureScreenshot(thumbnailPath, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    strcpy_s(state.thumbnailPath, thumbnailPath);
    state.hasScreenshot = false;  // Will be true when implemented

    state.timestamp = time(NULL);

    // Save metadata
    SaveMetadata(&state);

    g_saveStateMgr.totalSaves++;

    // Refresh list
    ScanSaveStates();

    return true;
}

/**
 * @brief Load a specific save state
 * Note: This is a simplified implementation that uses slot 1 for loading named saves
 */
bool CV64_SaveState_Load(const char* filename)
{
    // For now, we can't load arbitrary named saves without copying them to a slot first
    // This is a limitation of the mupen64plus API which only supports numbered slots
    // TODO: Implement file copying to temp slot for loading
    
    OutputDebugStringA("[CV64] Named save state loading not yet fully implemented\n");
    return false;
}

/**
 * @brief Delete a save state
 */
bool CV64_SaveState_Delete(const char* filename)
{
    // Delete save state file
    char statePath[MAX_PATH];
    sprintf_s(statePath, "%s\\%s", SAVE_STATE_DIR, filename);
    DeleteFileA(statePath);

    // Delete thumbnail
    char thumbPath[MAX_PATH];
    sprintf_s(thumbPath, "%s\\%s" THUMBNAIL_EXTENSION, SAVE_STATE_DIR, filename);
    DeleteFileA(thumbPath);

    // Delete metadata
    char metaPath[MAX_PATH];
    sprintf_s(metaPath, "%s\\%s" METADATA_EXTENSION, SAVE_STATE_DIR, filename);
    DeleteFileA(metaPath);

    // Refresh list
    ScanSaveStates();

    return true;
}

/**
 * @brief Capture current game state for metadata
 */
static bool CaptureCurrentGameState(CV64_SaveState* outState)
{
    if (!CV64_Memory_IsInitialized()) {
        return false;
    }

    // Get game info from verified decomp memory reads
    CV64_GameInfo gameInfo;
    if (CV64_Memory_GetGameInfo(&gameInfo)) {
        strcpy_s(outState->mapName, gameInfo.mapName);
        strcpy_s(outState->character, gameInfo.characterName);
        outState->health = gameInfo.health;
        outState->maxHealth = gameInfo.maxHealth;
        outState->mapID = gameInfo.mapId;
    } else {
        strcpy_s(outState->character, "Unknown");
    }

    // Timestamp
    outState->timestamp = time(NULL);

    return true;
}

/**
 * @brief Generate unique filename for save state
 */
static void GenerateSaveStateFilename(char* outPath, size_t size, const char* name)
{
    // Sanitize name (remove invalid characters)
    char sanitized[64];
    int j = 0;
    for (int i = 0; name[i] && j < 63; i++) {
        char c = name[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '_' || c == '-') {
            sanitized[j++] = c;
        } else if (c == ' ') {
            sanitized[j++] = '_';
        }
    }
    sanitized[j] = '\0';

    // Add timestamp for uniqueness
    time_t now = time(NULL);
    sprintf_s(outPath, size, "%s_%lld.st", sanitized, (long long)now);
}

/**
 * @brief Scan for existing save states
 */
static bool ScanSaveStates(void)
{
    g_saveStateMgr.stateCount = 0;

    // Search for .st files in save state directory
    char searchPath[MAX_PATH];
    sprintf_s(searchPath, "%s\\*.st", SAVE_STATE_DIR);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (g_saveStateMgr.stateCount >= MAX_SAVE_STATES) {
            break;
        }

        CV64_SaveState* state = &g_saveStateMgr.states[g_saveStateMgr.stateCount];
        strcpy_s(state->filename, findData.cFileName);

        // Load metadata if available
        if (!LoadMetadata(findData.cFileName, state)) {
            // No metadata, use filename as name
            strcpy_s(state->name, findData.cFileName);
            state->timestamp = 0;
        }

        // Check for thumbnail
        char thumbPath[MAX_PATH];
        sprintf_s(thumbPath, "%s\\%s" THUMBNAIL_EXTENSION, SAVE_STATE_DIR, findData.cFileName);
        state->hasScreenshot = (GetFileAttributesA(thumbPath) != INVALID_FILE_ATTRIBUTES);
        strcpy_s(state->thumbnailPath, thumbPath);

        g_saveStateMgr.stateCount++;

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // Sort by timestamp (newest first)
    std::sort(g_saveStateMgr.states, g_saveStateMgr.states + g_saveStateMgr.stateCount,
              [](const CV64_SaveState& a, const CV64_SaveState& b) {
                  return a.timestamp > b.timestamp;
              });

    return true;
}

/**
 * @brief Save metadata to JSON file
 */
static bool SaveMetadata(const CV64_SaveState* state)
{
    char metaPath[MAX_PATH];
    sprintf_s(metaPath, "%s\\%s" METADATA_EXTENSION, SAVE_STATE_DIR, state->filename);

    FILE* file = NULL;
    if (fopen_s(&file, metaPath, "w") != 0 || !file) {
        return false;
    }

    // Write JSON metadata (simple format)
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", state->name);
    fprintf(file, "  \"timestamp\": %lld,\n", (long long)state->timestamp);
    fprintf(file, "  \"mapName\": \"%s\",\n", state->mapName);
    fprintf(file, "  \"character\": \"%s\",\n", state->character);
    fprintf(file, "  \"health\": %d,\n", state->health);
    fprintf(file, "  \"maxHealth\": %d,\n", state->maxHealth);
    fprintf(file, "  \"mapID\": %d\n", state->mapID);
    fprintf(file, "}\n");

    fclose(file);
    return true;
}

/**
 * @brief Load metadata from JSON file
 */
static bool LoadMetadata(const char* filename, CV64_SaveState* outState)
{
    char metaPath[MAX_PATH];
    sprintf_s(metaPath, "%s\\%s" METADATA_EXTENSION, SAVE_STATE_DIR, filename);

    FILE* file = NULL;
    if (fopen_s(&file, metaPath, "r") != 0 || !file) {
        return false;
    }

    // Simple JSON parsing (just read key values)
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf_s(line, "  \"name\": \"%63[^\"]\"", outState->name, (unsigned)sizeof(outState->name)) == 1) continue;
        if (sscanf_s(line, "  \"timestamp\": %lld", &outState->timestamp) == 1) continue;
        if (sscanf_s(line, "  \"mapName\": \"%63[^\"]\"", outState->mapName, (unsigned)sizeof(outState->mapName)) == 1) continue;
        if (sscanf_s(line, "  \"character\": \"%31[^\"]\"", outState->character, (unsigned)sizeof(outState->character)) == 1) continue;
        if (sscanf_s(line, "  \"health\": %hu", &outState->health) == 1) continue;
        if (sscanf_s(line, "  \"maxHealth\": %hu", &outState->maxHealth) == 1) continue;
        if (sscanf_s(line, "  \"mapID\": %hd", &outState->mapID) == 1) continue;
    }

    fclose(file);
    return true;
}

/**
 * @brief Format timestamp for display
 */
static void FormatTimestamp(time_t timestamp, char* buffer, size_t size)
{
    if (timestamp == 0) {
        strcpy_s(buffer, size, "Unknown");
        return;
    }

    struct tm timeInfo;
    if (localtime_s(&timeInfo, &timestamp) == 0) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeInfo);
    } else {
        strcpy_s(buffer, size, "Invalid");
    }
}

/**
 * @brief Capture screenshot for thumbnail (stub - needs OpenGL implementation)
 */
bool CV64_SaveState_CaptureScreenshot(const char* outPath, int width, int height)
{
    // TODO: Implement OpenGL framebuffer capture
    // This requires reading the current OpenGL framebuffer and saving as PNG
    // For now, just log
    char msg[512];
    sprintf_s(msg, "[CV64] Screenshot capture not yet implemented: %s\n", outPath);
    OutputDebugStringA(msg);
    
    return false;  // Not implemented yet
}

/**
 * @brief Check if save state exists
 */
bool CV64_SaveState_Exists(const char* filename)
{
    char fullPath[MAX_PATH];
    sprintf_s(fullPath, "%s\\%s", SAVE_STATE_DIR, filename);
    return (GetFileAttributesA(fullPath) != INVALID_FILE_ATTRIBUTES);
}

/**
 * @brief Get manager statistics
 */
void CV64_SaveState_GetStats(CV64_SaveStateStats* outStats)
{
    outStats->totalStates = g_saveStateMgr.stateCount;
    outStats->quickSaveSlot = g_saveStateMgr.quickSaveSlot;
    outStats->totalSaves = g_saveStateMgr.totalSaves;
    outStats->totalLoads = g_saveStateMgr.totalLoads;
    outStats->diskUsageBytes = CV64_SaveState_GetDiskUsage();
}

/**
 * @brief Get total disk usage
 */
size_t CV64_SaveState_GetDiskUsage(void)
{
    size_t total = 0;

    char searchPath[MAX_PATH];
    sprintf_s(searchPath, "%s\\*.*", SAVE_STATE_DIR);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                total += (size_t)findData.nFileSizeLow + ((size_t)findData.nFileSizeHigh << 32);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    return total;
}

/**
 * @brief Update the state list in the dialog
 */
static void UpdateStateList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_SAVESTATE_LIST);
    if (!hList) return;

    // Clear existing items
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    // Add states to list
    for (int i = 0; i < g_saveStateMgr.stateCount; i++) {
        const CV64_SaveState* state = &g_saveStateMgr.states[i];

        char itemText[256];
        char timeStr[64];
        FormatTimestamp(state->timestamp, timeStr, sizeof(timeStr));

        // Format: "Save Name - Map Name - Date/Time"
        sprintf_s(itemText, "%s - %s - %s", 
                  state->name, 
                  state->mapName[0] ? state->mapName : "Unknown",
                  timeStr);

        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)itemText);
    }

    // Select first item if available
    if (g_saveStateMgr.stateCount > 0) {
        SendMessage(hList, LB_SETCURSEL, 0, 0);
        UpdateStateInfo(hDlg, 0);
    }
}

/**
 * @brief Update state info display
 */
static void UpdateStateInfo(HWND hDlg, int index)
{
    if (index < 0 || index >= g_saveStateMgr.stateCount) {
        return;
    }

    g_saveStateMgr.currentSelection = index;
    const CV64_SaveState* state = &g_saveStateMgr.states[index];

    // Update name field
    SetDlgItemTextA(hDlg, IDC_SAVESTATE_NAME, state->name);

    // Build info text
    char infoText[512];
    char timeStr[64];
    FormatTimestamp(state->timestamp, timeStr, sizeof(timeStr));

    sprintf_s(infoText, 
              "Map: %s\n"
              "Character: %s\n"
              "HP: %d/%d\n"
              "Date: %s\n"
              "File: %s",
              state->mapName[0] ? state->mapName : "Unknown",
              state->character[0] ? state->character : "Unknown",
              state->health, state->maxHealth,
              timeStr,
              state->filename);

    SetDlgItemTextA(hDlg, IDC_SAVESTATE_INFO, infoText);

    // Load thumbnail
    if (state->hasScreenshot) {
        LoadThumbnail(hDlg, state->thumbnailPath);
    }
}

/**
 * @brief Load thumbnail into picture control
 */
static void LoadThumbnail(HWND hDlg, const char* thumbnailPath)
{
    // Clean up previous thumbnail
    if (g_saveStateMgr.currentThumbnail) {
        DeleteObject(g_saveStateMgr.currentThumbnail);
        g_saveStateMgr.currentThumbnail = NULL;
    }

    // Load new thumbnail
    g_saveStateMgr.currentThumbnail = (HBITMAP)LoadImageA(NULL, thumbnailPath, 
                                                          IMAGE_BITMAP, 0, 0, 
                                                          LR_LOADFROMFILE);

    if (g_saveStateMgr.currentThumbnail) {
        SendDlgItemMessage(hDlg, IDC_SAVESTATE_THUMBNAIL, STM_SETIMAGE, 
                          IMAGE_BITMAP, (LPARAM)g_saveStateMgr.currentThumbnail);
    }
}

/**
 * @brief Save State Manager dialog procedure
 */
static INT_PTR CALLBACK SaveStateManagerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        EnableDarkModeForDialog(hDlg);

        // Center dialog
        RECT rcParent, rcDlg;
        HWND hParent = GetParent(hDlg);
        if (!hParent) hParent = GetDesktopWindow();

        GetWindowRect(hParent, &rcParent);
        GetWindowRect(hDlg, &rcDlg);

        int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
        int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;

        SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        // Populate list
        UpdateStateList(hDlg);

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch (wmId)
        {
        case IDC_SAVESTATE_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                HWND hList = GetDlgItem(hDlg, IDC_SAVESTATE_LIST);
                int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    UpdateStateInfo(hDlg, sel);
                }
            }
            break;

        case IDC_SAVESTATE_SAVE_NEW:
        {
            char name[64];
            GetDlgItemTextA(hDlg, IDC_SAVESTATE_NAME, name, sizeof(name));

            if (strlen(name) == 0) {
                MessageBoxA(hDlg, "Please enter a name for the save state.", "Name Required", MB_OK | MB_ICONWARNING);
                break;
            }

            if (CV64_SaveState_SaveNamed(name)) {
                MessageBoxA(hDlg, "Save state created successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                UpdateStateList(hDlg);
            } else {
                MessageBoxA(hDlg, "Failed to create save state!", "Error", MB_OK | MB_ICONERROR);
            }
            break;
        }

        case IDC_SAVESTATE_LOAD:
            if (g_saveStateMgr.currentSelection >= 0) {
                const CV64_SaveState* state = &g_saveStateMgr.states[g_saveStateMgr.currentSelection];
                if (CV64_SaveState_Load(state->filename)) {
                    MessageBoxA(hDlg, "Save state loaded successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxA(hDlg, "Failed to load save state!", "Error", MB_OK | MB_ICONERROR);
                }
            }
            break;

        case IDC_SAVESTATE_DELETE:
            if (g_saveStateMgr.currentSelection >= 0) {
                int result = MessageBoxA(hDlg,
                    "Are you sure you want to delete this save state?\nThis cannot be undone!",
                    "Confirm Delete",
                    MB_YESNO | MB_ICONWARNING);

                if (result == IDYES) {
                    const CV64_SaveState* state = &g_saveStateMgr.states[g_saveStateMgr.currentSelection];
                    CV64_SaveState_Delete(state->filename);
                    UpdateStateList(hDlg);
                    MessageBoxA(hDlg, "Save state deleted.", "Deleted", MB_OK | MB_ICONINFORMATION);
                }
            }
            break;

        case IDC_SAVESTATE_REFRESH:
            ScanSaveStates();
            UpdateStateList(hDlg);
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, wmId);
            return (INT_PTR)TRUE;
        }
        break;
    }

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

// Stub implementations for export/import/rename
bool CV64_SaveState_Rename(const char* filename, const char* newName) { return false; }
bool CV64_SaveState_Export(const char* filename, const char* exportPath) { return false; }
bool CV64_SaveState_Import(const char* importPath, const char* name) { return false; }
bool CV64_SaveState_GetList(CV64_SaveState* outStates, int maxStates, int* outCount) { return false; }
bool CV64_SaveState_GetMetadata(const char* filename, CV64_SaveState* outState) { return false; }
int CV64_SaveState_CleanupAutoSaves(int keepCount) { return 0; }
