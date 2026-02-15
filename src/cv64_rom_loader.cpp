/**
 * @file cv64_rom_loader.cpp
 * @brief Castlevania 64 PC Recomp - ROM Loader Implementation
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_rom_loader.h"
#include <Windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static std::string s_lastError;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

static void SetError(const std::string& error) {
    s_lastError = error;
    OutputDebugStringA("[CV64_ROM] Error: ");
    OutputDebugStringA(error.c_str());
    OutputDebugStringA("\n");
}

static void LogInfo(const std::string& info) {
    OutputDebugStringA("[CV64_ROM] ");
    OutputDebugStringA(info.c_str());
    OutputDebugStringA("\n");
}

static u32 SwapEndian32(u32 val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

static u16 SwapEndian16(u16 val) {
    return ((val >> 8) & 0xFF) | ((val << 8) & 0xFF00);
}

static std::filesystem::path GetExecutableDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

/*===========================================================================
 * ROM Format Detection
 *===========================================================================*/

int CV64_Rom_DetectFormat(const u8* data) {
    // Check first 4 bytes for ROM format signature
    // z64 (big-endian): 0x80 0x37 0x12 0x40
    // n64 (little-endian): 0x40 0x12 0x37 0x80
    // v64 (byte-swapped): 0x37 0x80 0x40 0x12
    
    if (data[0] == 0x80 && data[1] == 0x37) {
        return 0;  // z64 - big endian (correct format)
    } else if (data[0] == 0x40 && data[1] == 0x12) {
        return 1;  // n64 - little endian (needs word swap)
    } else if (data[0] == 0x37 && data[1] == 0x80) {
        return 2;  // v64 - byte swapped (needs byte swap)
    }
    
    return -1;  // Unknown format
}

bool CV64_Rom_Byteswap(u8* data, u64 size) {
    int format = CV64_Rom_DetectFormat(data);
    
    if (format == 0) {
        return false;  // Already correct format
    }
    
    if (format == 1) {
        // Little-endian (n64) - swap 32-bit words
        u32* words = (u32*)data;
        for (u64 i = 0; i < size / 4; i++) {
            words[i] = SwapEndian32(words[i]);
        }
        LogInfo("Converted ROM from little-endian (n64) to big-endian (z64)");
        return true;
    }
    
    if (format == 2) {
        // Byte-swapped (v64) - swap adjacent bytes
        for (u64 i = 0; i < size; i += 2) {
            u8 temp = data[i];
            data[i] = data[i + 1];
            data[i + 1] = temp;
        }
        LogInfo("Converted ROM from byte-swapped (v64) to big-endian (z64)");
        return true;
    }
    
    SetError("Unknown ROM format");
    return false;
}

/*===========================================================================
 * ROM Validation
 *===========================================================================*/

bool CV64_Rom_Validate(const u8* data, u64 size, CV64_RomInfo* info) {
    if (!data || !info) {
        SetError("Invalid parameters");
        return false;
    }
    
    memset(info, 0, sizeof(CV64_RomInfo));
    info->file_size = size;
    
    // Check minimum size (N64 header is 0x40 bytes)
    if (size < 0x1000) {
        SetError("ROM file too small");
        return false;
    }
    
    // Detect and handle byte swapping
    int format = CV64_Rom_DetectFormat(data);
    info->needs_byteswap = (format != 0);
    
    // Copy header (may need byte swapping)
    memcpy(&info->header, data, sizeof(CV64_RomHeaderRaw));
    
    // Handle byte-swapped headers
    if (format == 1) {
        // Little-endian - swap words in header
        info->header.clock_rate = SwapEndian32(info->header.clock_rate);
        info->header.program_counter = SwapEndian32(info->header.program_counter);
        info->header.release = SwapEndian32(info->header.release);
        info->header.crc1 = SwapEndian32(info->header.crc1);
        info->header.crc2 = SwapEndian32(info->header.crc2);
        info->header.cartridge_id = SwapEndian16(info->header.cartridge_id);
    } else if (format == 2) {
        // Byte-swapped - swap adjacent bytes in strings
        for (int i = 0; i < 20; i += 2) {
            char temp = info->header.name[i];
            info->header.name[i] = info->header.name[i + 1];
            info->header.name[i + 1] = temp;
        }
    }
    
    // Extract CRCs
    info->crc1 = info->header.crc1;
    info->crc2 = info->header.crc2;
    
    // Extract name (trim trailing spaces)
    strncpy(info->name, info->header.name, 20);
    info->name[20] = '\0';
    for (int i = 19; i >= 0 && info->name[i] == ' '; i--) {
        info->name[i] = '\0';
    }
    
    // Build game ID
    info->game_id[0] = 'N';  // Nintendo cartridge
    info->game_id[1] = (info->header.cartridge_id >> 8) & 0xFF;
    info->game_id[2] = info->header.cartridge_id & 0xFF;
    info->game_id[3] = info->header.country_code;
    info->game_id[4] = '\0';
    
    // Determine region
    switch (info->header.country_code) {
        case 'E':
            info->region = CV64_REGION_USA;
            break;
        case 'J':
            info->region = CV64_REGION_JAPAN;
            break;
        case 'P':
            info->region = CV64_REGION_EUROPE;
            break;
        default:
            info->region = CV64_REGION_UNKNOWN;
            break;
    }
    
    // Check if this is Castlevania 64
    // Cartridge ID should be "DC" for Castlevania/Dracula
    char cartId[3] = {
        (char)((info->header.cartridge_id >> 8) & 0xFF),
        (char)(info->header.cartridge_id & 0xFF),
        '\0'
    };
    
    bool isCV64 = (strcmp(cartId, "DC") == 0) ||
                  (strstr(info->name, "CASTLEVANIA") != NULL) ||
                  (strstr(info->name, "DRACULA") != NULL);
    
    info->is_cv64 = isCV64;
    
    // Determine version based on CRC
    if (info->crc1 == CV64_CRC1_USA_10 && info->crc2 == CV64_CRC2_USA_10) {
        info->version = CV64_VERSION_1_0;
    } else if (info->crc1 == CV64_CRC1_USA_12 && info->crc2 == CV64_CRC2_USA_12) {
        info->version = CV64_VERSION_1_2;
    } else if (info->crc1 == CV64_CRC1_JAPAN && info->crc2 == CV64_CRC2_JAPAN) {
        info->version = CV64_VERSION_1_0;  // Japan is v1.0
    } else if (info->crc1 == CV64_CRC1_EUROPE && info->crc2 == CV64_CRC2_EUROPE) {
        info->version = CV64_VERSION_1_0;  // Europe is v1.0
    } else {
        info->version = CV64_VERSION_UNKNOWN;
    }
    
    info->is_valid = true;
    
    // Log info
    char logBuf[256];
    sprintf_s(logBuf, sizeof(logBuf), "ROM: %s [%s] Region: %s CRC: %08X/%08X",
            info->name, info->game_id,
            CV64_Rom_GetRegionName(info->region),
            info->crc1, info->crc2);
    LogInfo(logBuf);
    
    return true;
}

bool CV64_Rom_Load(const char* path, CV64_RomInfo* info) {
    if (!path || !info) {
        SetError("Invalid parameters");
        return false;
    }
    
    LogInfo(std::string("Loading ROM: ") + path);
    
    // Open file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        SetError("Failed to open ROM file");
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read header (first 0x40 bytes)
    std::vector<u8> header(0x40);
    if (!file.read(reinterpret_cast<char*>(header.data()), 0x40)) {
        SetError("Failed to read ROM header");
        return false;
    }
    
    file.close();
    
    // Validate
    if (!CV64_Rom_Validate(header.data(), size, info)) {
        return false;
    }
    
    return true;
}

bool CV64_Rom_IsCV64(const CV64_RomInfo* info) {
    return info && info->is_valid && info->is_cv64;
}

const char* CV64_Rom_GetRegionName(CV64_RomRegion region) {
    switch (region) {
        case CV64_REGION_USA:    return "USA";
        case CV64_REGION_JAPAN:  return "Japan";
        case CV64_REGION_EUROPE: return "Europe";
        default:                 return "Unknown";
    }
}

const char* CV64_Rom_GetVersionName(CV64_RomVersion version) {
    switch (version) {
        case CV64_VERSION_1_0: return "1.0";
        case CV64_VERSION_1_1: return "1.1";
        case CV64_VERSION_1_2: return "1.2";
        default:               return "Unknown";
    }
}

bool CV64_Rom_FindROM(char* buffer, u32 buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false;
    }
    
    std::filesystem::path exeDir = GetExecutableDirectory();
    
    // Common ROM file patterns
    std::vector<std::string> patterns = {
        "Castlevania (U) (V1.2) [!].z64",
        "Castlevania (USA) (Rev B).z64",
        "Castlevania (USA).z64",
        "Castlevania.z64",
        "Castlevania64.z64",
        "CASTLEVANIA.z64",
        "cv64.z64",
        "Akumajou Dracula (J).z64",
        "Akumajou Dracula.z64"
    };
    
    // Search locations
    std::vector<std::filesystem::path> searchPaths = {
        exeDir / "assets",
        exeDir / "roms",
        exeDir,
        std::filesystem::path("C:/Roms"),
        std::filesystem::path(std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : "") / "Roms"
    };
    
    for (const auto& searchDir : searchPaths) {
        if (!std::filesystem::exists(searchDir)) {
            continue;
        }
        
        // Check explicit patterns first
        for (const auto& pattern : patterns) {
            std::filesystem::path romPath = searchDir / pattern;
            if (std::filesystem::exists(romPath)) {
                strncpy(buffer, romPath.string().c_str(), buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                LogInfo("Found ROM: " + romPath.string());
                return true;
            }
        }
        
        // Search for any .z64/.n64/.v64 files and check if they're CV64
        try {
            for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
                if (!entry.is_regular_file()) continue;
                
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".z64" || ext == ".n64" || ext == ".v64") {
                    CV64_RomInfo info;
                    if (CV64_Rom_Load(entry.path().string().c_str(), &info) && info.is_cv64) {
                        strncpy(buffer, entry.path().string().c_str(), buffer_size - 1);
                        buffer[buffer_size - 1] = '\0';
                        LogInfo("Found CV64 ROM: " + entry.path().string());
                        return true;
                    }
                }
            }
        } catch (...) {
            // Ignore directory access errors
        }
    }
    
    SetError("No Castlevania 64 ROM found");
    return false;
}
