# Castlevania 64 PC Recomp - Research Notes

## Source Materials

### Excel Spreadsheet Reference
Location: `C:\Users\patte\Downloads\Castlevania 64 - Research Spreadsheets.xlsx`

This spreadsheet contains valuable research data for the recomp project.
Key areas to investigate:
- Memory addresses
- Function signatures
- Game state variables
- Asset offsets
- Bug documentation

### cv64 Decomp Source
Location: `C:\Users\patte\cv64`

Key files for our implementation:
```
include/
??? game/
?   ??? controller.h      - Input handling
?   ??? gfx/
?   ?   ??? camera.h      - Camera structures
?   ??? objects/
?       ??? camera/
?           ??? cameraMgr.h - Camera manager
```

### RMG Reference
Location: `C:\Users\patte\source\repos\CV64_RMG\RMG`

Useful reference for:
- N64 emulator integration patterns
- Input handling approaches
- Configuration management
- Plugin architecture

---

## Camera System Analysis

### Original Camera Behavior

From `cameraMgr.h`:
```c
typedef struct cameraMgr_data {
    u32 player_camera_state;
    u32 camera_mode;
    u32 previous_camera_mode;
    Camera* camera_player;
    Vec3f camera_position;
    Vec3f camera_look_at_direction;
    // ... many more fields
} cameraMgr_data;
```

### Camera Modes
| Mode | Value | Description |
|------|-------|-------------|
| NORMAL | 0 | Standard third-person |
| BATTLE | 1 | Combat camera |
| ACTION | 2 | Action sequence |
| BOSS | 3 | Boss fight |
| READING | 4 | Reading text |
| FIRST_PERSON | 5 | First-person view |
| CUTSCENE | 6 | Cutscene camera |
| LOCKED | 7 | Fixed camera |

### Camera Patch Integration Points

1. **cameraMgr_loop()** - Main update function
   - Hook here to override camera angles
   
2. **cameraMgr_setReadingTextState()** - Text display
   - Lock camera during text

3. **Player position tracking**
   - Update look-at target

### Angle System

N64 uses 16-bit angles:
- 0x0000 = 0°
- 0x4000 = 90°
- 0x8000 = 180°
- 0xC000 = 270°

Conversion:
```c
float degrees = (s16_angle / 65536.0f) * 360.0f;
s16 angle = (degrees / 360.0f) * 65536;
```

---

## Controller System Analysis

### Original Controller Structure
```c
typedef struct Controller {
    u16 is_connected;
    u16 btns_held;
    u16 btns_pressed;
    s16 joy_x;      // -80 to +80
    s16 joy_y;      // -80 to +80
    s16 joy_ang;    // Calculated angle
    s16 joy_held;   // Magnitude
} Controller;
```

### D-PAD Issues in Original

The original game uses D-PAD for:
- Menu navigation
- Some in-game actions

Our patch redirects D-PAD to camera control, which means:
- Must track when D-PAD should control camera vs. menus
- Need mode detection (gameplay vs. menu)

### Solution: Dual-Purpose D-PAD

1. In gameplay: D-PAD ? Camera rotation
2. In menus: D-PAD ? Menu navigation
3. Toggle key: F1 switches modes

---

## Graphics Notes

### N64 Display Characteristics
- Native resolution: 320x240
- Color depth: 16-bit or 32-bit
- Frame rate: ~20-30 FPS
- Fog used extensively

### Widescreen Considerations

FOV correction formula:
```c
float widescreen_fov = original_fov * (screen_width / screen_height) / (4.0f / 3.0f);
```

### Draw Distance

Original fog hides draw distance limits.
For PC, we can:
1. Extend draw distance
2. Adjust fog accordingly
3. Or keep original behavior

---

## Audio Notes

### N64 Audio Characteristics
- Sample rate: ~32kHz
- Compression: VADPCM
- Reverb: Hardware RSP

### PC Audio Approach
- Resample to 48kHz
- Software reverb (optional)
- Support for HD music replacement

---

## Known Bugs to Fix

1. **Camera Clipping** - Camera passes through walls
2. **Collision Glitches** - Player gets stuck
3. **Softlocks** - Game freezes in certain conditions
4. **Audio Pops** - Audio glitches during transitions

---

## Memory Addresses (To Research)

| Purpose | Address | Notes |
|---------|---------|-------|
| Player Position | TBD | Vec3f |
| Camera State | TBD | cameraMgr_data* |
| Game State | TBD | Main state machine |
| Controller Input | TBD | Controller[4] |

---

## ROM Information

### USA Version
- Name: CASTLEVANIA
- ID: NDCE
- CRC: TBD

### Japan Version
- Name: AKUMAJOU DRACULA
- ID: NDCJ
- CRC: TBD

### Europe Version
- Name: CASTLEVANIA
- ID: NDCP
- CRC: TBD

---

## TODO List

### High Priority
- [ ] Complete camera patch integration
- [ ] Test with actual ROM
- [ ] Graphics backend implementation

### Medium Priority
- [ ] Audio system
- [ ] Save states
- [ ] Configuration UI

### Low Priority
- [ ] HD texture support
- [ ] Shader effects
- [ ] Achievement system

---

## References

- [cv64 decomp GitHub](https://github.com/cv64/cv64)
- [N64 Developer Documentation](http://n64devkit.square7.ch/)
- [RMG GitHub](https://github.com/Rosalie241/RMG)
- [mupen64plus](https://mupen64plus.org/)
