# mupen64plus-core Static Linking Integration Guide

## Overview

This guide explains how to integrate mupen64plus-core as a statically linked library instead of using the dynamic DLL (`mupen64plus.dll`). Static linking provides finer control over the emulator core.

## Files Created

1. **`mupen64plus-core-static/mupen64plus-core-static.vcxproj`**
   - Static library project that compiles mupen64plus-core source into a `.lib` file
   - Uses pure interpreter mode (NO_ASM defined)
   - Platform toolset: v143 (VS 2022)

2. **`mupen64plus-static.props`**
   - MSBuild property sheet with include paths and linker settings
   - Links against: SDL2, zlib, libpng, opengl32, ws2_32

3. **`include/cv64_m64p_static_wrapper.h`**
   - Header with extern declarations for static linking

4. **`include/cv64_m64p_static.h`**
   - Complete plugin API declarations for static linking

5. **`src/cv64_m64p_integration_static.cpp`**
   - Full implementation including:
     - Core initialization and shutdown
     - Plugin loading (dynamic DLLs for GFX/Audio/Input/RSP)
     - ROM management
     - Emulation control (Start/Stop/Pause/Resume/Reset)
     - Memory access (8/16/32-bit read/write)
     - Save states
     - Speed control
     - Frame callbacks
     - Controller input override

## Setup Instructions

### Step 1: Add the Static Library Project to the Solution

Open `CV64_RMG.slnx` in a text editor and ensure it includes the static library project:

```xml
<Solution>
  <Configurations>
    <Platform Name="x64" />
    <Platform Name="x86" />
  </Configurations>
  
  <Folder Name="/3rdParty/">
    <Project Path="mupen64plus-core-static/mupen64plus-core-static.vcxproj" />
  </Folder>
  
  <Project Path="CV64_RMG.vcxproj">
    <Dependency Project="mupen64plus-core-static/mupen64plus-core-static.vcxproj" />
  </Project>
</Solution>
```

### Step 2: Import the Props File in CV64_RMG.vcxproj

Add this line before `<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />`:

```xml
<Import Project="mupen64plus-static.props" />
```

### Step 3: Add Project Reference

Add a reference to the static library in CV64_RMG.vcxproj:

```xml
<ItemGroup>
  <ProjectReference Include="mupen64plus-core-static\mupen64plus-core-static.vcxproj">
    <Project>{a1b2c3d4-e5f6-7890-abcd-ef1234567890}</Project>
  </ProjectReference>
</ItemGroup>
```

### Step 4: Build Order

1. Build `mupen64plus-core-static` first (produces `mupen64plus-core-static.lib`)
2. Build `CV64_RMG` (links against the static library)

### Step 5: Use Static Integration

In your code, the static integration is controlled by the `CV64_STATIC_MUPEN64PLUS` preprocessor define:

```cpp
#ifdef CV64_STATIC_MUPEN64PLUS
    // Uses direct function calls to statically linked core
    CV64_M64P_Init(hwnd);
    CV64_M64P_LoadROM("castlevania.z64");
    CV64_M64P_Start();
#else
    // Uses dynamic DLL loading (mupen64plus.dll)
    CV64_M64P_Init(hwnd);
#endif
```

## Switching Between Static and Dynamic Linking

### To use Static Linking:
1. Define `CV64_STATIC_MUPEN64PLUS` in your preprocessor definitions
2. Link against `mupen64plus-core-static.lib`
3. No need for `mupen64plus.dll` at runtime (core only - plugins still needed)

### To use Dynamic Linking:
1. Remove `CV64_STATIC_MUPEN64PLUS` from preprocessor definitions
2. Ensure `mupen64plus.dll` is in the plugins directory
3. Uses the existing `cv64_m64p_integration.cpp` code

## Dependencies

The mupen64plus-core requires:
- **SDL2** - For input and windowing
- **zlib** - For compression (save states)
- **libpng** - For screenshots
- **OpenGL** - For video rendering
- **ws2_32** - Windows sockets (for netplay support)

## Quick Start Script

Run `apply_static_integration.ps1` to automatically configure the solution:

```powershell
.\apply_static_integration.ps1
```
- **OpenGL** - For video output

These are expected in: `RMG\Source\3rdParty\mupen64plus-win32-deps\`

## Advantages of Static Linking

1. **Direct Memory Access** - Call `DebugMemGetPointer()` directly without DLL indirection
2. **No DLL Search** - Core is embedded in the executable
3. **Custom Patches** - Easier to modify core behavior
4. **Debugging** - Step into core code during debugging

## Plugins (Still Dynamic)

Note that plugins (video, audio, RSP, input) are still loaded as DLLs to allow users to swap them:
- `GLideN64.dll` or `mupen64plus-video-*.dll` - Video plugin
- `mupen64plus-audio-sdl.dll` - Audio plugin  
- `mupen64plus-rsp-hle.dll` - RSP plugin

## Troubleshooting

### Unresolved External Symbols
If you get linker errors like `unresolved external symbol _CoreStartup`:
1. Ensure `mupen64plus-core-static.lib` is built first
2. Check that `$(OutDir)` is in your library search paths
3. Verify the static library is referenced correctly

### Missing Headers
If headers like `m64p_types.h` are not found:
1. Verify include paths in the props file
2. Check that the mupen64plus-core source exists in `RMG\Source\3rdParty\mupen64plus-core\`

### Runtime Errors
If the emulator fails to start:
1. Check that SDL2.dll is available
2. Ensure plugins are in the correct location
3. Check debug output for specific error messages
