# Castlevania 64 PC Recomp - Developer Documentation

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Building the Project](#building-the-project)
3. [Code Structure](#code-structure)
4. [Camera Patch Implementation](#camera-patch-implementation)
5. [Controller System](#controller-system)
6. [Adding New Patches](#adding-new-patches)
7. [Integration with cv64 Decomp](#integration-with-cv64-decomp)
8. [Memory Management](#memory-management)
9. [Graphics Pipeline](#graphics-pipeline)
10. [Audio System](#audio-system)

---

## Architecture Overview

The CV64 PC Recomp follows a layered architecture:

```
???????????????????????????????????????????????????
?              Application Layer                   ?
?         (CV64_RMG.cpp, Window, Main Loop)       ?
???????????????????????????????????????????????????
?               Patch System                       ?
?    (Camera Patch, Widescreen, Bug Fixes)        ?
???????????????????????????????????????????????????
?              Recomp Core                         ?
?     (cv64_recomp.h - Memory, State, ROM)        ?
???????????????????????????????????????????????????
?              Subsystems                          ?
?   ?????????????????????????????????????        ?
?   ? Graphics  ?   Audio   ?Controller ?        ?
?   ? System    ?  System   ?  System   ?        ?
?   ?????????????????????????????????????        ?
???????????????????????????????????????????????????
?           Platform Abstraction                   ?
?    (cv64_types.h - Types, Platform Detection)   ?
???????????????????????????????????????????????????
```

### Design Principles

1. **Modularity**: Each system is self-contained with clear interfaces
2. **N64 Compatibility**: Types and structures match original game
3. **PC-Native Performance**: Optimized for modern hardware
4. **Configurability**: Everything is configurable via INI files
5. **Extensibility**: Easy to add new patches and features

---

## Building the Project

### Prerequisites

- Visual Studio 2022 or later
- Windows SDK 10.0.19041.0 or later
- C++17 support

### Build Steps

1. Open `CV64_RMG.sln` in Visual Studio
2. Select configuration (Debug/Release) and platform (x64)
3. Build ? Build Solution (F7)

### Project Structure

```
CV64_RMG.sln
??? CV64_RMG.vcxproj          # Main project
??? include/                   # Headers
??? src/                       # Source files
??? patches/                   # Config files
??? RMG/                       # Reference (Rosalie's Mupen GUI)
```

### Dependencies

| Library | Purpose | Link Type |
|---------|---------|-----------|
| XInput | Controller input | Windows SDK |
| DirectX | Graphics (future) | Windows SDK |
| OpenGL | Graphics backend | Dynamic |

---

## Code Structure

### Header Files (include/)

| File | Description |
|------|-------------|
| `cv64_types.h` | Core type definitions (u8, s32, Vec3f, etc.) |
| `cv64_memory_map.h` | N64 memory addresses from cv64 decomp |
| `cv64_memory_hook.h` | Memory access for real-time patching |
| `cv64_controller.h` | Input handling API |
| `cv64_graphics.h` | Graphics rendering API |
| `cv64_gfx_plugin.h` | Graphics plugin integration (GLideN64) |
| `cv64_rom_loader.h` | ROM loading and validation |
| `cv64_audio.h` | Audio playback API |
| `cv64_camera_patch.h` | Camera control patch API |
| `cv64_patches.h` | Patch management system |
| `cv64_recomp.h` | Main recomp framework |
| `cv64_m64p_integration.h` | mupen64plus emulator integration |

### Source Files (src/)

| File | Description |
|------|-------------|
| `cv64_controller.cpp` | Input system implementation |
| `cv64_camera_patch.cpp` | Camera patch implementation |
| `cv64_memory_hook.cpp` | Memory hook implementation |
| `cv64_gfx_plugin.cpp` | Graphics plugin implementation |
| `cv64_rom_loader.cpp` | ROM loader implementation |
| `cv64_m64p_integration.cpp` | mupen64plus integration |

### Naming Conventions

- **Prefix**: All public symbols use `CV64_` prefix
- **Functions**: `CV64_System_Action()` format
- **Types**: `CV64_TypeName` format
- **Constants**: `CV64_CONSTANT_NAME` format
- **Static variables**: `s_variableName`
- **Global variables**: `g_variableName`

---

## Camera Patch Implementation

The camera patch is the **most important feature** of this recomp. Here's how it works:

### State Machine

```
???????????????  User Input   ???????????????
?   IDLE      ???????????????>?  ROTATING   ?
?             ?               ?             ?
???????????????               ???????????????
      ?                             ?
      ?      Timeout                ?
      ???????????????????????????????
```

### Key Functions

```cpp
// Initialize camera patch
CV64_CameraPatch_Init();

// Process input each frame
CV64_CameraPatch_ProcessDPad(dpad_state, delta_time);
CV64_CameraPatch_ProcessRightStick(x, y, delta_time);
CV64_CameraPatch_ProcessMouse(dx, dy, delta_time);

// Update camera position
CV64_CameraPatch_Update(delta_time);
```

### Integration Points

The camera patch hooks into these game functions:

1. **cameraMgr_loop()** - Main camera update
2. **cameraMgr_setReadingTextState()** - Lock during text
3. **Player position updates** - Track player for look-at

### Angle Calculation

```cpp
// Convert camera angles to position
float yaw_rad = DEG_TO_RAD(state.yaw);
float pitch_rad = DEG_TO_RAD(state.pitch);

float horizontal_dist = distance * cos(pitch_rad);
camera.x = target.x - horizontal_dist * sin(yaw_rad);
camera.y = target.y + distance * sin(pitch_rad);
camera.z = target.z - horizontal_dist * cos(yaw_rad);
```

---

## Controller System

### Button Mapping

The controller system maps modern inputs to N64 buttons:

```cpp
// N64 button masks
#define CONT_A      0x8000
#define CONT_B      0x4000
#define CONT_G      0x2000  // Z trigger
#define CONT_START  0x1000
#define CONT_UP     0x0800
#define CONT_DOWN   0x0400
#define CONT_LEFT   0x0200
#define CONT_RIGHT  0x0100
#define CONT_L      0x0020
#define CONT_R      0x0010
// C-buttons: 0x0008, 0x0004, 0x0002, 0x0001
```

### Input Sources

1. **Keyboard** - GetAsyncKeyState()
2. **XInput** - XInputGetState()
3. **Mouse** - GetCursorPos() with delta calculation

### D-PAD Camera Mode

When enabled, D-PAD input is intercepted:

```cpp
if (ctrl->dpad_camera_mode && config.enable_dpad_camera) {
    // Route D-PAD to camera patch
    CV64_CameraPatch_ProcessDPad(ctrl->dpad_raw, delta_time);
    
    // Remove D-PAD from game input
    ctrl->btns_held &= ~(CONT_UP | CONT_DOWN | CONT_LEFT | CONT_RIGHT);
}
```

---

## Adding New Patches

### Step 1: Define Patch ID

Add to `cv64_patches.h`:

```cpp
typedef enum CV64_PatchID {
    // ...existing patches...
    CV64_PATCH_MY_NEW_PATCH,
    CV64_PATCH_COUNT
} CV64_PatchID;
```

### Step 2: Create Header

Create `include/cv64_my_patch.h`:

```cpp
#ifndef CV64_MY_PATCH_H
#define CV64_MY_PATCH_H

#include "cv64_types.h"

CV64_API bool CV64_MyPatch_Init(void);
CV64_API void CV64_MyPatch_Shutdown(void);
CV64_API void CV64_MyPatch_Update(f32 delta_time);

#endif
```

### Step 3: Implement

Create `src/cv64_my_patch.cpp`:

```cpp
#include "../include/cv64_my_patch.h"

static bool s_initialized = false;

bool CV64_MyPatch_Init(void) {
    if (s_initialized) return true;
    // Initialize patch
    s_initialized = true;
    return true;
}

void CV64_MyPatch_Shutdown(void) {
    s_initialized = false;
}

void CV64_MyPatch_Update(f32 delta_time) {
    if (!s_initialized) return;
    // Patch logic here
}
```

### Step 4: Register

Add to main initialization in `CV64_RMG.cpp`:

```cpp
if (!CV64_MyPatch_Init()) {
    // Handle error
}
```

---

## Integration with cv64 Decomp

### Shared Structures

The project uses structures from cv64 decomp:

```cpp
// From include/game/controller.h
typedef struct Controller {
    u16 is_connected;
    u16 btns_held;
    u16 btns_pressed;
    s16 joy_x;
    s16 joy_y;
    // ...
} Controller;

// From include/game/gfx/camera.h
typedef struct Camera {
    s16 type;
    u16 flags;
    Vec3f position;
    Angle angle;
    Vec3f look_at_direction;
    Mat4f matrix;
    // ...
} Camera;
```

### Function Hooks

To modify game behavior, we hook into decomp functions:

```cpp
// Hook prototype
extern void cameraMgr_loop(cameraMgr* self);

// Our wrapper
void CV64_CameraMgr_Loop_Hook(cameraMgr* self) {
    // Apply our camera patch
    CV64_CameraPatch_OnCameraMgrUpdate(self);
    
    // Call original function
    cameraMgr_loop(self);
}
```

---

## Memory Management

### N64 Memory Map

The recomp emulates the N64 memory layout:

| Address Range | Description |
|---------------|-------------|
| 0x80000000 - 0x803FFFFF | Main RAM (4MB) |
| 0x80400000 - 0x807FFFFF | Expansion RAM |
| 0xA0000000 - 0xA3FFFFFF | Uncached mirror |

### Key CV64 Memory Addresses (from decomp)

| Address | Symbol | Description |
|---------|--------|-------------|
| `0x80363AB8` | `sys` | system_work - main game state |
| `0x8009B438` | `common_camera_game_view` | Main 3D camera |
| `0x80016A60` | `controller_Init` | Controller init function |
| `0x80066CE0` | `cameraMgr_loop_gameplay` | Camera update loop |
| `0x80066F10` | `cameraMgr_calcCameraState` | Camera state calculation |

### Memory Access (cv64_memory_map.h)

```cpp
// Convert N64 address to RDRAM offset
#define N64_ADDR_TO_OFFSET(addr) ((addr) & 0x003FFFFF)

// Read 32-bit value (big-endian)
static inline u32 CV64_ReadU32(u8* rdram, u32 addr) {
    u8* ptr = rdram + N64_ADDR_TO_OFFSET(addr);
    return ((u32)ptr[0] << 24) | ((u32)ptr[1] << 16) | 
           ((u32)ptr[2] << 8) | (u32)ptr[3];
}

// Write 32-bit value (big-endian)
static inline void CV64_WriteU32(u8* rdram, u32 addr, u32 value) {
    u8* ptr = rdram + N64_ADDR_TO_OFFSET(addr);
    ptr[0] = (value >> 24) & 0xFF;
    ptr[1] = (value >> 16) & 0xFF;
    ptr[2] = (value >> 8) & 0xFF;
    ptr[3] = value & 0xFF;
}
```

### Memory Hook System (cv64_memory_hook.cpp)

```cpp
// Set RDRAM pointer from mupen64plus
CV64_Memory_SetRDRAM(rdram, size);

// Read game state
CV64_N64_CameraMgrData camData;
CV64_Memory_ReadCameraData(&camData);

// Write modified camera position
CV64_Memory_WriteCameraPosition(x, y, z);

// Frame update (called each emulated frame)
CV64_Memory_FrameUpdate();
```

---

## Graphics Plugin Integration

### Supported Plugins (cv64_gfx_plugin.h)

| Plugin | Description |
|--------|-------------|
| GLideN64 | Recommended - good accuracy and performance |
| Angrylion Plus | Most accurate - slower |
| Parallel RDP | Vulkan-based - fast and accurate |

### Plugin Loading

```cpp
// Initialize plugin system
CV64_GfxPluginConfig config = {
    .plugin_type = CV64_GFX_PLUGIN_GLIDEN64,
    .window_width = 1280,
    .window_height = 720,
    .widescreen = true
};
CV64_GfxPlugin_Init(&config);

// Load plugin DLL
CV64_GfxPlugin_Load("plugins/mupen64plus-video-GLideN64.dll", CV64_GFX_PLUGIN_GLIDEN64);

// Start ROM
CV64_GfxPlugin_StartROM(&gfx_info);

// Per-frame
CV64_GfxPlugin_ProcessDList();
CV64_GfxPlugin_UpdateScreen();
```

---

## ROM Loading (cv64_rom_loader.h)

### ROM Validation

```cpp
CV64_RomInfo info;
if (CV64_Rom_Load("path/to/rom.z64", &info)) {
    if (info.is_cv64) {
        // Valid Castlevania 64 ROM
        printf("Region: %s\n", CV64_Rom_GetRegionName(info.region));
        printf("Version: %s\n", CV64_Rom_GetVersionName(info.version));
    }
}
```

### Supported Formats

| Extension | Format | Notes |
|-----------|--------|-------|
| .z64 | Big-endian | Native N64 format |
| .n64 | Little-endian | Needs word swap |
| .v64 | Byte-swapped | Needs byte swap |

---

## Graphics Pipeline

### Display List Processing

N64 graphics use display lists (DL):

```cpp
// Process N64 display list
CV64_Graphics_ProcessDisplayList(Gfx* dl);

// Display list commands include:
// - Geometry loading
// - Matrix operations
// - Texture setup
// - Drawing primitives
```

### Rendering Backends

Supported backends:

1. **OpenGL 3.3+** - Primary target
2. **Vulkan 1.0+** - High performance
3. **Direct3D 11** - Windows compatibility
4. **Direct3D 12** - Future proofing

---

## Audio System

### Audio Pipeline

```
N64 Audio Data ? Resampler ? Mixer ? Output Device
                    ?
              Enhancement
              (Reverb, EQ)
```

### Sample Rate Conversion

```cpp
// Convert N64 native rate to PC output
CV64_SampleRate native = CV64_SAMPLERATE_NATIVE;  // ~32kHz
CV64_SampleRate output = CV64_SAMPLERATE_48000;   // 48kHz

// Apply high-quality resampling
```

---

## Best Practices

### Error Handling

```cpp
bool CV64_Something_Init(void) {
    if (s_initialized) {
        return true;  // Already initialized
    }
    
    if (!dependency_init()) {
        CV64_LogError("Failed to initialize dependency");
        return false;
    }
    
    s_initialized = true;
    return true;
}
```

### Configuration

```cpp
// Always provide defaults
static void reset_config_defaults(void) {
    s_config.enabled = true;
    s_config.sensitivity = 1.0f;
    // etc.
}

// Validate on load
void CV64_ApplyConfig(void) {
    s_config.sensitivity = clamp(s_config.sensitivity, 0.1f, 10.0f);
}
```

### Frame-Rate Independence

```cpp
// Always use delta time
void CV64_Something_Update(f32 delta_time) {
    f32 movement = speed * delta_time;
}
```

---

## Resources

- [cv64 Decomp](https://github.com/cv64/cv64)
- [N64 Developer Documentation](http://n64devkit.square7.ch/)
- [RMG Source Code](https://github.com/Rosalie241/RMG)
- [mupen64plus API](https://mupen64plus.org/)
