# Castlevania 64 PC Recomp - Testing Instructions

## Build Status: ? SUCCESSFUL

The project has been built successfully:
- **Executable**: `x64\Release\CV64_RMG.exe` (176 KB)
- **Config files**: All `.ini` files copied to output

## ?? Missing: mupen64plus Plugins

The mupen64plus DLLs need to be downloaded manually:

### Option 1: Download RMG Release (Recommended)

1. Go to: https://github.com/Rosalie241/RMG/releases/tag/v0.6.7
2. Download: `RMG-Portable-Windows64-v0.6.7.zip`
3. Extract and copy these files:

| File | Copy To |
|------|---------|
| `mupen64plus.dll` | `x64\Release\` |
| `SDL2.dll` | `x64\Release\` |
| `Core\mupen64plus-video-GLideN64.dll` | `x64\Release\plugins\` |
| `Core\mupen64plus-audio-sdl2.dll` | `x64\Release\plugins\` |
| `Core\mupen64plus-input-sdl.dll` | `x64\Release\plugins\` |
| `Core\mupen64plus-rsp-hle.dll` | `x64\Release\plugins\` |
| `Config\GLideN64.ini` | `x64\Release\plugins\` |

### Option 2: Use Existing mupen64plus Installation

If you have another N64 emulator (Project64, RetroArch, etc.), you can copy the mupen64plus DLLs from there.

## ?? Add Your ROM

Place your Castlevania 64 ROM file in:
```
x64\Release\assets\Castlevania (U) (V1.2) [!].z64
```

Supported ROM names:
- `Castlevania (U) (V1.2) [!].z64` (recommended)
- `Castlevania (USA) (Rev B).z64`
- `Castlevania (USA).z64`
- `Castlevania.z64`
- `cv64.z64`

## ?? Run the Game

Once plugins and ROM are in place:
```
x64\Release\CV64_RMG.exe
```

## ?? Final Directory Structure

```
x64\Release\
??? CV64_RMG.exe           ? Built
??? mupen64plus.dll        ? Need to download
??? SDL2.dll               ? Need to download
??? cv64_camera.ini        ? Copied
??? cv64_controls.ini      ? Copied
??? cv64_graphics.ini      ? Copied
??? cv64_audio.ini         ? Copied
??? cv64_patches.ini       ? Copied
??? plugins\
?   ??? mupen64plus-video-GLideN64.dll  ? Need to download
?   ??? mupen64plus-audio-sdl2.dll      ? Need to download
?   ??? mupen64plus-input-sdl.dll       ? Need to download
?   ??? mupen64plus-rsp-hle.dll         ? Need to download
?   ??? GLideN64.ini                    ? Need to download
??? assets\
    ??? Castlevania (U) (V1.2) [!].z64  ? You provide this
```

## ?? Controls (Once Running)

| Action | Keyboard | Controller |
|--------|----------|------------|
| Movement | WASD | Left Stick |
| Attack | Z | A Button |
| Jump | X | B Button |
| Camera | Arrow Keys | Right Stick / D-Pad |
| Toggle Camera Mode | F1 | - |
| Quick Save | F5 | - |
| Quick Load | F9 | - |

## ? New Features Implemented

1. **Memory Map** (`include/cv64_memory_map.h`)
   - All CV64 memory addresses from decomp
   - Camera manager structure offsets
   - Controller addresses
   - Angle conversion utilities

2. **Graphics Plugin** (`src/cv64_gfx_plugin.cpp`)
   - GLideN64 integration
   - Plugin loading/management
   - Widescreen support
   - Screenshot capability

3. **ROM Loader** (`src/cv64_rom_loader.cpp`)
   - ROM validation
   - Region/version detection
   - Byteswap handling
   - Auto ROM finding

4. **Memory Hook** (`src/cv64_memory_hook.cpp`)
   - Real-time game state reading
   - Camera position override
   - D-PAD to camera redirection
   - Controller state manipulation

## ?? Troubleshooting

**"Could not find mupen64plus core library"**
- Download and copy mupen64plus.dll to x64\Release\

**"No graphics plugin found"**
- Download and copy GLideN64.dll to x64\Release\plugins\

**"No Castlevania 64 ROM found"**
- Place your ROM in x64\Release\assets\

**Game crashes on start**
- Ensure SDL2.dll is in x64\Release\
- Check that all plugin DLLs are present
