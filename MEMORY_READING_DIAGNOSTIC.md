# Memory Reading Diagnostic Guide

## Problem
Game-specific optimizations may not be reading emulator memory correctly, causing optimizations to not activate.

## How Memory Reading Works

### 1. **RDRAM Pointer Chain**
```
Mupen64Plus Core ? DebugMemGetPointer(M64P_DBG_PTR_RDRAM)
                 ?
CV64_M64P_Static_GetRDRAMPointer() (stores in s_staticRdram)
                 ?
CV64_M64P_GetRDRAMPointer() (public API)
                 ?
CV64_GameOpt_Initialize(rdram, size) (game-specific optimizations)
                 ?
s_rdram (local pointer in game_specific_optimizations.cpp)
```

### 2. **Memory Address Layout**
```
N64 Virtual Address Space:
0x80000000 - 0x807FFFFF : 8MB RDRAM (4MB on real N64)

Our Address Constants:
CV64_RAM_BASE = 0x80000000
CV64_RDRAM_SIZE = 0x400000 (4MB)

Reading Address 0x800F8CA0:
? Offset = 0x800F8CA0 - 0x80000000 = 0x000F8CA0
? Read: s_rdram[0x000F8CA0]
```

### 3. **Memory Read Functions**
```cpp
ReadU8(0x800F8CA0)  ? s_rdram[0x000F8CA0]
ReadU16(0x800F8CA0) ? (s_rdram[0x000F8CA0] << 8) | s_rdram[0x000F8CA1]
ReadU32(0x800F8CA0) ? Big-endian 32-bit read
ReadFloat(addr)     ? memcpy from ReadU32
```

---

## Diagnostic Checks

### ? Check 1: RDRAM Pointer Valid
**When:** On initialization
**What to look for:**
```
[CV64_GameOpt] RDRAM pointer: 0x00007FF123456789, Test read at 0x80000000 = 0x12345678
```

**If you see:**
- `RDRAM pointer: 0x0000000000000000` ? **FAILED** - Pointer is NULL
- `Test read = 0x00000000` ? **WARNING** - Memory might not be initialized yet

**Fix:**
- RDRAM pointer must be non-null
- Game must be running (not just ROM loaded)
- Call after emulation starts

---

### ? Check 2: Initialization Success
**When:** On game start
**What to look for:**
```
==================================================
[CV64_GameOpt] Game-Specific Optimizations Init
==================================================
[CV64_GameOpt] Using CV64 decomp knowledge for:
  ? Graphics optimization per game state
  ? Audio/RSP optimization per scene
  ? Input polling reduction
  ? RDP command optimization
  ? Map-specific optimizations
==================================================
```

**If you see:**
- `ERROR: Invalid RDRAM pointer=0x0 or size=0` ? **FAILED** - Not initialized
- Nothing at all ? **FAILED** - Function never called

---

### ? Check 3: State Detection Working
**When:** During gameplay (every 60 frames = 1 second)
**What to look for:**
```
[CV64_GameOpt] State=2 Map=2 Entities=35 Enemies=12 Particles=87 Fog=1
```

**Decode values:**
- State: 0=Unknown, 1=Menu, 2=Gameplay, 3=Cutscene, 4=Loading, 5=Paused
- Map: -1=Unknown, 0=Title, 1=Forest Intro, 2=Forest Main, 3=Castle Wall, etc.
- Entities: Active entity count (trees, items, decorations)
- Enemies: Active enemy count
- Particles: Active particle count (fog, effects)
- Fog: 0=disabled, 1=enabled

**If you see:**
- `State=0 Map=-1 Entities=0 Enemies=0 Particles=0 Fog=0` ? **WARNING** - All zeros means not reading correctly
- Nothing at all ? **FAILED** - UpdateGameState() not being called or s_rdram is null

---

### ? Check 4: Map Detection
**When:** On map change
**What to look for:**
```
[CV64_GameOpt] Map changed: 0 -> 2
```

**Common maps:**
- 0 ? Title Screen
- 1 ? Forest of Silence (Intro)
- 2 ? Forest of Silence (Main) ? **THIS IS THE PROBLEM MAP**
- 3 ? Castle Wall
- 4 ? Villa

**If you see:**
- Map stays at -1 or 0 ? **FAILED** - Not detecting map changes
- Map changes but stays at wrong value ? **WARNING** - Memory addresses may be incorrect

---

### ? Check 5: Forest Optimizations Active
**When:** In Forest of Silence
**What to look for in debug output:**
```
[CV64_GameOpt] Map changed: 1 -> 2
[CV64_GameOpt] State=2 Map=2 Entities=45 Enemies=15 Particles=157 Fog=1
```

**Expected behavior when Map=2:**
- Particle count should be high (100-200)
- Fog should be enabled (=1)
- Entities should be moderate (30-50)

**Forest-specific optimizations should activate:**
- Particle skipping: Keep only 25-33% of particles
- Fog quality: Force to 0 (minimal)
- Shadow quality: Force to 0 (baked only)
- Texture LOD: +2 (4x smaller textures)

---

## Common Issues & Solutions

### ? Issue 1: "RDRAM pointer: 0x0000000000000000"
**Cause:** RDRAM not available yet when CV64_GameOpt_Initialize() is called

**Solution:**
```cpp
// In OnFrameCallback() - initialization happens AFTER emulation starts
if (!CV64_Memory_IsInitialized()) {
    void* rdram = CV64_M64P_GetRDRAMPointer();
    if (rdram != NULL) {
        // Initialize game-specific optimizations
        CV64_GameOpt_Initialize((uint8_t*)rdram, 0x400000);
    }
}
```

---

### ? Issue 2: "State=0 Map=-1" (All Values Zero)
**Cause:** Memory addresses from CV64 decomp are incorrect or game version mismatch

**Check:**
1. ROM version must be **Castlevania 64 (U) V1.2**
2. Addresses are for NTSC-U version only
3. PAL or Japanese versions have different memory layout

**Known addresses (from decomp):**
```cpp
0x800F8CA0 - Game state flags
0x80389960 - Current map ID
0x800F8CB0 - Cutscene flag
0x800F8CB4 - Loading flag
0x8038D200 - Particle count
0x8037FFF4 - Enemy count
```

---

### ? Issue 3: No Debug Output at All
**Cause:** OutputDebugStringA() not visible in your debugger

**Solution:**
- Use **Visual Studio Debugger** (F5, not Ctrl+F5)
- Open **Output Window** (View ? Output)
- OR use **DebugView** from Sysinternals
- OR add file logging instead

---

### ? Issue 4: Memory Reads Return Garbage
**Cause:** Big-endian vs Little-endian issue

**Check:**
```cpp
// N64 is BIG ENDIAN
ReadU32(addr) = (s_rdram[offset] << 24) | (s_rdram[offset+1] << 16) | ...
                 ^^^^^^^^^^^^^^^^^ High byte first!
```

**If values look byte-swapped:**
- Check endianness of memory reads
- Verify DebugMemGetPointer() returns correct format

---

## Manual Testing Procedure

### Step 1: Launch with Debugger
```
1. Open Visual Studio
2. Press F5 (Debug mode)
3. Wait for game to load
```

### Step 2: Check Output Window
```
View ? Output ? Show output from: Debug
```

### Step 3: Look for Initialization
```
Search for: "[CV64_GameOpt]"
```

### Step 4: Start Game
```
1. Press SPACE to start emulation
2. Load Castlevania 64 ROM
3. Get to Forest of Silence
```

### Step 5: Verify Memory Reading
```
Every 1 second you should see:
[CV64_GameOpt] State=2 Map=2 Entities=... Particles=...
```

### Step 6: Verify Forest Optimizations
```
When Map=2 (Forest Main):
- Particle count should reduce from 150+ to ~40-50 rendered
- FPS should jump from 30 FPS to 55-60 FPS
```

---

## Expected Debug Output (Normal Operation)

### Initialization (On Game Start):
```
[CV64] Memory hook system initialized via frame callback
[CV64_GameOpt] RDRAM pointer: 0x00007FF1A2B3C4D5, Test read at 0x80000000 = 0x80371240
==================================================
[CV64_GameOpt] Game-Specific Optimizations Init
==================================================
[CV64_GameOpt] Using CV64 decomp knowledge for:
  ? Graphics optimization per game state
  ? Audio/RSP optimization per scene
  ? Input polling reduction
  ? RDP command optimization
  ? Map-specific optimizations
==================================================
[CV64] Game-specific optimizations initialized
```

### During Gameplay (Every 1 Second):
```
[CV64_GameOpt] State=2 Map=2 Entities=42 Enemies=8 Particles=143 Fog=1
```

### On Map Change:
```
[CV64_GameOpt] Map changed: 1 -> 2
[CV64_GameOpt] State changed: 1 -> 2
```

### On Shutdown:
```
[CV64_GameOpt] Shutting down game-specific optimizations
[CV64_GameOpt] Final Statistics:
  Particles Skipped: 8432/11234 (75%)
  Entities Culled: 234/567 (41%)
  Audio Tasks Skipped: 432/1234 (35%)
  Input Checks Skipped: 12345/15000 (82%)
  RDP Commands Optimized: 567/890 (64%)
```

---

## Quick Diagnostic Commands

### Check if pointer is valid:
```cpp
void* rdram = CV64_M64P_GetRDRAMPointer();
char msg[128];
sprintf_s(msg, "RDRAM: %p\n", rdram);
OutputDebugStringA(msg);
```

### Manual memory read test:
```cpp
if (s_rdram) {
    uint32_t testValue = ReadU32(0x80000000);
    char msg[128];
    sprintf_s(msg, "Read test: 0x%08X\n", testValue);
    OutputDebugStringA(msg);
}
```

### Force debug output every frame:
```cpp
// In UpdateGameState(), comment out the 60-frame throttle:
// static uint32_t debugFrameCounter = 0;
// if (++debugFrameCounter >= 60) {
//     debugFrameCounter = 0;

// Just always log:
char debugMsg[512];
sprintf_s(debugMsg, ...);
OutputDebugStringA(debugMsg);
```

---

## Success Criteria

? **All checks passing:**
1. RDRAM pointer is non-null
2. Test read returns non-zero value
3. State changes detected
4. Map changes detected  
5. Entity/particle counts are reasonable (>0)
6. Forest optimizations activate (Map=2)
7. FPS improves in Forest from ~30 to ~58-60

? **If all checks pass but still slow:**
- Optimizations ARE reading memory correctly
- Problem is somewhere else (RDP, threading, GlideN64, etc.)
- Check other performance systems

? **If checks failing:**
- Memory reading is broken
- Fix initialization sequence
- Verify ROM version
- Check memory addresses

---

## Contact/Debug

If memory reading still doesn't work after all checks:

1. **Capture full debug log** (Output window ? Save As)
2. **Note ROM version** (U 1.0? 1.2? PAL? JP?)
3. **Check emulation state** (Is game actually running?)
4. **Verify mupen64plus core version** (Some versions changed memory API)

**The debug output added in this fix will help identify the exact problem!**
