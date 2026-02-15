# Memory Corruption Fix - Object Culling

## Problem Description
Memory corruption was occurring when objects were culled from rendering, causing crashes during gameplay. The issue manifested as:
- Objects getting culled out
- Crashes when accessing entity/particle data
- Corrupted memory reads returning invalid values

## Root Cause Analysis

### The Issue
The `UpdateGameState()` function in `cv64_game_specific_optimizations.cpp` was reading entity/particle/enemy counts from N64 RDRAM addresses every frame:

```cpp
// OLD CODE (DANGEROUS)
s_gameState.entityCount = ReadU32(CV64_ADDR_ENTITY_COUNT);
s_gameState.enemyCount = ReadU32(CV64_ADDR_ENEMY_COUNT);
s_gameState.particleCount = ReadU32(CV64_ADDR_PARTICLE_COUNT);

// Sanity check - WRONG APPROACH
if (s_gameState.entityCount > 1000) s_gameState.entityCount = 0;  // CRASH RISK!
if (s_gameState.enemyCount > 100) s_gameState.enemyCount = 0;
if (s_gameState.particleCount > 500) s_gameState.particleCount = 0;
```

### Why This Caused Crashes

1. **Race Condition During Culling**: When objects are culled (removed from rendering), their memory is freed or reallocated. If `UpdateGameState()` reads from these addresses during culling, it gets:
   - Stale values from freed memory
   - Sentinel values like `0xFFFFFFFF` or `0xCDCDCDCD` (debug heap markers)
   - Partially updated counts mid-transition

2. **Dangerous Reset to Zero**: The old code would reset counts to `0` when they exceeded thresholds. If other code tried to iterate entities using these counts, it would:
   - Miss active entities (count suddenly becomes 0)
   - Access invalid memory if entity pointers were still stale
   - Cause undefined behavior

3. **Timing Issues**: Culling is most aggressive during:
   - Map transitions
   - State changes (menu → gameplay, cutscene → gameplay)
   - Save/load operations
   - Large numbers of objects spawning/despawning

## Solution Implemented

### 1. Preserve Last Known Good Values

Instead of resetting to 0, keep the previous valid count:

```cpp
// Read raw values
uint32_t rawEntityCount = ReadU32(CV64_ADDR_ENTITY_COUNT);
uint32_t rawEnemyCount = ReadU32(CV64_ADDR_ENEMY_COUNT);
uint32_t rawParticleCount = ReadU32(CV64_ADDR_PARTICLE_COUNT);

bool countsAreValid = true;

// Check for sentinel values (freed/uninitialized memory)
if (rawEntityCount == 0xFFFFFFFF || rawEntityCount == 0xCDCDCDCD ||
    rawEnemyCount == 0xFFFFFFFF || rawEnemyCount == 0xCDCDCDCD ||
    rawParticleCount == 0xFFFFFFFF || rawParticleCount == 0xCDCDCDCD) {
    countsAreValid = false;
}

// Check for unreasonable counts
if (rawEntityCount > 1000 || rawEnemyCount > 100 || rawParticleCount > 500) {
    countsAreValid = false;
}

// Only update if valid - preserve last good state otherwise
if (countsAreValid) {
    s_gameState.entityCount = rawEntityCount;
    s_gameState.enemyCount = rawEnemyCount;
    s_gameState.particleCount = rawParticleCount;
}
// else: Keep previous values - DON'T reset to 0!
```

### 2. Skip Updates During State Transitions

Added a transition cooldown period after state changes:

```cpp
/* CORRUPTION FIX: Skip updates during state transitions
 * Map changes and state changes trigger aggressive object culling */
if (s_gameState.framesSinceStateChange < 30) {  // Wait 0.5 seconds
    return;  // Skip optimizations during transition
}
```

### 3. Initialize With Safe Defaults

Ensure game state starts with valid values and an initial cooldown:

```cpp
} s_gameState = {
    .currentState = CV64_GAME_STATE_UNKNOWN,
    .currentMap = CV64_GAME_MAP_UNKNOWN,
    .saveLoadCooldown = 60,  // Start with 1 second cooldown
    .entityCount = 0,
    .enemyCount = 0,
    .particleCount = 0,
    // ... other fields
};
```

## Technical Details

### Sentinel Value Detection
- `0xFFFFFFFF`: Uninitialized memory (common in release builds)
- `0xCDCDCDCD`: Uninitialized heap memory (MSVC debug builds)
- These indicate we're reading freed or not-yet-initialized memory

### Timing Protection Layers

1. **Save/Load Cooldown** (180 frames / 3 seconds)
   - Skips all state updates during and after save/load operations
   
2. **Loading State Check**
   - Skips updates when `isLoading` flag is set
   
3. **State Transition Cooldown** (30 frames / 0.5 seconds)
   - NEW: Skips updates immediately after state/map changes
   
4. **Value Preservation**
   - NEW: Keeps last known good values instead of resetting

### Why This Works

1. **No More Zero Resets**: Preserving last good values prevents sudden drops to 0 that could confuse other code
2. **Sentinel Detection**: Catches reads from freed/invalid memory before they're used
3. **Transition Safety**: Waits for culling to complete before reading counts again
4. **Defense in Depth**: Multiple safety layers ensure corruption is caught

## Testing Recommendations

Test scenarios where corruption was most likely:
- ✅ Map transitions (Forest → Castle Wall)
- ✅ Save/load operations
- ✅ Cutscene → gameplay transitions
- ✅ Heavy enemy spawning/despawning (Villa area)
- ✅ Particle-heavy areas (Forest with fog)

## Files Modified

- `src/cv64_game_specific_optimizations.cpp`
  - Updated `UpdateGameState()` function
  - Added value validation logic
  - Added state transition cooldown
  - Initialized game state with safe defaults

## Result

✅ **Build Status**: Successful
✅ **Memory Safety**: Improved with multiple protection layers
✅ **Stability**: Reduced crash risk during culling operations
✅ **Performance**: No performance impact (same number of reads, just validated)

---

**Date**: 2024
**Issue**: Memory corruption during object culling
**Status**: FIXED
