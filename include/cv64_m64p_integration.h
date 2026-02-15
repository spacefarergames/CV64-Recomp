/**
 * @file cv64_m64p_integration.h
 * @brief Castlevania 64 PC Recomp - mupen64plus Integration Layer
 * 
 * This module integrates the official mupen64plus emulation core with our
 * Castlevania 64 specific patches and enhancements.
 * 
 * mupen64plus Source: https://github.com/mupen64plus/mupen64plus-core
 * 
 * Required plugins (from official mupen64plus repositories):
 *   - mupen64plus-core: https://github.com/mupen64plus/mupen64plus-core
 *   - mupen64plus-video-GLideN64: https://github.com/gonetz/GLideN64
 *   - mupen64plus-audio-sdl: https://github.com/mupen64plus/mupen64plus-audio-sdl
 *   - mupen64plus-input-sdl: https://github.com/mupen64plus/mupen64plus-input-sdl
 *   - mupen64plus-rsp-hle: https://github.com/mupen64plus/mupen64plus-rsp-hle
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_M64P_INTEGRATION_H
#define CV64_M64P_INTEGRATION_H

#include "cv64_types.h"
#include <filesystem>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * mupen64plus Types (from m64p_types.h)
 *===========================================================================*/

typedef void* m64p_dynlib_handle;

typedef enum {
    M64ERR_SUCCESS = 0,
    M64ERR_NOT_INIT,
    M64ERR_ALREADY_INIT,
    M64ERR_INCOMPATIBLE,
    M64ERR_INPUT_ASSERT,
    M64ERR_INPUT_INVALID,
    M64ERR_INPUT_NOT_FOUND,
    M64ERR_NO_MEMORY,
    M64ERR_FILES,
    M64ERR_INTERNAL,
    M64ERR_INVALID_STATE,
    M64ERR_PLUGIN_FAIL,
    M64ERR_SYSTEM_FAIL,
    M64ERR_UNSUPPORTED,
    M64ERR_WRONG_TYPE
} m64p_error;

typedef enum {
    M64EMU_STOPPED = 1,
    M64EMU_RUNNING,
    M64EMU_PAUSED
} m64p_emu_state;

typedef enum {
    M64PLUGIN_NULL = 0,
    M64PLUGIN_RSP = 1,
    M64PLUGIN_GFX,
    M64PLUGIN_AUDIO,
    M64PLUGIN_INPUT,
    M64PLUGIN_CORE
} m64p_plugin_type;

typedef enum {
    M64CMD_NOP = 0,
    M64CMD_ROM_OPEN,
    M64CMD_ROM_CLOSE,
    M64CMD_ROM_GET_HEADER,
    M64CMD_ROM_GET_SETTINGS,
    M64CMD_EXECUTE,
    M64CMD_STOP,
    M64CMD_PAUSE,
    M64CMD_RESUME,
    M64CMD_CORE_STATE_QUERY,
    M64CMD_STATE_LOAD,
    M64CMD_STATE_SAVE,
    M64CMD_STATE_SET_SLOT,
    M64CMD_RESET,
    M64CMD_ADVANCE_FRAME
} m64p_command;

typedef enum {
    M64CORE_EMU_STATE = 1,
    M64CORE_VIDEO_MODE,
    M64CORE_SAVESTATE_SLOT,
    M64CORE_SPEED_FACTOR,
    M64CORE_SPEED_LIMITER
} m64p_core_param;

/*===========================================================================
 * CV64 Integration State
 *===========================================================================*/

typedef enum CV64_IntegrationState {
    CV64_INTEGRATION_UNINITIALIZED = 0,
    CV64_INTEGRATION_CORE_LOADED,
    CV64_INTEGRATION_PLUGINS_LOADED,
    CV64_INTEGRATION_ROM_LOADED,
    CV64_INTEGRATION_RUNNING,
    CV64_INTEGRATION_PAUSED,
    CV64_INTEGRATION_ERROR
} CV64_IntegrationState;

/*===========================================================================
 * ROM Header Structure
 *===========================================================================*/

typedef struct CV64_RomHeader {
    u8  init_PI_BSB_DOM1_LAT_REG;
    u8  init_PI_BSB_DOM1_PGS_REG;
    u8  init_PI_BSB_DOM1_PWD_REG;
    u8  init_PI_BSB_DOM1_PGS_REG2;
    u32 clock_rate;
    u32 program_counter;
    u32 release;
    u32 crc1;
    u32 crc2;
    u8  reserved1[8];
    char name[20];
    u8  reserved2[7];
    u8  manufacturer_id;
    u16 cartridge_id;
    u8  country_code;
    u8  reserved3;
} CV64_RomHeader;

/*===========================================================================
 * Memory Access Addresses (Castlevania 64 Specific)
 *===========================================================================*/

/* These are N64 RAM addresses for CV64 game data */
#define CV64_ADDR_RDRAM_BASE        0x80000000
#define CV64_ADDR_RDRAM_SIZE        0x00400000  /* 4MB */
#define CV64_ADDR_RDRAM_END         0x803FFFFF

/* Game-specific addresses (to be filled from decomp research) */
#define CV64_ADDR_CONTROLLER_DATA   0x80000000  /* Placeholder */
#define CV64_ADDR_CAMERA_MGR        0x80000000  /* Placeholder */
#define CV64_ADDR_PLAYER_DATA       0x80000000  /* Placeholder */
#define CV64_ADDR_GAME_STATE        0x80000000  /* Placeholder */

/*===========================================================================
 * Core Integration API
 *===========================================================================*/

/**
 * @brief Initialize mupen64plus integration
 * @param hwnd Window handle for video output
 * @return true on success
 */
CV64_API bool CV64_M64P_Init(void* hwnd);

/**
 * @brief Shutdown mupen64plus integration
 */
CV64_API void CV64_M64P_Shutdown(void);

/**
 * @brief Load plugins manually (optional - Init does this automatically)
 * @return true on success
 */
CV64_API bool CV64_M64P_LoadPlugins(void);

/**
 * @brief List all found plugins (for debugging)
 */
CV64_API void CV64_M64P_ListPlugins(void);

/**
 * @brief Get current integration state
 */
CV64_API CV64_IntegrationState CV64_M64P_GetState(void);

/**
 * @brief Get last error message
 */
CV64_API const char* CV64_M64P_GetLastError(void);

/*===========================================================================
 * ROM Loading API
 *===========================================================================*/

/**
 * @brief Load the embedded ROM (preferred - no external files needed)
 * @return true on success, false if no embedded ROM or load failed
 */
CV64_API bool CV64_M64P_LoadEmbeddedROM(void);

/**
 * @brief Load a ROM file
 * @param rom_path Path to ROM file
 * @return true on success
 */
CV64_API bool CV64_M64P_LoadROM(const char* rom_path);

/**
 * @brief Close current ROM
 */
CV64_API void CV64_M64P_CloseROM(void);

/**
 * @brief Get ROM header info
 * @param header Output header structure
 * @return true on success
 */
CV64_API bool CV64_M64P_GetROMHeader(CV64_RomHeader* header);

/**
 * @brief Check if ROM is Castlevania 64
 * @return true if CV64 ROM
 */
CV64_API bool CV64_M64P_IsCV64ROM(void);

/*===========================================================================
 * Emulation Control API
 *===========================================================================*/

/**
 * @brief Start emulation
 * @return true on success
 */
CV64_API bool CV64_M64P_Start(void);

/**
 * @brief Stop emulation
 */
CV64_API void CV64_M64P_Stop(void);

/**
 * @brief Pause emulation
 */
CV64_API void CV64_M64P_Pause(void);

/**
 * @brief Resume emulation
 */
CV64_API void CV64_M64P_Resume(void);

/**
 * @brief Reset emulation (soft or hard)
 * @param hard true for hard reset
 */
CV64_API void CV64_M64P_Reset(bool hard);

/**
 * @brief Advance one frame (for frame stepping)
 */
CV64_API void CV64_M64P_AdvanceFrame(void);

/**
 * @brief Check if emulation is running
 */
CV64_API bool CV64_M64P_IsRunning(void);

/**
 * @brief Check if emulation is paused
 */
CV64_API bool CV64_M64P_IsPaused(void);

/*===========================================================================
 * Memory Access API
 *===========================================================================*/

/**
 * @brief Read byte from N64 memory
 * @param address N64 address (0x80000000 - 0x803FFFFF)
 * @return Value at address
 */
CV64_API u8 CV64_M64P_ReadMemory8(u32 address);

/**
 * @brief Read 16-bit value from N64 memory
 */
CV64_API u16 CV64_M64P_ReadMemory16(u32 address);

/**
 * @brief Read 32-bit value from N64 memory
 */
CV64_API u32 CV64_M64P_ReadMemory32(u32 address);

/**
 * @brief Write byte to N64 memory
 */
CV64_API void CV64_M64P_WriteMemory8(u32 address, u8 value);

/**
 * @brief Write 16-bit value to N64 memory
 */
CV64_API void CV64_M64P_WriteMemory16(u32 address, u16 value);

/**
 * @brief Write 32-bit value to N64 memory
 */
CV64_API void CV64_M64P_WriteMemory32(u32 address, u32 value);

/**
 * @brief Get direct pointer to N64 RDRAM
 * @return Pointer to RDRAM base, or NULL if not available
 */
CV64_API void* CV64_M64P_GetRDRAMPointer(void);

/*===========================================================================
 * Save State API
 *===========================================================================*/

/**
 * @brief Save state to slot
 * @param slot Slot number (0-9)
 * @return true on success
 */
CV64_API bool CV64_M64P_SaveState(int slot);

/**
 * @brief Load state from slot
 * @param slot Slot number (0-9)
 * @return true on success
 */
CV64_API bool CV64_M64P_LoadState(int slot);

/**
 * @brief Set current save slot
 * @param slot Slot number (0-9)
 */
CV64_API void CV64_M64P_SetSaveSlot(int slot);

/*===========================================================================
 * Speed Control API
 *===========================================================================*/

/**
 * @brief Set speed factor
 * @param factor Speed factor (100 = normal)
 */
CV64_API void CV64_M64P_SetSpeedFactor(int factor);

/**
 * @brief Get current speed factor
 */
CV64_API int CV64_M64P_GetSpeedFactor(void);

/**
 * @brief Enable/disable speed limiter
 */
CV64_API void CV64_M64P_SetSpeedLimiter(bool enabled);

/*===========================================================================
 * Frame Callback API (for our patches)
 *===========================================================================*/

/**
 * @brief Frame callback function type
 * Called every frame after emulation but before rendering
 */
typedef void (*CV64_FrameCallback)(void* context);

/**
 * @brief Set frame callback for our patches
 * @param callback Callback function
 * @param context User context data
 */
CV64_API void CV64_M64P_SetFrameCallback(CV64_FrameCallback callback, void* context);

/*===========================================================================
 * Input Override API (for our controller patch)
 *===========================================================================*/

/**
 * @brief Controller state structure matching N64 input
 */
typedef struct CV64_M64P_ControllerState {
    s8 x_axis;
    s8 y_axis;
    u16 buttons;
} CV64_M64P_ControllerState;

/**
 * @brief Set controller input override
 * @param controller Controller index (0-3)
 * @param state Controller state
 */
CV64_API void CV64_M64P_SetControllerOverride(int controller, const CV64_M64P_ControllerState* state);

/**
 * @brief Clear controller input override
 * @param controller Controller index (0-3)
 */
CV64_API void CV64_M64P_ClearControllerOverride(int controller);

#ifdef __cplusplus
}
#endif

#endif /* CV64_M64P_INTEGRATION_H */
