/**
 * @file cv64_recomp.h
 * @brief Castlevania 64 PC Recomp - Main Recompilation Framework
 * 
 * This is the core header for the Castlevania 64 PC recompilation.
 * It provides the main interface between the recompiled game code
 * and the PC runtime environment.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_RECOMP_H
#define CV64_RECOMP_H

#include "cv64_types.h"
#include "cv64_controller.h"
#include "cv64_graphics.h"
#include "cv64_audio.h"
#include "cv64_camera_patch.h"
#include "cv64_patches.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Version Information
 *===========================================================================*/

#define CV64_RECOMP_VERSION_MAJOR   0
#define CV64_RECOMP_VERSION_MINOR   1
#define CV64_RECOMP_VERSION_PATCH   0
#define CV64_RECOMP_VERSION_STRING  "0.1.0-dev"

/*===========================================================================
 * ROM Information
 *===========================================================================*/

typedef enum CV64_RomRegion {
    CV64_REGION_UNKNOWN = 0,
    CV64_REGION_USA,        /**< NTSC-U: Castlevania */
    CV64_REGION_EUR,        /**< PAL: Castlevania */
    CV64_REGION_JPN,        /**< NTSC-J: Akumaj? Dracula Mokushiroku */
} CV64_RomRegion;

typedef struct CV64_RomInfo {
    char name[21];          /**< Internal ROM name */
    char id[5];             /**< Game ID (e.g., "NDCP") */
    CV64_RomRegion region;
    u32 crc1;
    u32 crc2;
    u32 size;
    bool is_valid;
} CV64_RomInfo;

/* Known ROM CRCs */
#define CV64_CRC_USA_1    0x00000000  /* TODO: Fill with actual CRC */
#define CV64_CRC_EUR_1    0x00000000
#define CV64_CRC_JPN_1    0x00000000

/*===========================================================================
 * Recomp State
 *===========================================================================*/

typedef enum CV64_RecompState {
    CV64_STATE_UNINITIALIZED = 0,
    CV64_STATE_INITIALIZED,
    CV64_STATE_ROM_LOADED,
    CV64_STATE_RUNNING,
    CV64_STATE_PAUSED,
    CV64_STATE_ERROR,
} CV64_RecompState;

/*===========================================================================
 * Main Recomp API
 *===========================================================================*/

/**
 * @brief Initialize the recomp framework
 * @param hwnd Window handle for graphics
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_Init(void* hwnd);

/**
 * @brief Shutdown the recomp framework
 */
CV64_API void CV64_Recomp_Shutdown(void);

/**
 * @brief Load ROM file
 * @param rom_path Path to ROM file
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_LoadROM(const char* rom_path);

/**
 * @brief Unload current ROM
 */
CV64_API void CV64_Recomp_UnloadROM(void);

/**
 * @brief Get loaded ROM info
 * @return Pointer to ROM info, or NULL if no ROM loaded
 */
CV64_API const CV64_RomInfo* CV64_Recomp_GetROMInfo(void);

/**
 * @brief Start emulation
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_Start(void);

/**
 * @brief Stop emulation
 */
CV64_API void CV64_Recomp_Stop(void);

/**
 * @brief Pause emulation
 */
CV64_API void CV64_Recomp_Pause(void);

/**
 * @brief Resume emulation
 */
CV64_API void CV64_Recomp_Resume(void);

/**
 * @brief Run one frame of emulation
 */
CV64_API void CV64_Recomp_RunFrame(void);

/**
 * @brief Get current recomp state
 * @return Current state
 */
CV64_API CV64_RecompState CV64_Recomp_GetState(void);

/**
 * @brief Check if emulation is running
 * @return TRUE if running
 */
CV64_API bool CV64_Recomp_IsRunning(void);

/*===========================================================================
 * Save State Functions
 *===========================================================================*/

/**
 * @brief Save state to file
 * @param slot Save slot (0-9)
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_SaveState(u32 slot);

/**
 * @brief Load state from file
 * @param slot Save slot (0-9)
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_LoadState(u32 slot);

/**
 * @brief Save state to custom path
 * @param filepath Save file path
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_SaveStateToFile(const char* filepath);

/**
 * @brief Load state from custom path
 * @param filepath Save file path
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_LoadStateFromFile(const char* filepath);

/*===========================================================================
 * Memory Access Functions
 *===========================================================================*/

/**
 * @brief Read 8-bit value from N64 memory
 * @param address N64 address
 * @return Value at address
 */
CV64_API u8 CV64_Recomp_ReadU8(u32 address);

/**
 * @brief Read 16-bit value from N64 memory
 * @param address N64 address
 * @return Value at address
 */
CV64_API u16 CV64_Recomp_ReadU16(u32 address);

/**
 * @brief Read 32-bit value from N64 memory
 * @param address N64 address
 * @return Value at address
 */
CV64_API u32 CV64_Recomp_ReadU32(u32 address);

/**
 * @brief Write 8-bit value to N64 memory
 * @param address N64 address
 * @param value Value to write
 */
CV64_API void CV64_Recomp_WriteU8(u32 address, u8 value);

/**
 * @brief Write 16-bit value to N64 memory
 * @param address N64 address
 * @param value Value to write
 */
CV64_API void CV64_Recomp_WriteU16(u32 address, u16 value);

/**
 * @brief Write 32-bit value to N64 memory
 * @param address N64 address
 * @param value Value to write
 */
CV64_API void CV64_Recomp_WriteU32(u32 address, u32 value);

/*===========================================================================
 * Configuration Functions
 *===========================================================================*/

/**
 * @brief Load all configuration files
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_LoadAllConfigs(void);

/**
 * @brief Save all configuration files
 * @return TRUE on success
 */
CV64_API bool CV64_Recomp_SaveAllConfigs(void);

/**
 * @brief Get configuration directory path
 * @return Path string
 */
CV64_API const char* CV64_Recomp_GetConfigPath(void);

/**
 * @brief Get save data directory path
 * @return Path string
 */
CV64_API const char* CV64_Recomp_GetSavePath(void);

/*===========================================================================
 * Version Info Functions
 *===========================================================================*/

/**
 * @brief Get version string
 * @return Version string
 */
CV64_API const char* CV64_Recomp_GetVersionString(void);

/**
 * @brief Get build date string
 * @return Build date
 */
CV64_API const char* CV64_Recomp_GetBuildDate(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_RECOMP_H */
