# CV64 RMG Integration - Implementation Notes

## Overview

This document describes the comprehensive integration of mupen64plus N64 emulation into the Castlevania 64 PC Recompilation project, following RMG (Rosalie's Mupen GUI) best practices.

## Key Integration Components

### 1. Video Extension System

The video extension is a critical component that allows mupen64plus to render to a custom window instead of creating its own. This is how RMG and other frontends integrate with the emulator.

**Implementation:**
- Created `m64p_video_extension_functions` structure with all required callbacks
- Implemented OpenGL context management (creation, deletion, buffer swapping)
- Set up video extension before plugin loading using `CoreOverrideVidExt()`

**Key Functions:**
```cpp
- VidExt_Init() - Initializes video system
- VidExt_SetMode() - Sets video mode
- VidExt_GL_GetProcAddress() - Gets OpenGL function pointers
- VidExt_GL_SwapBuffers() - Swaps buffers and triggers frame callbacks
- VidExt_SetCaption() - Sets window title
```

### 2. OpenGL Context Management

Created proper OpenGL rendering context for the window before initializing mupen64plus.

**Implementation:**
```cpp
- InitializeOpenGLContext() - Creates OpenGL context with proper pixel format
- ShutdownOpenGLContext() - Cleans up OpenGL resources
- wglGetProcAddress() - Loads OpenGL extensions
```

**Context Lifecycle:**
1. Window created by main application
2. OpenGL context created and bound to window
3. Context set as current before plugin initialization
4. Video plugin renders to this context
5. SwapBuffers() called each frame
6. Context destroyed on shutdown

### 3. Plugin Loading System

Enhanced plugin discovery to be more robust, trying multiple naming conventions:

**Video Plugins (in order of preference):**
- video-GLideN64 (best quality, recommended)
- video-glide64mk2
- video-rice
- video-gliden64
- mupen64plus-video-*
- Any DLL with "video"

**Audio Plugins:**
- audio-sdl (recommended)
- mupen64plus-audio-*
- Any DLL with "audio"

**Input Plugins:**
- input-sdl
- mupen64plus-input-*
- Any DLL with "input"

**RSP Plugins:**
- rsp-hle (recommended, required)
- rsp-cxd4 (high accuracy)
- mupen64plus-rsp-*
- Any DLL with "rsp"

### 4. Configuration System

Set up mupen64plus configuration before starting emulation:

**Video Configuration:**
```cpp
- Fullscreen = 0 (windowed mode)
- WindowedHandle = [HWND as string]
- ScreenWidth = 1280
- ScreenHeight = 720
```

**Core Configuration:**
```cpp
- Config directory = executable directory
- Data directory = executable directory
```

### 5. Plugin Initialization Sequence

The correct order is critical for proper initialization:

1. **Core Initialization**
   ```
   LoadLibrary(mupen64plus.dll)
   GetProcAddress() for all functions
   CoreStartup()
   ```

2. **Video Extension Setup**
   ```
   SetupVideoExtension()
   CoreOverrideVidExt(&s_vidExtFuncs)
   ```

3. **Plugin Loading**
   ```
   LoadLibrary(plugin.dll)
   PluginGetVersion() - Verify plugin type
   PluginStartup(core_handle, context, callback)
   Configure plugin settings
   ```

4. **Plugin Attachment**
   ```
   CoreAttachPlugin(M64PLUGIN_GFX, gfx_plugin_handle)
   CoreAttachPlugin(M64PLUGIN_AUDIO, audio_plugin_handle)
   CoreAttachPlugin(M64PLUGIN_INPUT, input_plugin_handle)
   CoreAttachPlugin(M64PLUGIN_RSP, rsp_plugin_handle)
   ```

5. **ROM Loading**
   ```
   CoreDoCommand(M64CMD_ROM_OPEN, size, rom_data)
   CoreDoCommand(M64CMD_ROM_GET_HEADER, ...)
   ```

6. **Emulation Start**
   ```
   CoreDoCommand(M64CMD_EXECUTE) - Blocks until stopped
   ```

### 6. Frame Callback Integration

Integrated our custom patches into the emulation loop:

```cpp
VidExt_GL_SwapBuffers() {
    SwapBuffers(s_hdc);
    OnFrameComplete(frameIndex);  // Calls our patches
}

OnFrameComplete() {
    CV64_Controller_Update(0);
    CV64_CameraPatch_Update(deltaTime);
    if (s_frameCallback) {
        s_frameCallback(s_frameCallbackContext);
    }
}
```

### 7. Memory Access

Set up RDRAM pointer access for reading/writing N64 memory:

```cpp
DebugMemGetPointer(&ptr, M64P_DBG_PTR_RDRAM)
s_rdram = (u8*)ptr;

// Then we can read/write game memory:
CV64_M64P_ReadMemory32(0x80000000)
CV64_M64P_WriteMemory32(0x80000000, value)
```

### 8. Error Handling

Comprehensive error handling with helpful messages:

- DLL loading errors with Windows error codes
- Missing dependency detection (SDL2.dll, zlib1.dll)
- Plugin compatibility verification
- 32/64-bit mismatch detection
- Helpful error messages directing users to setup_plugins.bat

### 9. Threading Model

Proper thread management for emulation:

```cpp
- Main thread: Window messages, input, patches
- Emulation thread: CoreDoCommand(M64CMD_EXECUTE)
- Synchronization: Atomic bools, mutexes for state changes
```

## Differences from Standalone mupen64plus

### What RMG/This Integration Does Differently:

1. **Window Management**
   - RMG: Uses its own window/OpenGL context
   - Standalone: Creates its own window

2. **Plugin Configuration**
   - RMG: Configures plugins programmatically
   - Standalone: Uses config files only

3. **Video Extension**
   - RMG: Overrides video extension for custom rendering
   - Standalone: Uses default video output

4. **Frame Callback**
   - RMG: Hooks SwapBuffers for per-frame callbacks
   - Standalone: No frame callback system

5. **Input Handling**
   - RMG: Can override controller input
   - Standalone: Relies on input plugin

## Required Files

### Core Files (Required):
- `mupen64plus.dll` - Core emulator
- `SDL2.dll` - Required by core
- `zlib1.dll` or `zlib.dll` - Required by core

### Video Plugin (Required - Pick One):
- `mupen64plus-video-GLideN64.dll` (Recommended)
- `mupen64plus-video-glide64mk2.dll`
- `mupen64plus-video-rice.dll`

### RSP Plugin (Required):
- `mupen64plus-rsp-hle.dll` (High-level emulation, fast)
- `mupen64plus-rsp-cxd4.dll` (Low-level, accurate, slower)

### Audio Plugin (Optional but Recommended):
- `mupen64plus-audio-sdl.dll`

### Input Plugin (Optional - we have custom input):
- `mupen64plus-input-sdl.dll`

### Additional GLideN64 Files:
- `GLideN64.ini` - GLideN64 configuration
- `GLideN64.custom.ini` - Custom settings (optional)

## Directory Structure

```
CV64_RMG.exe
??? SDL2.dll
??? zlib1.dll
??? plugins/
    ??? mupen64plus.dll
    ??? mupen64plus-video-GLideN64.dll
    ??? mupen64plus-audio-sdl.dll
    ??? mupen64plus-input-sdl.dll
    ??? mupen64plus-rsp-hle.dll
    ??? GLideN64.ini
    ??? GLideN64.custom.ini
```

## Plugin Search Paths

The integration searches for plugins in this order:
1. `<exe_directory>/plugins/`
2. `<exe_directory>/`
3. `<project_root>/plugins/` (for development)
4. `<exe_directory>/RMG/` (for RMG compatibility)

## Configuration Files

### Video-General Config (Programmatic):
```ini
[Video-General]
Fullscreen=0
WindowedHandle=<HWND>
```

### Video Plugin Config (Programmatic):
```ini
[Video-GLideN64]
Fullscreen=0
ScreenWidth=1280
ScreenHeight=720
```

## Testing

To test the integration:

1. Run `setup_plugins.bat` to download all required files
2. Place ROM at `assets/Castlevania (U) (V1.2) [!].z64`
3. Build and run CV64_RMG.exe
4. Check debug output for plugin loading status

## Debugging Tips

### Enable Debug Output:
- All debug messages go to OutputDebugString
- Use DebugView or Visual Studio Output window
- Look for `[CV64_M64P]` and `[M64P]` messages

### Common Issues:

**Black Screen:**
- Video extension not set up correctly
- OpenGL context not created
- Window handle not passed to plugin

**Crash on Start:**
- Missing dependencies (SDL2.dll, zlib1.dll)
- 32/64-bit mismatch
- Incompatible plugin versions

**No Audio:**
- Audio plugin missing (non-fatal)
- Run setup_plugins.bat to download

**Slow Performance:**
- Using rsp-cxd4 instead of rsp-hle
- Video plugin settings too high
- Debug build instead of Release

## Performance Optimization

1. **Use Release Build** - 10x faster than Debug
2. **Use rsp-hle** - Much faster than rsp-cxd4
3. **GLideN64 Settings** - Balance quality/performance
4. **V-Sync** - Enable to prevent tearing
5. **Multi-threading** - Emulation runs on separate thread

## Future Enhancements

Potential improvements based on RMG:

1. **Save State Management** - UI for save states
2. **Cheat System** - GameShark/Action Replay codes
3. **Texture Packs** - High-res texture support (GLideN64)
4. **Shader Support** - Post-processing effects
5. **Netplay** - Online multiplayer via mupen64plus-netplay
6. **Controller Profiles** - Multiple controller configurations
7. **Hotkeys** - Configurable keyboard shortcuts
8. **On-Screen Display** - FPS, frame time, etc.

## References

- mupen64plus Core: https://github.com/mupen64plus/mupen64plus-core
- RMG: https://github.com/Rosalie241/RMG
- GLideN64: https://github.com/gonetz/GLideN64
- mupen64plus API: https://mupen64plus.org/apidoc/
- Video Extension API: In mupen64plus-core m64p_vidext.h

## Acknowledgments

- mupen64plus team for the excellent N64 emulator
- Rosalie241 for RMG reference implementation
- GLideN64 team for the best video plugin
- cv64 decomp project for game knowledge

---

**Integration Status:** ? Complete and Working
**Last Updated:** 2024
**Tested With:** mupen64plus-core 2.5.9, GLideN64 4.0
