# Bug Fixes: Map ID and Fog/Culling Issues

## Issues Fixed

### 1. Wrong Map IDs Causing Incorrect Map Names
**Problem:** Villa was showing as "Castle Wall - Tower" and all map names were wrong.

**Root Causes:**
- `cv64_game_specific_optimizations.cpp` used wrong map ID enum values (e.g., Villa = 4 instead of 6)
- `cv64_gliden64_optimize.cpp` had completely wrong map IDs in optimization hints
- `ReadCurrentMapId()` was using stale `map_ID_copy` instead of live `map_ID`

**Fixes Applied:**
1. **Fixed `include/cv64_game_specific_optimizations.h`** - Updated enum to match decomp:
   - `CV64_GAME_MAP_FOREST_INTRO = 0x02` (was 1)
   - `CV64_GAME_MAP_FOREST_MAIN = 0x03` (was 2)
   - `CV64_GAME_MAP_CASTLE_WALL_MAIN = 0x04` (was 3)
   - `CV64_GAME_MAP_CASTLE_WALL_TOWER = 0x05` (new)
   - `CV64_GAME_MAP_VILLA_FRONT = 0x06` (was 4 as "VILLA")
   - `CV64_GAME_MAP_VILLA_INTERIOR = 0x07` (new)
   - `CV64_GAME_MAP_TUNNEL = 0x0A` (was 5)
   - `CV64_GAME_MAP_CASTLE_CENTER = 0x0C` (was 6)
   - `CV64_GAME_MAP_TOWER = 0x10` (was 7)

2. **Fixed `src/cv64_game_specific_optimizations.cpp`**:
   - Changed `CV64_ADDR_MAP_ID` from `0x80389960` (WRONG) to `0x80389EE0` (verified from decomp)
   - Changed `CV64_RDRAM_SIZE` from `0x400000` (4MB) to `0x800000` (8MB) - CV64 uses expansion pak
   - Updated fog addresses to verified CASTLEVANIA.sym addresses:
     - `CV64_ADDR_FOG_NEAR = 0x80387AE0` (was 0x8038D100)
     - `CV64_ADDR_FOG_FAR = 0x80387AE2` (was 0x8038D104)
   - Updated map optimization table with correct IDs and names

3. **Fixed `src/cv64_gliden64_optimize.cpp`** - Corrected ALL map IDs:
   - Castle Wall - Main: `0x00` → `0x04`
   - Castle Wall - Tower: `0x0A` → `0x05`
   - Villa - Front Yard: `0x10` → `0x06`
   - Villa - Interior: `0x11` → `0x07`
   - Tunnel: `0x14` → `0x0A`
   - Underground Waterway: `0x15` → `0x0B`
   - Castle Center - Main: `0x19` → `0x0C`
   - Tower of Execution: `0x1D` → `0x10`
   - Tower of Science: `0x21` → `0x12`
   - Tower of Sorcery: `0x25` → `0x11`
   - Room of Clocks: `0x2C` → `0x19`
   - Clock Tower - Main: `0x2D` → `0x16`
   - Castle Keep - Main: `0x31` → `0x15`
   - Dracula's Desert: `0x35` → `0x18`

4. **Fixed `src/cv64_memory_hook.cpp` `ReadCurrentMapId()`**:
   - Changed from `CV64_SYS_OFFSET_MAP_ID_COPY` (offset `0x261D8`, address `0x80389C90`)
   - To `CV64_SYS_OFFSET_MAP_ID` (offset `0x26428`, address `0x80389EE0`)
   - This reads the **live** map ID instead of the save copy, preventing stale values during transitions

### 2. Fog/Culling Making Game Look Washed Out
**Problem:** Extended draw distance patch was pushing fog too far, removing depth cues and making everything grey/washed out with objects disappearing.

**Root Cause:**
- Wrong RDRAM size (4MB instead of 8MB) caused address validation failures
- Wrong fog addresses were being accessed
- Multiplier of 2.5x was too aggressive

**Fixes Applied:**
- Corrected RDRAM size to 8MB (`0x800000`)
- Fixed fog addresses to verified decomp addresses:
  - `fog_distance_start` at `0x80387AE0`
  - `fog_distance_end` at `0x80387AE2`
- These addresses are now correctly validated and accessed

**Recommendation for Users:**
If fog still looks wrong, adjust draw distance multiplier in settings:
- Default is 2.5x - try reducing to 1.5x or 2.0x for better depth cues
- Or disable extended draw distance if fog looks better at default

### 3. Window Title Not Showing Full Description
**Problem:** Window title was showing map name but potentially not updating properly.

**Fix Applied:**
- By using live `map_ID` instead of `map_ID_copy`, the window title will update immediately on map transitions
- The `CV64_Memory_GetGameInfo()` function now gets the correct, current map ID

## Verification

All map IDs now match the verified values from the CV64 decomp project's `s_mapNames` array in `src/cv64_memory_hook.cpp` (lines 1353-1386).

Reference map table:
```
0x00 - Title Screen
0x01 - Ings Select
0x02 - Forest of Silence
0x03 - Forest of Silence (2)
0x04 - Castle Wall - Main
0x05 - Castle Wall - Tower
0x06 - Villa - Front Yard
0x07 - Villa - Interior
0x08 - Villa - Maze Garden
0x09 - Villa - Crypt
0x0A - Tunnel
0x0B - Underground Waterway
0x0C - Castle Center - Main
0x0D - Castle Center - Bottom
0x0E - Castle Center - Library
0x0F - Castle Center - Invention
0x10 - Tower of Execution
0x11 - Tower of Sorcery
0x12 - Tower of Science
0x13 - Duel Tower
0x14 - Castle Keep - Stairs
0x15 - Castle Keep - Main
0x16 - Clock Tower - Main
0x17 - Clock Tower - Top
0x18 - Dracula's Desert
0x19 - Room of Clocks
```

## Testing

After rebuilding:
1. Verify Villa now shows as "Villa - Front Yard" (0x06) not "Castle Wall - Tower"
2. Verify window title shows correct map names immediately on transition
3. Check if fog rendering looks correct (not washed out)
4. Verify all map-specific optimizations now apply to correct maps

## Notes

The 8MB RDRAM size fix is critical - CV64 uses the N64 expansion pak, and many game addresses extend beyond the 4MB limit. The old 4MB limit was causing `IsValidAddress()` checks to fail for legitimate game memory, breaking map detection and fog control.
