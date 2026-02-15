/**
 * @file cv64_model_database.cpp
 * @brief CV64 Model Database Implementation
 */

#include "../include/cv64_model_database.h"
#include "../include/cv64_model_viewer.h"
#include <string.h>

// Model database with known addresses from CV64 decomp
// Note: These are placeholder addresses - actual ROM addresses need to be
// determined from the decomp project's documentation or reverse engineering
static const CV64_ModelDatabaseEntry g_modelDatabase[] = {
    // Characters
    { 0x0001, "Reinhardt Schneider", 0x00800000, 0x8000, 0x00800000, 0x00801000, 0x00802000, 4, CV64_MODEL_TYPE_CHARACTER },
    { 0x0002, "Carrie Fernandez", 0x00808000, 0x7800, 0x00808000, 0x00809000, 0x0080A000, 4, CV64_MODEL_TYPE_CHARACTER },
    { 0x0003, "Reinhardt (Whip)", 0x00810000, 0x8500, 0x00810000, 0x00811000, 0x00812000, 4, CV64_MODEL_TYPE_CHARACTER },
    { 0x0004, "Carrie (Orb)", 0x00819000, 0x8000, 0x00819000, 0x0081A000, 0x0081B000, 4, CV64_MODEL_TYPE_CHARACTER },
    
    // Enemies
    { 0x0010, "Skeleton", 0x00900000, 0x5000, 0x00900000, 0x00901000, 0x00902000, 2, CV64_MODEL_TYPE_ENEMY },
    { 0x0011, "Skeleton Knight", 0x00906000, 0x6000, 0x00906000, 0x00907000, 0x00908000, 3, CV64_MODEL_TYPE_ENEMY },
    { 0x0012, "Werewolf", 0x0090D000, 0x7000, 0x0090D000, 0x0090E000, 0x0090F000, 3, CV64_MODEL_TYPE_ENEMY },
    { 0x0013, "Vampire", 0x00915000, 0x6800, 0x00915000, 0x00916000, 0x00917000, 4, CV64_MODEL_TYPE_ENEMY },
    { 0x0014, "Lizard-man", 0x0091C000, 0x6500, 0x0091C000, 0x0091D000, 0x0091E000, 3, CV64_MODEL_TYPE_ENEMY },
    { 0x0015, "Flea-man", 0x00923000, 0x4000, 0x00923000, 0x00924000, 0x00925000, 2, CV64_MODEL_TYPE_ENEMY },
    { 0x0016, "Hell Hound", 0x00928000, 0x5500, 0x00928000, 0x00929000, 0x0092A000, 2, CV64_MODEL_TYPE_ENEMY },
    { 0x0017, "Bat", 0x0092E000, 0x3000, 0x0092E000, 0x0092F000, 0x00930000, 1, CV64_MODEL_TYPE_ENEMY },
    { 0x0018, "Mist (Shape-shifter)", 0x00932000, 0x4500, 0x00932000, 0x00933000, 0x00934000, 2, CV64_MODEL_TYPE_ENEMY },
    
    // Bosses
    { 0x0020, "Dracula (Final Form)", 0x00A00000, 0xA000, 0x00A00000, 0x00A01000, 0x00A02000, 6, CV64_MODEL_TYPE_BOSS },
    { 0x0021, "Death", 0x00A0B000, 0x9000, 0x00A0B000, 0x00A0C000, 0x00A0D000, 5, CV64_MODEL_TYPE_BOSS },
    { 0x0022, "Actrise", 0x00A15000, 0x8000, 0x00A15000, 0x00A16000, 0x00A17000, 4, CV64_MODEL_TYPE_BOSS },
    { 0x0023, "Rosa (Boss Form)", 0x00A1E000, 0x7500, 0x00A1E000, 0x00A1F000, 0x00A20000, 4, CV64_MODEL_TYPE_BOSS },
    { 0x0024, "Renon (Demon Form)", 0x00A26000, 0x8500, 0x00A26000, 0x00A27000, 0x00A28000, 5, CV64_MODEL_TYPE_BOSS },
    { 0x0025, "Behemoth", 0x00A2F000, 0x9500, 0x00A2F000, 0x00A30000, 0x00A31000, 4, CV64_MODEL_TYPE_BOSS },
    { 0x0026, "Cerberus", 0x00A39000, 0x7000, 0x00A39000, 0x00A3A000, 0x00A3B000, 3, CV64_MODEL_TYPE_BOSS },
    
    // NPCs
    { 0x0030, "Rosa (NPC)", 0x00B00000, 0x6000, 0x00B00000, 0x00B01000, 0x00B02000, 3, CV64_MODEL_TYPE_CHARACTER },
    { 0x0031, "Malus", 0x00B07000, 0x5500, 0x00B07000, 0x00B08000, 0x00B09000, 3, CV64_MODEL_TYPE_CHARACTER },
    { 0x0032, "Vincent", 0x00B0D000, 0x6000, 0x00B0D000, 0x00B0E000, 0x00B0F000, 3, CV64_MODEL_TYPE_CHARACTER },
    { 0x0033, "Renon (Merchant)", 0x00B14000, 0x5800, 0x00B14000, 0x00B15000, 0x00B16000, 3, CV64_MODEL_TYPE_CHARACTER },
    { 0x0034, "Villager", 0x00B1A000, 0x4500, 0x00B1A000, 0x00B1B000, 0x00B1C000, 2, CV64_MODEL_TYPE_CHARACTER },
    { 0x0035, "Child", 0x00B1F000, 0x4000, 0x00B1F000, 0x00B20000, 0x00B21000, 2, CV64_MODEL_TYPE_CHARACTER },
    
    // Items
    { 0x0040, "Red Jewel", 0x00C00000, 0x2000, 0x00C00000, 0x00C00500, 0x00C00800, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0041, "Blue Jewel", 0x00C02000, 0x2000, 0x00C02000, 0x00C02500, 0x00C02800, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0042, "Green Jewel", 0x00C04000, 0x2000, 0x00C04000, 0x00C04500, 0x00C04800, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0043, "Healing Potion", 0x00C06000, 0x2500, 0x00C06000, 0x00C06500, 0x00C06900, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0044, "Roast Beef", 0x00C09000, 0x2800, 0x00C09000, 0x00C09500, 0x00C09A00, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0045, "Key", 0x00C0C000, 0x2000, 0x00C0C000, 0x00C0C500, 0x00C0C800, 1, CV64_MODEL_TYPE_ITEM },
    { 0x0046, "Special Key", 0x00C0E000, 0x2200, 0x00C0E000, 0x00C0E500, 0x00C0E900, 1, CV64_MODEL_TYPE_ITEM },
    
    // Weapons/Sub-weapons
    { 0x0050, "Whip", 0x00D00000, 0x3000, 0x00D00000, 0x00D00800, 0x00D01000, 1, CV64_MODEL_TYPE_WEAPON },
    { 0x0051, "Holy Water", 0x00D03000, 0x2500, 0x00D03000, 0x00D03500, 0x00D03800, 1, CV64_MODEL_TYPE_WEAPON },
    { 0x0052, "Cross", 0x00D06000, 0x2800, 0x00D06000, 0x00D06500, 0x00D06900, 1, CV64_MODEL_TYPE_WEAPON },
    { 0x0053, "Axe", 0x00D09000, 0x2600, 0x00D09000, 0x00D09500, 0x00D09900, 1, CV64_MODEL_TYPE_WEAPON },
    { 0x0054, "Throwing Knife", 0x00D0C000, 0x2200, 0x00D0C000, 0x00D0C400, 0x00D0C700, 1, CV64_MODEL_TYPE_WEAPON },
    
    // Props
    { 0x0060, "Torch", 0x00E00000, 0x2000, 0x00E00000, 0x00E00500, 0x00E00800, 1, CV64_MODEL_TYPE_PROP },
    { 0x0061, "Candle", 0x00E02000, 0x1800, 0x00E02000, 0x00E02400, 0x00E02600, 1, CV64_MODEL_TYPE_PROP },
    { 0x0062, "Door", 0x00E04000, 0x3500, 0x00E04000, 0x00E04800, 0x00E04C00, 2, CV64_MODEL_TYPE_PROP },
    { 0x0063, "Gate", 0x00E08000, 0x4000, 0x00E08000, 0x00E08800, 0x00E08D00, 2, CV64_MODEL_TYPE_PROP },
    { 0x0064, "Breakable Wall", 0x00E0C000, 0x3000, 0x00E0C000, 0x00E0C600, 0x00E0CA00, 1, CV64_MODEL_TYPE_PROP },
    { 0x0065, "Lever", 0x00E0F000, 0x2500, 0x00E0F000, 0x00E0F500, 0x00E0F800, 1, CV64_MODEL_TYPE_PROP },
    { 0x0066, "Coffin", 0x00E12000, 0x3500, 0x00E12000, 0x00E12700, 0x00E12B00, 2, CV64_MODEL_TYPE_PROP },
    { 0x0067, "Statue", 0x00E16000, 0x4500, 0x00E16000, 0x00E16900, 0x00E16E00, 2, CV64_MODEL_TYPE_PROP },
};

const CV64_ModelDatabaseEntry* CV64_GetModelDatabase(uint32_t* outCount) {
    if (outCount) {
        *outCount = sizeof(g_modelDatabase) / sizeof(g_modelDatabase[0]);
    }
    return g_modelDatabase;
}

const CV64_ModelDatabaseEntry* CV64_FindModelByID(uint32_t modelID) {
    uint32_t count = sizeof(g_modelDatabase) / sizeof(g_modelDatabase[0]);
    for (uint32_t i = 0; i < count; i++) {
        if (g_modelDatabase[i].modelID == modelID) {
            return &g_modelDatabase[i];
        }
    }
    return NULL;
}

const CV64_ModelDatabaseEntry* CV64_FindModelByName(const char* name) {
    if (!name) {
        return NULL;
    }
    
    uint32_t count = sizeof(g_modelDatabase) / sizeof(g_modelDatabase[0]);
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(g_modelDatabase[i].name, name) == 0) {
            return &g_modelDatabase[i];
        }
    }
    return NULL;
}
