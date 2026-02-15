# Window Title Flickering Fix

## Problem Description

**Issue**: Window title was alternating rapidly between two states, causing visual flickering.

**Root Cause**: Memory values for player character and difficulty were unstable or being read at the wrong time, causing the title to flip between "in-game" and "menu" states every second.

---

## Solution Implemented

### 1. **Debouncing with Stability Counter**

**Before**:
```cpp
// Read values once per second
if (++counter >= 60) {
    playerChar = ReadByte(addr);
    SetCaption(BuildTitle(playerChar));  // Immediate update
}
```

**After**:
```cpp
// Track value stability across multiple reads
static u32 stableCount = 0;
static u8 lastPlayerChar = 0xFF;

if (++counter >= 180) {  // Every 3 seconds
    u8 newChar = ReadByte(addr);
    
    if (newChar == lastPlayerChar) {
        stableCount++;  // Value is stable
    } else {
        stableCount = 0;  // Value changed, reset
        lastPlayerChar = newChar;
        return;  // Don't update yet
    }
    
    // Only update after 2 stable reads (6 seconds total)
    if (stableCount >= 2) {
        SetCaption(BuildTitle(newChar));
    }
}
```

**Result**: Title only updates when values are proven stable.

### 2. **Increased Update Interval**

| Setting | Before | After | Reason |
|---------|--------|-------|--------|
| Update Frequency | Every 60 frames (1 sec) | Every 180 frames (3 sec) | Reduces memory read frequency |
| Stability Checks | 0 | 2 consecutive reads | Requires 6 seconds of stable values |
| Total Delay | 1 second | 6-9 seconds | Prevents flickering |

**Tradeoff**: Title takes longer to update, but never flickers.

### 3. **Multiple Memory Location Fallback**

**Problem**: Original memory addresses might be temporary/unstable.

**Solution**: Try multiple locations in priority order:

```cpp
// Priority 1: Save file structure (most stable)
const u32 ADDR_SAVE_DATA = 0x800F8C00;
playerChar = RDRAM[SAVE_DATA + 0];
difficulty = RDRAM[SAVE_DATA + 1];

// Priority 2: Character select variables (fallback)
if (playerChar > 1 || difficulty > 1) {
    playerChar = RDRAM[0x8008D790];
    difficulty = RDRAM[0x8008D791];
}
```

**Why**: Save file data is more stable once game is loaded.

### 4. **Prevent Redundant Caption Updates**

**Problem**: Setting caption even when unchanged causes OS overhead.

**Solution**: Cache last caption and compare:

```cpp
static char lastCaption[256] = "";

sprintf(caption, "GLideN64 - Reinhardt (Easy)");

if (strcmp(caption, lastCaption) != 0) {
    strcpy(lastCaption, caption);
    CoreVideo_SetCaption(caption);  // Only update if changed
}
```

**Result**: No OS calls unless caption actually changes.

---

## Technical Details

### State Machine

```
[Startup]
    ↓
[Read Values Every 3 Seconds]
    ↓
[Values Same as Last Read?]
    ├─ YES → Increment stableCount
    └─ NO  → Reset stableCount, save new values, SKIP UPDATE
    ↓
[stableCount >= 2?]
    ├─ YES → Update Title (if caption changed)
    └─ NO  → SKIP UPDATE, wait for next read
```

### Debounce Logic

| Time | Read # | Value | stableCount | Action |
|------|--------|-------|-------------|--------|
| 0s   | 1      | 0xFF  | 0           | Save value, skip update |
| 3s   | 2      | 0xFF  | 1           | Still stable, skip update |
| 6s   | 3      | 0xFF  | 2           | ✅ Update title (stable) |
| 9s   | 4      | 0xFF  | 3           | No change, skip update |
| 12s  | 5      | 0x00  | 0           | Value changed! Reset, skip update |
| 15s  | 6      | 0x00  | 1           | New value stable, skip update |
| 18s  | 7      | 0x00  | 2           | ✅ Update title (stable) |

### Memory Safety

All memory reads protected:
```cpp
if (RDRAM != nullptr && RDRAMSize > 0) {
    u32 offset = (addr & 0x007FFFFF);
    if (offset + size < RDRAMSize) {
        value = RDRAM[offset];
    }
}
```

---

## Testing Results

### Before Fix
- ❌ Title flickered every 1 second
- ❌ Alternated between "Reinhardt" and "Unknown"
- ❌ Distracting visual issue
- ❌ Possible performance impact from rapid caption changes

### After Fix
- ✅ Title updates smoothly after 6 seconds
- ✅ No flickering or rapid changes
- ✅ Stable display once in-game
- ✅ Zero performance impact

### Edge Cases Tested

| Scenario | Result |
|----------|--------|
| Rapid character switching | ✅ Waits for stability |
| Loading screen | ✅ Stays on last valid title |
| Invalid memory values | ✅ Falls back gracefully |
| Menu → Game transition | ✅ Updates after 6 seconds |
| Game → Menu transition | ✅ Updates after 6 seconds |

---

## Configuration Options (for future)

If you want to adjust behavior:

```cpp
// Change update interval (frames at 60 FPS)
static u32 titleUpdateCounter = 0;
if (++titleUpdateCounter >= 180) {  // Change 180 to adjust
    // 60 = 1 second
    // 120 = 2 seconds
    // 180 = 3 seconds (current)
    // 300 = 5 seconds
}

// Change stability requirement (reads needed)
if (stableCount >= 2) {  // Change 2 to adjust
    // 1 = Update after 1 stable read (3 seconds)
    // 2 = Update after 2 stable reads (6 seconds) [current]
    // 3 = Update after 3 stable reads (9 seconds)
}
```

**Recommended**: Keep at 180 frames / 2 stable reads for best balance.

---

## Why This Works

### Root Causes Addressed

1. **Unstable Memory Addresses**
   - Solution: Try multiple locations, fallback to most stable
   
2. **Timing Issues**
   - Solution: Read less frequently (every 3 seconds instead of 1)
   
3. **Transient Values**
   - Solution: Require 2 consecutive stable reads (6 seconds total)
   
4. **Redundant Updates**
   - Solution: Compare caption before setting (prevent OS overhead)

### Tradeoffs

| Aspect | Before | After | Impact |
|--------|--------|-------|--------|
| Update Speed | 1 second | 6+ seconds | Slower but stable |
| Flickering | Yes | No | Major UX improvement |
| CPU Overhead | Low | Lower | Fewer OS calls |
| Memory Reads | 60/min | 20/min | 66% reduction |

**Conclusion**: Slower updates are a small price for zero flickering.

---

## Alternative Solutions Considered

### 1. Faster Polling (Every Frame)
**Pros**: Instant updates  
**Cons**: High CPU overhead, still flickers  
**Verdict**: ❌ Rejected

### 2. Different Memory Addresses
**Pros**: Might be more stable  
**Cons**: Hard to find, may not exist  
**Verdict**: ⚠️ Partially implemented (fallback addresses)

### 3. Disable Feature
**Pros**: No flickering  
**Cons**: Loses functionality  
**Verdict**: ❌ Not acceptable

### 4. Debouncing (Chosen Solution)
**Pros**: Eliminates flicker, low overhead, keeps feature  
**Cons**: Slower updates  
**Verdict**: ✅ **Implemented**

---

## Code Changes

**File**: `RMG/Source/3rdParty/mupen64plus-video-GLideN64/src/Graphics/OpenGLContext/mupen64plus/mupen64plus_DisplayWindow.cpp`

**Lines Changed**: ~100 lines in `_swapBuffers()`

**New Includes**: `<cstring>` for strcmp/strcpy

**Build Status**: ✅ Successful

---

## User Experience

### What Users Will Notice

**Before**:
- Window title changes rapidly
- Distracting flickering effect
- Hard to read current state

**After**:
- Window title stable and clear
- Updates smoothly when character/difficulty changes
- Professional appearance

### When Title Updates

1. **Game Launch**: Shows generic title immediately
2. **Character Selection**: Updates 6 seconds after selection
3. **In-Game**: Stable display
4. **Return to Menu**: Updates 6 seconds after menu appears
5. **Character Switch**: Updates 6 seconds after new selection

**Note**: 6-second delay is intentional to ensure stability.

---

## Future Improvements (Optional)

### Potential Enhancements

1. **Adaptive Polling**
   - Poll faster during menus
   - Poll slower during gameplay
   - Saves CPU while maintaining responsiveness

2. **Better Memory Addresses**
   - Work with CV64 decomp team to find most stable addresses
   - Use pointers instead of fixed addresses
   - Reduce chance of instability

3. **Visual Feedback**
   - Fade in/out title changes
   - Add subtle indicator when title is updating
   - Make transitions smoother

**Priority**: Low - current solution works well

---

## Conclusion

✅ **Problem Solved**: No more flickering window titles  
✅ **Build Status**: Successful  
✅ **Performance**: Improved (fewer OS calls)  
✅ **User Experience**: Much better  

The window title now provides useful information without being distracting.

---

**Status**: ✅ FIXED AND TESTED  
**Date**: 2024  
**Impact**: High (major UX improvement)
