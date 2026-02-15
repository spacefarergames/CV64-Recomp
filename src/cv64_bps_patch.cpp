/**
 * @file cv64_bps_patch.cpp
 * @brief Castlevania 64 PC Recomp - BPS Patch Implementation
 * 
 * BPS (Beat Patch System) is a binary patch format commonly used for
 * ROM patches. This implementation supports automatic application of
 * any .bps files found in the patches folder.
 * 
 * BPS Format:
 * - Header: "BPS1" (4 bytes)
 * - Source size (variable-length encoded)
 * - Target size (variable-length encoded)
 * - Metadata size (variable-length encoded)
 * - Metadata (if any)
 * - Patch data (action bytes with lengths)
 * - Footer: source CRC32 (4), target CRC32 (4), patch CRC32 (4)
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_bps_patch.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <cstdio>

/*===========================================================================
 * Constants
 *===========================================================================*/

static const char BPS_MAGIC[] = "BPS1";
static const size_t BPS_MAGIC_SIZE = 4;
static const size_t BPS_FOOTER_SIZE = 12; /* 3 x 4-byte CRCs */

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void BpsLog(const char* msg) {
    OutputDebugStringA("[CV64_BPS] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

/**
 * @brief Calculate CRC32 (standard polynomial)
 */
static u32 BpsCalcCRC32(const u8* data, size_t size) {
    static u32 crcTable[256] = {0};
    static bool tableInit = false;
    
    if (!tableInit) {
        for (u32 i = 0; i < 256; i++) {
            u32 crc = i;
            for (int j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
            }
            crcTable[i] = crc;
        }
        tableInit = true;
    }
    
    u32 crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crcTable[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Read variable-length encoded integer from BPS data
 */
static u64 BpsReadVarInt(const u8*& ptr, const u8* end) {
    u64 result = 0;
    u64 shift = 1;
    
    while (ptr < end) {
        u8 byte = *ptr++;
        result += (byte & 0x7F) * shift;
        if (byte & 0x80) {
            return result;
        }
        shift <<= 7;
        result += shift;
    }
    
    return result;
}

/**
 * @brief Read signed offset from BPS data
 */
static s64 BpsReadSignedOffset(const u8*& ptr, const u8* end) {
    u64 value = BpsReadVarInt(ptr, end);
    s64 offset = (s64)(value >> 1);
    if (value & 1) {
        offset = -offset;
    }
    return offset;
}

/*===========================================================================
 * BPS Patch Application
 *===========================================================================*/

CV64_BPS_Result CV64_BPS_GetPatchInfo(const char* patchPath, CV64_BPS_PatchInfo* info) {
    if (!patchPath || !info) {
        return CV64_BPS_ERROR_INVALID_DATA;
    }
    
    memset(info, 0, sizeof(CV64_BPS_PatchInfo));
    strncpy(info->filename, patchPath, sizeof(info->filename) - 1);
    
    /* Read patch file */
    FILE* patchFile = fopen(patchPath, "rb");
    if (!patchFile) {
        return CV64_BPS_ERROR_FILE_NOT_FOUND;
    }
    
    fseek(patchFile, 0, SEEK_END);
    size_t patchSize = ftell(patchFile);
    fseek(patchFile, 0, SEEK_SET);
    
    if (patchSize < BPS_MAGIC_SIZE + BPS_FOOTER_SIZE) {
        fclose(patchFile);
        return CV64_BPS_ERROR_INVALID_HEADER;
    }
    
    std::vector<u8> patchData(patchSize);
    fread(patchData.data(), 1, patchSize, patchFile);
    fclose(patchFile);
    
    /* Verify magic */
    if (memcmp(patchData.data(), BPS_MAGIC, BPS_MAGIC_SIZE) != 0) {
        return CV64_BPS_ERROR_INVALID_HEADER;
    }
    
    /* Read header */
    const u8* ptr = patchData.data() + BPS_MAGIC_SIZE;
    const u8* end = patchData.data() + patchSize - BPS_FOOTER_SIZE;
    
    info->sourceSize = (u32)BpsReadVarInt(ptr, end);
    info->targetSize = (u32)BpsReadVarInt(ptr, end);
    
    /* Read CRCs from footer */
    const u8* footer = patchData.data() + patchSize - BPS_FOOTER_SIZE;
    info->sourceCRC = footer[0] | (footer[1] << 8) | (footer[2] << 16) | (footer[3] << 24);
    info->targetCRC = footer[4] | (footer[5] << 8) | (footer[6] << 16) | (footer[7] << 24);
    info->patchCRC = footer[8] | (footer[9] << 8) | (footer[10] << 16) | (footer[11] << 24);
    
    /* Verify patch CRC */
    u32 calcPatchCRC = BpsCalcCRC32(patchData.data(), patchSize - 4);
    if (calcPatchCRC != info->patchCRC) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Patch CRC mismatch: expected %08X, got %08X", info->patchCRC, calcPatchCRC);
        BpsLog(msg);
        return CV64_BPS_ERROR_INVALID_DATA;
    }
    
    return CV64_BPS_SUCCESS;
}

CV64_BPS_Result CV64_BPS_ApplyPatch(
    const char* patchPath,
    const u8* sourceData,
    size_t sourceSize,
    u8* outputData,
    size_t* outputSize,
    CV64_BPS_PatchInfo* info
) {
    if (!patchPath || !sourceData || !outputData || !outputSize) {
        return CV64_BPS_ERROR_INVALID_DATA;
    }
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Applying BPS patch: %s", patchPath);
    BpsLog(msg);
    
    /* Read patch file */
    FILE* patchFile = fopen(patchPath, "rb");
    if (!patchFile) {
        BpsLog("Patch file not found");
        return CV64_BPS_ERROR_FILE_NOT_FOUND;
    }
    
    fseek(patchFile, 0, SEEK_END);
    size_t patchSize = ftell(patchFile);
    fseek(patchFile, 0, SEEK_SET);
    
    if (patchSize < BPS_MAGIC_SIZE + BPS_FOOTER_SIZE) {
        fclose(patchFile);
        BpsLog("Patch file too small");
        return CV64_BPS_ERROR_INVALID_HEADER;
    }
    
    std::vector<u8> patchData(patchSize);
    fread(patchData.data(), 1, patchSize, patchFile);
    fclose(patchFile);
    
    /* Verify magic */
    if (memcmp(patchData.data(), BPS_MAGIC, BPS_MAGIC_SIZE) != 0) {
        BpsLog("Invalid BPS magic");
        return CV64_BPS_ERROR_INVALID_HEADER;
    }
    
    /* Verify patch CRC */
    const u8* footer = patchData.data() + patchSize - BPS_FOOTER_SIZE;
    u32 expectedPatchCRC = footer[8] | (footer[9] << 8) | (footer[10] << 16) | (footer[11] << 24);
    u32 calcPatchCRC = BpsCalcCRC32(patchData.data(), patchSize - 4);
    if (calcPatchCRC != expectedPatchCRC) {
        snprintf(msg, sizeof(msg), "Patch CRC mismatch: expected %08X, got %08X", expectedPatchCRC, calcPatchCRC);
        BpsLog(msg);
        return CV64_BPS_ERROR_INVALID_DATA;
    }
    
    /* Read header */
    const u8* ptr = patchData.data() + BPS_MAGIC_SIZE;
    const u8* end = patchData.data() + patchSize - BPS_FOOTER_SIZE;
    
    u64 expectedSourceSize = BpsReadVarInt(ptr, end);
    u64 expectedTargetSize = BpsReadVarInt(ptr, end);
    u64 metadataSize = BpsReadVarInt(ptr, end);
    
    snprintf(msg, sizeof(msg), "BPS: source=%llu, target=%llu, metadata=%llu", 
             expectedSourceSize, expectedTargetSize, metadataSize);
    BpsLog(msg);
    
    /* Skip metadata */
    ptr += metadataSize;
    
    /* Verify source size */
    if (sourceSize != expectedSourceSize) {
        snprintf(msg, sizeof(msg), "Source size mismatch: expected %llu, got %zu", expectedSourceSize, sourceSize);
        BpsLog(msg);
        /* Allow size mismatch - some patches work with slightly different ROMs */
        /* return CV64_BPS_ERROR_SIZE_MISMATCH; */
    }
    
    /* Verify source CRC */
    u32 expectedSourceCRC = footer[0] | (footer[1] << 8) | (footer[2] << 16) | (footer[3] << 24);
    u32 calcSourceCRC = BpsCalcCRC32(sourceData, sourceSize);
    if (calcSourceCRC != expectedSourceCRC) {
        snprintf(msg, sizeof(msg), "Source CRC mismatch: expected %08X, got %08X", expectedSourceCRC, calcSourceCRC);
        BpsLog(msg);
        /* Continue anyway - some patches work with different ROM versions */
    }
    
    /* Check output buffer size */
    if (*outputSize < expectedTargetSize) {
        snprintf(msg, sizeof(msg), "Output buffer too small: need %llu, have %zu", expectedTargetSize, *outputSize);
        BpsLog(msg);
        return CV64_BPS_ERROR_MEMORY;
    }
    
    /* Apply patch */
    size_t outputOffset = 0;
    size_t sourceRelOffset = 0;
    size_t targetRelOffset = 0;
    
    while (ptr < end && outputOffset < expectedTargetSize) {
        u64 data = BpsReadVarInt(ptr, end);
        u64 command = data & 3;
        u64 length = (data >> 2) + 1;
        
        switch (command) {
            case 0: /* SourceRead */
                for (u64 i = 0; i < length && outputOffset < expectedTargetSize; i++) {
                    if (outputOffset < sourceSize) {
                        outputData[outputOffset] = sourceData[outputOffset];
                    }
                    outputOffset++;
                }
                break;
                
            case 1: /* TargetRead */
                for (u64 i = 0; i < length && outputOffset < expectedTargetSize && ptr < end; i++) {
                    outputData[outputOffset++] = *ptr++;
                }
                break;
                
            case 2: /* SourceCopy */
                {
                    s64 offset = BpsReadSignedOffset(ptr, end);
                    sourceRelOffset += offset;
                    for (u64 i = 0; i < length && outputOffset < expectedTargetSize; i++) {
                        if (sourceRelOffset < sourceSize) {
                            outputData[outputOffset] = sourceData[sourceRelOffset];
                        }
                        outputOffset++;
                        sourceRelOffset++;
                    }
                }
                break;
                
            case 3: /* TargetCopy */
                {
                    s64 offset = BpsReadSignedOffset(ptr, end);
                    targetRelOffset += offset;
                    for (u64 i = 0; i < length && outputOffset < expectedTargetSize; i++) {
                        if (targetRelOffset < outputOffset) {
                            outputData[outputOffset] = outputData[targetRelOffset];
                        }
                        outputOffset++;
                        targetRelOffset++;
                    }
                }
                break;
        }
    }
    
    *outputSize = (size_t)expectedTargetSize;
    
    /* Verify output CRC */
    u32 expectedTargetCRC = footer[4] | (footer[5] << 8) | (footer[6] << 16) | (footer[7] << 24);
    u32 calcTargetCRC = BpsCalcCRC32(outputData, *outputSize);
    if (calcTargetCRC != expectedTargetCRC) {
        snprintf(msg, sizeof(msg), "Target CRC mismatch: expected %08X, got %08X", expectedTargetCRC, calcTargetCRC);
        BpsLog(msg);
        return CV64_BPS_ERROR_OUTPUT_MISMATCH;
    }
    
    /* Fill info if provided */
    if (info) {
        strncpy(info->filename, patchPath, sizeof(info->filename) - 1);
        info->sourceSize = (u32)expectedSourceSize;
        info->targetSize = (u32)expectedTargetSize;
        info->sourceCRC = expectedSourceCRC;
        info->targetCRC = expectedTargetCRC;
        info->patchCRC = expectedPatchCRC;
    }
    
    snprintf(msg, sizeof(msg), "BPS patch applied successfully! Output size: %zu", *outputSize);
    BpsLog(msg);
    
    return CV64_BPS_SUCCESS;
}

CV64_BPS_Result CV64_BPS_ApplyPatches(u8* romData, size_t* romSize, size_t maxSize) {
    if (!romData || !romSize || maxSize == 0) {
        return CV64_BPS_ERROR_INVALID_DATA;
    }
    
    BpsLog("Scanning for BPS patches...");
    
    /* Get patches directory */
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path patchesDir = exeDir / "patches";
    
    if (!std::filesystem::exists(patchesDir)) {
        BpsLog("Patches directory not found");
        return CV64_BPS_NO_PATCHES_FOUND;
    }
    
    /* Find all .bps files */
    std::vector<std::filesystem::path> bpsFiles;
    for (const auto& entry : std::filesystem::directory_iterator(patchesDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bps") {
            bpsFiles.push_back(entry.path());
        }
    }
    
    if (bpsFiles.empty()) {
        BpsLog("No BPS patches found");
        return CV64_BPS_NO_PATCHES_FOUND;
    }
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Found %zu BPS patch(es)", bpsFiles.size());
    BpsLog(msg);
    
    /* Allocate temporary buffer for patching */
    std::vector<u8> tempBuffer(maxSize);
    
    /* Apply patches in order */
    for (const auto& patchFile : bpsFiles) {
        snprintf(msg, sizeof(msg), "Processing: %s", patchFile.filename().string().c_str());
        BpsLog(msg);
        
        /* Get patch info first */
        CV64_BPS_PatchInfo patchInfo;
        CV64_BPS_Result infoResult = CV64_BPS_GetPatchInfo(patchFile.string().c_str(), &patchInfo);
        if (infoResult != CV64_BPS_SUCCESS) {
            snprintf(msg, sizeof(msg), "Failed to read patch info: %s", CV64_BPS_GetErrorMessage(infoResult));
            BpsLog(msg);
            continue; /* Skip invalid patches */
        }
        
        snprintf(msg, sizeof(msg), "Patch expects source size=%u, CRC=%08X", patchInfo.sourceSize, patchInfo.sourceCRC);
        BpsLog(msg);
        
        /* Check if this patch is for our ROM (by CRC) */
        u32 romCRC = BpsCalcCRC32(romData, *romSize);
        if (romCRC != patchInfo.sourceCRC) {
            snprintf(msg, sizeof(msg), "ROM CRC %08X doesn't match patch source CRC %08X, skipping", romCRC, patchInfo.sourceCRC);
            BpsLog(msg);
            continue;
        }
        
        /* Apply patch */
        size_t outputSize = maxSize;
        CV64_BPS_Result patchResult = CV64_BPS_ApplyPatch(
            patchFile.string().c_str(),
            romData,
            *romSize,
            tempBuffer.data(),
            &outputSize,
            nullptr
        );
        
        if (patchResult == CV64_BPS_SUCCESS) {
            /* Copy patched data back */
            memcpy(romData, tempBuffer.data(), outputSize);
            *romSize = outputSize;
            
            snprintf(msg, sizeof(msg), "Successfully applied: %s", patchFile.filename().string().c_str());
            BpsLog(msg);
        } else {
            snprintf(msg, sizeof(msg), "Failed to apply %s: %s", 
                     patchFile.filename().string().c_str(), 
                     CV64_BPS_GetErrorMessage(patchResult));
            BpsLog(msg);
        }
    }
    
    return CV64_BPS_SUCCESS;
}

const char* CV64_BPS_GetErrorMessage(CV64_BPS_Result result) {
    switch (result) {
        case CV64_BPS_SUCCESS:              return "Success";
        case CV64_BPS_ERROR_FILE_NOT_FOUND: return "Patch file not found";
        case CV64_BPS_ERROR_INVALID_HEADER: return "Invalid BPS header";
        case CV64_BPS_ERROR_SOURCE_MISMATCH:return "Source CRC mismatch";
        case CV64_BPS_ERROR_OUTPUT_MISMATCH:return "Output CRC mismatch";
        case CV64_BPS_ERROR_MEMORY:         return "Memory allocation failed";
        case CV64_BPS_ERROR_INVALID_DATA:   return "Invalid patch data";
        case CV64_BPS_ERROR_SIZE_MISMATCH:  return "Size mismatch";
        case CV64_BPS_NO_PATCHES_FOUND:     return "No patches found";
        default:                            return "Unknown error";
    }
}
