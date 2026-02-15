# Static Plugin Integration Guide

## Overview

This document describes the static plugin integration for CV64_RMG, which embeds GLideN64 (graphics), RSP-HLE (RSP processing), SDL Audio, and a custom input plugin directly into the executable without requiring external DLLs.

## Architecture

```
CV64_RMG.exe
??? mupen64plus-core-static.lib    (N64 emulation core)
??? mupen64plus-rsp-hle-static.lib (RSP High-Level Emulation)
??? mupen64plus-video-gliden64-static.lib (OpenGL graphics)
??? CV64 SDL Audio (built-in)      (Real audio output)
??? Custom Input Plugin (built-in)
```

## Files Created/Modified

### New Header Files
- `include/cv64_gliden64_static.h` - GLideN64 static function declarations
- `include/cv64_rsp_hle_static.h` - RSP-HLE static function declarations

### New Source Files
- `src/cv64_audio_sdl.cpp` - SDL-based audio plugin with real audio output
- `src/cv64_gliden64_wrapper.cpp` - GLideN64 wrapper for static linking
- `src/cv64_rsp_hle_wrapper.cpp` - RSP-HLE wrapper for static linking

### New Project Files
- `mupen64plus-video-gliden64-static/mupen64plus-video-gliden64-static.vcxproj` - GLideN64 static lib project
- `mupen64plus-rsp-hle-static/mupen64plus-rsp-hle-static.vcxproj` - RSP-HLE static lib project

### Modified Files
- `src/cv64_static_plugins.cpp` - Updated to use real plugins
- `mupen64plus-static.props` - Added linker dependencies for all static libs
- `CV64_RMG.vcxproj` - Added project references for static libs

## Configuration

The static plugin system is now pre-configured in `mupen64plus-static.props`:

```cpp
/* All plugins enabled by default via preprocessor definitions */
CV64_STATIC_MUPEN64PLUS  - Enable static mupen64plus linking
CV64_USE_GLIDEN64        - Enable real GLideN64 graphics
CV64_USE_RSP_HLE         - Enable real RSP-HLE
CV64_USE_CUSTOM_INPUT    - Enable custom input plugin
```

## Building the Static Libraries

All static libraries are now built automatically as project dependencies.

### Build Order (handled automatically by Visual Studio):

1. **mupen64plus-core-static.lib** - Core emulator
2. **mupen64plus-rsp-hle-static.lib** - RSP High-Level Emulation  
3. **mupen64plus-video-gliden64-static.lib** - GLideN64 graphics
4. **CV64_RMG.exe** - Main application (links all libs)

### Manual Build (if needed):

```powershell
# Build all projects in correct order
msbuild CV64_RMG.slnx /p:Configuration=Release /p:Platform=x64
```

### Library Dependencies (configured in mupen64plus-static.props):

- `mupen64plus-core-static.lib`
- `mupen64plus-rsp-hle-static.lib`
- `mupen64plus-video-gliden64-static.lib`
- `SDL2.lib`
- `opengl32.lib`
- `glu32.lib`
- `ws2_32.lib`
- `winmm.lib`
- `imm32.lib`
- `version.lib`
- `setupapi.lib`

## Symbol Renaming

To avoid symbol conflicts when linking multiple plugins statically, we use preprocessor defines to rename exported functions:

### RSP-HLE Symbols
```
PluginStartup    -> rsphle_PluginStartup
PluginShutdown   -> rsphle_PluginShutdown
PluginGetVersion -> rsphle_PluginGetVersion
DoRspCycles      -> rsphle_DoRspCycles
InitiateRSP      -> rsphle_InitiateRSP
RomClosed        -> rsphle_RomClosed
```

### GLideN64 Symbols
```
PluginStartup    -> gliden64_PluginStartup
PluginShutdown   -> gliden64_PluginShutdown
PluginGetVersion -> gliden64_PluginGetVersion
InitiateGFX      -> gliden64_InitiateGFX
RomOpen          -> gliden64_RomOpen
RomClosed        -> gliden64_RomClosed
ProcessDList     -> gliden64_ProcessDList
ProcessRDPList   -> gliden64_ProcessRDPList
UpdateScreen     -> gliden64_UpdateScreen
... (and more)
```

## Custom Input Plugin

The custom input plugin (`src/cv64_input_plugin.cpp`) provides:
- XInput gamepad support
- Keyboard input (via CV64_Controller system)
- Memory Pak emulation with save/load
- VRU stub functions (for compatibility)

### Input Plugin Functions
- `inputPluginStartup` / `inputPluginShutdown`
- `inputPluginGetVersion`
- `inputInitiateControllers`
- `inputGetKeys` - Main input polling
- `inputControllerCommand` - Memory Pak read/write
- VRU stubs: `inputSendVRUWord`, `inputSetMicState`, etc.

## Fallback Mode

When static libraries are not available, the system falls back to dummy plugins:
- `dummyvideo_*` - Minimal video plugin (blank screen)
- `dummyrsp_*` - Core's built-in RSP HLE
- `dummyinput_*` - No-op input

## Plugin Registration

Plugins are registered with the mupen64plus core via:
```cpp
CoreRegisterGfxPlugin(&gfx_funcs);
CoreRegisterRspPlugin(&rsp_funcs);
CoreRegisterAudioPlugin(&audio_funcs);
CoreRegisterInputPlugin(&input_funcs);
```

This bypasses the normal DLL loading mechanism entirely.

## Troubleshooting

### Unresolved External Symbols
- Ensure static libraries are built and linked
- Check that symbol renaming defines are applied in vcxproj files
- Verify `extern "C"` linkage for all plugin functions

### Graphics Issues
- GLideN64 requires OpenGL 3.3+ support
- Check GPU driver compatibility
- Verify `opengl32.lib` and `glu32.lib` are linked

### Input Not Working
- Verify `CV64_USE_CUSTOM_INPUT` is defined
- Check `inputGetKeys` is being called (debug logging)
- Ensure controller is connected before ROM load
