# Quick Test Guide - Verify Memory Reading Fix

## ?? Quick Test (5 minutes)

### Step 1: Build and Run
```
1. Build solution (Ctrl+Shift+B)
2. Start with debugger (F5)
3. Open Output window (View ? Output)
```

### Step 2: Check Initialization
**Look for these messages:**
```
[CV64_STATIC] RDRAM pointer obtained in state callback: 0x...
[CV64_STATIC] Memory hook system initialized - native patches enabled!
[CV64] Memory hook system initialized via frame callback
[CV64_GameOpt] RDRAM pointer: 0x..., Test read = 0x...
```

? **PASS:** All messages appear with non-zero addresses
? **FAIL:** See "WARNING: Failed to get RDRAM pointer"

---

### Step 3: Start Game
```
1. Press SPACE to start
2. Load into Forest of Silence
```

### Step 4: Check Per-Frame Updates
**Look for (every ~1 second):**
```
[CV64_GameOpt] State=2 Map=2 Entities=42 Enemies=8 Particles=143 Fog=1
```

? **PASS:** See messages with reasonable values (Particles 100-200)
? **FAIL:** All zeros or no messages

---

### Step 5: Verify FPS
**In Forest of Silence:**
- Before fix: ~28-35 FPS
- After fix: ~58-60 FPS

? **PASS:** Smooth 60 FPS
? **FAIL:** Still ~30 FPS (optimizations not working)

---

### Step 6: Check Shutdown
**On exit, look for:**
```
[CV64_GameOpt] Final Statistics:
  Particles Skipped: 8432/11234 (75%)
  Entities Culled: 234/567 (41%)
```

? **PASS:** Non-zero values
? **FAIL:** All zeros

---

## ? Success = All 6 Steps Pass

## ? If Any Step Fails
1. Copy full Output window log
2. Check MEMORY_READING_FIX_COMPLETE.md
3. Look for specific error message in diagnostic guide

---

## Quick Visual Test

### What You Should See:
- **Title Screen:** Smooth 60 FPS
- **Forest of Silence:** 
  - Smooth 60 FPS (not 30!)
  - Less fog particles visible
  - Distant trees lower quality
  - Still looks good, just runs way faster

### What You Should NOT See:
- Stuttering in Forest
- FPS counter showing ~30
- Heavy slowdowns
- No visible optimization (all particles rendering)

---

## Debug Output Cheat Sheet

### ? Good Output:
```
RDRAM pointer: 0x00007FF... (non-zero)
Test read = 0x80371240 (non-zero)  
State=2 Map=2 Entities=42 (reasonable values)
Particles Skipped: 8432/11234 (high %)
```

### ? Bad Output:
```
RDRAM pointer: 0x0000000000000000 (NULL!)
Test read = 0x00000000 (all zeros)
State=0 Map=-1 Entities=0 (all zeros)
Particles Skipped: 0 (never ran)
```

---

## Expected Performance

| Location | FPS | Particles | Feel |
|----------|-----|-----------|------|
| **Title Screen** | 60 | Few | Smooth |
| **Forest Intro** | 58-60 | 40-80 | Smooth |
| **Forest Main** | 58-60 | 50-100 | Smooth |
| **Villa** | 58-60 | 60-120 | Smooth |
| **Castle** | 60 | 20-50 | Smooth |

**Before fix:** Forest was 28-35 FPS (unplayable slowdowns)
**After fix:** Forest is 58-60 FPS (smooth gameplay)

---

## One-Liner Test

**Run this in Output window filter:**
```
Search for: [CV64_GameOpt]
```

**Should see:**
- Initialization messages (on start)
- State updates (every second during play)
- Final statistics (on exit)

**If you see nothing:**
- Frame callback not being invoked
- Check MEMORY_READING_FIX_COMPLETE.md for troubleshooting

---

**The fix is complete - now verify it works!** ??
