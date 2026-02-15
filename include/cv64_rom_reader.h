/**
 * @file cv64_rom_reader.h
 * @brief CV64 ROM Reader - Read and parse data from ROM file
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_ROM_READER_H
#define CV64_ROM_READER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ROM file handle
typedef struct CV64_ROMFile_t* CV64_ROMFile;

/**
 * @brief Open a ROM file for reading
 * @param romPath Path to the ROM file
 * @return ROM file handle or NULL on failure
 */
CV64_ROMFile CV64_ROM_Open(const char* romPath);

/**
 * @brief Close a ROM file
 * @param rom ROM file handle
 */
void CV64_ROM_Close(CV64_ROMFile rom);

/**
 * @brief Read data from ROM at specified offset
 * @param rom ROM file handle
 * @param offset Offset in ROM
 * @param buffer Output buffer
 * @param size Number of bytes to read
 * @return Number of bytes read
 */
size_t CV64_ROM_Read(CV64_ROMFile rom, uint32_t offset, void* buffer, size_t size);

/**
 * @brief Get ROM file size
 * @param rom ROM file handle
 * @return ROM size in bytes
 */
size_t CV64_ROM_GetSize(CV64_ROMFile rom);

/**
 * @brief Check if ROM file is valid
 * @param rom ROM file handle
 * @return true if valid
 */
bool CV64_ROM_IsValid(CV64_ROMFile rom);

#ifdef __cplusplus
}
#endif

#endif // CV64_ROM_READER_H
