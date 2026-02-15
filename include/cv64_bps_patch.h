/**
 * @file cv64_bps_patch.h
 * @brief Castlevania 64 PC Recomp - BPS Patch System
 * 
 * Automatically applies BPS patches from the patches folder to the ROM.
 * Supports standard BPS format patches.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_BPS_PATCH_H
#define CV64_BPS_PATCH_H

#include "cv64_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BPS patch result codes
 */
typedef enum CV64_BPS_Result {
    CV64_BPS_SUCCESS = 0,           /**< Patch applied successfully */
    CV64_BPS_ERROR_FILE_NOT_FOUND,  /**< Patch file not found */
    CV64_BPS_ERROR_INVALID_HEADER,  /**< Invalid BPS header */
    CV64_BPS_ERROR_SOURCE_MISMATCH, /**< Source CRC doesn't match */
    CV64_BPS_ERROR_OUTPUT_MISMATCH, /**< Output CRC doesn't match after patching */
    CV64_BPS_ERROR_MEMORY,          /**< Memory allocation failed */
    CV64_BPS_ERROR_INVALID_DATA,    /**< Invalid patch data */
    CV64_BPS_ERROR_SIZE_MISMATCH,   /**< Size mismatch */
    CV64_BPS_NO_PATCHES_FOUND       /**< No patches found in folder */
} CV64_BPS_Result;

/**
 * @brief BPS patch info structure
 */
typedef struct CV64_BPS_PatchInfo {
    char filename[260];             /**< Patch filename */
    u32 sourceSize;                 /**< Expected source file size */
    u32 targetSize;                 /**< Output file size after patching */
    u32 sourceCRC;                  /**< Expected source CRC32 */
    u32 targetCRC;                  /**< Expected target CRC32 */
    u32 patchCRC;                   /**< Patch file CRC32 */
} CV64_BPS_PatchInfo;

/**
 * @brief Apply all BPS patches from the patches folder to the ROM
 * 
 * @param romData Pointer to ROM data (will be modified in place)
 * @param romSize Pointer to ROM size (may be updated if patch changes size)
 * @param maxSize Maximum buffer size for output
 * @return CV64_BPS_Result result code
 */
CV64_API CV64_BPS_Result CV64_BPS_ApplyPatches(u8* romData, size_t* romSize, size_t maxSize);

/**
 * @brief Apply a single BPS patch to data
 * 
 * @param patchPath Path to the BPS patch file
 * @param sourceData Input data to patch
 * @param sourceSize Size of input data
 * @param outputData Output buffer for patched data
 * @param outputSize Pointer to output size (set to buffer size, updated with actual)
 * @param info Optional output for patch info
 * @return CV64_BPS_Result result code
 */
CV64_API CV64_BPS_Result CV64_BPS_ApplyPatch(
    const char* patchPath,
    const u8* sourceData,
    size_t sourceSize,
    u8* outputData,
    size_t* outputSize,
    CV64_BPS_PatchInfo* info
);

/**
 * @brief Get info about a BPS patch without applying it
 * 
 * @param patchPath Path to the BPS patch file
 * @param info Output for patch info
 * @return CV64_BPS_Result result code
 */
CV64_API CV64_BPS_Result CV64_BPS_GetPatchInfo(const char* patchPath, CV64_BPS_PatchInfo* info);

/**
 * @brief Get human-readable error message for result code
 * 
 * @param result BPS result code
 * @return const char* Error message string
 */
CV64_API const char* CV64_BPS_GetErrorMessage(CV64_BPS_Result result);

#ifdef __cplusplus
}
#endif

#endif /* CV64_BPS_PATCH_H */
