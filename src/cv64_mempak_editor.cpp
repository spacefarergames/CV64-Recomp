/**
 * @file cv64_mempak_editor.cpp
 * @brief CV64 Memory Pak Editor Implementation
 * 
 * Provides GUI for editing Castlevania 64 save files in memory pak format.
 * Uses the CV64 decomp project's save structure to decode and display save data.
 */

#include "../include/cv64_mempak_editor.h"
#include "../framework.h"
#include "../Resource.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <CommCtrl.h>
#include <thread>
#include <chrono>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Dialog control IDs
#define IDC_MEMPAK_SLOT_LIST        2000
#define IDC_MEMPAK_CHARACTER        2001
#define IDC_MEMPAK_HEALTH           2002
#define IDC_MEMPAK_MAX_HEALTH       2003
#define IDC_MEMPAK_JEWELS           2004
#define IDC_MEMPAK_MAP_ID           2005
#define IDC_MEMPAK_SUBWEAPON        2006
#define IDC_MEMPAK_DIFFICULTY       2007
#define IDC_MEMPAK_PLAYTIME         2008
#define IDC_MEMPAK_ENEMIES_KILLED   2009
#define IDC_MEMPAK_SAVE_NAME        2010
#define IDC_MEMPAK_REFRESH          2011
#define IDC_MEMPAK_SAVE             2012
#define IDC_MEMPAK_EXPORT           2013
#define IDC_MEMPAK_IMPORT           2014
#define IDC_MEMPAK_DELETE           2015
#define IDC_MEMPAK_STATUS           2016
#define IDC_MEMPAK_SCREENSHOT       2017  // New screenshot control

// Maximum number of save slots in a memory pak
#define MAX_SAVE_SLOTS 16

// Module state
static struct {
    bool initialized;
    uint8_t mempakData[0x8000];  // 32KB memory pak
    char currentFile[MAX_PATH];
    int currentSlot;
    bool modified;
    char screenshotPath[MAX_PATH];  // Screenshot for this mempak
    HBITMAP screenshotBitmap;       // Loaded screenshot bitmap
    ULONG_PTR gdiplusToken;         // GDI+ token
} g_mempak = { 0 };

// Character names
static const char* g_characterNames[] = {
    "Reinhardt Schneider",
    "Carrie Fernandez"
};

// Subweapon names (common CV64 subweapons)
static const char* g_subweaponNames[] = {
    "None",
    "Holy Water",
    "Cross",
    "Knife",
    "Axe"
};

// Difficulty names
static const char* g_difficultyNames[] = {
    "Normal",
    "Hard"
};

// Forward declarations
extern void EnableDarkModeForDialog(HWND hDlg);
extern HBRUSH HandleDarkModeCtlColor(HWND hDlg, UINT message, HDC hdc, HWND hCtl);

// External screenshot function from mupen64plus
extern "C" void TakeScreenshotToPath(const char* filepath);

static INT_PTR CALLBACK MempakEditorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void LoadMempakToUI(HWND hDlg);
static void SaveUIToMempak(HWND hDlg);
static void UpdateSlotList(HWND hDlg);
static void UpdateSlotData(HWND hDlg, int slotIndex);
static bool FindSaveSlots(const uint8_t* mempakData, int* outSlotOffsets, int* outSlotCount);
static void FormatPlayTime(uint32_t frames, char* buffer, size_t bufferSize);
static void ExportSlot(HWND hDlg, int slotIndex);
static void ImportSlot(HWND hDlg, int slotIndex);
static void DeleteSlot(HWND hDlg, int slotIndex);
static void ApplyUIChangesToSlot(HWND hDlg, int slotIndex);
static int GetCurrentSlotOffset(int slotIndex);
static void CaptureScreenshotDelayed();
static void LoadScreenshotBitmap();
static void UpdateScreenshotDisplay(HWND hDlg);

/**
 * @brief Initialize the memory pak editor system
 */
bool CV64_MempakEditor_Init(void)
{
    if (g_mempak.initialized) {
        return true;
    }

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_mempak.gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize memory pak data to empty
    memset(g_mempak.mempakData, 0, sizeof(g_mempak.mempakData));
    strcpy_s(g_mempak.currentFile, "save\\mempak0.mpk");
    g_mempak.currentSlot = -1;
    g_mempak.modified = false;
    g_mempak.screenshotPath[0] = '\0';
    g_mempak.screenshotBitmap = NULL;
    g_mempak.initialized = true;

    OutputDebugStringA("[CV64] Memory Pak Editor initialized\n");
    return true;
}

/**
 * @brief Shutdown the memory pak editor system
 */
void CV64_MempakEditor_Shutdown(void)
{
    if (!g_mempak.initialized) {
        return;
    }

    // Clean up screenshot bitmap
    if (g_mempak.screenshotBitmap) {
        DeleteObject(g_mempak.screenshotBitmap);
        g_mempak.screenshotBitmap = NULL;
    }

    // Shutdown GDI+
    GdiplusShutdown(g_mempak.gdiplusToken);

    g_mempak.initialized = false;
    OutputDebugStringA("[CV64] Memory Pak Editor shutdown\n");
}

/**
 * @brief Show the memory pak editor dialog
 */
bool CV64_MempakEditor_ShowDialog(HWND hParent)
{
    if (!g_mempak.initialized) {
        CV64_MempakEditor_Init();
    }

    // Try to load the default mempak file
    CV64_MempakEditor_LoadFile(g_mempak.currentFile);

    // Show the dialog
    INT_PTR result = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MEMPAK_EDITOR), 
                               hParent, MempakEditorDlgProc);

    return (result == IDOK);
}

/**
 * @brief Load memory pak file from disk
 */
bool CV64_MempakEditor_LoadFile(const char* filename)
{
    // Create save directory if it doesn't exist
    char saveDir[MAX_PATH];
    strcpy_s(saveDir, filename);
    char* lastSlash = strrchr(saveDir, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        CreateDirectoryA(saveDir, NULL);  // Ignore error if already exists
    }

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "rb");
    
    if (err != 0 || !file) {
        // If file doesn't exist, create a new formatted mempak
        if (err == ENOENT) {
            char msg[512];
            sprintf_s(msg, "[CV64] Memory pak file not found, creating new: %s\n", filename);
            OutputDebugStringA(msg);
            
            // Initialize with empty mempak (formatted but no saves)
            memset(g_mempak.mempakData, 0, sizeof(g_mempak.mempakData));
            
            // Format the mempak with proper N64 structure
            // This would need proper N64 mempak formatting - for now just zero it
            // TODO: Implement proper N64 mempak formatting with ID blocks
            
            strcpy_s(g_mempak.currentFile, filename);
            g_mempak.modified = true;
            
            // Save the empty formatted mempak
            CV64_MempakEditor_SaveFile(filename);
            return true;
        }
        
        char msg[512];
        sprintf_s(msg, "[CV64] Failed to open memory pak file (error %d): %s\n", err, filename);
        OutputDebugStringA(msg);
        return false;
    }

    size_t bytesRead = fread(g_mempak.mempakData, 1, sizeof(g_mempak.mempakData), file);
    fclose(file);

    if (bytesRead != sizeof(g_mempak.mempakData)) {
        char msg[512];
        sprintf_s(msg, "[CV64] Warning: Memory pak file size mismatch: %zu bytes (expected %zu)\n", 
                  bytesRead, sizeof(g_mempak.mempakData));
        OutputDebugStringA(msg);
        
        // Zero out the rest if partial read
        if (bytesRead < sizeof(g_mempak.mempakData)) {
            memset(g_mempak.mempakData + bytesRead, 0, sizeof(g_mempak.mempakData) - bytesRead);
        }
    }

    strcpy_s(g_mempak.currentFile, filename);
    g_mempak.modified = false;

    char msg[512];
    sprintf_s(msg, "[CV64] Memory pak loaded successfully: %s (%zu bytes)\n", filename, bytesRead);
    OutputDebugStringA(msg);

    return true;
}

/**
 * @brief Save memory pak file to disk
 */
bool CV64_MempakEditor_SaveFile(const char* filename)
{
    FILE* file = NULL;
    if (fopen_s(&file, filename, "wb") != 0 || !file) {
        char msg[512];
        sprintf_s(msg, "[CV64] Failed to create memory pak file: %s\n", filename);
        OutputDebugStringA(msg);
        return false;
    }

    size_t bytesWritten = fwrite(g_mempak.mempakData, 1, sizeof(g_mempak.mempakData), file);
    fclose(file);

    if (bytesWritten != sizeof(g_mempak.mempakData)) {
        char msg[512];
        sprintf_s(msg, "[CV64] Failed to write complete memory pak file: %zu bytes written\n", 
                  bytesWritten);
        OutputDebugStringA(msg);
        return false;
    }

    strcpy_s(g_mempak.currentFile, filename);
    g_mempak.modified = false;

    char msg[512];
    sprintf_s(msg, "[CV64] Memory pak saved to: %s\n", filename);
    OutputDebugStringA(msg);

    return true;
}

/**
 * @brief Calculate checksum for save data
 * Uses a simple additive checksum of all bytes before the checksum field
 */
uint32_t CV64_MempakEditor_CalculateChecksum(const CV64_SaveData* data)
{
    uint32_t sum = 0;
    const uint8_t* bytes = (const uint8_t*)data;
    size_t checksumOffset = offsetof(CV64_SaveData, checksum);
    
    // Sum all bytes before the checksum field
    for (size_t i = 0; i < checksumOffset; i++) {
        sum += bytes[i];
    }
    
    // N64 checksums often use complement
    return sum ^ 0xFFFFFFFF;
}

/**
 * @brief Verify checksum of save data
 */
bool CV64_MempakEditor_VerifyChecksum(const CV64_SaveData* data)
{
    uint32_t calculated = CV64_MempakEditor_CalculateChecksum(data);
    return (calculated == data->checksum);
}

/**
 * @brief Get offset for a specific slot index
 */
static int GetCurrentSlotOffset(int slotIndex)
{
    int slotOffsets[MAX_SAVE_SLOTS];
    int slotCount = 0;
    FindSaveSlots(g_mempak.mempakData, slotOffsets, &slotCount);
    
    if (slotIndex >= 0 && slotIndex < slotCount) {
        return slotOffsets[slotIndex];
    }
    return -1;
}

/**
 * @brief Find save slots in memory pak data
 * 
 * Memory pak structure:
 * - Page 0-2: System data (ID blocks, index tables)
 * - Page 3+: Save data (16 pages per note/save)
 * 
 * CV64 saves are 512 bytes each, stored in note format.
 */
static bool FindSaveSlots(const uint8_t* mempakData, int* outSlotOffsets, int* outSlotCount)
{
    *outSlotCount = 0;

    // Start searching after system pages (first 3 pages = 768 bytes)
    // Each page is 256 bytes
    const int pageSize = 256;
    
    // Search for CV64 save data patterns
    // CV64 saves start at page boundaries and are 512 bytes (2 pages)
    for (int page = 5; page < 123 && *outSlotCount < MAX_SAVE_SLOTS; page += 2) {
        int offset = page * pageSize;
        const CV64_SaveData* saveData = (const CV64_SaveData*)(mempakData + offset);
        
        // Check if this looks like valid CV64 save data
        // Character should be 0 or 1
        // Health and maxHealth should be reasonable
        // Also check that the data isn't all zeros or all 0xFF
        if (saveData->character <= 1 && 
            saveData->health > 0 && saveData->health <= 999 &&
            saveData->maxHealth >= saveData->health && saveData->maxHealth <= 999 &&
            saveData->mapID < 100) {  // Reasonable map ID range
            
            outSlotOffsets[*outSlotCount] = offset;
            (*outSlotCount)++;
        }
    }

    return (*outSlotCount > 0);
}

/**
 * @brief Format play time from frames to readable string
 */
static void FormatPlayTime(uint32_t frames, char* buffer, size_t bufferSize)
{
    // N64 runs at 60 FPS (NTSC)
    uint32_t totalSeconds = frames / 60;
    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;
    
    sprintf_s(buffer, bufferSize, "%02u:%02u:%02u", hours, minutes, seconds);
}

/**
 * @brief Update the slot list with available saves
 */
static void UpdateSlotList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_MEMPAK_SLOT_LIST);
    if (!hList) return;

    // Clear existing items
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    // Find save slots
    int slotOffsets[MAX_SAVE_SLOTS];
    int slotCount = 0;
    FindSaveSlots(g_mempak.mempakData, slotOffsets, &slotCount);

    // Add slots to list
    for (int i = 0; i < slotCount; i++) {
        const CV64_SaveData* saveData = (const CV64_SaveData*)(g_mempak.mempakData + slotOffsets[i]);
        
        char slotName[256];
        const char* character = (saveData->character <= 1) ? g_characterNames[saveData->character] : "Unknown";
        
        // Format: "Slot 1: Reinhardt - Map 5 - HP: 100/200"
        sprintf_s(slotName, "Slot %d: %s - Map %d - HP: %d/%d", 
                  i + 1, character, saveData->mapID, saveData->health, saveData->maxHealth);
        
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)slotName);
    }

    // Select first slot if available
    if (slotCount > 0) {
        SendMessage(hList, LB_SETCURSEL, 0, 0);
        UpdateSlotData(hDlg, 0);
    }
}

/**
 * @brief Update UI with slot data
 */
static void UpdateSlotData(HWND hDlg, int slotIndex)
{
    int slotOffsets[MAX_SAVE_SLOTS];
    int slotCount = 0;
    FindSaveSlots(g_mempak.mempakData, slotOffsets, &slotCount);

    if (slotIndex < 0 || slotIndex >= slotCount) {
        return;
    }

    g_mempak.currentSlot = slotIndex;
    const CV64_SaveData* saveData = (const CV64_SaveData*)(g_mempak.mempakData + slotOffsets[slotIndex]);

    // Update character
    SetDlgItemTextA(hDlg, IDC_MEMPAK_CHARACTER, 
                    (saveData->character <= 1) ? g_characterNames[saveData->character] : "Unknown");

    // Update stats
    SetDlgItemInt(hDlg, IDC_MEMPAK_HEALTH, saveData->health, FALSE);
    SetDlgItemInt(hDlg, IDC_MEMPAK_MAX_HEALTH, saveData->maxHealth, FALSE);
    SetDlgItemInt(hDlg, IDC_MEMPAK_JEWELS, saveData->jewels, FALSE);
    SetDlgItemInt(hDlg, IDC_MEMPAK_MAP_ID, saveData->mapID, FALSE);

    // Update subweapon
    SetDlgItemTextA(hDlg, IDC_MEMPAK_SUBWEAPON, 
                    (saveData->subweapon < _countof(g_subweaponNames)) ? 
                    g_subweaponNames[saveData->subweapon] : "Unknown");

    // Update difficulty
    SetDlgItemTextA(hDlg, IDC_MEMPAK_DIFFICULTY, 
                    (saveData->difficulty < _countof(g_difficultyNames)) ? 
                    g_difficultyNames[saveData->difficulty] : "Unknown");

    // Update play time
    char playTimeStr[64];
    FormatPlayTime(saveData->playTime, playTimeStr, sizeof(playTimeStr));
    SetDlgItemTextA(hDlg, IDC_MEMPAK_PLAYTIME, playTimeStr);

    // Update enemies killed
    SetDlgItemInt(hDlg, IDC_MEMPAK_ENEMIES_KILLED, saveData->enemiesKilled, FALSE);

    // Update save name
    char saveName[17];
    memcpy(saveName, saveData->saveName, 16);
    saveName[16] = '\0';
    SetDlgItemTextA(hDlg, IDC_MEMPAK_SAVE_NAME, saveName);

    // Verify checksum
    bool checksumValid = CV64_MempakEditor_VerifyChecksum(saveData);
    char status[128];
    sprintf_s(status, "Checksum: %s (0x%08X)", 
              checksumValid ? "Valid" : "INVALID", saveData->checksum);
    SetDlgItemTextA(hDlg, IDC_MEMPAK_STATUS, status);
}

/**
 * @brief Apply UI changes to the current slot in memory
 */
static void ApplyUIChangesToSlot(HWND hDlg, int slotIndex)
{
    int offset = GetCurrentSlotOffset(slotIndex);
    if (offset < 0) return;

    CV64_SaveData* saveData = (CV64_SaveData*)(g_mempak.mempakData + offset);

    // Update editable fields from UI with validation
    UINT health = GetDlgItemInt(hDlg, IDC_MEMPAK_HEALTH, NULL, FALSE);
    UINT maxHealth = GetDlgItemInt(hDlg, IDC_MEMPAK_MAX_HEALTH, NULL, FALSE);
    UINT jewels = GetDlgItemInt(hDlg, IDC_MEMPAK_JEWELS, NULL, FALSE);
    UINT mapID = GetDlgItemInt(hDlg, IDC_MEMPAK_MAP_ID, NULL, FALSE);
    UINT enemiesKilled = GetDlgItemInt(hDlg, IDC_MEMPAK_ENEMIES_KILLED, NULL, FALSE);

    // Validate values
    if (health > 999) health = 999;
    if (maxHealth > 999) maxHealth = 999;
    if (maxHealth < health) maxHealth = health;  // Ensure maxHealth >= health
    if (jewels > 9999) jewels = 9999;
    if (mapID > 255) mapID = 255;
    if (enemiesKilled > 9999) enemiesKilled = 9999;

    saveData->health = (uint16_t)health;
    saveData->maxHealth = (uint16_t)maxHealth;
    saveData->jewels = (uint16_t)jewels;
    saveData->mapID = (uint8_t)mapID;
    saveData->enemiesKilled = (uint16_t)enemiesKilled;

    // Update save name
    char saveName[17] = {0};
    GetDlgItemTextA(hDlg, IDC_MEMPAK_SAVE_NAME, saveName, 17);
    memcpy(saveData->saveName, saveName, 16);

    // Recalculate checksum
    saveData->checksum = CV64_MempakEditor_CalculateChecksum(saveData);

    // Mark as modified
    g_mempak.modified = true;

    // Update checksum display
    char status[128];
    sprintf_s(status, "Checksum: Valid (0x%08X) - MODIFIED", saveData->checksum);
    SetDlgItemTextA(hDlg, IDC_MEMPAK_STATUS, status);

    OutputDebugStringA("[CV64] Slot data updated and checksum recalculated\n");
}

/**
 * @brief Export a save slot to a separate file
 */
static void ExportSlot(HWND hDlg, int slotIndex)
{
    int offset = GetCurrentSlotOffset(slotIndex);
    if (offset < 0) {
        MessageBoxA(hDlg, "Invalid slot index!", "Export Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Open file dialog
    char filename[MAX_PATH] = "cv64_save_slot.sav";
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "CV64 Save Files (*.sav)\0*.sav\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = "Export Save Slot";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "sav";

    if (!GetSaveFileNameA(&ofn)) {
        return;  // User cancelled
    }

    // Write slot data to file
    FILE* file = NULL;
    if (fopen_s(&file, filename, "wb") != 0 || !file) {
        MessageBoxA(hDlg, "Failed to create export file!", "Export Error", MB_OK | MB_ICONERROR);
        return;
    }

    const CV64_SaveData* saveData = (const CV64_SaveData*)(g_mempak.mempakData + offset);
    size_t written = fwrite(saveData, 1, sizeof(CV64_SaveData), file);
    fclose(file);

    if (written != sizeof(CV64_SaveData)) {
        MessageBoxA(hDlg, "Failed to write complete save data!", "Export Error", MB_OK | MB_ICONERROR);
    } else {
        MessageBoxA(hDlg, "Save slot exported successfully!", "Export Complete", MB_OK | MB_ICONINFORMATION);
    }
}

/**
 * @brief Import a save slot from a file
 */
static void ImportSlot(HWND hDlg, int slotIndex)
{
    int offset = GetCurrentSlotOffset(slotIndex);
    if (offset < 0) {
        MessageBoxA(hDlg, "Invalid slot index!", "Import Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Open file dialog
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "CV64 Save Files (*.sav)\0*.sav\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = "Import Save Slot";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "sav";

    if (!GetOpenFileNameA(&ofn)) {
        return;  // User cancelled
    }

    // Read slot data from file
    FILE* file = NULL;
    if (fopen_s(&file, filename, "rb") != 0 || !file) {
        MessageBoxA(hDlg, "Failed to open import file!", "Import Error", MB_OK | MB_ICONERROR);
        return;
    }

    CV64_SaveData tempSave;
    size_t bytesRead = fread(&tempSave, 1, sizeof(CV64_SaveData), file);
    fclose(file);

    if (bytesRead != sizeof(CV64_SaveData)) {
        MessageBoxA(hDlg, "Invalid save file size!", "Import Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Verify checksum
    if (!CV64_MempakEditor_VerifyChecksum(&tempSave)) {
        int result = MessageBoxA(hDlg, 
            "Warning: Save file has invalid checksum!\nImport anyway?", 
            "Checksum Warning", 
            MB_YESNO | MB_ICONWARNING);
        if (result != IDYES) {
            return;
        }
    }

    // Copy to mempak
    memcpy(g_mempak.mempakData + offset, &tempSave, sizeof(CV64_SaveData));
    g_mempak.modified = true;

    // Refresh UI
    UpdateSlotList(hDlg);
    UpdateSlotData(hDlg, slotIndex);

    MessageBoxA(hDlg, "Save slot imported successfully!", "Import Complete", MB_OK | MB_ICONINFORMATION);
}

/**
 * @brief Delete a save slot (clear all data)
 */
static void DeleteSlot(HWND hDlg, int slotIndex)
{
    int offset = GetCurrentSlotOffset(slotIndex);
    if (offset < 0) {
        MessageBoxA(hDlg, "Invalid slot index!", "Delete Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Confirm deletion
    int result = MessageBoxA(hDlg, 
        "Are you sure you want to delete this save slot?\nThis cannot be undone!", 
        "Confirm Delete", 
        MB_YESNO | MB_ICONWARNING);
    
    if (result != IDYES) {
        return;
    }

    // Zero out the save data
    memset(g_mempak.mempakData + offset, 0, sizeof(CV64_SaveData));
    g_mempak.modified = true;

    // Refresh UI
    UpdateSlotList(hDlg);

    MessageBoxA(hDlg, "Save slot deleted successfully!", "Delete Complete", MB_OK | MB_ICONINFORMATION);
}

/**
 * @brief Memory Pak Editor dialog procedure
 */
static INT_PTR CALLBACK MempakEditorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        EnableDarkModeForDialog(hDlg);
        
        // Center dialog on parent
        RECT rcParent, rcDlg;
        HWND hParent = GetParent(hDlg);
        if (!hParent) hParent = GetDesktopWindow();
        
        GetWindowRect(hParent, &rcParent);
        GetWindowRect(hDlg, &rcDlg);
        
        int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
        int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;
        
        SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        
        // Try to load screenshot if it exists
        char screenshotPath[MAX_PATH];
        strcpy_s(screenshotPath, g_mempak.currentFile);
        char* ext = strrchr(screenshotPath, '.');
        if (ext) {
            strcpy_s(ext, MAX_PATH - (ext - screenshotPath), ".png");
        } else {
            strcat_s(screenshotPath, ".png");
        }
        
        // Check if screenshot exists
        if (GetFileAttributesA(screenshotPath) != INVALID_FILE_ATTRIBUTES) {
            CV64_MempakEditor_SetScreenshot(screenshotPath);
        }
        
        // Update UI with loaded data
        UpdateSlotList(hDlg);
        UpdateScreenshotDisplay(hDlg);
        
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        
        switch (wmId)
        {
        case IDC_MEMPAK_SLOT_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                // Apply changes from previous slot before switching
                if (g_mempak.currentSlot >= 0) {
                    ApplyUIChangesToSlot(hDlg, g_mempak.currentSlot);
                }
                
                HWND hList = GetDlgItem(hDlg, IDC_MEMPAK_SLOT_LIST);
                int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    UpdateSlotData(hDlg, sel);
                }
            }
            break;

        case IDC_MEMPAK_REFRESH:
            if (g_mempak.modified) {
                int result = MessageBoxA(hDlg, 
                    "Discard unsaved changes and reload?", 
                    "Confirm Refresh", 
                    MB_YESNO | MB_ICONWARNING);
                if (result != IDYES) {
                    break;
                }
            }
            CV64_MempakEditor_LoadFile(g_mempak.currentFile);
            UpdateSlotList(hDlg);
            MessageBoxA(hDlg, "Memory pak reloaded from disk.", "Refresh", MB_OK | MB_ICONINFORMATION);
            break;

        case IDC_MEMPAK_SAVE:
            // Apply current UI changes before saving
            if (g_mempak.currentSlot >= 0) {
                ApplyUIChangesToSlot(hDlg, g_mempak.currentSlot);
            }
            
            if (CV64_MempakEditor_SaveFile(g_mempak.currentFile)) {
                MessageBoxA(hDlg, "Memory pak saved successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                UpdateSlotList(hDlg);  // Refresh to show updated checksums
            } else {
                MessageBoxA(hDlg, "Failed to save memory pak!", "Error", MB_OK | MB_ICONERROR);
            }
            break;

        case IDC_MEMPAK_EXPORT:
            if (g_mempak.currentSlot >= 0) {
                // Apply current changes before exporting
                ApplyUIChangesToSlot(hDlg, g_mempak.currentSlot);
                ExportSlot(hDlg, g_mempak.currentSlot);
            } else {
                MessageBoxA(hDlg, "Please select a save slot first.", "No Slot Selected", MB_OK | MB_ICONWARNING);
            }
            break;

        case IDC_MEMPAK_IMPORT:
            if (g_mempak.currentSlot >= 0) {
                ImportSlot(hDlg, g_mempak.currentSlot);
            } else {
                MessageBoxA(hDlg, "Please select a save slot first.", "No Slot Selected", MB_OK | MB_ICONWARNING);
            }
            break;

        case IDC_MEMPAK_DELETE:
            if (g_mempak.currentSlot >= 0) {
                DeleteSlot(hDlg, g_mempak.currentSlot);
            } else {
                MessageBoxA(hDlg, "Please select a save slot first.", "No Slot Selected", MB_OK | MB_ICONWARNING);
            }
            break;

        case IDC_MEMPAK_HEALTH:
        case IDC_MEMPAK_MAX_HEALTH:
        case IDC_MEMPAK_JEWELS:
        case IDC_MEMPAK_MAP_ID:
        case IDC_MEMPAK_ENEMIES_KILLED:
        case IDC_MEMPAK_SAVE_NAME:
            // Mark as modified when user edits a field
            if (HIWORD(wParam) == EN_CHANGE) {
                // Only mark as modified if dialog is already initialized
                static bool s_initializing = false;
                if (!s_initializing) {
                    g_mempak.modified = true;
                }
            }
            break;

        case IDOK:
        case IDCANCEL:
            // Apply final changes before closing
            if (g_mempak.currentSlot >= 0 && g_mempak.modified) {
                ApplyUIChangesToSlot(hDlg, g_mempak.currentSlot);
            }
            
            if (g_mempak.modified) {
                int result = MessageBoxA(hDlg, 
                    "Save changes to memory pak?", 
                    "Unsaved Changes", 
                    MB_YESNOCANCEL | MB_ICONQUESTION);
                
                if (result == IDCANCEL) {
                    return (INT_PTR)TRUE;
                } else if (result == IDYES) {
                    if (!CV64_MempakEditor_SaveFile(g_mempak.currentFile)) {
                        MessageBoxA(hDlg, "Failed to save memory pak!", "Error", MB_OK | MB_ICONERROR);
                        return (INT_PTR)TRUE;
                    }
                }
            }
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

    case WM_DRAWITEM:
    {
        // Handle drawing the screenshot
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlID == IDC_MEMPAK_SCREENSHOT) {
            // Clear background
            FillRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            
            if (g_mempak.screenshotBitmap) {
                // Draw the bitmap
                HDC hdcMem = CreateCompatibleDC(pDIS->hDC);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, g_mempak.screenshotBitmap);
                
                // Get bitmap dimensions
                BITMAP bm;
                GetObject(g_mempak.screenshotBitmap, sizeof(bm), &bm);
                
                // Calculate scaling to fit the control
                int ctrlWidth = pDIS->rcItem.right - pDIS->rcItem.left;
                int ctrlHeight = pDIS->rcItem.bottom - pDIS->rcItem.top;
                
                float scaleX = (float)ctrlWidth / bm.bmWidth;
                float scaleY = (float)ctrlHeight / bm.bmHeight;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                
                int drawWidth = (int)(bm.bmWidth * scale);
                int drawHeight = (int)(bm.bmHeight * scale);
                int offsetX = (ctrlWidth - drawWidth) / 2;
                int offsetY = (ctrlHeight - drawHeight) / 2;
                
                // Draw scaled bitmap
                SetStretchBltMode(pDIS->hDC, HALFTONE);
                StretchBlt(pDIS->hDC, 
                          pDIS->rcItem.left + offsetX, 
                          pDIS->rcItem.top + offsetY,
                          drawWidth, drawHeight,
                          hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                
                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
            } else {
                // No screenshot - draw placeholder text
                SetBkMode(pDIS->hDC, TRANSPARENT);
                SetTextColor(pDIS->hDC, RGB(128, 128, 128));
                DrawTextA(pDIS->hDC, "No Screenshot", -1, &pDIS->rcItem, 
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            return (INT_PTR)TRUE;
        }
        break;
    }
    }

    return (INT_PTR)FALSE;
}

/**
 * @brief Get save slot data from memory pak
 */
bool CV64_MempakEditor_GetSlot(int slotIndex, CV64_SaveData* outData)
{
    int slotOffsets[MAX_SAVE_SLOTS];
    int slotCount = 0;
    FindSaveSlots(g_mempak.mempakData, slotOffsets, &slotCount);

    if (slotIndex < 0 || slotIndex >= slotCount || !outData) {
        return false;
    }

    memcpy(outData, g_mempak.mempakData + slotOffsets[slotIndex], sizeof(CV64_SaveData));
    return true;
}

/**
 * @brief Write save slot data to memory pak
 */
bool CV64_MempakEditor_SetSlot(int slotIndex, const CV64_SaveData* data)
{
    int slotOffsets[MAX_SAVE_SLOTS];
    int slotCount = 0;
    FindSaveSlots(g_mempak.mempakData, slotOffsets, &slotCount);

    if (slotIndex < 0 || slotIndex >= slotCount || !data) {
        return false;
    }

    memcpy(g_mempak.mempakData + slotOffsets[slotIndex], data, sizeof(CV64_SaveData));
    g_mempak.modified = true;
    return true;
}

/**
 * @brief Load screenshot bitmap from file
 */
static void LoadScreenshotBitmap()
{
    // Clean up old bitmap
    if (g_mempak.screenshotBitmap) {
        DeleteObject(g_mempak.screenshotBitmap);
        g_mempak.screenshotBitmap = NULL;
    }

    if (g_mempak.screenshotPath[0] == '\0') {
        return;
    }

    // Check if file exists
    DWORD attribs = GetFileAttributesA(g_mempak.screenshotPath);
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        OutputDebugStringA("[CV64] Screenshot file not found\n");
        return;
    }

    // Convert to wide string for GDI+
    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, g_mempak.screenshotPath, -1, wPath, MAX_PATH);

    // Load PNG using GDI+
    Bitmap* bitmap = new Bitmap(wPath);
    if (bitmap && bitmap->GetLastStatus() == Ok) {
        // Convert to HBITMAP
        Color background(0, 0, 0);
        bitmap->GetHBITMAP(background, &g_mempak.screenshotBitmap);
        
        char msg[512];
        sprintf_s(msg, "[CV64] Screenshot loaded successfully: %s (%dx%d)\n", 
                  g_mempak.screenshotPath, bitmap->GetWidth(), bitmap->GetHeight());
        OutputDebugStringA(msg);
    } else {
        char msg[512];
        sprintf_s(msg, "[CV64] Failed to load screenshot: %s (GDI+ error)\n", g_mempak.screenshotPath);
        OutputDebugStringA(msg);
    }
    
    delete bitmap;
}

/**
 * @brief Update screenshot display in dialog
 */
static void UpdateScreenshotDisplay(HWND hDlg)
{
    HWND hScreenshot = GetDlgItem(hDlg, IDC_MEMPAK_SCREENSHOT);
    if (!hScreenshot) {
        return;
    }

    // Load the bitmap if we have a path
    if (g_mempak.screenshotPath[0] != '\0' && !g_mempak.screenshotBitmap) {
        LoadScreenshotBitmap();
    }

    // Trigger repaint
    InvalidateRect(hScreenshot, NULL, TRUE);
}

// Static variable to track pending screenshot
static struct {
    bool pending;
    char path[MAX_PATH];
    DWORD triggerTime;
} g_pendingScreenshot = { false, "", 0 };

/**
 * @brief Schedule screenshot capture after a delay
 */
static void CaptureScreenshotDelayed()
{
    // Build screenshot path based on mempak file
    char screenshotPath[MAX_PATH];
    strcpy_s(screenshotPath, g_mempak.currentFile);
    
    // Replace .mpk extension with .png
    char* ext = strrchr(screenshotPath, '.');
    if (ext) {
        strcpy_s(ext, MAX_PATH - (ext - screenshotPath), ".png");
    } else {
        strcat_s(screenshotPath, ".png");
    }
    
    // Ensure save directory exists
    char saveDir[MAX_PATH];
    strcpy_s(saveDir, screenshotPath);
    char* lastSlash = strrchr(saveDir, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        CreateDirectoryA(saveDir, NULL);  // Ignore error if already exists
    }
    
    // Store pending screenshot info
    g_pendingScreenshot.pending = true;
    strcpy_s(g_pendingScreenshot.path, screenshotPath);
    g_pendingScreenshot.triggerTime = GetTickCount() + 3000;  // 3 seconds from now
    
    // Store the path for later display
    strcpy_s(g_mempak.screenshotPath, screenshotPath);
    
    char msg[512];
    sprintf_s(msg, "[CV64] Screenshot scheduled to: %s (will capture in 3 seconds)\n", screenshotPath);
    OutputDebugStringA(msg);
}

/**
 * @brief Check and execute pending screenshot (call from main thread)
 */
void CV64_MempakEditor_ProcessPendingScreenshot(void)
{
    if (!g_pendingScreenshot.pending) {
        return;
    }
    
    // Check if it's time to take the screenshot
    if (GetTickCount() >= g_pendingScreenshot.triggerTime) {
        char msg[512];
        sprintf_s(msg, "[CV64] Taking screenshot to: %s\n", g_pendingScreenshot.path);
        OutputDebugStringA(msg);
        
        TakeScreenshotToPath(g_pendingScreenshot.path);
        
        sprintf_s(msg, "[CV64] Screenshot captured to: %s\n", g_pendingScreenshot.path);
        OutputDebugStringA(msg);
        
        g_pendingScreenshot.pending = false;
    }
}

/**
 * @brief Called when mempak is saved (triggers screenshot capture)
 */
void CV64_MempakEditor_OnMempakSave(void)
{
    if (!g_mempak.initialized) {
        return;
    }
    
    OutputDebugStringA("[CV64] Mempak save detected, scheduling screenshot capture\n");
    CaptureScreenshotDelayed();
}

/**
 * @brief Set screenshot path for the current mempak
 */
void CV64_MempakEditor_SetScreenshot(const char* screenshotPath)
{
    if (!g_mempak.initialized) {
        CV64_MempakEditor_Init();
    }
    
    if (screenshotPath) {
        strcpy_s(g_mempak.screenshotPath, screenshotPath);
        LoadScreenshotBitmap();
    } else {
        g_mempak.screenshotPath[0] = '\0';
        if (g_mempak.screenshotBitmap) {
            DeleteObject(g_mempak.screenshotBitmap);
            g_mempak.screenshotBitmap = NULL;
        }
    }
    
    char msg[512];
    sprintf_s(msg, "[CV64] Screenshot path set: %s\n", 
              screenshotPath ? screenshotPath : "(none)");
    OutputDebugStringA(msg);
}

/**
 * @brief Get screenshot path for the current mempak
 */
const char* CV64_MempakEditor_GetScreenshot(void)
{
    if (!g_mempak.initialized || g_mempak.screenshotPath[0] == '\0') {
        return NULL;
    }
    return g_mempak.screenshotPath;
}

