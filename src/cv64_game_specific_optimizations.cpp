/**
 * @file cv64_game_specific_optimizations.cpp
 * @brief Castlevania 64 - Game-Specific Performance Optimizations
 * 
 * This file uses knowledge from the CV64 decomp project to make targeted
 * optimizations based on known memory addresses, game states, and systems.
 * 
 * DECOMP-BASED OPTIMIZATIONS:
 * 1. Graphics - Skip rendering for specific game states
 * 2. RSP - Optimize audio based on current BGM/SFX
 * 3. Input - Reduce polling during cutscenes/menus
 * 4. RDP - Optimize known expensive effects
 * 5. Sound - Pre-cache known audio sequences
 * 
 * References:
 * - CV64 decomp: https://github.com/blazkowolf/cv64
 * - Memory map documentation
 * - Game system analysis
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_game_specific_optimizations.h"
#include "../include/cv64_performance_optimizations.h"
#include "../include/cv64_rdp_optimizations.h"
#include "../include/cv64_memory_hook.h"

#include <Windows.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

/*===========================================================================
 * CV64 Decomp - Known Memory Addresses (from decomp project)
 *===========================================================================*/

// Base addresses (NTSC-U 1.0)
#define CV64_RAM_BASE           0x80000000
#define CV64_RDRAM_SIZE         0x800000   // 8MB RDRAM (CV64 uses expansion pak)

// Game state addresses (from verified decomp CASTLEVANIA.sym)
#define CV64_ADDR_GAME_STATE    0x800D7980  // inPlayerGameplayLoop (verified from CASTLEVANIA.sym)
#define CV64_ADDR_MAP_ID        0x80389EE0  // Current map ID (sym: map_ID at system_work + 0x26428) — s16
#define CV64_ADDR_CAMERA_MODE   0x80342230  // Camera mode (sym: cameraMgr)
#define CV64_ADDR_PAUSE_STATE   0x80389EEE  // Pause menu state (sym: current_opened_menu) — u16
#define CV64_ADDR_CUTSCENE_FLAG 0x80389EF8  // Cutscene ID (sym: cutscene_ID) — u16
#define CV64_ADDR_LOADING_FLAG  0x80389EE8  // Level loading (sym: map_fade_out_time) — u16

// Graphics system addresses (verified from CASTLEVANIA.sym)
#define CV64_ADDR_FOG_NEAR      0x80387AE0  // Fog near plane (sym: fog_distance_start)
#define CV64_ADDR_FOG_FAR       0x80387AE2  // Fog far plane (sym: fog_distance_end)
#define CV64_ADDR_FOG_COLOR     0x80387AC8  // Fog color (sym: map_fog_color) — u32

// Input system addresses
#define CV64_ADDR_CONTROLLER_DATA 0x80363AB8 // Controller input buffer (system_work)

/*===========================================================================
 * Game State Detection
 *===========================================================================*/

static struct {
CV64_GameStateType currentState;
CV64_GameMapID currentMap;
    bool inCutscene;
    bool isLoading;
    bool isPaused;
    bool isSaveLoadMenuActive;    // ADDED: Track save/load menu state
    uint32_t framesSinceStateChange;
    uint32_t saveLoadCooldown;    // ADDED: Cooldown after save/load operations
    uint32_t entityCount;
    uint32_t enemyCount;
    uint32_t particleCount;
    bool fogEnabled;
    uint8_t currentBGM;
} s_gameState = {
    .currentState = CV64_GAME_STATE_UNKNOWN,
    .currentMap = CV64_GAME_MAP_UNKNOWN,
    .inCutscene = false,
    .isLoading = false,
    .isPaused = false,
    .isSaveLoadMenuActive = false,
    .framesSinceStateChange = 0,
    .saveLoadCooldown = 60,  // Start with cooldown to allow game to initialize
    .entityCount = 0,
    .enemyCount = 0,
    .particleCount = 0,
    .fogEnabled = false,
    .currentBGM = 0
};

static uint8_t* s_rdram = nullptr;

/*===========================================================================
 * Memory Access Helpers
 *===========================================================================*/

static inline bool IsValidAddress(uint32_t addr, uint32_t size = 1) {
    if (addr < CV64_RAM_BASE) return false;
    if (s_rdram == nullptr) return false;

    uint32_t offset = addr - CV64_RAM_BASE;
    // Check that both start and end of access are within bounds
    if (offset >= CV64_RDRAM_SIZE) return false;
    if (offset + size > CV64_RDRAM_SIZE) return false;

    return true;
}

static inline uint8_t ReadU8(uint32_t addr) {
    if (!IsValidAddress(addr, 1)) return 0;
    uint32_t offset = addr - CV64_RAM_BASE;
    return s_rdram[offset ^ 3];
}

static inline uint16_t ReadU16(uint32_t addr) {
    if (!IsValidAddress(addr, 2)) return 0;
    uint32_t offset = addr - CV64_RAM_BASE;
    return *(uint16_t*)(s_rdram + (offset ^ 2));
}

static inline uint32_t ReadU32(uint32_t addr) {
    if (!IsValidAddress(addr, 4)) return 0;
    uint32_t offset = addr - CV64_RAM_BASE;
    return *(uint32_t*)(s_rdram + offset);
}

static inline float ReadFloat(uint32_t addr) {
    uint32_t bits = ReadU32(addr);
    float result;
    memcpy(&result, &bits, sizeof(float));
    return result;
}

/*===========================================================================
 * Game State Detection
 *===========================================================================*/

static void UpdateGameState() {
/* SAFETY CHECK: Verify RDRAM is valid and addresses are accessible
 * During menus and loading screens, game data may not be initialized */
if (!s_rdram) {
    s_gameState.currentState = CV64_GAME_STATE_UNKNOWN;
    s_gameState.currentMap = CV64_GAME_MAP_UNKNOWN;
    return;
}
    
/* Handle save/load cooldown - skip all updates during this period */
if (s_gameState.saveLoadCooldown > 0) {
    s_gameState.saveLoadCooldown--;
    s_gameState.framesSinceStateChange++;
    return;  // Skip all state reading during cooldown
}
    
CV64_GameStateType oldState = s_gameState.currentState;
CV64_GameMapID oldMap = s_gameState.currentMap;
    
/* Check inPlayerGameplayLoop — 0 means we're in menus/loading,
 * but we should still read map_ID and other state for detection */
uint32_t gameplayActive = ReadU32(CV64_ADDR_GAME_STATE);
    
// Read current state from memory — use correct sizes per decomp types
s_gameState.currentMap = (CV64_GameMapID)(int16_t)ReadU16(CV64_ADDR_MAP_ID);
s_gameState.inCutscene = ReadU16(CV64_ADDR_CUTSCENE_FLAG) != 0;  // cutscene_ID is u16
s_gameState.isLoading = ReadU16(CV64_ADDR_LOADING_FLAG) != 0;    // map_fade_out_time is u16
s_gameState.isPaused = ReadU16(CV64_ADDR_PAUSE_STATE) != 0;      // current_opened_menu is u16
    
/* CRITICAL: Detect save/load menu states
 * Reading memory during save/load operations causes freezes */
uint16_t pauseMenuState = ReadU16(CV64_ADDR_PAUSE_STATE);  // current_opened_menu is u16
bool wasSaveLoadActive = s_gameState.isSaveLoadMenuActive;
s_gameState.isSaveLoadMenuActive = (pauseMenuState >= 0x10 && pauseMenuState <= 0x30);
    
// Entering save/load menu - set cooldown
if (s_gameState.isSaveLoadMenuActive && !wasSaveLoadActive) {
    char debugMsg[256];
    sprintf_s(debugMsg, "[CV64_GameOpt] SAFETY: Entering save/load menu (state=0x%04X), setting cooldown\n", pauseMenuState);
    OutputDebugStringA(debugMsg);
    s_gameState.saveLoadCooldown = 120;  // 2 seconds at 60 FPS
}
    
// Exiting save/load menu - set extended cooldown
if (!s_gameState.isSaveLoadMenuActive && wasSaveLoadActive) {
    OutputDebugStringA("[CV64_GameOpt] SAFETY: Exited save/load menu, setting EXTENDED cooldown\n");
    s_gameState.saveLoadCooldown = 180;  // 3 seconds - memory needs time to stabilize
}
    
// If loading flag is set, also set cooldown
if (s_gameState.isLoading) {
    s_gameState.saveLoadCooldown = 120;  // Reset cooldown while loading
}
    
// Determine state — use gameplayActive flag as additional signal
if (s_gameState.isLoading) {
    s_gameState.currentState = CV64_GAME_STATE_LOADING;
} else if (s_gameState.isPaused) {
    s_gameState.currentState = CV64_GAME_STATE_PAUSED;
} else if (s_gameState.inCutscene) {
    s_gameState.currentState = CV64_GAME_STATE_CUTSCENE;
} else if (gameplayActive == 0) {
    s_gameState.currentState = CV64_GAME_STATE_MENU;
} else {
    s_gameState.currentState = CV64_GAME_STATE_GAMEPLAY;
}
    
    // Track state changes
    if (oldState != s_gameState.currentState) {
        s_gameState.framesSinceStateChange = 0;
        
        // DEBUG: Log state changes
        char debugMsg[256];
        sprintf_s(debugMsg, "[CV64_GameOpt] State changed: %d -> %d\n", oldState, s_gameState.currentState);
        OutputDebugStringA(debugMsg);
    } else {
        s_gameState.framesSinceStateChange++;
    }
    
    // DEBUG: Log map changes
    if (oldMap != s_gameState.currentMap) {
        char debugMsg[256];
        sprintf_s(debugMsg, "[CV64_GameOpt] Map changed: %d -> %d\n", oldMap, s_gameState.currentMap);
        OutputDebugStringA(debugMsg);
    }
    
    /* Entity/enemy/particle counts: No verified decomp addresses exist for these.
     * Keep them at 0 — they are not used for critical logic. */
    s_gameState.entityCount = 0;
    s_gameState.enemyCount = 0;
    s_gameState.particleCount = 0;

    // Read fog color (u32) — non-zero means fog is active
    s_gameState.fogEnabled = ReadU32(CV64_ADDR_FOG_COLOR) != 0;
    s_gameState.currentBGM = 0;  // No verified BGM address in decomp
    
    // DEBUG: Log every 60 frames (once per second at 60 FPS)
    // Note: Only in debug builds to avoid performance cost
#ifdef _DEBUG
    static uint32_t debugFrameCounter = 0;
    if (++debugFrameCounter >= 60) {
        debugFrameCounter = 0;
        char debugMsg[512];
        sprintf_s(debugMsg, 
            "[CV64_GameOpt] State=%d Map=%d Entities=%u Enemies=%u Particles=%u Fog=%d\n",
            s_gameState.currentState,
            s_gameState.currentMap,
            s_gameState.entityCount,
            s_gameState.enemyCount,
            s_gameState.particleCount,
            s_gameState.fogEnabled);
        OutputDebugStringA(debugMsg);
    }
#endif
}

/*===========================================================================
 * Graphics Optimizations (Game-Specific)
 *===========================================================================*/

static struct {
    bool skipParticles;
    bool reduceFogQuality;
    bool skipDistantEnemies;
    bool reduceShadowQuality;
    uint32_t particlesSkipped;
    uint32_t entitiesSkipped;
} s_graphicsOpt = {};

/**
 * Optimize graphics based on current game state
 */
void CV64_GameOpt_OptimizeGraphics() {
    s_graphicsOpt.skipParticles = false;
    s_graphicsOpt.reduceFogQuality = false;
    s_graphicsOpt.skipDistantEnemies = false;
    s_graphicsOpt.reduceShadowQuality = false;
    
    // NOTE: Graphics optimizations disabled - they require GlideN64 modification
    // All flags remain false (safe defaults)
}

bool CV64_GameOpt_ShouldSkipParticle(uint32_t particleIndex) {
    // DISABLED: This optimization requires GlideN64 modification
    // Returning false = don't skip any particles (safe default)
    return false;
}

bool CV64_GameOpt_ShouldSkipEntity(uint32_t entityIndex, float distanceFromPlayer) {
    // DISABLED: This optimization requires GlideN64 modification
    // Returning false = don't skip any entities (safe default)
    return false;
}

int CV64_GameOpt_GetShadowQuality() {
    // DISABLED: This optimization requires GlideN64 modification
    // Return 2 = high quality (safe default, no visual changes)
    return 2;
}

int CV64_GameOpt_GetFogQuality() {
    // DISABLED: This optimization requires GlideN64 modification
    // Return 2 = high quality (safe default, no visual changes)
    return 2;
}

/*===========================================================================
 * RSP/Audio Optimizations (Game-Specific)
 *===========================================================================*/

static struct {
    bool reduceSFXQuality;
    bool skipReverbDuringCutscene;
    uint32_t sfxQueueSize;
    uint32_t audioTasksSkipped;
} s_audioOpt = {};

/**
 * Optimize audio based on current game state
 * CONSERVATIVE: Only apply minimal, safe optimizations
 */
void CV64_GameOpt_OptimizeAudio() {
    // Reset all flags to safe defaults
    s_audioOpt.reduceSFXQuality = false;
    s_audioOpt.skipReverbDuringCutscene = false;
    
    /* DISABLED: All audio optimizations disabled for stability
     * Audio processing remains at full quality at all times
     * No reverb skipping, no quality reduction
     */
    
    // Leave all flags as false - full quality always
}

bool CV64_GameOpt_ShouldSkipReverbProcessing() {
    return s_audioOpt.skipReverbDuringCutscene;
}

int CV64_GameOpt_GetAudioQuality() {
    if (s_audioOpt.reduceSFXQuality) {
        return 1; // Lower sample rate, mono mixing
    }
    return 2; // High quality
}

bool CV64_GameOpt_ShouldPrioritizeBGM() {
    // Always prioritize BGM during cutscenes
    return s_gameState.currentState == CV64_GAME_STATE_CUTSCENE;
}

/*===========================================================================
 * Input Optimizations (Game-Specific)
 *===========================================================================*/

static struct {
    bool reducePollingRate;
    uint32_t pollInterval;
    uint32_t inputChecksSkipped;
} s_inputOpt = {};

/**
 * Optimize input polling based on current game state
 * CONSERVATIVE: Always poll every frame for maximum responsiveness
 */
void CV64_GameOpt_OptimizeInput() {
    // Reset to safe defaults
    s_inputOpt.reducePollingRate = false;
    s_inputOpt.pollInterval = 1; // Always poll every frame
    
    /* DISABLED: All input polling optimizations disabled
     * Always poll input every frame regardless of game state
     * This ensures maximum responsiveness and prevents any
     * potential issues with missed input
     */
    
    // Leave pollInterval at 1 (every frame) always
}

bool CV64_GameOpt_ShouldPollInput(uint32_t frameNumber) {
    if (!s_inputOpt.reducePollingRate) return true;
    
    if (frameNumber % s_inputOpt.pollInterval == 0) {
        return true;
    }
    
    s_inputOpt.inputChecksSkipped++;
    return false;
}

uint32_t CV64_GameOpt_GetInputPollInterval() {
    return s_inputOpt.pollInterval;
}

/*===========================================================================
 * RDP Optimizations (Game-Specific)
 *===========================================================================*/

static struct {
    bool skipComplexBlending;
    bool reduceTextureQuality;
    bool skipZBufferReads;
    uint32_t rdpCommandsSkipped;
} s_rdpOpt = {};

/**
 * Optimize RDP commands based on current game state
 */
void CV64_GameOpt_OptimizeRDP() {
    s_rdpOpt.skipComplexBlending = false;
    s_rdpOpt.reduceTextureQuality = false;
    s_rdpOpt.skipZBufferReads = false;
    
    // NOTE: RDP optimizations disabled - they require core modification
    // All flags remain false (safe defaults)
}

bool CV64_GameOpt_ShouldSkipComplexBlending() {
    // DISABLED: This optimization requires core modification
    // Returning false = normal blending (safe default)
    return false;
}

bool CV64_GameOpt_ShouldReduceTextureQuality() {
    // DISABLED: This optimization requires GlideN64 modification
    // Returning false = full texture quality (safe default)
    return false;
}

int CV64_GameOpt_GetTextureLODBias() {
    // DISABLED: This optimization requires GlideN64 modification
    // Return 0 = no LOD bias (safe default, full texture resolution)
    return 0;
}

bool CV64_GameOpt_ShouldSkipZBufferRead() {
    // DISABLED: This optimization requires core modification
    // Returning false = normal Z-buffer operation (safe default)
    return false;
}

bool CV64_GameOpt_ShouldDecimateGeometry(float distanceFromCamera) {
    // DISABLED: This optimization requires GlideN64 modification
    // Returning false = full geometry detail (safe default)
    return false;
}

/*===========================================================================
 * Map-Specific Optimizations
 *===========================================================================*/

typedef struct {
    CV64_GameMapID mapId;
    const char* mapName;
    bool hasHeavyFog;
    bool hasLotOfParticles;
    bool hasComplexGeometry;
    bool hasLotOfEnemies;
} CV64_MapOptimization;

static const CV64_MapOptimization s_mapOptimizations[] = {
    { CV64_GAME_MAP_FOREST, "Forest of Silence", true, true, true, true },
    { CV64_GAME_MAP_CASTLE_WALL_MAIN, "Castle Wall", false, false, true, true },
    { CV64_GAME_MAP_CASTLE_WALL_TOWER, "Castle Wall - Tower", false, false, true, false },
    { CV64_GAME_MAP_VILLA_FRONT, "Villa - Front Yard", true, true, false, true },
    { CV64_GAME_MAP_VILLA_HALLWAY, "Villa - Hallway", false, true, true, true },
    { CV64_GAME_MAP_VILLA_MAZE, "Villa - Maze Garden", true, false, false, false },
    { CV64_GAME_MAP_TUNNEL, "Tunnel", true, false, false, false },
    { CV64_GAME_MAP_WATERWAY, "Underground Waterway", true, false, false, true },
    { CV64_GAME_MAP_CASTLE_CENTER_MACHINE, "Machine Tower", false, false, true, false },
    { CV64_GAME_MAP_CASTLE_CENTER, "Castle Center", false, false, true, true },
    { CV64_GAME_MAP_DUEL_TOWER, "Duel Tower", false, false, true, false },
    { CV64_GAME_MAP_ART_TOWER, "Art Tower", false, false, true, false },
};

const CV64_MapOptimization* GetMapOptimization(CV64_GameMapID mapId) {
    for (size_t i = 0; i < sizeof(s_mapOptimizations) / sizeof(s_mapOptimizations[0]); i++) {
        if (s_mapOptimizations[i].mapId == mapId) {
            return &s_mapOptimizations[i];
        }
    }
    return nullptr;
}

void CV64_GameOpt_ApplyMapSpecificOptimizations() {
    /* DISABLED: Map-specific optimizations can cause issues
     * Keep all map optimizations disabled for maximum stability
     */
    const CV64_MapOptimization* mapOpt = GetMapOptimization(s_gameState.currentMap);
    if (!mapOpt) return;
    
    // DO NOT apply map-specific optimizations - keep defaults
    // All optimization flags remain false (safe defaults)
    
    // NOTE: This function is called but does nothing to prevent any
    // potential rendering issues from aggressive optimizations
}

/*===========================================================================
 * Statistics
 *===========================================================================*/

void CV64_GameOpt_GetStats(CV64_GameOptStats* stats) {
    if (!stats) return;
    
    stats->currentState = s_gameState.currentState;
    stats->currentMap = s_gameState.currentMap;
    stats->framesSinceStateChange = s_gameState.framesSinceStateChange;
    
    stats->particlesSkipped = s_graphicsOpt.particlesSkipped;
    stats->entitiesSkipped = s_graphicsOpt.entitiesSkipped;
    stats->audioTasksSkipped = s_audioOpt.audioTasksSkipped;
    stats->inputChecksSkipped = s_inputOpt.inputChecksSkipped;
    stats->rdpCommandsSkipped = s_rdpOpt.rdpCommandsSkipped;
    
    stats->entityCount = s_gameState.entityCount;
    stats->enemyCount = s_gameState.enemyCount;
    stats->particleCount = s_gameState.particleCount;
}

void CV64_GameOpt_ResetStats() {
    s_graphicsOpt.particlesSkipped = 0;
    s_graphicsOpt.entitiesSkipped = 0;
    s_audioOpt.audioTasksSkipped = 0;
    s_inputOpt.inputChecksSkipped = 0;
    s_rdpOpt.rdpCommandsSkipped = 0;
}

/*===========================================================================
 * Per-Frame Update
 *===========================================================================*/

void CV64_GameOpt_Update() {
    // Safety check: Don't update if not initialized
    if (!s_rdram) {
        return;
    }
    
    // Update game state detection
    UpdateGameState();

    /* SAFETY: Skip optimization updates if in save/load state
     * This prevents accessing memory during dangerous operations */
    if (s_gameState.saveLoadCooldown > 0 || 
        s_gameState.isSaveLoadMenuActive ||
        s_gameState.isLoading) {
        return;  // Skip all optimizations during save/load
    }

    /* CORRUPTION FIX: Skip updates during state transitions
     * Map changes and state changes trigger aggressive object culling
     * which causes memory to be freed/reallocated */
    if (s_gameState.framesSinceStateChange < 30) {  // Wait 0.5 seconds after state change
        return;  // Skip optimizations during transition period
    }
    
    // Apply optimizations based on current state
    CV64_GameOpt_OptimizeGraphics();
    CV64_GameOpt_OptimizeAudio();
    CV64_GameOpt_OptimizeInput();
    CV64_GameOpt_OptimizeRDP();
    
    // Apply map-specific optimizations
    CV64_GameOpt_ApplyMapSpecificOptimizations();
}

/*===========================================================================
 * Initialization
 *===========================================================================*/

bool CV64_GameOpt_Initialize(uint8_t* rdram, uint32_t rdramSize) {
    if (!rdram || rdramSize != CV64_RDRAM_SIZE) {
        char errorMsg[256];
        sprintf_s(errorMsg, "[CV64_GameOpt] ERROR: Invalid RDRAM pointer=%p or size=%u (expected %u)\n", 
            rdram, rdramSize, CV64_RDRAM_SIZE);
        OutputDebugStringA(errorMsg);
        return false;
    }
    
    s_rdram = rdram;
    
    // Verify RDRAM is accessible by reading first few bytes
    uint32_t testRead = ReadU32(CV64_RAM_BASE);
    char debugMsg[256];
    sprintf_s(debugMsg, "[CV64_GameOpt] RDRAM pointer: %p, Test read at 0x%08X = 0x%08X\n", 
        s_rdram, CV64_RAM_BASE, testRead);
    OutputDebugStringA(debugMsg);
    
    // Reset state
    memset(&s_gameState, 0, sizeof(s_gameState));
    memset(&s_graphicsOpt, 0, sizeof(s_graphicsOpt));
    memset(&s_audioOpt, 0, sizeof(s_audioOpt));
    memset(&s_inputOpt, 0, sizeof(s_inputOpt));
    memset(&s_rdpOpt, 0, sizeof(s_rdpOpt));
    
    s_gameState.currentState = CV64_GAME_STATE_UNKNOWN;
    s_gameState.currentMap = CV64_GAME_MAP_UNKNOWN;
    
    OutputDebugStringA("==================================================\n");
    OutputDebugStringA("[CV64_GameOpt] Game-Specific Optimizations Init\n");
    OutputDebugStringA("==================================================\n");
    OutputDebugStringA("[CV64_GameOpt] Mode: CONSERVATIVE (All opts disabled)\n");
    OutputDebugStringA("\n");
    OutputDebugStringA("[CV64_GameOpt] ACTIVE features:\n");
    OutputDebugStringA("  ✓ Game state monitoring (safe)\n");
    OutputDebugStringA("  ✓ Map detection (safe)\n");
    OutputDebugStringA("  ✓ Statistics tracking (safe)\n");
    OutputDebugStringA("\n");
    OutputDebugStringA("[CV64_GameOpt] DISABLED optimizations:\n");
    OutputDebugStringA("  ✗ Graphics (disabled for stability)\n");
    OutputDebugStringA("  ✗ Audio/RSP (disabled for stability)\n");
    OutputDebugStringA("  ✗ Input polling reduction (disabled for stability)\n");
    OutputDebugStringA("  ✗ RDP (disabled for stability)\n");
    OutputDebugStringA("  ✗ Particle/Entity skipping (disabled)\n");
    OutputDebugStringA("  ✗ Texture/Shadow/Fog quality (disabled)\n");
    OutputDebugStringA("  ✗ Map-specific optimizations (disabled)\n");
    OutputDebugStringA("\n");
    OutputDebugStringA("[CV64_GameOpt] Running in SAFE MODE for stability\n");
    OutputDebugStringA("==================================================\n");
    
    return true;
}

void CV64_GameOpt_Shutdown() {
    OutputDebugStringA("[CV64_GameOpt] Shutting down game-specific optimizations\n");
    
    // Print final statistics
    CV64_GameOptStats stats;
    CV64_GameOpt_GetStats(&stats);
    
    char buffer[512];
    sprintf_s(buffer, sizeof(buffer),
        "[CV64_GameOpt] Final Statistics:\n"
        "  Particles Skipped: %u\n"
        "  Entities Skipped: %u\n"
        "  Audio Tasks Skipped: %u\n"
        "  Input Checks Skipped: %u\n"
        "  RDP Commands Skipped: %u\n",
        stats.particlesSkipped,
        stats.entitiesSkipped,
        stats.audioTasksSkipped,
        stats.inputChecksSkipped,
        stats.rdpCommandsSkipped
    );
    OutputDebugStringA(buffer);
    
    s_rdram = nullptr;
}



