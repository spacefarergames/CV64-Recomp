/**
 * @file cv64_overlays.h
 * @brief Castlevania 64 PC Recomp - ROM Overlay Table
 * 
 * Defines the overlay segments used by CV64. Each overlay is a block of
 * MIPS code that the game loads from ROM into a specific RAM region at
 * runtime. Multiple overlays may share the same RAM base address, meaning
 * only one can be active at a time (e.g., all map object overlays load at
 * 0x8018EB10, swapped per map).
 * 
 * This table is essential for the static recompiler to:
 *   - Locate all executable code regions in the ROM
 *   - Resolve cross-overlay function calls
 *   - Dispatch to the correct recompiled overlay based on game state
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_OVERLAYS_H
#define CV64_OVERLAYS_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Overlay Categories
 *===========================================================================*/

typedef enum CV64_OverlayCategory {
    CV64_OVL_CAT_PLAYER = 0,       /**< Player character code */
    CV64_OVL_CAT_SYSTEM,            /**< System / UI screens */
    CV64_OVL_CAT_MAP_OBJECTS,       /**< Map-specific actors, hazards, obstacles */
} CV64_OverlayCategory;

/*===========================================================================
 * Overlay IDs
 *===========================================================================*/

typedef enum CV64_OverlayID {
    /* Player overlays */
    CV64_OVL_REINHARDT = 0,
    CV64_OVL_CARRIE,

    /* System overlays */
    CV64_OVL_GAMENOTE_DELETE,
    CV64_OVL_KONAMI_LOGO,

    /* Map object overlays (naming: P<ID>OBJECTNAME convention) */
    CV64_OVL_MAP_MORI,
    CV64_OVL_MAP_TOU,
    CV64_OVL_MAP_TOUOKUJI,
    CV64_OVL_MAP_NAKANIWA,
    CV64_OVL_MAP_BEKKAN_1F,
    CV64_OVL_MAP_BEKKAN_2F,
    CV64_OVL_MAP_MEIRO_TEIEN,
    CV64_OVL_MAP_CHIKA_KODO,
    CV64_OVL_MAP_CHIKA_SUIRO,
    CV64_OVL_MAP_HONMARU_B1F,
    CV64_OVL_MAP_HONMARU_1F,
    CV64_OVL_MAP_HONMARU_2F,
    CV64_OVL_MAP_HONMARU_3F_MINAMI,
    CV64_OVL_MAP_HONMARU_4F_MINAMI,
    CV64_OVL_MAP_HONMARU_3F_KITA,
    CV64_OVL_MAP_HONMARU_5F,
    CV64_OVL_MAP_SHOKEI_TOU,
    CV64_OVL_MAP_MAHOU_TOU,
    CV64_OVL_MAP_KAGAKU_TOU,
    CV64_OVL_MAP_KETTOU_TOU,
    CV64_OVL_MAP_TURO_TOKEITOU,
    CV64_OVL_MAP_TENSHU,
    CV64_OVL_MAP_TOKEITOU_NAI,
    CV64_OVL_MAP_DRACULA,
    CV64_OVL_MAP_ROSE,
    CV64_OVL_MAP_BEKKAN_BOSS,
    CV64_OVL_MAP_TOU_TURO,
    CV64_OVL_MAP_ENDING,

    CV64_OVL_COUNT
} CV64_OverlayID;

/*===========================================================================
 * Overlay Descriptor
 *===========================================================================*/

typedef struct CV64_OverlayInfo {
    CV64_OverlayID      id;             /**< Overlay identifier */
    CV64_OverlayCategory category;      /**< Overlay category */
    const char*         name;           /**< Human-readable name */
    u32                 rom_start;      /**< Start offset in ROM */
    u32                 rom_end;        /**< End offset in ROM (exclusive) */
    u32                 ram_start;      /**< N64 RAM load address (start) */
    u32                 ram_end;        /**< N64 RAM load address (end) */
} CV64_OverlayInfo;

/*===========================================================================
 * Overlay Table
 *===========================================================================*/

/** Size of an overlay in ROM bytes */
#define CV64_OVL_ROM_SIZE(ovl) ((ovl).rom_end - (ovl).rom_start)

/** Size of an overlay in RAM bytes */
#define CV64_OVL_RAM_SIZE(ovl) ((ovl).ram_end - (ovl).ram_start)

/**
 * @brief Master overlay table (ROM order).
 * 
 * All addresses are for the USA (NTSC-U) v1.0 ROM.
 * Indexed by CV64_OverlayID.
 */
static const CV64_OverlayInfo cv64_overlay_table[CV64_OVL_COUNT] = {
    /* Player overlays - loaded at 0x803D13E0, mutually exclusive */
    { CV64_OVL_REINHARDT,           CV64_OVL_CAT_PLAYER,      "Reinhardt",             0x697040, 0x6A42D0, 0x803D13E0, 0x803DE670 },
    { CV64_OVL_CARRIE,              CV64_OVL_CAT_PLAYER,      "Carrie",                0x6A42D0, 0x6B12F0, 0x803D13E0, 0x803DE400 },

    /* System overlays */
    { CV64_OVL_GAMENOTE_DELETE,     CV64_OVL_CAT_SYSTEM,      "GameNote Delete",       0x6B12F0, 0x6B4FC0, 0x801CC000, 0x801CFCD0 },
    { CV64_OVL_KONAMI_LOGO,         CV64_OVL_CAT_SYSTEM,      "Konami / KCEK Logo",    0x6B4FC0, 0x6B5480, 0x801CFCD0, 0x801D0190 },

    /* Map object overlays - most load at 0x8018EB10, mutually exclusive */
    { CV64_OVL_MAP_MORI,            CV64_OVL_CAT_MAP_OBJECTS,  "MORI",                  0x6B5480, 0x6BAB80, 0x8018EB10, 0x80194210 },
    { CV64_OVL_MAP_TOU,             CV64_OVL_CAT_MAP_OBJECTS,  "TOU",                   0x6BAB80, 0x6BFBF0, 0x8018EB10, 0x80193B80 },
    { CV64_OVL_MAP_TOUOKUJI,        CV64_OVL_CAT_MAP_OBJECTS,  "TOUOKUJI",              0x6BFBF0, 0x6C1460, 0x8018EB10, 0x80190380 },
    { CV64_OVL_MAP_NAKANIWA,        CV64_OVL_CAT_MAP_OBJECTS,  "NAKANIWA",              0x6C1460, 0x6C2F20, 0x8018EB10, 0x801905D0 },
    { CV64_OVL_MAP_BEKKAN_1F,       CV64_OVL_CAT_MAP_OBJECTS,  "BEKKAN 1F",             0x6C2F20, 0x6C3300, 0x8018EB10, 0x8018EEF0 },
    { CV64_OVL_MAP_BEKKAN_2F,       CV64_OVL_CAT_MAP_OBJECTS,  "BEKKAN 2F",             0x6C3300, 0x6C50E0, 0x8018EB10, 0x801908F0 },
    { CV64_OVL_MAP_MEIRO_TEIEN,     CV64_OVL_CAT_MAP_OBJECTS,  "MEIRO TEIEN",           0x6C50E0, 0x6C5110, 0x8018EB10, 0x8018EB40 },
    { CV64_OVL_MAP_CHIKA_KODO,      CV64_OVL_CAT_MAP_OBJECTS,  "CHIKA KODO",            0x6C5110, 0x6C77D0, 0x8018EB10, 0x801911D0 },
    { CV64_OVL_MAP_CHIKA_SUIRO,     CV64_OVL_CAT_MAP_OBJECTS,  "CHIKA SUIRO",           0x6C77D0, 0x6C86A0, 0x8018EB10, 0x8018F9E0 },
    { CV64_OVL_MAP_HONMARU_B1F,     CV64_OVL_CAT_MAP_OBJECTS,  "HONMARU B1F",           0x6C86A0, 0x6C9BF0, 0x8018EB10, 0x80190060 },
    { CV64_OVL_MAP_HONMARU_1F,      CV64_OVL_CAT_MAP_OBJECTS,  "HONMARU 1F",            0x6C9BF0, 0x6CB850, 0x8018EB10, 0x80190770 },
    { CV64_OVL_MAP_HONMARU_2F,      CV64_OVL_CAT_MAP_OBJECTS,  "HONMARU 2F",            0x6CB850, 0x6CD050, 0x8018EB10, 0x80190310 },
    { CV64_OVL_MAP_HONMARU_3F_MINAMI, CV64_OVL_CAT_MAP_OBJECTS, "HONMARU 3F MINAMI",    0x6CD050, 0x6CD740, 0x8018EB10, 0x8018F200 },
    { CV64_OVL_MAP_HONMARU_4F_MINAMI, CV64_OVL_CAT_MAP_OBJECTS, "HONMARU 4F MINAMI",    0x6CD740, 0x6CE4B0, 0x8018EB10, 0x8018F880 },
    { CV64_OVL_MAP_HONMARU_3F_KITA, CV64_OVL_CAT_MAP_OBJECTS,  "HONMARU 3F KITA",       0x6CE4B0, 0x6CE9A0, 0x8018EB10, 0x8018F000 },
    { CV64_OVL_MAP_HONMARU_5F,      CV64_OVL_CAT_MAP_OBJECTS,  "HONMARU 5F",            0x6CE9A0, 0x6CF420, 0x8018EB10, 0x8018F590 },
    { CV64_OVL_MAP_SHOKEI_TOU,      CV64_OVL_CAT_MAP_OBJECTS,  "SHOKEI TOU",            0x6CF420, 0x6D35A0, 0x8018EB10, 0x80192C90 },
    { CV64_OVL_MAP_MAHOU_TOU,       CV64_OVL_CAT_MAP_OBJECTS,  "MAHOU TOU",             0x6D35A0, 0x6D74F0, 0x8018EB10, 0x80192A60 },
    { CV64_OVL_MAP_KAGAKU_TOU,      CV64_OVL_CAT_MAP_OBJECTS,  "KAGAKU TOU",            0x6D74F0, 0x6E31D0, 0x8018EB10, 0x8019A7F0 },
    { CV64_OVL_MAP_KETTOU_TOU,      CV64_OVL_CAT_MAP_OBJECTS,  "KETTOU TOU",            0x6E31D0, 0x6E86B0, 0x8018EB10, 0x80193FF0 },
    { CV64_OVL_MAP_TURO_TOKEITOU,   CV64_OVL_CAT_MAP_OBJECTS,  "TURO TOKEITOU",         0x6E86B0, 0x6EA480, 0x8018EB10, 0x801908E0 },
    { CV64_OVL_MAP_TENSHU,          CV64_OVL_CAT_MAP_OBJECTS,  "TENSHU",                0x6EA480, 0x6EBD90, 0x8018EB10, 0x80190420 },
    { CV64_OVL_MAP_TOKEITOU_NAI,    CV64_OVL_CAT_MAP_OBJECTS,  "TOKEITOU NAI",          0x6EBD90, 0x6F0150, 0x8018EB10, 0x80192ED0 },
    { CV64_OVL_MAP_DRACULA,         CV64_OVL_CAT_MAP_OBJECTS,  "DRACULA",               0x6F0150, 0x6F0A50, 0x8018EB10, 0x8018F410 },
    { CV64_OVL_MAP_ROSE,            CV64_OVL_CAT_MAP_OBJECTS,  "ROSE",                  0x6F0A50, 0x6F11B0, 0x8018EB10, 0x8018F270 },
    { CV64_OVL_MAP_BEKKAN_BOSS,     CV64_OVL_CAT_MAP_OBJECTS,  "BEKKAN BOSS",           0x6F11B0, 0x6F1B80, 0x8018EB10, 0x8018F4E0 },
    { CV64_OVL_MAP_TOU_TURO,        CV64_OVL_CAT_MAP_OBJECTS,  "TOU TURO",              0x6F1B80, 0x6F21D0, 0x8018EB10, 0x8018F160 },
    { CV64_OVL_MAP_ENDING,          CV64_OVL_CAT_MAP_OBJECTS,  "ENDING",                0x6F21D0, 0x6F3BD0, 0x8018EB10, 0x80190510 },
};

/*===========================================================================
 * Overlay Lookup API
 *===========================================================================*/

/**
 * @brief Get overlay info by ID
 * @param id Overlay ID
 * @return Pointer to overlay info, or NULL if invalid
 */
static inline const CV64_OverlayInfo* CV64_Overlay_GetByID(CV64_OverlayID id) {
    if (id < 0 || id >= CV64_OVL_COUNT) {
        return NULL;
    }
    return &cv64_overlay_table[id];
}

/**
 * @brief Find which overlay owns a given N64 RAM address.
 * 
 * Because multiple overlays share the same RAM base, you must also
 * supply the currently active overlay ID for the relevant category
 * to disambiguate. If active_id is CV64_OVL_COUNT, all overlays are
 * searched and the first match is returned (useful for static analysis).
 * 
 * @param ram_addr N64 RAM address to look up
 * @param active_id Currently active overlay ID, or CV64_OVL_COUNT to search all
 * @return Pointer to matching overlay info, or NULL if not found
 */
static inline const CV64_OverlayInfo* CV64_Overlay_FindByRAMAddr(u32 ram_addr, CV64_OverlayID active_id) {
    if (active_id < CV64_OVL_COUNT) {
        const CV64_OverlayInfo* ovl = &cv64_overlay_table[active_id];
        if (ram_addr >= ovl->ram_start && ram_addr < ovl->ram_end) {
            return ovl;
        }
        return NULL;
    }

    for (u32 i = 0; i < CV64_OVL_COUNT; i++) {
        const CV64_OverlayInfo* ovl = &cv64_overlay_table[i];
        if (ram_addr >= ovl->ram_start && ram_addr < ovl->ram_end) {
            return ovl;
        }
    }
    return NULL;
}

/**
 * @brief Find an overlay by its ROM start offset
 * @param rom_offset ROM offset
 * @return Pointer to matching overlay info, or NULL if not found
 */
static inline const CV64_OverlayInfo* CV64_Overlay_FindByROMOffset(u32 rom_offset) {
    for (u32 i = 0; i < CV64_OVL_COUNT; i++) {
        const CV64_OverlayInfo* ovl = &cv64_overlay_table[i];
        if (rom_offset >= ovl->rom_start && rom_offset < ovl->rom_end) {
            return ovl;
        }
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* CV64_OVERLAYS_H */
