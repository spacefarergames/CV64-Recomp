/**
 * @file cv64_model_database.h
 * @brief CV64 Model Database - Known model addresses from decomp project
 * 
 * This file contains known model and texture addresses from the
 * Castlevania 64 decomp project. These addresses are for the USA v1.2 ROM.
 * 
 * Reference: https://github.com/blazkowolf/cv64
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_MODEL_DATABASE_H
#define CV64_MODEL_DATABASE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Model database entry
typedef struct {
    uint32_t modelID;
    const char* name;
    uint32_t romOffset;          // Offset in ROM file
    uint32_t dataSize;           // Size of model data
    uint32_t displayListOffset;  // Display list offset
    uint32_t vertexOffset;       // Vertex data offset
    uint32_t textureOffset;      // Texture data offset (if separate)
    uint32_t textureCount;       // Number of textures
    uint8_t modelType;           // CV64_ModelType
} CV64_ModelDatabaseEntry;

// Known model addresses from CV64 decomp (USA v1.2)
// These addresses are based on the decompilation project

// Character models
#define CV64_MODEL_REINHARDT_BASE       0x01000000  // Reinhardt base model
#define CV64_MODEL_REINHARDT_WHIP       0x01010000  // Reinhardt with whip
#define CV64_MODEL_CARRIE_BASE          0x01020000  // Carrie base model
#define CV64_MODEL_CARRIE_ORB           0x01030000  // Carrie with orb

// Enemy models
#define CV64_MODEL_SKELETON             0x02000000  // Skeleton enemy
#define CV64_MODEL_SKELETON_KNIGHT      0x02010000  // Skeleton knight
#define CV64_MODEL_WEREWOLF             0x02020000  // Werewolf
#define CV64_MODEL_VAMPIRE              0x02030000  // Vampire
#define CV64_MODEL_LIZARDMAN            0x02040000  // Lizard-man
#define CV64_MODEL_FLEAMAN              0x02050000  // Flea-man
#define CV64_MODEL_MIST                 0x02060000  // Mist (shape-shifter)

// Boss models
#define CV64_MODEL_DRACULA_BOSS         0x03000000  // Dracula boss form
#define CV64_MODEL_DEATH                0x03010000  // Death boss
#define CV64_MODEL_ACTRISE              0x03020000  // Actrise boss
#define CV64_MODEL_ROSA                 0x03030000  // Rosa boss
#define CV64_MODEL_RENON                0x03040000  // Renon boss form
#define CV64_MODEL_BEHEMOTH             0x03050000  // Behemoth boss

// NPC models
#define CV64_MODEL_ROSA_NPC             0x04000000  // Rosa (NPC form)
#define CV64_MODEL_MALUS                0x04010000  // Malus
#define CV64_MODEL_VINCENT              0x04020000  // Vincent
#define CV64_MODEL_RENON_NPC            0x04030000  // Renon (NPC form)
#define CV64_MODEL_VILLAGER             0x04040000  // Generic villager

// Item/Object models
#define CV64_MODEL_JEWEL_RED            0x05000000  // Red jewel
#define CV64_MODEL_JEWEL_BLUE           0x05010000  // Blue jewel
#define CV64_MODEL_JEWEL_GREEN          0x05020000  // Green jewel
#define CV64_MODEL_POTION               0x05030000  // Healing potion
#define CV64_MODEL_ROAST_BEEF           0x05040000  // Roast beef
#define CV64_MODEL_KEY                  0x05050000  // Key item
#define CV64_MODEL_SPECIAL_ITEM         0x05060000  // Special items

// Weapon models
#define CV64_MODEL_WHIP                 0x06000000  // Whip
#define CV64_MODEL_HOLY_WATER           0x06010000  // Holy water
#define CV64_MODEL_CROSS                0x06020000  // Cross
#define CV64_MODEL_AXE                  0x06030000  // Axe
#define CV64_MODEL_KNIFE                0x06040000  // Throwing knife

// Environmental models
#define CV64_MODEL_TORCH                0x07000000  // Torch
#define CV64_MODEL_CANDLE               0x07010000  // Candle
#define CV64_MODEL_DOOR                 0x07020000  // Door
#define CV64_MODEL_GATE                 0x07030000  // Gate
#define CV64_MODEL_BREAKABLE_WALL       0x07040000  // Breakable wall
#define CV64_MODEL_LEVER                0x07050000  // Lever/switch

/**
 * @brief Get the model database
 * @param outCount Output count of models in database
 * @return Pointer to model database array
 */
const CV64_ModelDatabaseEntry* CV64_GetModelDatabase(uint32_t* outCount);

/**
 * @brief Find model entry by ID
 * @param modelID Model ID to find
 * @return Pointer to model entry or NULL if not found
 */
const CV64_ModelDatabaseEntry* CV64_FindModelByID(uint32_t modelID);

/**
 * @brief Find model entry by name
 * @param name Model name to find
 * @return Pointer to model entry or NULL if not found
 */
const CV64_ModelDatabaseEntry* CV64_FindModelByName(const char* name);

#ifdef __cplusplus
}
#endif

#endif // CV64_MODEL_DATABASE_H
