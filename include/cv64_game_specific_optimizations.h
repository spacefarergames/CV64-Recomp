/**
 * @file cv64_game_specific_optimizations.h
 * @brief Castlevania 64 - Game-Specific Performance Optimization API
 * 
 * Leverages CV64 decomp knowledge to make targeted optimizations
 * based on known memory addresses and game systems.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_GAME_SPECIFIC_OPTIMIZATIONS_H
#define CV64_GAME_SPECIFIC_OPTIMIZATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Types
 *===========================================================================*/

/**
 * Current game state
 */
typedef enum {
    CV64_GAME_STATE_UNKNOWN = 0,
    CV64_GAME_STATE_MENU,
    CV64_GAME_STATE_GAMEPLAY,
    CV64_GAME_STATE_CUTSCENE,
    CV64_GAME_STATE_LOADING,
    CV64_GAME_STATE_PAUSED,
    CV64_GAME_STATE_INVENTORY,
    CV64_GAME_STATE_MAP_SCREEN
} CV64_GameStateType;

/**
 * Map identifiers (from verified CV64 decomp - matches cv64_memory_hook.cpp)
 * See src/cv64_memory_hook.cpp s_mapNames[] for full list
 */
typedef enum {
    CV64_GAME_MAP_UNKNOWN = -1,
    CV64_GAME_MAP_FOREST = 0x00,              // MORI - Forest of Silence
    CV64_GAME_MAP_CASTLE_WALL_MAIN = 0x01,    // TOU - Castle Wall main
    CV64_GAME_MAP_CASTLE_WALL_TOWER = 0x02,   // TOUOKUJI - Castle Wall tower
    CV64_GAME_MAP_VILLA_FRONT = 0x03,         // NAKANIWA - Villa front yard
    CV64_GAME_MAP_VILLA_HALLWAY = 0x04,       // HIDE - Villa hallway
    CV64_GAME_MAP_VILLA_MAZE = 0x05,          // TOP - Villa maze/storeroom
    CV64_GAME_MAP_TUNNEL = 0x06,              // EAR - Tunnel
    CV64_GAME_MAP_WATERWAY = 0x07,            // MIZU - Underground Waterway
    CV64_GAME_MAP_CASTLE_CENTER_MACHINE = 0x08, // MACHINE_TOWER
    CV64_GAME_MAP_CASTLE_CENTER = 0x09,       // MEIRO - Castle Center main
    CV64_GAME_MAP_CLOCK_TOWER = 0x0A,         // TOKEITOU_NAI - Clock Tower interior
    CV64_GAME_MAP_CLOCK_TOWER_APPROACH = 0x0B, // TOKEITOU_MAE
    CV64_GAME_MAP_CASTLE_KEEP_B1F = 0x0C,     // HONMARU_B1F
    CV64_GAME_MAP_ROOM_OF_CLOCKS = 0x0D,      // SHOKUDOU
    CV64_GAME_MAP_TOWER_SCIENCE_UPPER = 0x0E,  // SHINDENTOU_UE
    CV64_GAME_MAP_TOWER_SCIENCE_MID = 0x0F,    // SHINDENTOU_NAKA
    CV64_GAME_MAP_DUEL_TOWER = 0x10,          // KETTOUTOU
    CV64_GAME_MAP_ART_TOWER = 0x11,           // MAKAIMURA
} CV64_GameMapID;

/**
 * Game-specific optimization statistics
 */
typedef struct {
    CV64_GameStateType currentState;   ///< Current game state
    int currentMap;                    ///< Current map ID
    uint32_t framesSinceStateChange;   ///< Frames since last state change
    
    // Optimization stats
    uint32_t particlesSkipped;         ///< Particles skipped
    uint32_t entitiesSkipped;          ///< Entities skipped
    uint32_t audioTasksSkipped;        ///< Audio tasks skipped
    uint32_t inputChecksSkipped;       ///< Input checks skipped
    uint32_t rdpCommandsSkipped;       ///< RDP commands skipped
    
    // Game state info
    uint32_t entityCount;              ///< Current entity count
    uint32_t enemyCount;               ///< Current enemy count
    uint32_t particleCount;            ///< Current particle count
} CV64_GameOptStats;

/*===========================================================================
 * Initialization
 *===========================================================================*/

/**
 * Initialize game-specific optimizations
 * 
 * @param rdram Pointer to emulated RDRAM
 * @param rdramSize Size of RDRAM (should be 4MB)
 * @return true on success
 */
bool CV64_GameOpt_Initialize(uint8_t* rdram, uint32_t rdramSize);

/**
 * Shutdown game-specific optimizations
 */
void CV64_GameOpt_Shutdown(void);

/**
 * Update optimizations (call once per frame)
 * Reads game state from memory and adjusts optimizations
 */
void CV64_GameOpt_Update(void);

/*===========================================================================
 * Graphics Optimizations
 *===========================================================================*/

/**
 * Check if particle should be skipped
 * 
 * @param particleIndex Index of particle
 * @return true if should skip rendering
 */
bool CV64_GameOpt_ShouldSkipParticle(uint32_t particleIndex);

/**
 * Check if entity should be skipped
 * 
 * @param entityIndex Index of entity
 * @param distanceFromPlayer Distance from player in game units
 * @return true if should skip rendering
 */
bool CV64_GameOpt_ShouldSkipEntity(uint32_t entityIndex, float distanceFromPlayer);

/**
 * Get recommended shadow quality level
 * 
 * @return 1 = low quality, 2 = high quality
 */
int CV64_GameOpt_GetShadowQuality(void);

/**
 * Get recommended fog quality level
 * 
 * @return 1 = low quality, 2 = high quality
 */
int CV64_GameOpt_GetFogQuality(void);

/*===========================================================================
 * Audio/RSP Optimizations
 *===========================================================================*/

/**
 * Check if reverb processing should be skipped
 * 
 * @return true if should skip reverb
 */
bool CV64_GameOpt_ShouldSkipReverbProcessing(void);

/**
 * Get recommended audio quality level
 * 
 * @return 1 = low quality, 2 = high quality
 */
int CV64_GameOpt_GetAudioQuality(void);

/**
 * Check if BGM should be prioritized over SFX
 * 
 * @return true if BGM should have priority
 */
bool CV64_GameOpt_ShouldPrioritizeBGM(void);

/*===========================================================================
 * Input Optimizations
 *===========================================================================*/

/**
 * Check if input should be polled this frame
 * 
 * @param frameNumber Current frame number
 * @return true if should poll input
 */
bool CV64_GameOpt_ShouldPollInput(uint32_t frameNumber);

/**
 * Get input polling interval (in frames)
 * 
 * @return Number of frames between polls
 */
uint32_t CV64_GameOpt_GetInputPollInterval(void);

/*===========================================================================
 * RDP Optimizations
 *===========================================================================*/

/**
 * Check if complex blending should be skipped
 * 
 * @return true if should skip complex blending
 */
bool CV64_GameOpt_ShouldSkipComplexBlending(void);

/**
 * Check if texture quality should be reduced
 * 
 * @return true if should reduce quality
 */
bool CV64_GameOpt_ShouldReduceTextureQuality(void);

/**
 * Check if Z-buffer reads should be skipped
 * 
 * @return true if should skip Z-buffer reads
 */
bool CV64_GameOpt_ShouldSkipZBufferRead(void);

/**
 * Get texture LOD (Level of Detail) bias
 * Higher values = lower resolution textures
 * 
 * @return LOD bias (0 = no bias, 1 = +1 mip level, 2 = +2 mip levels)
 */
int CV64_GameOpt_GetTextureLODBias(void);

/**
 * Check if geometry should be decimated for distant objects
 * 
 * @param distanceFromCamera Distance of object from camera
 * @return true if should use lower poly models
 */
bool CV64_GameOpt_ShouldDecimateGeometry(float distanceFromCamera);

/*===========================================================================
 * Statistics
 *===========================================================================*/

/**
 * Get game-specific optimization statistics
 * 
 * @param stats Output structure for statistics
 */
void CV64_GameOpt_GetStats(CV64_GameOptStats* stats);

/**
 * Reset statistics
 */
void CV64_GameOpt_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_GAME_SPECIFIC_OPTIMIZATIONS_H */
