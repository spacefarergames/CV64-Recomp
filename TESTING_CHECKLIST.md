# Final Testing & Verification Checklist

## Pre-Build Verification

### Code Changes Applied ?
- [x] `frontend.c` - CoreGetStaticHandle and debug callback accessors added
- [x] `cv64_static_plugins.cpp` - All plugins call PluginStartup with proper handle
- [x] `CV64_RMG.cpp` - Window stays visible, WM_PAINT skips rendering during emulation

### Build Configuration
- [ ] Release or Debug build configuration selected
- [ ] All projects in solution build successfully
- [ ] No linker errors
- [ ] No compiler warnings (or acceptable warnings only)

---

## Build Process

### Step 1: Clean Build
```
1. Right-click Solution in Solution Explorer
2. Select "Clean Solution"
3. Wait for completion
```

### Step 2: Rebuild
```
1. Right-click Solution in Solution Explorer
2. Select "Rebuild Solution"
3. Watch for any errors in Output window
```

### Expected Output
```
========== Rebuild All: X succeeded, 0 failed, 0 skipped ==========
```

---

## Post-Build Verification

### Check Output Files
- [ ] `CV64_RMG.exe` exists in output directory
- [ ] `SDL2.dll` exists in output directory (if needed)
- [ ] `assets` folder with `Retro_CASTLE.bmp` exists

---

## Runtime Testing

### Test 1: Application Launch
1. **Launch** `CV64_RMG.exe`
2. **Expected**: Window appears with Castlevania splash screen
3. **Expected**: Title shows "Castlevania 64 PC Recomp"
4. **Expected**: Text shows "Press SPACE to start"

**Result**: [ ] PASS  [ ] FAIL

---

### Test 2: Debug Output - Plugin Initialization
1. **Launch** with debugger attached or use DebugView
2. **Look for** these messages in debug output:

```
[CV64_STATIC_PLUGIN] === Registering all static plugins ===
[CV64_STATIC_PLUGIN] Registering static GFX plugin...
[CV64_STATIC_PLUGIN] Calling gliden64_PluginStartup...
[CV64_STATIC_PLUGIN] gliden64_PluginStartup succeeded
[CV64_STATIC_PLUGIN] GFX plugin registered

[CV64_STATIC_PLUGIN] Registering static Audio plugin...
[CV64_STATIC_PLUGIN] Audio plugin registered

[CV64_STATIC_PLUGIN] Registering static Input plugin...
[CV64_STATIC_PLUGIN] Calling inputPluginStartup...
[CV64_STATIC_PLUGIN] inputPluginStartup succeeded
[CV64_STATIC_PLUGIN] Input plugin registered

[CV64_STATIC_PLUGIN] Registering static RSP plugin...
[CV64_STATIC_PLUGIN] Calling rsphle_PluginStartup...
[CV64_STATIC_PLUGIN] rsphle_PluginStartup succeeded
[CV64_STATIC_PLUGIN] RSP plugin registered

[CV64_STATIC_PLUGIN] === All static plugins registered ===
```

**Result**: [ ] PASS  [ ] FAIL

---

### Test 3: ROM Loading
1. **Press** SPACE key
2. **Look for** these messages in debug output:

```
[CV64_STATIC] Attempting to load embedded ROM...
[CV64_STATIC] Found embedded ROM: xxxxx bytes
[CV64_STATIC] Calling CoreDoCommand(M64CMD_ROM_OPEN, ...)
[M64P-CORE][INFO] Name: CASTLEVANIA
[CV64_STATIC] Embedded ROM loaded: CASTLEVANIA
[CV64_STATIC] Calling plugin_start for all plugins...
[GLideN64] _initiateGFX: RDRAM=..., DMEM=..., IMEM=...
[CV64_AUDIO_SDL] InitiateAudio called
[CV64_AUDIO_SDL] SDL audio initialized successfully
[CV64-Input] === InitiateControllers called ===
[CV64-Input] === InitiateControllers COMPLETE ===
```

**Critical**: Should NOT see:
```
? Exception thrown at 0x0000000000000000
? Access violation
? rsphle_PluginStartup failed
```

**Result**: [ ] PASS  [ ] FAIL

---

### Test 4: Graphics Display
1. **After** ROM loads (SPACE pressed)
2. **Expected**: 
   - Window title changes to "CASTLEVANIA"
   - Window stays visible (NOT hidden)
   - N64 game graphics appear in window
   - Nintendo 64 logo or game intro visible

**Visual Check**:
- [ ] Window visible
- [ ] Graphics rendering
- [ ] No black screen
- [ ] Colors look correct
- [ ] No graphical glitches

**Result**: [ ] PASS  [ ] FAIL

---

### Test 5: Audio
1. **During** game intro/gameplay
2. **Expected**:
   - Game music plays
   - Sound effects audible
   - No crackling or distortion
   - Volume at reasonable level

**Audio Check**:
- [ ] Music playing
- [ ] Sound effects working
- [ ] Audio quality good
- [ ] No stuttering

**Result**: [ ] PASS  [ ] FAIL

---

### Test 6: Input
1. **Try** keyboard keys:
   - Arrow keys for D-Pad
   - Enter for Start
   - Other keys for A/B/C buttons (check input mapping)

2. **Try** controller (if connected):
   - Analog stick for movement
   - Buttons for game actions

**Input Check**:
- [ ] Keyboard responds
- [ ] Controller responds (if connected)
- [ ] Controls feel responsive
- [ ] No input lag

**Result**: [ ] PASS  [ ] FAIL

---

### Test 7: Window Management
1. **Resize** window by dragging edges
2. **Expected**: 
   - Window resizes smoothly
   - Graphics scale/adjust
   - No crashes

**Result**: [ ] PASS  [ ] FAIL

---

### Test 8: Emulation Control
1. **Press** ESC - Should pause
2. **Press** ESC again - Should resume
3. **Press** F5 - Quick save (if implemented)
4. **Press** F9 - Quick load (if implemented)

**Result**: [ ] PASS  [ ] FAIL

---

### Test 9: Shutdown
1. **Close** window with X button
2. **Expected**:
   - Clean exit
   - No crash on shutdown
   - No memory access violations

**Result**: [ ] PASS  [ ] FAIL

---

## Troubleshooting

### If Plugin Initialization Fails

**Check**:
1. `CoreGetStaticHandle()` returns non-NULL
2. `GetDebugCallback()` returns valid function pointer
3. All plugins have `extern "C"` linkage

**Debug Output to Look For**:
```
ERROR: CoreGetStaticHandle returned NULL
ERROR: rsphle_PluginStartup failed with error: X
```

---

### If Graphics Don't Display

**Check**:
1. Window is visible (not hidden)
2. `g_emulationStarted` is true
3. WM_PAINT skips rendering when emulation active
4. Video extension initialized: `[CV64_VidExt] Video extension initialized`

**Visual Symptoms**:
- ? Window hidden = Remove any `ShowWindow(SW_HIDE)` calls
- ? Black screen = Check video extension setup
- ? Flickering = Check WM_PAINT logic

---

### If Audio Doesn't Play

**Check**:
1. Audio plugin initialized: `[CV64_AUDIO_SDL] SDL audio initialized successfully`
2. SDL2.dll present
3. Audio device available on system

---

### If Input Doesn't Work

**Check**:
1. Input plugin initialized: `[CV64-Input] === InitiateControllers COMPLETE ===`
2. Controller detection messages in debug output
3. Input mapping configured

---

## Success Criteria

### Minimum Requirements (ALL must pass)
- [ ] Application launches without crash
- [ ] Plugin initialization succeeds (debug output confirms)
- [ ] ROM loads without crash (no 0x00000000 exception)
- [ ] Window stays visible during emulation
- [ ] Graphics display correctly
- [ ] Audio plays
- [ ] Input responds
- [ ] Application closes cleanly

### Ideal Results (Goal)
- [ ] Smooth 60 FPS gameplay
- [ ] No visual glitches
- [ ] Perfect audio sync
- [ ] Responsive input
- [ ] Stable emulation
- [ ] Savestate functionality works

---

## Final Verification

Date Tested: ______________

Tester: ______________

### Overall Result
[ ] ALL TESTS PASS - Game runs successfully! ???
[ ] SOME TESTS FAIL - See troubleshooting section
[ ] MOST TESTS FAIL - Review all code changes

### Notes
```
(Add any additional observations, performance notes, or issues found)





```

---

## If All Tests Pass

**Congratulations!** ??

Castlevania 64 is now running properly with:
- ? Static plugin initialization working
- ? Graphics displaying via GLideN64
- ? Audio playing through SDL
- ? Input responding correctly
- ? Single-window seamless experience

The game is ready to play!
