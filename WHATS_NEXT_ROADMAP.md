# What's Next: Future Enhancements & Improvements

## Current Status: ? Core Functionality Complete

The game now runs successfully with:
- ? Static plugin initialization
- ? Graphics rendering (GLideN64)
- ? Audio playback (SDL Audio)
- ? Input handling (Controller + Keyboard)
- ? Single-window experience

---

## Roadmap Categories

1. [Critical Fixes](#critical-fixes) - Issues that affect gameplay
2. [Performance Optimizations](#performance-optimizations) - Speed and efficiency
3. [Quality of Life](#quality-of-life-improvements) - User experience
4. [Original Game Bugs](#original-game-bugs-konami-never-fixed) - Fix Konami's bugs!
5. [Camera Improvements](#camera-improvements) - Already started, needs completion
6. [HD Enhancements](#hd-enhancements) - Modern graphics features
7. [Modding Support](#modding-support) - Community content
8. [Advanced Features](#advanced-features) - New capabilities

---

## Critical Fixes

### 1. Error Handling & Stability
**Priority**: HIGH
**Status**: Needs Work

#### Issues to Fix:
- [ ] Add comprehensive error handling for plugin failures
- [ ] Graceful degradation if audio plugin fails
- [ ] Handle ROM load failures more elegantly (show error dialog)
- [ ] Crash recovery and reporting system
- [ ] Auto-save before potential crash points

#### Implementation:
```cpp
// Add try-catch wrappers around critical sections
// Implement crash dump generation
// Add error reporting dialog with debug info
// Create recovery save states automatically
```

**Files to Modify**:
- `src/cv64_m64p_integration_static.cpp` - Add error handling
- `CV64_RMG.cpp` - Add crash recovery logic
- Create new: `src/cv64_error_reporting.cpp`

---

### 2. Memory Leak Detection
**Priority**: HIGH
**Status**: Unknown

#### Tasks:
- [ ] Profile memory usage during extended play
- [ ] Fix any memory leaks in plugin initialization
- [ ] Ensure clean shutdown releases all resources
- [ ] Add memory monitoring/reporting

#### Tools to Use:
- Visual Studio Memory Profiler
- Application Verifier
- Debug heap checks

**Expected Issues**:
- Possible texture memory leaks in GLideN64
- SDL audio buffer accumulation
- Config handles not being released

---

### 3. Multi-ROM Support
**Priority**: MEDIUM
**Status**: Partially Implemented

#### Current State:
- ? Embedded ROM loads
- ? Can't load external ROMs from menu
- ? No ROM browser/selector
- ? No ROM history/favorites

#### Features to Add:
- [ ] File browser dialog for ROM selection
- [ ] Recent ROMs list (MRU)
- [ ] ROM info display (region, version, status)
- [ ] ROM patching support (for fan translations/patches)
- [ ] Multi-game support (switch between CV64 and Legacy of Darkness)

**Files to Create**:
- `src/cv64_rom_browser.cpp` - File selection dialog
- `src/cv64_rom_manager.cpp` - ROM database and history

---

## Performance Optimizations

### 1. Frame Pacing & VSync
**Priority**: MEDIUM
**Status**: Basic Implementation

#### Issues:
- Current frame limiting is crude (Sleep-based)
- No proper VSync integration
- Potential screen tearing
- Inconsistent frame times

#### Improvements:
```cpp
// Implement proper frame pacing:
- Use SwapInterval for VSync control
- Add frame time smoothing
- Triple buffering support
- Adaptive VSync (G-Sync/FreeSync compatible)
```

**Files to Modify**:
- `src/cv64_vidext.cpp` - VSync implementation
- `CV64_RMG.cpp` - Frame timing overhaul

---

### 2. Multi-Threading Optimization
**Priority**: MEDIUM
**Status**: ? IMPLEMENTED (cv64_threading.cpp)

#### Implementation Complete:
The threading system is now implemented in:
- `include/cv64_threading.h` - Header with full API
- `src/cv64_threading.cpp` - Implementation

#### Architecture Overview:
```
???????????????????????????????????????????????????????????????????
?                    CV64 Threading Architecture                   ?
???????????????????????????????????????????????????????????????????
?                                                                  ?
?  ???????????????     ???????????????     ???????????????       ?
?  ? Main Thread ??????? Graphics    ??????? Frame       ?       ?
?  ? (R4300 CPU) ?     ? Thread      ?     ? Presentation?       ?
?  ?             ?     ? (OpenGL)    ?     ?             ?       ?
?  ???????????????     ???????????????     ???????????????       ?
?         ?                                                       ?
?         ?            ???????????????                           ?
?         ?????????????? Audio       ? (SDL Audio Callback)      ?
?                      ? Thread      ?                           ?
?                      ???????????????                           ?
?         ?                                                       ?
?         ?            ???????????????                           ?
?         ?????????????? Worker Pool ? (Async tasks)             ?
?                      ? Threads     ?                           ?
?                      ???????????????                           ?
?         ?                                                       ?
?         ?            ???????????????                           ?
?         ?????????????? RSP Thread  ? (EXPERIMENTAL)            ?
?                      ? (Audio HLE) ?                           ?
?                      ???????????????                           ?
?                                                                  ?
???????????????????????????????????????????????????????????????????
```

#### Features Implemented:
- [x] Graphics Thread - Async frame presentation with triple buffering
- [x] Audio Ring Buffer - Smooth audio with reduced underruns
- [x] Worker Thread Pool - For async tasks (texture loading, etc.)
- [x] RSP Thread (EXPERIMENTAL) - Parallel audio mixing
- [x] Thread Statistics - Performance monitoring
- [x] Frame Pacing - Proper VSync-aware frame timing

#### API Usage:
```cpp
#include "cv64_threading.h"

// Initialize at startup
CV64_ThreadConfig config = {
    .enableAsyncGraphics = true,
    .enableAsyncAudio = true,
    .enableWorkerThreads = true,
    .workerThreadCount = 4,
    .graphicsQueueDepth = 2,  // Triple buffering
    .enableParallelRSP = false // Experimental - disabled by default
};
CV64_Threading_Init(&config);

// Queue frame for async presentation
CV64_Graphics_QueueFrame(frameData, 1280, 720);

// Queue async task
CV64_Worker_QueueTask(LoadTextureAsync, textureParams, OnTextureLoaded, userData);

// Get performance stats
CV64_ThreadStats stats;
CV64_Threading_GetStats(&stats);
printf("Avg present latency: %.2f ms\n", stats.avgPresentLatencyMs);

// Shutdown
CV64_Threading_Shutdown();
```

#### Important Limitations:
?? **N64 emulation is inherently synchronous** because:
- RSP must complete before RDP can render
- Audio DMA timing tied to VI interrupts
- Memory shared between all components

The threading system provides async processing **where safe**:
- Frame presentation (after render complete)
- Audio mixing/output (SDL handles this)
- Texture uploads (background loading)
- Non-critical I/O tasks

**Files Implemented**:
- `include/cv64_threading.h` - Full API header
- `src/cv64_threading.cpp` - Thread management implementation

---

### 3. GPU Performance / GLideN64 Optimization
**Priority**: MEDIUM
**Status**: ? IMPLEMENTED (cv64_gliden64_optimize.cpp)

#### Implementation Complete:
The GLideN64 optimization system is now implemented in:
- `include/cv64_gliden64_optimize.h` - Optimization API header
- `src/cv64_gliden64_optimize.cpp` - CV64-specific optimizations

#### Features Implemented:
- [x] **Texture Cache Optimization** - CV64-aware texture prioritization
  - Known CV64 textures database for cache prioritization
  - UI textures (priority 255), Player textures (240), Environment (200)
  - Automatic low-priority texture flushing on area change
  - Texture preloading for known high-frequency textures

- [x] **Draw Call Batching** - Reduced GPU overhead
  - Compatible draw call detection and batching
  - Configurable max batch size (default 1024 triangles)
  - State change reduction to minimize GPU switches

- [x] **Area-Specific Optimizations** - Per-area rendering hints
  - Forest of Silence: Heavy fog optimization
  - Villa: Complex lighting settings
  - Castle Center: Aggressive texture caching (256MB suggested)
  - Underground Waterway: Water effect optimization
  - All 15+ CV64 areas have optimization profiles

- [x] **Performance Profiles**:
  - BALANCED (default) - Good quality/performance trade-off
  - QUALITY - Maximum visual quality
  - PERFORMANCE - Maximum frame rate
  - LOW_END - For integrated graphics

- [x] **CV64-Specific GLideN64 Config Optimizations**:
  - Shader storage enabled (reduces compilation stutter)
  - Threaded video enabled (better CPU utilization)
  - Fragment depth write disabled (not needed for CV64)
  - Halos removal enabled (cleaner texture edges)
  - 128MB texture cache (optimized for CV64's variety)
  - Native res texrects in optimized mode
  - Stripped backgrounds mode

#### API Usage:
```cpp
#include "cv64_gliden64_optimize.h"

// Initialize with balanced profile
CV64_Optimize_Init(NULL);

// Or use specific profile
CV64_Optimize_SetProfile(CV64_OPT_PROFILE_PERFORMANCE);

// Frame hooks (called automatically)
CV64_Optimize_FrameBegin();
CV64_Optimize_FrameEnd();

// Texture hints
CV64_Optimize_TextureHint(textureCRC, CV64_TEX_CATEGORY_PLAYER);

// Area change notification
CV64_Optimize_OnAreaChange(mapId);

// Get optimization stats
CV64_OptStats stats;
CV64_Optimize_GetStats(&stats);
printf("Cache hit rate: %.1f%%\n", stats.textureCacheHitRate);
printf("Draw calls saved: %llu\n", stats.drawCallsSaved);
```

**Files Implemented**:
- `include/cv64_gliden64_optimize.h` - Full optimization API
- `src/cv64_gliden64_optimize.cpp` - CV64-specific implementations
- `src/cv64_config_bridge.cpp` - GLideN64 config optimizations

---

## Quality of Life Improvements

### 1. Configuration UI
**Priority**: HIGH
**Status**: Non-existent

#### Features Needed:
- [ ] In-app settings menu (ImGui integration?)
- [ ] Graphics settings (resolution, AA, filtering)
- [ ] Audio settings (volume, device selection)
- [ ] Input remapping interface
- [ ] Save/load configuration presets

**Proposed Implementation**:
```cpp
// Add ImGui for in-game overlay
#include <imgui.h>

// Settings menu accessible via F10
void CV64_ShowSettingsMenu() {
    ImGui::Begin("Settings");
    
    if (ImGui::CollapsingHeader("Graphics")) {
        // Resolution, AA, texture filtering, etc.
    }
    
    if (ImGui::CollapsingHeader("Audio")) {
        // Volume sliders, device selection
    }
    
    if (ImGui::CollapsingHeader("Input")) {
        // Controller remapping
    }
    
    ImGui::End();
}
```

**Files Created**:
- `src/cv64_input_remapping.cpp` - ? Input remapping implementation
- `include/cv64_input_remapping.h` - ? Input remapping API

**Files to Create**:
- `src/cv64_settings_ui.cpp` - ImGui settings interface
- `include/cv64_settings_ui.h`

---

### 2. Save State Management
**Priority**: HIGH
**Status**: ? **IMPLEMENTED**

#### Implementation Complete:
The save state management system is now implemented in:
- `include/cv64_savestate_manager.h` - Full save state API
- `src/cv64_savestate_manager.cpp` - Manager implementation

#### Features Implemented:
- [x] **Quick Save/Load** - F5/F9 for slot 0 (quick save/load)
- [x] **Named Save States** - Create saves with custom names
- [x] **Metadata System** - JSON metadata with game info (map, character, HP, timestamp)
- [x] **Save State Browser** - Dialog UI to view, load, and manage saves
- [x] **Save State List** - Shows all saves sorted by date
- [x] **Delete Save States** - Remove unwanted saves
- [x] **Statistics Tracking** - Track total saves/loads
- [x] **Disk Usage Monitoring** - See total space used by saves

#### Features Planned (Future):
- [ ] Screenshot Thumbnails - Visual previews (OpenGL capture needed)
- [ ] Save State Rename - Change save names after creation
- [ ] Export/Import - Share saves with others
- [ ] Auto-save - Automatic saves at intervals
- [ ] Save State Comparison - Compare two saves side-by-side

#### API Usage:
```cpp
#include "cv64_savestate_manager.h"

// Initialize
CV64_SaveState_Init();

// Quick save to slot 0 (F5)
CV64_SaveState_QuickSave(0);

// Quick load from slot 0 (F9)
CV64_SaveState_QuickLoad(0);

// Named save state
CV64_SaveState_SaveNamed("Before Boss Fight");

// Show save state browser
CV64_SaveState_ShowDialog(hMainWindow);

// Get statistics
CV64_SaveStateStats stats;
CV64_SaveState_GetStats(&stats);
printf("Total saves: %d\n", stats.totalStates);

// Cleanup
CV64_SaveState_Shutdown();
```

#### How It Works:
The save state manager integrates with mupen64plus's internal save state system:
- Uses M64P save slots (0-9)
- Slot 0 reserved for quick save/load
- Slots 1-9 used for named saves
- Metadata stored separately in JSON format
- Thumbnails will be captured when OpenGL integration is added

#### Keyboard Shortcuts:
- **F5** - Quick Save (slot 0)
- **F9** - Quick Load (slot 0)
- **Shift+F5** - Save with custom name (future)
- **Shift+F9** - Load save browser (future)

**Files Implemented**:
- `include/cv64_savestate_manager.h` - Save state API
- `src/cv64_savestate_manager.cpp` - Implementation
- Integrated into main menu: **Game ? Save State Manager...**

---

### 3. Enhanced Debug Tools
**Priority**: MEDIUM
**Status**: Partial Implementation

#### Implemented Features:
- [x] **60 FPS Menu Cheat** - In-game menus now run at 60fps instead of 30fps
  - Gameshark codes: `8009202C 0000` and `8009202E 0000`
  - Enabled by default on startup
  - Can be toggled via `CV64_Memory_Set60FpsMenuCheat(bool)`

- [x] **60 FPS Gameplay Cheat** - Entire game runs at 60fps instead of 30fps
  - Gameshark codes: `80387ACE 0000`, `80387AD0 0000`, `80387D74 0000`, `80363AB8 0000`
  - Enabled by default on startup
  - Can be toggled via `CV64_Memory_Set60FpsGameCheat(bool)`

- [x] **Dynamic Window Title** - Shows current game state in window title
  - Displays current map name (e.g., "Forest of Silence", "Castle Wall - Main")
  - Shows player HP when available (e.g., "HP: 100/100")
  - Shows FPS counter
  - Example: `Castlevania 64 - Forest of Silence | HP: 100/100 | FPS: 60`
  - API: `CV64_Memory_GetGameInfo()`, `CV64_Memory_GetCurrentMapName()`

#### Debug Features to Add:
- [ ] Memory viewer/editor
- [ ] Cheat code manager (UI for enabling/disabling cheats)
- [ ] Game variable inspector
- [x] Position/coordinate display (via window title - map name)
- [ ] Speedrun timer
- [ ] Level select/warp menu
- [ ] Unlock all items/abilities
- [ ] Health/magic editor

**Files Implemented**:
- `src/cv64_memory_hook.cpp` - Gameshark cheat system, game info API, map names
- `include/cv64_memory_hook.h` - Cheat API and game info declarations
- `CV64_RMG.cpp` - Window title updates with game info

**Files to Create**:
- `src/cv64_debug_menu.cpp` - Debug overlay
- `src/cv64_cheat_manager.cpp` - Cheat code system UI

---

### 4. Screenshot & Recording
**Priority**: LOW
**Status**: Not Implemented

#### Features:
- [ ] Screenshot capture (F12)
- [ ] Screenshot auto-naming (timestamp)
- [ ] Screenshot folder organization
- [ ] Video recording (AVI/MP4)
- [ ] GIF creation (short clips)
- [ ] Replay recording/playback (TAS support)

**Files to Create**:
- `src/cv64_screenshot.cpp`
- `src/cv64_video_capture.cpp` (FFmpeg integration)

---

## Original Game Bugs (Konami Never Fixed!)

### 1. The Invisible Bridge Bug
**Location**: Castle Center - Tower
**Description**: Sometimes the bridge becomes invisible but still solid
**Status**: Original game bug

#### Fix Strategy:
- Identify bridge object in memory
- Monitor visibility state
- Force correct rendering state
- Patch ROM code or intercept draw calls

**Implementation**:
```cpp
// In cv64_game_patches.cpp
void PatchInvisibleBridge() {
    // Read bridge object state from game memory
    uint32_t bridgeState = CV64_M64P_ReadMemory32(BRIDGE_STATE_ADDR);
    
    // Force visibility flag if bridge is solid
    if (IsBridgeSolid(bridgeState) && !IsBridgeVisible(bridgeState)) {
        CV64_M64P_WriteMemory32(BRIDGE_STATE_ADDR, bridgeState | VISIBLE_FLAG);
    }
}
```

---

### 2. Camera Clipping Through Walls
**Location**: Various tight spaces
**Description**: Camera goes through walls, making it hard to see
**Status**: Partially fixed by camera patch

#### Remaining Issues:
- [ ] Some corners still clip
- [ ] Boss arena camera issues
- [ ] Staircase camera problems

#### Improvements:
```cpp
// Enhanced collision detection for camera
void ImprovedCameraCollision() {
    // Cast multiple rays from player to camera
    // Use shortest non-colliding distance
    // Apply smooth interpolation
    // Add camera "push" when too close
}
```

**Files to Modify**:
- `src/cv64_camera_patch.cpp` - Enhanced collision

---

### 3. Fall Damage Inconsistency
**Location**: Throughout game
**Description**: Small falls sometimes deal damage, large falls sometimes don't
**Status**: Original game bug

#### Fix Strategy:
- Monitor player fall distance accurately
- Consistent damage calculation
- Add fall damage threshold adjustment (difficulty setting?)

---

### 4. Wonky Hitboxes
**Location**: All combat
**Description**: Some attacks miss when they should hit, vice versa
**Status**: Core game mechanics issue

#### Potential Fixes:
- [ ] Improve weapon hitbox detection
- [ ] Add visual hitbox debug overlay
- [ ] Adjust problematic enemy hitboxes
- [ ] Add "auto-aim" assist option

---

### 5. Loading Zone Stuck Points
**Location**: Various doorways
**Description**: Character gets stuck in loading zones requiring reset
**Status**: Critical bug

#### Fix:
```cpp
// Detect stuck state
void DetectStuckInLoadingZone() {
    if (InLoadingZone() && !IsMoving() && TimeSinceLastMovement() > 3.0f) {
        // Force unstuck: nudge player position
        NudgePlayerPosition();
        // Or: force zone transition
        // Or: add "unstuck" key binding
    }
}
```

---

### 6. Audio Desync Issues
**Location**: Cutscenes
**Description**: Audio drifts out of sync during long cutscenes
**Status**: Emulation timing issue

#### Solutions:
- [ ] Better audio buffer management
- [ ] Frame-accurate audio sync
- [ ] Cutscene timing fixes
- [ ] Skip cutscene option (works around the issue!)

---

### 7. Armor Sub-Weapon Inventory Bug
**Location**: Menu system
**Description**: Sometimes sub-weapons disappear when equipping armor
**Status**: Inventory management bug

#### Fix:
- Monitor inventory state changes
- Validate inventory after equipment changes
- Restore lost items if detected

---

### 8. Boss AI Freeze Bug
**Location**: Various boss fights
**Description**: Boss stops responding to player actions
**Status**: AI state machine bug

#### Fix Strategy:
- Detect frozen AI state
- Force AI state reset
- Add AI watchdog timer
- Reset boss to attacking state

---

## Camera Improvements

### Current Status:
- ? Basic camera patch infrastructure exists
- ? D-Pad camera mode toggle
- ?? Needs refinement

### Enhancements Needed:

#### 1. Modern Camera System
**Priority**: HIGH

Features:
- [ ] Free camera mode (full 360° rotation)
- [ ] Smooth camera interpolation
- [ ] Adjustable camera distance
- [ ] Multiple camera presets (classic, modern, cinematic)
- [ ] Camera sensitivity adjustment
- [ ] Inverted Y-axis option
- [ ] Camera reset to behind player (L-targeting style)

**Implementation**:
```cpp
// src/cv64_camera_advanced.cpp

class ModernCamera {
    CameraMode mode;          // Classic, Free, Modern, Cinematic
    float distance;           // Camera distance from player
    float pitch, yaw;         // Camera angles
    float sensitivity;        // Mouse/stick sensitivity
    bool invertY;            // Invert Y-axis
    
    void UpdateFreeCam();     // User-controlled camera
    void UpdateModernCam();   // Souls-like camera
    void SmoothTransition();  // Interpolate camera changes
    void CollisionCheck();    // Keep camera outside walls
};
```

---

#### 2. Boss Camera Lock
**Priority**: MEDIUM

Features:
- [ ] Auto-lock camera to boss during fights
- [ ] Smooth boss tracking
- [ ] Maintain optimal viewing angle
- [ ] Allow manual override
- [ ] Disable during cutscenes

---

#### 3. Cinematic Camera Events
**Priority**: LOW

Features:
- [ ] Trigger camera angles at key moments
- [ ] Dramatic reveal shots
- [ ] Custom camera paths for cutscenes
- [ ] Camera shake effects (attacks, earthquakes)

---

## HD Enhancements

### 1. HD Texture Pack Support
**Priority**: HIGH
**Status**: Infrastructure exists (GLideN64)

#### Implementation Tasks:
- [ ] Create HD texture pack template
- [ ] Document texture naming conventions
- [ ] Provide texture dumping tools
- [ ] Auto-load HD textures if present
- [ ] HD texture pack manager (enable/disable packs)

**Directory Structure**:
```
assets/
  hires_texture/
    cv64_official/          # Official HD pack (if created)
    cv64_community_v1/      # Community packs
    cv64_photorealistic/
```

---

### 2. Widescreen Patches
**Priority**: HIGH
**Status**: GLideN64 supports widescreen

#### Improvements:
- [ ] Proper HUD scaling for 16:9, 21:9
- [ ] Fix stretched UI elements
- [ ] Adjust FOV for ultrawide
- [ ] Menu adjustments for widescreen
- [ ] Cutscene letterboxing options

**Files to Create**:
- `src/cv64_widescreen_patch.cpp`

---

### 3. Enhanced Lighting
**Priority**: MEDIUM

Features:
- [ ] Per-pixel lighting (if possible)
- [ ] Enhanced fog effects
- [ ] Improved shadow rendering
- [ ] Ambient occlusion (SSAO)
- [ ] HDR lighting support

---

### 4. Anti-Aliasing Options
**Priority**: MEDIUM

Implement:
- [ ] MSAA (2x, 4x, 8x)
- [ ] FXAA (fast, good quality)
- [ ] SMAA (better quality)
- [ ] TAA (temporal, smoothest)

---

### 5. Post-Processing Effects
**Priority**: LOW

Optional Effects:
- [ ] Color correction/grading
- [ ] Vignette effect
- [ ] Film grain (toggleable)
- [ ] Motion blur (toggleable)
- [ ] Depth of field
- [ ] Bloom/glow effects

---

## Modding Support

### 1. Mod Loading System
**Priority**: MEDIUM
**Status**: Not Implemented

#### Features:
- [ ] Mod folder structure
- [ ] Mod manifest format (JSON/TOML)
- [ ] Mod priority/load order
- [ ] Conflict detection
- [ ] Enable/disable mods from menu

**Mod Structure**:
```
mods/
  better_camera/
    mod.json          # Mod metadata
    patches/          # Code patches
    textures/         # Custom textures
    models/           # Custom 3D models
    sounds/           # Custom sounds
  randomizer/
    mod.json
    code/
```

**mod.json Example**:
```json
{
  "name": "Better Camera Mod",
  "version": "1.0.0",
  "author": "Community",
  "description": "Improved camera controls and angles",
  "patches": [
    "patches/camera_improvements.lua"
  ],
  "textures": "textures/",
  "priority": 100
}
```

---

### 2. Lua Scripting Support
**Priority**: LOW

Features:
- [ ] Embed Lua interpreter
- [ ] Expose game memory API
- [ ] Allow custom logic patches
- [ ] Hot-reload scripts
- [ ] Script debugging

**API Example**:
```lua
-- mods/custom_behavior/script.lua

function OnFrame()
    local playerHealth = ReadMemory32(PLAYER_HEALTH_ADDR)
    
    if playerHealth < 10 then
        -- Low health warning
        ShowMessage("Low Health!")
    end
end

function OnBossFight()
    -- Custom boss behavior
    local bossState = ReadMemory32(BOSS_STATE_ADDR)
    -- Modify difficulty, patterns, etc.
end
```

---

### 3. Model/Animation Import
**Priority**: LOW

Features:
- [ ] Import custom player models
- [ ] Import custom enemy models
- [ ] Custom weapon models
- [ ] Animation replacement
- [ ] Skin system

---

### 4. Randomizer Support
**Priority**: MEDIUM

Features:
- [ ] Item randomizer
- [ ] Enemy randomizer
- [ ] Level layout randomizer
- [ ] Color randomizer
- [ ] Chaos mode (random everything!)
- [ ] Randomizer seeds for racing

**Files to Create**:
- `src/cv64_randomizer.cpp`
- `src/cv64_randomizer_ui.cpp`

---

## Advanced Features

### 1. Online Multiplayer (Experimental)
**Priority**: LOW
**Status**: Concept only

#### Potential Modes:
- [ ] Co-op (two players, shared progress)
- [ ] Race mode (speedrun competition)
- [ ] Boss rush co-op
- [ ] PvP arena (versus mode)

**Challenges**:
- Game not designed for multiplayer
- Sync issues
- Network code complexity
- Gameplay balance

---

### 2. Practice Mode
**Priority**: MEDIUM

Features:
- [ ] Instant level warp
- [ ] Infinite health toggle
- [ ] Infinite magic toggle
- [ ] One-hit kill mode
- [ ] Frame-by-frame advance
- [ ] Save position/restore position
- [ ] Boss practice mode (fight any boss instantly)

**Use Cases**:
- Speedrun practice
- Learning boss patterns
- Testing strategies
- Recording footage

---

### 3. Built-in Speedrun Tools
**Priority**: MEDIUM

Features:
- [ ] Accurate speedrun timer
- [ ] Split timer integration (LiveSplit compatible)
- [ ] Auto-split on key events
- [ ] Speedrun stats tracking
- [ ] Comparison to best times
- [ ] Speedrun verification (recording)

**Files to Create**:
- `src/cv64_speedrun_timer.cpp`
- `src/cv64_livesplit_integration.cpp`

---

### 4. Achievement System
**Priority**: LOW

Features:
- [ ] Custom achievement definitions
- [ ] Achievement tracking
- [ ] Achievement pop-ups
- [ ] Progress tracking
- [ ] Unlock rewards (HD textures, etc.)
- [ ] Stats tracking (deaths, time played, etc.)

**Example Achievements**:
```cpp
Achievement achievements[] = {
    { "Speed Demon", "Complete game under 2 hours", TYPE_TIME },
    { "Pacifist", "Complete with minimal kills", TYPE_COMBAT },
    { "Collector", "Find all hidden items", TYPE_EXPLORATION },
    { "Deathless", "Complete without dying", TYPE_SKILL },
    { "True Ending", "Unlock the secret ending", TYPE_STORY }
};
```

---

### 5. Replay System
**Priority**: LOW

Features:
- [ ] Record full gameplay inputs
- [ ] Replay playback
- [ ] Save replay files
- [ ] Share replays
- [ ] Replay verification (TAS detection)
- [ ] Ghost racing (race against replay)

---

## Input & Accessibility

### 1. Enhanced Controller Support
**Priority**: HIGH

Features:
- [ ] Full controller remapping
- [ ] Multiple controller profiles
- [ ] Controller button prompts (show correct buttons)
- [ ] Gyro aiming (Switch Pro, DualSense)
- [ ] Adaptive triggers (DualSense)
- [ ] Rumble customization
- [ ] Simultaneous keyboard + controller

---

### 2. Accessibility Options
**Priority**: MEDIUM

Features:
- [ ] Color blind modes
- [ ] High contrast mode
- [ ] Subtitles/closed captions (for cutscenes)
- [ ] Adjustable text size
- [ ] Audio cues for visual events
- [ ] Visual cues for audio events
- [ ] One-handed mode remapping
- [ ] Auto-aim assist
- [ ] Slow-motion mode

---

### 3. Mouse & Keyboard Optimization
**Priority**: MEDIUM

Features:
- [ ] Improved mouse camera control
- [ ] Proper mouse sensitivity
- [ ] Raw input support
- [ ] All keys remappable
- [ ] Mouse gestures (quick items)
- [ ] Keyboard shortcuts for everything

---

## Documentation & Community

### 1. User Manual
**Priority**: HIGH
**Status**: Needs Creation

Contents:
- Installation guide
- Controls reference
- Settings explanation
- Troubleshooting
- FAQ
- Known issues

---

### 2. Modding Documentation
**Priority**: MEDIUM

Contents:
- Mod creation guide
- API reference
- Memory map documentation
- Scripting tutorial
- Example mods
- Best practices

---

### 3. Development Documentation
**Priority**: MEDIUM

Contents:
- Build guide
- Code architecture
- Plugin system explanation
- Contribution guidelines
- Coding standards
- Testing procedures

---

## Performance Metrics & Goals

### Target Performance:
- **FPS**: Locked 60 FPS (native N64 frame rate)
- **Input Lag**: < 1 frame (16.67ms)
- **Memory**: < 500 MB RAM usage
- **CPU**: < 30% usage on modern CPUs
- **GPU**: Runs on integrated graphics

### Optimization Priorities:
1. Eliminate any stuttering
2. Minimize input lag
3. Reduce memory usage
4. Improve startup time
5. Optimize plugin overhead

---

## Release Roadmap

### Version 1.0 (Current) ?
- [x] Core emulation working
- [x] Graphics rendering
- [x] Audio playback
- [x] Input handling
- [x] Basic save states

### Version 1.1 (Next Release)
**Target**: 2-4 weeks

**Focus**: Stability & Polish
- [ ] Error handling improvements
- [ ] Configuration system
- [ ] Save state manager
- [ ] Bug fixes from testing
- [ ] Performance profiling

### Version 1.5 (Major Update)
**Target**: 2-3 months

**Focus**: Quality of Life
- [ ] Settings UI (ImGui)
- [ ] HD texture support finalized
- [ ] Widescreen patches
- [ ] Enhanced camera system
- [ ] Debug tools

### Version 2.0 (Major Release)
**Target**: 6 months

**Focus**: Enhancements & Modding
- [ ] Mod loading system
- [ ] Randomizer support
- [ ] Original game bug fixes
- [ ] Practice mode
- [ ] Advanced graphics options

### Version 2.5+
**Target**: 1+ year

**Focus**: Advanced Features
- [ ] Lua scripting
- [ ] Replay system
- [ ] Achievement system
- [ ] Experimental multiplayer
- [ ] Community features

---

## Community Contribution Opportunities

### Easy Contributions:
- [ ] HD texture creation
- [ ] Configuration presets
- [ ] Bug reports
- [ ] Documentation improvements
- [ ] Translation support

### Medium Contributions:
- [ ] Camera patches
- [ ] Mod creation
- [ ] Testing & QA
- [ ] UI improvements
- [ ] Performance testing

### Advanced Contributions:
- [ ] Core engine improvements
- [ ] Plugin development
- [ ] Game logic patches
- [ ] Randomizer implementation
- [ ] Multiplayer experiments

---

## Known Technical Debt

### Code Quality Issues:
- [ ] Refactor plugin initialization (reduce duplication)
- [ ] Better separation of concerns
- [ ] More comprehensive error handling
- [ ] Unit tests needed
- [ ] Integration tests needed
- [ ] Memory leak audits
- [ ] Code documentation (Doxygen)

### Architecture Improvements:
- [ ] Plugin hot-reloading
- [ ] Better state management
- [ ] Event system overhaul
- [ ] Message bus for components
- [ ] Dependency injection

---

## Conclusion

This roadmap prioritizes:
1. **Stability** - Make it rock-solid
2. **Performance** - Keep it fast
3. **Quality of Life** - Make it enjoyable
4. **Original Bug Fixes** - Fix what Konami couldn't
5. **Enhancements** - Make it better than the original
6. **Modding** - Empower the community

**The journey from "it runs" to "it's amazing" starts now!** ??

---

## How to Contribute

1. Pick a feature from this roadmap
2. Check if it's already being worked on (GitHub issues)
3. Implement and test
4. Submit a pull request
5. Enjoy making gaming history!

**Together we can create the definitive way to play Castlevania 64!** ????
