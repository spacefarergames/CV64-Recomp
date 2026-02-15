/**
 * @file cv64_embedded_rom.h
 * @brief Embedded ROM access interface
 * 
 * Provides access to the ROM embedded as a Windows resource.
 */

#ifndef CV64_EMBEDDED_ROM_H
#define CV64_EMBEDDED_ROM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get pointer to embedded ROM data
 * @param[out] size Receives the size of the ROM in bytes
 * @return Pointer to ROM data, or NULL if not found
 */
const uint8_t* CV64_GetEmbeddedRom(size_t* size);

/**
 * @brief Check if embedded ROM is available
 * @return 1 if embedded ROM exists, 0 otherwise
 */
int CV64_HasEmbeddedRom(void);

/**
 * @brief Load the embedded ROM into mupen64plus core
 * @return M64ERR_SUCCESS on success, error code otherwise
 */
int CV64_LoadEmbeddedRom(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_EMBEDDED_ROM_H */
