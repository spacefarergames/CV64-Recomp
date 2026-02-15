/**
 * @file cv64_rom_loader.h
 * @brief Castlevania 64 PC Recomp - ROM Loader and Validator
 * 
 * Loads and validates Castlevania 64 ROM files and provides
 * ROM-specific information for patches.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_ROM_LOADER_H
#define CV64_ROM_LOADER_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * ROM Region/Version Information
 *===========================================================================*/

typedef enum CV64_RomRegion {
    CV64_REGION_UNKNOWN = 0,
    CV64_REGION_USA,        /* NDCE - Castlevania (USA) */
    CV64_REGION_JAPAN,      /* NDCJ - Akumajou Dracula (Japan) */
    CV64_REGION_EUROPE,     /* NDCP - Castlevania (Europe) */
    CV64_REGION_COUNT
} CV64_RomRegion;

typedef enum CV64_RomVersion {
    CV64_VERSION_UNKNOWN = 0,
    CV64_VERSION_1_0,
    CV64_VERSION_1_1,
    CV64_VERSION_1_2,       /* Recommended version */
    CV64_VERSION_COUNT
} CV64_RomVersion;

/*===========================================================================
 * ROM Header (N64 ROM format)
 *===========================================================================*/

#pragma pack(push, 1)
typedef struct CV64_RomHeaderRaw {
    u8  pi_bsb_dom1_lat_reg;    /* 0x00: Initial PI_BSD_DOM1_LAT_REG */
    u8  pi_bsb_dom1_pgs_reg;    /* 0x01: Initial PI_BSD_DOM1_PGS_REG */
    u8  pi_bsb_dom1_pwd_reg;    /* 0x02: Initial PI_BSD_DOM1_PWD_REG */
    u8  pi_bsb_dom1_pgs_reg2;   /* 0x03: Initial PI_BSD_DOM1_PGS_REG (again) */
    u32 clock_rate;             /* 0x04: Clock rate override */
    u32 program_counter;        /* 0x08: Boot address / entry point */
    u32 release;                /* 0x0C: Release address */
    u32 crc1;                   /* 0x10: CRC1 checksum */
    u32 crc2;                   /* 0x14: CRC2 checksum */
    u8  reserved1[8];           /* 0x18: Reserved */
    char name[20];              /* 0x20: Internal name (null-padded) */
    u8  reserved2[7];           /* 0x34: Reserved */
    u8  manufacturer_id;        /* 0x3B: Manufacturer ID (N = Nintendo) */
    u16 cartridge_id;           /* 0x3C: Cartridge ID (e.g., "DC" for CV64) */
    char country_code;          /* 0x3E: Country code (E=USA, J=Japan, P=Europe) */
    u8  version;                /* 0x3F: ROM version */
} CV64_RomHeaderRaw;
#pragma pack(pop)

/*===========================================================================
 * Parsed ROM Information
 *===========================================================================*/

typedef struct CV64_RomInfo {
    /* Validated flag */
    bool is_valid;
    bool is_cv64;               /* Is this Castlevania 64? */
    
    /* Raw header */
    CV64_RomHeaderRaw header;
    
    /* Parsed information */
    char name[21];              /* Null-terminated name */
    char game_id[5];            /* e.g., "NDCE" */
    CV64_RomRegion region;
    CV64_RomVersion version;
    u32 crc1;
    u32 crc2;
    
    /* ROM data */
    u64 file_size;
    
    /* Byteswap status */
    bool needs_byteswap;        /* ROM needs byte swapping */
    
} CV64_RomInfo;

/*===========================================================================
 * Known CRC Values for Castlevania 64
 *===========================================================================*/

/* USA Version 1.0 */
#define CV64_CRC1_USA_10    0xF68C0A40
#define CV64_CRC2_USA_10    0x93239C78

/* USA Version 1.2 (Recommended) */
#define CV64_CRC1_USA_12    0xF2A30BB8
#define CV64_CRC2_USA_12    0x83D59D04

/* Japan */
#define CV64_CRC1_JAPAN     0x5C1D2E48
#define CV64_CRC2_JAPAN     0x4F68C35B

/* Europe */
#define CV64_CRC1_EUROPE    0x53A09F3C
#define CV64_CRC2_EUROPE    0x7A52B60A

/*===========================================================================
 * API Functions
 *===========================================================================*/

/**
 * @brief Load and validate a ROM file
 * @param path Path to ROM file
 * @param info Output ROM information
 * @return true if ROM loaded successfully
 */
CV64_API bool CV64_Rom_Load(const char* path, CV64_RomInfo* info);

/**
 * @brief Validate ROM data in memory
 * @param data ROM data
 * @param size ROM size in bytes
 * @param info Output ROM information
 * @return true if ROM is valid
 */
CV64_API bool CV64_Rom_Validate(const u8* data, u64 size, CV64_RomInfo* info);

/**
 * @brief Check if ROM is Castlevania 64
 * @param info ROM information
 * @return true if CV64
 */
CV64_API bool CV64_Rom_IsCV64(const CV64_RomInfo* info);

/**
 * @brief Get ROM region name
 * @param region Region enum
 * @return Region name string
 */
CV64_API const char* CV64_Rom_GetRegionName(CV64_RomRegion region);

/**
 * @brief Get ROM version name
 * @param version Version enum
 * @return Version name string
 */
CV64_API const char* CV64_Rom_GetVersionName(CV64_RomVersion version);

/**
 * @brief Byteswap ROM data if needed
 * @param data ROM data (modified in place)
 * @param size ROM size
 * @return true if byteswap was performed
 */
CV64_API bool CV64_Rom_Byteswap(u8* data, u64 size);

/**
 * @brief Detect ROM format (z64, n64, v64)
 * @param data First 4 bytes of ROM
 * @return 0=z64 (big), 1=n64 (little), 2=v64 (byteswapped)
 */
CV64_API int CV64_Rom_DetectFormat(const u8* data);

/**
 * @brief Find Castlevania 64 ROM in common locations
 * @param buffer Output path buffer
 * @param buffer_size Buffer size
 * @return true if ROM found
 */
CV64_API bool CV64_Rom_FindROM(char* buffer, u32 buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* CV64_ROM_LOADER_H */
