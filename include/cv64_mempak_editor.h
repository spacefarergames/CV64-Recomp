/**
 * @file cv64_mempak_editor.h
 * @brief CV64 Memory Pak Editor - Edit save files stored in Controller Pak
 * 
 * This module provides a GUI for viewing and editing Castlevania 64 save data
 * stored in N64 Controller Pak files (mempak0.mpk). Based on the CV64 decomp
 * project's save file structure.
 * 
 * CV64 Save File Structure (from cv64 decomp):
 * - Each save slot is 512 bytes (0x200 bytes)
 * - Multiple save slots per memory pak
 * - Contains player stats, inventory, map progression, etc.
 */

#ifndef CV64_MEMPAK_EDITOR_H
#define CV64_MEMPAK_EDITOR_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CV64 Save File Structure (512 bytes per save slot)
 * 
 * Based on the CV64 decomp project's save file format.
 * Reference: https://github.com/blazkowolf/cv64
 */
#pragma pack(push, 1)
typedef struct {
    // Character/Game Progress (0x00-0x0F)
    uint8_t  character;           // 0x00: 0=Reinhardt, 1=Carrie
    uint8_t  subweapon;           // 0x01: Current sub-weapon
    uint16_t health;              // 0x02: Current health
    uint16_t maxHealth;           // 0x04: Maximum health
    uint16_t jewels;              // 0x06: Number of jewels collected
    uint8_t  mapID;               // 0x08: Current map ID
    uint8_t  spawnID;             // 0x09: Spawn point ID
    uint8_t  flags[6];            // 0x0A-0x0F: Various game flags
    
    // Inventory (0x10-0x3F)
    uint8_t  inventory[48];       // 0x10-0x3F: Item inventory slots
    
    // Map Progression (0x40-0x7F)
    uint8_t  mapFlags[64];        // 0x40-0x7F: Map completion flags
    
    // Game Stats (0x80-0xBF)
    uint32_t playTime;            // 0x80: Play time in frames
    uint16_t enemiesKilled;       // 0x84: Total enemies killed
    uint16_t damageReceived;      // 0x86: Total damage received
    uint8_t  difficulty;          // 0x88: Difficulty level
    uint8_t  reserved[55];        // 0x89-0xBF: Reserved/unused
    
    // Boss/Special Flags (0xC0-0xFF)
    uint8_t  bossFlags[64];       // 0xC0-0xFF: Boss defeated flags
    
    // Checksums and Metadata (0x100-0x1FF)
    uint32_t checksum;            // 0x100: Save file checksum
    uint8_t  saveName[16];        // 0x104: Save file name (player-set)
    uint8_t  padding[236];        // 0x114-0x1FF: Padding to 512 bytes
} CV64_SaveData;
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(CV64_SaveData) == 512, "CV64_SaveData must be 512 bytes");
#endif

/**
 * @brief Initialize the memory pak editor system
 * @return true if initialization succeeded
 */
bool CV64_MempakEditor_Init(void);

/**
 * @brief Shutdown the memory pak editor system
 */
void CV64_MempakEditor_Shutdown(void);

/**
 * @brief Show the memory pak editor dialog
 * @param hParent Parent window handle
 * @return true if dialog was closed with OK
 */
bool CV64_MempakEditor_ShowDialog(HWND hParent);

/**
 * @brief Load memory pak file from disk
 * @param filename Path to the .mpk file (default: "save/mempak0.mpk")
 * @return true if file was loaded successfully
 */
bool CV64_MempakEditor_LoadFile(const char* filename);

/**
 * @brief Save memory pak file to disk
 * @param filename Path to the .mpk file (default: "save/mempak0.mpk")
 * @return true if file was saved successfully
 */
bool CV64_MempakEditor_SaveFile(const char* filename);

/**
 * @brief Get save slot data from memory pak
 * @param slotIndex Save slot index (0-based)
 * @param outData Output buffer for save data (512 bytes)
 * @return true if slot data was read successfully
 */
bool CV64_MempakEditor_GetSlot(int slotIndex, CV64_SaveData* outData);

/**
 * @brief Write save slot data to memory pak
 * @param slotIndex Save slot index (0-based)
 * @param data Save data to write (512 bytes)
 * @return true if slot data was written successfully
 */
bool CV64_MempakEditor_SetSlot(int slotIndex, const CV64_SaveData* data);

/**
 * @brief Calculate checksum for save data
 * @param data Save data to checksum
 * @return Calculated checksum value
 */
uint32_t CV64_MempakEditor_CalculateChecksum(const CV64_SaveData* data);

/**
 * @brief Verify checksum of save data
 * @param data Save data to verify
 * @return true if checksum is valid
 */
bool CV64_MempakEditor_VerifyChecksum(const CV64_SaveData* data);

/**
 * @brief Set screenshot path for the current mempak
 * @param screenshotPath Path to the screenshot image (PNG format)
 */
void CV64_MempakEditor_SetScreenshot(const char* screenshotPath);

/**
 * @brief Get screenshot path for the current mempak
 * @return Path to screenshot, or NULL if none exists
 */
const char* CV64_MempakEditor_GetScreenshot(void);

/**
 * @brief Trigger screenshot capture after mempak save (called internally)
 */
void CV64_MempakEditor_OnMempakSave(void);

/**
 * @brief Process pending screenshot capture (call from main loop)
 */
void CV64_MempakEditor_ProcessPendingScreenshot(void);

#ifdef __cplusplus
}
#endif

#endif // CV64_MEMPAK_EDITOR_H
