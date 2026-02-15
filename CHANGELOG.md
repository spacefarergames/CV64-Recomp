# Castlevania 64 PC Recomp - Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2024-XX-XX

### Added
- **Memory Map Integration** (`cv64_memory_map.h`)
  - All memory addresses from cv64 decomp (symbol_addrs.txt)
  - System work structure at 0x80363AB8
  - Camera manager offsets and structures
  - Controller data addresses
  - Angle conversion utilities (N64 s16 ? degrees/radians)
  - Big-endian memory read/write helpers

- **Graphics Plugin Integration** (`cv64_gfx_plugin.cpp`)
  - GLideN64 plugin loading and management
  - Angrylion Plus support
  - Parallel RDP (Vulkan) support
  - Plugin auto-detection
  - Widescreen configuration
  - Screenshot capability
  - FPS tracking

- **ROM Loader** (`cv64_rom_loader.cpp`)
  - ROM validation and header parsing
  - Region detection (USA/Japan/Europe)
  - Version detection (1.0, 1.1, 1.2)
  - CRC verification for known CV64 ROMs
  - Byteswap handling (z64/n64/v64 formats)
  - Auto ROM finder in common locations

- **Memory Hook System** (`cv64_memory_hook.cpp`)
  - Real-time N64 RDRAM access
  - Camera data reading/writing
  - Controller state manipulation
  - D-PAD to camera input redirection
  - Game state detection (gameplay vs menu/cutscene)
  - Debug state dumping

- **Build & Test Scripts**
  - `build_and_test.bat` - One-click build and setup
  - `scripts/copy_plugins.ps1` - Plugin copy automation
  - `download_plugins.bat` - Plugin download helper

- **Documentation**
  - `TESTING.md` - Complete testing instructions
  - Updated QUICKSTART.md with build steps
  - Memory address reference documentation

### Changed
- Project now builds with all new source files
- Camera patch now integrates with actual game memory
- D-PAD input is intercepted and redirected to camera control

### Technical
- Added CV64_API macro for proper DLL linkage
- Implemented endian-aware memory access
- Camera hook reads player position from N64 RAM
- Frame callback updates patches each emulated frame

## [0.1.0] - 2024-XX-XX

### Added
- Initial project structure
- RMG (Rosalie's Mupen GUI) cloned for reference
- Core type definitions (`cv64_types.h`)
- Controller input system with XInput support
- **D-PAD Camera Control Patch** - THE most requested feature!
  - D-PAD rotation support
  - Right analog stick support
  - Mouse camera control
  - Configurable sensitivity
  - Pitch limits
  - Camera smoothing
- Configuration file templates (INI format)
- Comprehensive documentation
- Basic Windows application framework
- 16:9 widescreen window by default

### Technical
- Project integrates with cv64 decomp structures
- Modular patch system designed for extensibility
- Frame-rate independent input processing

---

## Memory Addresses Reference (from cv64 decomp)

Key addresses used for camera hook:

| Address | Description |
|---------|-------------|
| `0x80363AB8` | system_work (main game state) |
| `0x8009B438` | common_camera_game_view |
| `0x80016A60` | controller_Init |
| `0x80066CE0` | cameraMgr_loop_gameplay |
| `0x80066F10` | cameraMgr_calcCameraState |

---

## Camera Patch Details

### Input Methods
- **D-PAD / Arrow Keys**: Digital camera rotation
- **Right Analog Stick**: Smooth analog rotation
- **Mouse**: PC-native camera control (when captured)

### Features
- Configurable rotation speed
- X/Y axis inversion options
- Pitch angle limits (prevent over-rotation)
- Optional camera smoothing
- Auto-center mode (optional)
- Mode-aware (respects cutscenes, first-person, etc.)

### Configuration
All settings in `patches/cv64_camera.ini`:
```ini
[Camera]
enabled=true
enable_dpad=true
enable_right_stick=true
enable_mouse=true

[Sensitivity]
dpad_rotation_speed=2.0
stick_sensitivity_x=1.5
mouse_sensitivity_x=0.15
```

---

## Credits

- **cv64 Decomp Team** - Game decompilation
- **Rosalie241** - RMG and mupen64plus work
- **Konami** - Original game

## License

Educational and preservation purposes only.
Castlevania 64 is property of Konami.
