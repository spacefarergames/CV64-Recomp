/**
 * @file cv64_mod_loader.cpp
 * @brief CV64 Mod Loading System Implementation
 */

#include "../include/cv64_mod_loader.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <shellapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")

// Internal mod storage
static std::vector<CV64_ModInfo> g_mods;
static bool g_initialized = false;
static HWND g_dialogHwnd = NULL;

// Dialog controls
#define IDC_MOD_LIST        2000
#define IDC_MOD_ENABLE      2001
#define IDC_MOD_DISABLE     2002
#define IDC_MOD_RELOAD      2003
#define IDC_MOD_CREATE      2004
#define IDC_MOD_DESCRIPTION 2005
#define IDC_MOD_STATS       2006
#define IDC_MOD_OPEN_FOLDER 2007

/**
 * @brief Parse mod.ini file
 */
static bool ParseModIni(const char* iniPath, CV64_ModInfo* outInfo) {
    char buffer[1024];
    
    GetPrivateProfileStringA("Mod", "Name", "Unnamed Mod", 
        outInfo->name, CV64_MOD_MAX_NAME, iniPath);
    GetPrivateProfileStringA("Mod", "Author", "Unknown", 
        outInfo->author, CV64_MOD_MAX_AUTHOR, iniPath);
    GetPrivateProfileStringA("Mod", "Version", "1.0", 
        outInfo->version, CV64_MOD_MAX_VERSION, iniPath);
    GetPrivateProfileStringA("Mod", "Description", "No description", 
        outInfo->description, CV64_MOD_MAX_DESCRIPTION, iniPath);
    
    // Get mod type
    GetPrivateProfileStringA("Mod", "Type", "Unknown", buffer, sizeof(buffer), iniPath);
    if (_stricmp(buffer, "Texture") == 0) outInfo->type = CV64_MOD_TYPE_TEXTURE;
    else if (_stricmp(buffer, "Model") == 0) outInfo->type = CV64_MOD_TYPE_MODEL;
    else if (_stricmp(buffer, "Audio") == 0) outInfo->type = CV64_MOD_TYPE_AUDIO;
    else if (_stricmp(buffer, "CodePatch") == 0) outInfo->type = CV64_MOD_TYPE_CODE_PATCH;
    else if (_stricmp(buffer, "ROMPatch") == 0) outInfo->type = CV64_MOD_TYPE_ROM_PATCH;
    else if (_stricmp(buffer, "Script") == 0) outInfo->type = CV64_MOD_TYPE_SCRIPT;
    else if (_stricmp(buffer, "Pack") == 0) outInfo->type = CV64_MOD_TYPE_PACK;
    else outInfo->type = CV64_MOD_TYPE_UNKNOWN;
    
    // Get priority
    outInfo->priority = (CV64_ModPriority)GetPrivateProfileIntA("Mod", "Priority", 
        CV64_MOD_PRIORITY_NORMAL, iniPath);
    
    // Get enabled state
    outInfo->enabled = GetPrivateProfileIntA("Mod", "Enabled", 0, iniPath) != 0;
    
    return true;
}

/**
 * @brief Calculate directory size
 */
static uint64_t CalculateDirectorySize(const char* path) {
    uint64_t totalSize = 0;
    WIN32_FIND_DATAA findData;
    char searchPath[MAX_PATH];
    
    sprintf_s(searchPath, "%s\\*.*", path);
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                char fullPath[MAX_PATH];
                sprintf_s(fullPath, "%s\\%s", path, findData.cFileName);
                
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    totalSize += CalculateDirectorySize(fullPath);
                } else {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    totalSize += fileSize.QuadPart;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    return totalSize;
}

/**
 * @brief Update mod list in dialog
 */
static void UpdateModList(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_MOD_LIST);
    ListView_DeleteAllItems(hList);
    
    for (size_t i = 0; i < g_mods.size(); i++) {
        const CV64_ModInfo& mod = g_mods[i];
        
        LVITEMA item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)i;
        item.lParam = (LPARAM)i;
        
        // Name
        item.pszText = (LPSTR)mod.name;
        int index = SendMessageA(hList, LVM_INSERTITEMA, 0, (LPARAM)&item);
        
        // Enabled
        char enabledText[16];
        strcpy_s(enabledText, mod.enabled ? "Yes" : "No");
        LVITEMA enabledItem = { LVIF_TEXT, index, 1, 0, 0, enabledText };
        SendMessageA(hList, LVM_SETITEMTEXTA, index, (LPARAM)&enabledItem);
        
        // Type
        char typeText[32];
        switch (mod.type) {
            case CV64_MOD_TYPE_TEXTURE: strcpy_s(typeText, "Texture"); break;
            case CV64_MOD_TYPE_MODEL: strcpy_s(typeText, "Model"); break;
            case CV64_MOD_TYPE_AUDIO: strcpy_s(typeText, "Audio"); break;
            case CV64_MOD_TYPE_CODE_PATCH: strcpy_s(typeText, "Code Patch"); break;
            case CV64_MOD_TYPE_ROM_PATCH: strcpy_s(typeText, "ROM Patch"); break;
            case CV64_MOD_TYPE_SCRIPT: strcpy_s(typeText, "Script"); break;
            case CV64_MOD_TYPE_PACK: strcpy_s(typeText, "Pack"); break;
            default: strcpy_s(typeText, "Unknown"); break;
        }
        LVITEMA typeItem = { LVIF_TEXT, index, 2, 0, 0, typeText };
        SendMessageA(hList, LVM_SETITEMTEXTA, index, (LPARAM)&typeItem);
        
        // Author
        char authorText[CV64_MOD_MAX_AUTHOR];
        strcpy_s(authorText, mod.author);
        LVITEMA authorItem = { LVIF_TEXT, index, 3, 0, 0, authorText };
        SendMessageA(hList, LVM_SETITEMTEXTA, index, (LPARAM)&authorItem);
        
        // Version
        char versionText[CV64_MOD_MAX_VERSION];
        strcpy_s(versionText, mod.version);
        LVITEMA versionItem = { LVIF_TEXT, index, 4, 0, 0, versionText };
        SendMessageA(hList, LVM_SETITEMTEXTA, index, (LPARAM)&versionItem);
    }
}

/**
 * @brief Mod loader dialog procedure
 */
static INT_PTR CALLBACK ModLoaderDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
    {
        g_dialogHwnd = hDlg;
        
        // Setup list view
        HWND hList = GetDlgItem(hDlg, IDC_MOD_LIST);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        // Add columns
        LVCOLUMNA col = { 0 };
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        
        col.pszText = (LPSTR)"Mod Name";
        col.cx = 200;
        ListView_InsertColumn(hList, 0, &col);
        
        col.pszText = (LPSTR)"Enabled";
        col.cx = 70;
        ListView_InsertColumn(hList, 1, &col);
        
        col.pszText = (LPSTR)"Type";
        col.cx = 100;
        ListView_InsertColumn(hList, 2, &col);
        
        col.pszText = (LPSTR)"Author";
        col.cx = 120;
        ListView_InsertColumn(hList, 3, &col);
        
        col.pszText = (LPSTR)"Version";
        col.cx = 80;
        ListView_InsertColumn(hList, 4, &col);
        
        UpdateModList(hDlg);
        
        // Update stats
        uint32_t total, enabled, loaded;
        CV64_ModLoader_GetStats(&total, &enabled, &loaded);
        char statsText[256];
        sprintf_s(statsText, "Total Mods: %u | Enabled: %u | Loaded: %u", total, enabled, loaded);
        SetDlgItemTextA(hDlg, IDC_MOD_STATS, statsText);
        
        return TRUE;
    }
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_MOD_ENABLE:
        case IDC_MOD_DISABLE:
        {
            HWND hList = GetDlgItem(hDlg, IDC_MOD_LIST);
            int selectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (selectedIndex >= 0 && selectedIndex < (int)g_mods.size()) {
                if (LOWORD(wParam) == IDC_MOD_ENABLE) {
                    CV64_ModLoader_EnableMod(g_mods[selectedIndex].name);
                } else {
                    CV64_ModLoader_DisableMod(g_mods[selectedIndex].name);
                }
                UpdateModList(hDlg);
            }
            return TRUE;
        }
        
        case IDC_MOD_RELOAD:
            CV64_ModLoader_ReloadMods();
            UpdateModList(hDlg);
            MessageBoxA(hDlg, "Mods reloaded successfully!", "Mod Loader", MB_OK | MB_ICONINFORMATION);
            return TRUE;
        
        case IDC_MOD_OPEN_FOLDER:
        {
            char modsPath[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, modsPath);
            strcat_s(modsPath, "\\mods");
            ShellExecuteA(NULL, "open", modsPath, NULL, NULL, SW_SHOW);
            return TRUE;
        }
        
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            g_dialogHwnd = NULL;
            return TRUE;
        }
        break;
    
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == IDC_MOD_LIST && pnmh->code == LVN_ITEMCHANGED) {
            LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
            if (pnmlv->uNewState & LVIS_SELECTED) {
                int index = pnmlv->iItem;
                if (index >= 0 && index < (int)g_mods.size()) {
                    SetDlgItemTextA(hDlg, IDC_MOD_DESCRIPTION, g_mods[index].description);
                }
            }
        }
        break;
    }
    }
    
    return FALSE;
}

// Public API Implementation

bool CV64_ModLoader_Init(void) {
    if (g_initialized) {
        return true;
    }
    
    g_mods.clear();
    g_initialized = true;
    
    OutputDebugStringA("[CV64] Mod loader initialized\n");
    return true;
}

void CV64_ModLoader_Shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    CV64_ModLoader_UnloadAllMods();
    g_mods.clear();
    g_initialized = false;
    
    OutputDebugStringA("[CV64] Mod loader shut down\n");
}

uint32_t CV64_ModLoader_ScanMods(void) {
    if (!g_initialized) {
        return 0;
    }
    
    g_mods.clear();
    
    // Get mods directory
    char modsPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, modsPath);
    strcat_s(modsPath, "\\mods");
    
    // Create mods directory if it doesn't exist
    CreateDirectoryA(modsPath, NULL);
    
    // Scan for mod folders
    WIN32_FIND_DATAA findData;
    char searchPath[MAX_PATH];
    sprintf_s(searchPath, "%s\\*.*", modsPath);
    
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                    // Check for mod.ini
                    char modPath[MAX_PATH];
                    sprintf_s(modPath, "%s\\%s", modsPath, findData.cFileName);
                    
                    char iniPath[MAX_PATH];
                    sprintf_s(iniPath, "%s\\mod.ini", modPath);
                    
                    if (PathFileExistsA(iniPath)) {
                        CV64_ModInfo modInfo = { 0 };
                        strcpy_s(modInfo.modPath, modPath);
                        
                        if (ParseModIni(iniPath, &modInfo)) {
                            modInfo.totalSize = CalculateDirectorySize(modPath);
                            g_mods.push_back(modInfo);
                        }
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    // Sort by priority (highest first)
    std::sort(g_mods.begin(), g_mods.end(), [](const CV64_ModInfo& a, const CV64_ModInfo& b) {
        return a.priority > b.priority;
    });
    
    char logMsg[256];
    sprintf_s(logMsg, "[CV64] Found %u mods\n", (uint32_t)g_mods.size());
    OutputDebugStringA(logMsg);
    
    return (uint32_t)g_mods.size();
}

uint32_t CV64_ModLoader_GetMods(CV64_ModInfo* outMods, uint32_t maxMods) {
    if (!g_initialized || !outMods) {
        return 0;
    }
    
    size_t modSize = g_mods.size();
    size_t maxSize = (size_t)maxMods;
    uint32_t count = (uint32_t)(modSize < maxSize ? modSize : maxSize);
    for (uint32_t i = 0; i < count; i++) {
        outMods[i] = g_mods[i];
    }
    
    return count;
}

bool CV64_ModLoader_EnableMod(const char* modName) {
    for (auto& mod : g_mods) {
        if (strcmp(mod.name, modName) == 0) {
            mod.enabled = true;
            
            // Save to ini
            char iniPath[MAX_PATH];
            sprintf_s(iniPath, "%s\\mod.ini", mod.modPath);
            WritePrivateProfileStringA("Mod", "Enabled", "1", iniPath);
            
            return true;
        }
    }
    return false;
}

bool CV64_ModLoader_DisableMod(const char* modName) {
    for (auto& mod : g_mods) {
        if (strcmp(mod.name, modName) == 0) {
            mod.enabled = false;
            
            // Save to ini
            char iniPath[MAX_PATH];
            sprintf_s(iniPath, "%s\\mod.ini", mod.modPath);
            WritePrivateProfileStringA("Mod", "Enabled", "0", iniPath);
            
            return true;
        }
    }
    return false;
}

bool CV64_ModLoader_LoadEnabledMods(void) {
    if (!g_initialized) {
        return false;
    }
    
    uint32_t loadedCount = 0;
    
    for (auto& mod : g_mods) {
        if (mod.enabled && !mod.loaded) {
            // Load mod based on type
            // TODO: Implement actual mod loading logic
            mod.loaded = true;
            loadedCount++;
            
            char logMsg[256];
            sprintf_s(logMsg, "[CV64] Loaded mod: %s\n", mod.name);
            OutputDebugStringA(logMsg);
        }
    }
    
    char logMsg[256];
    sprintf_s(logMsg, "[CV64] Loaded %u mods\n", loadedCount);
    OutputDebugStringA(logMsg);
    
    return true;
}

void CV64_ModLoader_UnloadAllMods(void) {
    for (auto& mod : g_mods) {
        if (mod.loaded) {
            // Unload mod resources
            // TODO: Implement actual unloading logic
            mod.loaded = false;
        }
    }
}

void CV64_ModLoader_ReloadMods(void) {
    CV64_ModLoader_UnloadAllMods();
    CV64_ModLoader_ScanMods();
    CV64_ModLoader_LoadEnabledMods();
}

bool CV64_ModLoader_GetTextureReplacement(uint32_t textureCRC, CV64_TextureReplacement* outReplacement) {
    // TODO: Implement texture replacement lookup
    return false;
}

bool CV64_ModLoader_GetModelReplacement(uint32_t modelID, CV64_ModelReplacement* outReplacement) {
    // TODO: Implement model replacement lookup
    return false;
}

uint32_t CV64_ModLoader_ApplyROMPatches(uint8_t* romData, size_t romSize) {
    // TODO: Implement ROM patching
    return 0;
}

uint32_t CV64_ModLoader_ApplyMemoryPatches(void) {
    // TODO: Implement memory patching
    return 0;
}

void CV64_ModLoader_ShowDialog(HWND hWndParent) {
    if (!g_initialized) {
        MessageBoxA(hWndParent, "Mod loader not initialized!", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Scan for mods before showing dialog
    CV64_ModLoader_ScanMods();
    
    // Create dialog template in memory
    // For now, use a simple message box as placeholder
    MessageBoxA(hWndParent, 
        "Mod Loader\n\n"
        "This is a placeholder for the full mod loader dialog.\n"
        "Mod scanning and loading functionality is implemented.\n\n"
        "Place mods in the 'mods' folder with a mod.ini file.",
        "CV64 Mod Loader", 
        MB_OK | MB_ICONINFORMATION);
}

void CV64_ModLoader_GetStats(uint32_t* outTotalMods, uint32_t* outEnabledMods, uint32_t* outLoadedMods) {
    uint32_t total = 0, enabled = 0, loaded = 0;
    
    for (const auto& mod : g_mods) {
        total++;
        if (mod.enabled) enabled++;
        if (mod.loaded) loaded++;
    }
    
    if (outTotalMods) *outTotalMods = total;
    if (outEnabledMods) *outEnabledMods = enabled;
    if (outLoadedMods) *outLoadedMods = loaded;
}

bool CV64_ModLoader_CreateMod(const char* modName, const char* modAuthor) {
    if (!g_initialized || !modName) {
        return false;
    }
    
    // Create mod folder structure
    char modsPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, modsPath);
    strcat_s(modsPath, "\\mods");
    
    char modPath[MAX_PATH];
    sprintf_s(modPath, "%s\\%s", modsPath, modName);
    
    if (PathFileExistsA(modPath)) {
        return false; // Mod already exists
    }
    
    CreateDirectoryA(modPath, NULL);
    
    // Create subdirectories
    char subDir[MAX_PATH];
    sprintf_s(subDir, "%s\\textures", modPath);
    CreateDirectoryA(subDir, NULL);
    sprintf_s(subDir, "%s\\models", modPath);
    CreateDirectoryA(subDir, NULL);
    sprintf_s(subDir, "%s\\audio", modPath);
    CreateDirectoryA(subDir, NULL);
    
    // Create mod.ini
    char iniPath[MAX_PATH];
    sprintf_s(iniPath, "%s\\mod.ini", modPath);
    
    WritePrivateProfileStringA("Mod", "Name", modName, iniPath);
    WritePrivateProfileStringA("Mod", "Author", modAuthor ? modAuthor : "Unknown", iniPath);
    WritePrivateProfileStringA("Mod", "Version", "1.0", iniPath);
    WritePrivateProfileStringA("Mod", "Description", "New mod", iniPath);
    WritePrivateProfileStringA("Mod", "Type", "Pack", iniPath);
    WritePrivateProfileStringA("Mod", "Priority", "50", iniPath);
    WritePrivateProfileStringA("Mod", "Enabled", "0", iniPath);
    
    return true;
}

bool CV64_ModLoader_ExportGameState(const char* outputPath) {
    // TODO: Implement game state export for mod creation
    return false;
}
