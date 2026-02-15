/**
 * @file cv64_mod_loader.h
 * @brief CV64 Mod Loading System - Load and manage game modifications
 * 
 * This module provides a comprehensive mod loading system for Castlevania 64.
 * Supports texture replacements, model swaps, audio replacements, gameplay patches,
 * and ROM patches. Mods are loaded from the "mods" folder and can be enabled/disabled.
 * 
 * Supported Mod Types:
 * - Texture Replacements (.png/.dds)
 * - 3D Model Replacements (.obj/.n64)
 * - Audio Replacements (.wav/.mp3/.ogg)
 * - Code Patches (memory patches)
 * - ROM Patches (.ips/.bps)
 * - Script Mods (Lua scripts for gameplay tweaks)
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_MOD_LOADER_H
#define CV64_MOD_LOADER_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum mod name length
#define CV64_MOD_MAX_NAME 128
#define CV64_MOD_MAX_AUTHOR 64
#define CV64_MOD_MAX_VERSION 32
#define CV64_MOD_MAX_DESCRIPTION 512
#define CV64_MOD_MAX_PATH 512

// Mod types
typedef enum {
    CV64_MOD_TYPE_TEXTURE = 0,        // Texture replacement
    CV64_MOD_TYPE_MODEL,              // 3D model replacement
    CV64_MOD_TYPE_AUDIO,              // Audio replacement
    CV64_MOD_TYPE_CODE_PATCH,         // Memory/code patch
    CV64_MOD_TYPE_ROM_PATCH,          // ROM file patch
    CV64_MOD_TYPE_SCRIPT,             // Lua script
    CV64_MOD_TYPE_PACK,               // Mod pack (contains multiple mods)
    CV64_MOD_TYPE_UNKNOWN
} CV64_ModType;

// Mod priority (for load order)
typedef enum {
    CV64_MOD_PRIORITY_LOWEST = 0,
    CV64_MOD_PRIORITY_LOW = 25,
    CV64_MOD_PRIORITY_NORMAL = 50,
    CV64_MOD_PRIORITY_HIGH = 75,
    CV64_MOD_PRIORITY_HIGHEST = 100
} CV64_ModPriority;

// Mod metadata (from mod.ini file)
typedef struct {
    char name[CV64_MOD_MAX_NAME];
    char author[CV64_MOD_MAX_AUTHOR];
    char version[CV64_MOD_MAX_VERSION];
    char description[CV64_MOD_MAX_DESCRIPTION];
    char modPath[CV64_MOD_MAX_PATH];
    
    CV64_ModType type;
    CV64_ModPriority priority;
    
    bool enabled;
    bool loaded;
    bool hasErrors;
    
    // Statistics
    uint32_t fileCount;
    uint64_t totalSize;
    
    // Dependencies
    char requiredMods[8][CV64_MOD_MAX_NAME];
    uint32_t requiredModCount;
    
    // Internal
    void* userData;  // Mod-specific data
} CV64_ModInfo;

// Texture replacement entry
typedef struct {
    uint32_t originalCRC;     // CRC32 of original N64 texture
    char replacementPath[CV64_MOD_MAX_PATH];
    uint32_t width;
    uint32_t height;
    bool loaded;
    void* glTextureHandle;    // OpenGL texture handle
} CV64_TextureReplacement;

// Model replacement entry
typedef struct {
    uint32_t modelID;         // Original N64 model ID
    char replacementPath[CV64_MOD_MAX_PATH];
    uint32_t vertexCount;
    uint32_t triangleCount;
    bool loaded;
    void* glBufferHandle;     // OpenGL VBO handle
} CV64_ModelReplacement;

/**
 * @brief Initialize the mod loading system
 * @return true if initialization succeeded
 */
bool CV64_ModLoader_Init(void);

/**
 * @brief Shutdown the mod loading system
 */
void CV64_ModLoader_Shutdown(void);

/**
 * @brief Scan the mods folder and load all available mods
 * @return Number of mods found
 */
uint32_t CV64_ModLoader_ScanMods(void);

/**
 * @brief Get the list of available mods
 * @param outMods Output array to receive mod info
 * @param maxMods Maximum number of mods to return
 * @return Number of mods returned
 */
uint32_t CV64_ModLoader_GetMods(CV64_ModInfo* outMods, uint32_t maxMods);

/**
 * @brief Enable a specific mod
 * @param modName Name of the mod to enable
 * @return true if mod was enabled successfully
 */
bool CV64_ModLoader_EnableMod(const char* modName);

/**
 * @brief Disable a specific mod
 * @param modName Name of the mod to disable
 * @return true if mod was disabled successfully
 */
bool CV64_ModLoader_DisableMod(const char* modName);

/**
 * @brief Load all enabled mods
 * @return true if all mods loaded successfully
 */
bool CV64_ModLoader_LoadEnabledMods(void);

/**
 * @brief Unload all mods
 */
void CV64_ModLoader_UnloadAllMods(void);

/**
 * @brief Reload all enabled mods (useful after changes)
 */
void CV64_ModLoader_ReloadMods(void);

/**
 * @brief Check if a texture should be replaced
 * @param textureCRC CRC32 of the original texture
 * @param outReplacement Output replacement info if found
 * @return true if replacement exists
 */
bool CV64_ModLoader_GetTextureReplacement(uint32_t textureCRC, CV64_TextureReplacement* outReplacement);

/**
 * @brief Check if a model should be replaced
 * @param modelID Original N64 model ID
 * @param outReplacement Output replacement info if found
 * @return true if replacement exists
 */
bool CV64_ModLoader_GetModelReplacement(uint32_t modelID, CV64_ModelReplacement* outReplacement);

/**
 * @brief Apply ROM patches from loaded mods
 * @param romData ROM data buffer
 * @param romSize Size of ROM data
 * @return Number of patches applied
 */
uint32_t CV64_ModLoader_ApplyROMPatches(uint8_t* romData, size_t romSize);

/**
 * @brief Apply memory patches from loaded mods
 * @return Number of patches applied
 */
uint32_t CV64_ModLoader_ApplyMemoryPatches(void);

/**
 * @brief Show the mod loader dialog
 * @param hWndParent Parent window handle
 */
void CV64_ModLoader_ShowDialog(HWND hWndParent);

/**
 * @brief Get mod loader statistics
 * @param outTotalMods Output total mod count
 * @param outEnabledMods Output enabled mod count
 * @param outLoadedMods Output successfully loaded mod count
 */
void CV64_ModLoader_GetStats(uint32_t* outTotalMods, uint32_t* outEnabledMods, uint32_t* outLoadedMods);

/**
 * @brief Create a new mod folder structure
 * @param modName Name of the new mod
 * @param modAuthor Author name
 * @return true if mod folder was created successfully
 */
bool CV64_ModLoader_CreateMod(const char* modName, const char* modAuthor);

/**
 * @brief Export current game state for mod creation
 * @param outputPath Path to export to
 * @return true if export succeeded
 */
bool CV64_ModLoader_ExportGameState(const char* outputPath);

#ifdef __cplusplus
}
#endif

#endif // CV64_MOD_LOADER_H
