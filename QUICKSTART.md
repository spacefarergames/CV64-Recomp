# Castlevania 64 PC Recomp - Quick Start Guide

## ?? Getting Started

### Step 1: Requirements

- Windows 10/11 64-bit
- Visual Studio 2022 (for building)
- Xbox controller (recommended) or keyboard/mouse
- Castlevania 64 ROM file (legally obtained)
- ~200MB disk space

### Step 2: Build the Project

1. Open `CV64_RMG.sln` in Visual Studio 2022
2. Set configuration to **Release** and platform to **x64**
3. Build the solution (Ctrl+Shift+B)
4. Or run `build_and_test.bat` from command line

### Step 3: Setup mupen64plus Plugins

Run the plugin setup script:
```powershell
.\scripts\copy_plugins.ps1 -Configuration Release
```

Or manually download from [RMG Releases](https://github.com/Rosalie241/RMG/releases) and copy:
- `mupen64plus.dll` ? `x64\Release\`
- `mupen64plus-video-GLideN64.dll` ? `x64\Release\plugins\`
- `mupen64plus-audio-sdl2.dll` ? `x64\Release\plugins\`
- `mupen64plus-input-sdl.dll` ? `x64\Release\plugins\`
- `mupen64plus-rsp-hle.dll` ? `x64\Release\plugins\`

### Step 4: Add Your ROM

Place your Castlevania 64 ROM in `x64\Release\assets\`:
- Recommended: `Castlevania (U) (V1.2) [!].z64`
- Also works: `Castlevania (USA).z64`, `Castlevania.z64`

### Step 5: First Launch

1. Run `x64\Release\CV64_RMG.exe`
2. The main window will open showing the splash screen
3. If ROM auto-detection fails: File ? Open ROM
4. Press Start or Enter to begin!

---

## ??? Controls Overview

### Keyboard (Default)

```
Movement:  WASD
Attack:    Z
Jump:      X
Z-Trigger: C
Start:     Enter
Camera:    Arrow Keys (with D-PAD camera mode)
```

### Xbox Controller

```
Movement:     Left Stick
Attack:       A
Jump:         B
Z-Trigger:    LB
Start:        Start
Camera:       Right Stick (or D-PAD with mode enabled)
```

---

## ?? D-PAD Camera Control

**THIS IS THE BEST NEW FEATURE!**

By default, D-PAD camera mode is **ENABLED**. This means:
- Arrow keys (keyboard) or D-PAD (controller) rotate the camera
- Right analog stick also rotates the camera
- You have full 360° control!

### Toggle Camera Mode

Press **F1** to toggle D-PAD camera mode on/off.

When OFF:
- D-PAD functions as original (menu navigation, etc.)

When ON:
- D-PAD rotates the camera
- Use analog stick for movement instead

---

## ?? Configuration

### Location

All config files are in the `patches/` folder:
- `cv64_camera.ini` - Camera settings
- `cv64_controls.ini` - Control settings
- `cv64_graphics.ini` - Graphics settings
- `cv64_audio.ini` - Audio settings
- `cv64_patches.ini` - Patch toggles

### Quick Adjustments

#### Camera Too Fast?
Edit `patches/cv64_camera.ini`:
```ini
dpad_rotation_speed=1.0  ; Lower = slower (default 2.0)
```

#### Invert Camera?
```ini
invert_y=true
```

#### Mouse Sensitivity?
```ini
mouse_sensitivity_x=0.10  ; Lower = less sensitive
mouse_sensitivity_y=0.10
```

---

## ?? Hotkeys

| Key | Action |
|-----|--------|
| F1 | Toggle D-PAD Camera Mode |
| F2 | Toggle Camera Patch |
| F5 | Quick Save |
| F9 | Quick Load |
| Ctrl+R | Reset Camera |
| ESC | Pause Game |

---

## ? Troubleshooting

### Camera Not Working?

1. Make sure camera patch is enabled (F2)
2. Check that D-PAD camera mode is on (F1)
3. Verify `patches/cv64_camera.ini` has `enabled=true`

### Controller Not Detected?

1. Connect controller before launching
2. Make sure XInput driver is installed
3. Try a different USB port

### Low FPS?

1. Lower internal resolution scale in `cv64_graphics.ini`
2. Disable anti-aliasing
3. Enable V-Sync

### No Sound?

1. Check `cv64_audio.ini` settings
2. Make sure master volume > 0
3. Try different audio backend

---

## ?? Support

For issues and questions:
1. Check the documentation files (README.md, DEVELOPER.md)
2. Review RESEARCH.md for known issues
3. Check the GitHub issues page

---

**Enjoy your enhanced Castlevania 64 experience!**

*"Die, monster! You don't belong in this world!"*
