/**
 * @file cv64_savestate_manager.h
 * @brief CV64 Save State Manager - Enhanced save state system with thumbnails and metadata
 * 
 * This module provides an advanced save state management system with:
 * - Unlimited named save states
 * - Screenshot thumbnails
 * - Metadata (timestamp, map name, character, stats)
 * - Browser UI with visual previews
 * - Quick save/load functionality (F5/F9)
 * - Export/import save states
 */

#ifndef CV64_SAVESTATE_MANAGER_H
#define CV64_SAVESTATE_MANAGER_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save state metadata structure
 */
typedef struct {
    char name[64];              // User-defined name
    char filename[MAX_PATH];    // .st file path
    char thumbnailPath[MAX_PATH]; // .png thumbnail path
    time_t timestamp;           // Save timestamp
    
    // Game state info
    char mapName[64];           // Current map/area
    char character[32];         // Reinhardt or Carrie
    uint16_t health;            // Current HP
    uint16_t maxHealth;         // Max HP
    uint16_t jewels;            // Jewel count
    uint32_t playTime;          // Play time in frames
    int16_t mapID;              // Map ID (s16 in decomp)
    
    // UI state
    bool selected;              // Currently selected in UI
    bool hasScreenshot;         // Has valid thumbnail
} CV64_SaveState;

/**
 * @brief Save state manager statistics
 */
typedef struct {
    int totalStates;            // Total save states
    int quickSaveSlot;          // Current quick save slot (F5)
    uint64_t totalSaves;        // Lifetime saves
    uint64_t totalLoads;        // Lifetime loads
    size_t diskUsageBytes;      // Total disk space used
} CV64_SaveStateStats;

/**
 * @brief Initialize the save state manager
 * @return true if initialization succeeded
 */
bool CV64_SaveState_Init(void);

/**
 * @brief Shutdown the save state manager
 */
void CV64_SaveState_Shutdown(void);

/**
 * @brief Show the save state manager dialog
 * @param hParent Parent window handle
 * @return true if dialog was closed with OK
 */
bool CV64_SaveState_ShowDialog(HWND hParent);

/**
 * @brief Quick save to slot (F5 functionality)
 * @param slotIndex Slot index (0-9 for quick slots)
 * @return true if save succeeded
 */
bool CV64_SaveState_QuickSave(int slotIndex);

/**
 * @brief Quick load from slot (F9 functionality)
 * @param slotIndex Slot index (0-9 for quick slots)
 * @return true if load succeeded
 */
bool CV64_SaveState_QuickLoad(int slotIndex);

/**
 * @brief Save state with custom name
 * @param name User-defined name for the save state
 * @return true if save succeeded
 */
bool CV64_SaveState_SaveNamed(const char* name);

/**
 * @brief Load a specific save state by filename
 * @param filename Save state filename (relative to save/states/)
 * @return true if load succeeded
 */
bool CV64_SaveState_Load(const char* filename);

/**
 * @brief Delete a save state
 * @param filename Save state filename
 * @return true if deletion succeeded
 */
bool CV64_SaveState_Delete(const char* filename);

/**
 * @brief Rename a save state
 * @param filename Save state filename
 * @param newName New name for the save state
 * @return true if rename succeeded
 */
bool CV64_SaveState_Rename(const char* filename, const char* newName);

/**
 * @brief Export save state to external file
 * @param filename Internal save state filename
 * @param exportPath Full path to export location
 * @return true if export succeeded
 */
bool CV64_SaveState_Export(const char* filename, const char* exportPath);

/**
 * @brief Import save state from external file
 * @param importPath Full path to import file
 * @param name Name for the imported save state
 * @return true if import succeeded
 */
bool CV64_SaveState_Import(const char* importPath, const char* name);

/**
 * @brief Get list of all save states
 * @param outStates Array to store save states (allocated by caller)
 * @param maxStates Maximum number of states to return
 * @param outCount Number of states actually returned
 * @return true if successful
 */
bool CV64_SaveState_GetList(CV64_SaveState* outStates, int maxStates, int* outCount);

/**
 * @brief Get save state metadata
 * @param filename Save state filename
 * @param outState Output structure for metadata
 * @return true if metadata was retrieved
 */
bool CV64_SaveState_GetMetadata(const char* filename, CV64_SaveState* outState);

/**
 * @brief Capture screenshot for save state thumbnail
 * @param outPath Output path for PNG thumbnail
 * @param width Thumbnail width (default 320)
 * @param height Thumbnail height (default 180)
 * @return true if capture succeeded
 */
bool CV64_SaveState_CaptureScreenshot(const char* outPath, int width, int height);

/**
 * @brief Get manager statistics
 * @param outStats Output structure for statistics
 */
void CV64_SaveState_GetStats(CV64_SaveStateStats* outStats);

/**
 * @brief Check if a save state exists
 * @param filename Save state filename
 * @return true if save state exists
 */
bool CV64_SaveState_Exists(const char* filename);

/**
 * @brief Get total disk usage by save states
 * @return Total bytes used by save states and thumbnails
 */
size_t CV64_SaveState_GetDiskUsage(void);

/**
 * @brief Clean up old auto-saves (keep only last N)
 * @param keepCount Number of auto-saves to keep
 * @return Number of auto-saves deleted
 */
int CV64_SaveState_CleanupAutoSaves(int keepCount);

#ifdef __cplusplus
}
#endif

#endif // CV64_SAVESTATE_MANAGER_H
