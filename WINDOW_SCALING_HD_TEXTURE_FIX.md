# Window Scaling and HD Texture Fix

## Issues Fixed

### Issue 1: Game Not Stretching to Window Size
**Problem**: The game rendered at a fixed resolution and appeared in a black box when the window was maximized or resized. It didn't scale to fill the window.

**Root Cause**: 
1. AspectRatio was set to `2` (force 16:9) which maintains aspect ratio but doesn't stretch
2. GLideN64 used configured `ScreenWidth`/`ScreenHeight` from INI file instead of actual window size
3. Viewport wasn't being updated properly on window resize

**Fix Applied**:
1. Changed default `AspectRatio` mode from `2` (force 16:9) to `0` (stretch to fill)
2. Updated `CV64_Config_ApplyToGLideN64()` to use actual window size from video extension
3. Updated `VidExt_SetMode()` to use actual window client area size
4. Updated `VidExt_NotifyResize()` to properly update viewport on window resize

**Files Modified**:
- `src/cv64_config_bridge.cpp` - Use window size, default to stretch mode
- `src/cv64_vidext.cpp` - Proper viewport management
- `patches/cv64_graphics.ini` - Updated defaults

---

### Issue 2: HD Textures Not Being Detected
**Problem**: HD texture packs weren't loading even with `tx_hires_enable=true`

**Root Causes**:
1. `txHresAltCRC` was disabled - required for Rice format texture packs
2. `txStrongCRC` wasn't being set
3. `txForce16bpp` might have been on, reducing texture quality

**Fix Applied**:
1. Enabled `tx_hires_alt_crc=true` by default (required for Rice format HD packs)
2. Added `tx_strong_crc=true` for better texture matching
3. Force `txForce16bpp=false` to allow full quality HD textures
4. Added comprehensive debug logging for HD texture configuration

**Files Modified**:
- `src/cv64_config_bridge.cpp` - Enable alternate CRC, strong CRC, disable force 16bpp
- `patches/cv64_graphics.ini` - Updated defaults

---

## Configuration Changes

### cv64_graphics.ini Defaults Changed

**AspectRatio Section**:
```ini
[AspectRatio]
; 0=stretch to fill window (recommended for proper window resizing)
aspect_mode=0
stretch=true
```

**TextureFilter Section**:
```ini
[TextureFilter]
; Enable alternate CRC - required for Rice format HD texture packs
tx_hires_alt_crc=true
; Enable strong CRC for better texture matching
tx_strong_crc=true
```

---

## AspectRatio Modes Explained

| Mode | Name | Description |
|------|------|-------------|
| 0 | Stretch | Stretch to fill entire window (may distort aspect ratio) |
| 1 | Force 4:3 | Maintain 4:3 aspect ratio with letterboxing |
| 2 | Force 16:9 | Maintain 16:9 aspect ratio with letterboxing |
| 3 | Adjust 4:3 | Adjust for 4:3 display |
| 4 | Adjust 16:9 | Adjust for 16:9 display |

**Default changed from 2 ? 0** to allow game to fill window on resize/maximize.

---

## HD Texture Pack Requirements

For HD textures to work, GLideN64 looks for textures in:
- `assets/hires_texture/CASTLEVANIA/`
- `assets/hires_texture/CASTLEVANIA_HIRESTEXTURES/`

**Texture Pack Formats Supported**:
1. **GLideN64 Native** - Uses CRC-based naming
2. **Rice Format** - Requires `tx_hires_alt_crc=true` (now enabled by default)
3. **HTX Compressed** - Pre-compressed texture packs

**Debug Output** now shows:
```
[CV64_CONFIG] HD Texture path: C:\Games\CV64\assets\hires_texture
[CV64_CONFIG] HD Textures enabled: YES
[CV64_CONFIG] HD Texture Alt CRC: YES
[CV64_CONFIG] HD Texture Full Alpha: YES
[CV64_CONFIG] HD Texture File Storage: YES
[CV64_CONFIG] GLideN64 will look for textures in: ...\CASTLEVANIA or ...\CASTLEVANIA_HIRESTEXTURES
```

---

## Technical Details

### Viewport Management
```cpp
// VidExt_SetMode now uses actual window size:
RECT clientRect;
GetClientRect(s_mainWindow, &clientRect);
s_width = clientRect.right - clientRect.left;
s_height = clientRect.bottom - clientRect.top;
glViewport(0, 0, s_width, s_height);
```

### Config Bridge Window Size Detection
```cpp
// CV64_Config_ApplyToGLideN64 gets actual window size:
int vidExtWidth = 0, vidExtHeight = 0;
CV64_VidExt_GetSize(&vidExtWidth, &vidExtHeight);
if (vidExtWidth > 0 && vidExtHeight > 0) {
    screenWidth = vidExtWidth;
    screenHeight = vidExtHeight;
}
ConfigSetParameter(videoGeneral, "ScreenWidth", M64TYPE_INT, &screenWidth);
ConfigSetParameter(videoGeneral, "ScreenHeight", M64TYPE_INT, &screenHeight);
```

### HD Texture CRC Settings
```cpp
// Enable alternate CRC for Rice format packs (default: true)
bool txHiresAltCrc = s_graphicsIni.GetBool("TextureFilter", "tx_hires_alt_crc", true);
ConfigSetParameter(videoGliden64, "txHresAltCRC", M64TYPE_BOOL, &txHiresAltCrc);

// Enable strong CRC for better matching
bool txStrongCRC = s_graphicsIni.GetBool("TextureFilter", "tx_strong_crc", true);
ConfigSetParameter(videoGliden64, "txStrongCRC", M64TYPE_BOOL, &txStrongCRC);

// Disable force 16bpp for full quality
bool txForce16bpp = false;
ConfigSetParameter(videoGliden64, "txForce16bpp", M64TYPE_BOOL, &txForce16bpp);
```

---

## Testing Checklist

### Window Scaling
- [ ] Launch game at default 1280x720
- [ ] Maximize window - game should fill entire window
- [ ] Resize window to different size - game should scale
- [ ] Restore window - game returns to normal size

### HD Textures
- [ ] Place HD texture pack in `assets/hires_texture/CASTLEVANIA/`
- [ ] Check debug output shows "HD Textures enabled: YES"
- [ ] Check debug output shows "HD Texture Alt CRC: YES"
- [ ] Verify HD textures appear in-game
- [ ] Test with Rice format texture pack
- [ ] Test with GLideN64 native format texture pack

---

## User Instructions

### To Disable Stretch (Maintain Aspect Ratio)
Edit `patches/cv64_graphics.ini`:
```ini
[AspectRatio]
aspect_mode=2   ; Force 16:9 with letterboxing
stretch=false
```

### To Enable HD Textures
1. Download a Castlevania 64 HD texture pack
2. Extract to `assets/hires_texture/CASTLEVANIA/` or `assets/hires_texture/CASTLEVANIA_HIRESTEXTURES/`
3. Ensure `patches/cv64_graphics.ini` has:
   ```ini
   [Enhancements]
   tx_hires_enable=true
   tx_hires_full_alpha=true
   
   [TextureFilter]
   tx_hires_alt_crc=true
   ```
4. Launch game - textures should load automatically

---

## Build Verification
? Build successful
? Window scaling changes applied
? HD texture configuration changes applied
? Debug logging enhanced
