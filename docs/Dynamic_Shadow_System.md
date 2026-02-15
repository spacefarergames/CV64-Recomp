# Dynamic Shadow System for Castlevania 64

## Overview

This document describes the **dynamic shadow system** that replaces the original static circular shadow blob (entity 219 at ROM address `0x80054EB0`) with a real-time silhouette of the player character's rendered geometry.

## How It Works

### Architecture

The system captures the player's rendered triangles each frame and projects them into an offscreen framebuffer from a top-down orthographic view, creating a silhouette texture that moves and animates with the player.

### Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Frame Start: clearColorBuffer() → beginFrame()           │
│    - Clears the capture flag for the new frame              │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. Triangle Batches: drawTriangles()                         │
│    - Each batch is drawn to the main framebuffer            │
│    - Then checkAndCapturePlayer() is called:                │
│      • Read player position from RDRAM (0x80389CE0)         │
│      • Compare with current modelview matrix translation    │
│      • If match (within tolerance): this is the player!     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. Silhouette Capture: _renderSilhouette()                  │
│    - Switch to shadow FBO (128×128 RGBA8 texture)          │
│    - Clear to transparent (first capture only per frame)    │
│    - Map screen-space vertices → FBO normalized coords      │
│    - Draw triangles as flat black (0,0,0,alpha=intensity)  │
│    - Restore main FBO and viewport                          │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. Shadow Rendering: Game draws shadow entity (219)         │
│    - TextureCache binds the shadow texture                  │
│    - Now points to m_pSilhouetteTexture (not the blob!)    │
│    - Shadow quad shows the animated player silhouette       │
└─────────────────────────────────────────────────────────────┘
```

## Key Components

### Player Position Detection

**RDRAM Layout (Castlevania 64):**
- **Player Pointer**: `0x80389CE0` (system_work + 0x26228)
- **Player Position**: 
  - X: `PlayerPtr + 0x10` (float)
  - Y: `PlayerPtr + 0x14` (float)
  - Z: `PlayerPtr + 0x18` (float)

**Matching Logic:**
```cpp
// Read player world position from RDRAM
f32 playerX, playerY, playerZ;
_readPlayerPosition(playerX, playerY, playerZ);

// Compare with modelview matrix translation
const f32 mvX = gSP.matrix.modelView[gSP.matrix.modelViewi][3][0];
const f32 mvY = gSP.matrix.modelView[gSP.matrix.modelViewi][3][1];
const f32 mvZ = gSP.matrix.modelView[gSP.matrix.modelViewi][3][2];

// If distance < tolerance: this is the player
if (sqrt(dx² + dy² + dz²) < 8.0f) {
    // Capture this geometry
}
```

### Silhouette Rendering

The captured triangles are re-rendered into the shadow FBO using a simple mapping:

1. **Coordinate Transformation:**
   - Find bounding box of screen-space vertices
   - Expand with 15% padding
   - Make square (maintain aspect ratio)
   - Map to NDC [-1, +1] within FBO viewport

2. **Vertex Attributes:**
   - Position: Remapped screen coords → NDC
   - Color: Black `(0,0,0)` with `alpha = m_intensity`
   - Mode: Flat shading (no lighting, no texturing)

3. **Rendering State:**
   - FBO: `m_shadowFBO` (128×128)
   - Blend: `GL_ONE, GL_ONE` (additive – accumulates silhouette)
   - Depth: Disabled (2D projection)

### Fallback to Circular Blob

The system gracefully falls back to the procedural circular shadow if:
- Dynamic shadows are disabled (`m_dynamicEnabled = false`)
- RDRAM is not available or invalid
- Player position cannot be read
- No player geometry is detected in the frame

## Configuration

### ShadowTexture API

```cpp
// Enable/disable the entire shadow system
g_shadowTexture.setEnabled(bool enabled);

// Enable/disable dynamic capture (vs. static blob)
g_shadowTexture.setDynamicEnabled(bool enabled);

// Set shadow darkness (0.0 = transparent, 1.0 = opaque)
g_shadowTexture.setIntensity(float intensity);

// Set shadow texture resolution (16–512)
g_shadowTexture.setShadowSize(u32 size);
```

### Default Settings

- **Shadow Size**: 128×128 pixels
- **Intensity**: 0.7 (70% opaque)
- **Enabled**: `true`
- **Dynamic Enabled**: `true`
- **Position Tolerance**: 8.0 world units

## Implementation Details

### Memory Layout

**ShadowTexture Members:**
- `m_pTexture`: Public texture pointer (points to silhouette or blob)
- `m_pTextureData`: CPU buffer for procedural blob fallback
- `m_pSilhouetteTexture`: FBO colour attachment (the dynamic shadow)
- `m_shadowFBO`: Offscreen framebuffer for capture
- `m_capturedThisFrame`: Prevents duplicate clearing per frame

### Performance Considerations

1. **Capture Overhead**: ~1–2 additional draw calls per frame
   - Only triggered when player geometry is detected
   - FBO is small (128×128) → low fill-rate cost

2. **Memory**: ~64 KB per shadow texture (128×128 RGBA8)
   - Static blob: 1 texture
   - Dynamic: 2 textures (blob + silhouette)

3. **RDRAM Reads**: 5× 32-bit reads per `drawTriangles()` call
   - Pointer: 1 read
   - Position: 3 reads (X, Y, Z)
   - Fast (cached by CPU)

## Limitations & Future Work

### Current Limitations

1. **Single Player Only**: Only captures one player character
2. **Screen-Space Projection**: Not a true 3D projected shadow
3. **No Sub-pixel AA**: Silhouette edges may appear aliased
4. **No Shadow Softening**: Hard silhouette edges (no blur/fade)

### Potential Enhancements

1. **Gaussian Blur**: Apply a blur pass to soften the silhouette
2. **Perspective Projection**: Use the light direction to cast a proper shadow
3. **Multiple Shadows**: Support enemies and multiple characters
4. **Texture Address Detection**: Hook `gDP.textureImage.address == 0x54EB0` to detect shadow draws explicitly
5. **Shader-Based Capture**: Use a dedicated shader for cleaner FBO rendering

## Integration Points

### Modified Files

- **`ShadowTexture.h`**: Added FBO members, capture API
- **`ShadowTexture.cpp`**: Implemented dynamic capture logic
- **`GraphicsDrawer.cpp`**:
  - `drawTriangles()`: Added `checkAndCapturePlayer()` hook
  - `clearColorBuffer()`: Added `beginFrame()` call

### Dependencies

- **GLideN64 Core**: `gfxContext`, `textureCache()`, `frameBufferList()`
- **N64 State**: `gSP.matrix.modelView`, `gDP.scissor`, `RDRAM`
- **Graphics API**: `createFramebuffer()`, `bindFramebuffer()`, `drawTriangles()`

## Testing

### Verification Steps

1. **Check Shadow Movement**:
   - Move the player character
   - Shadow silhouette should follow and animate
   - Compare to original circular blob

2. **Test Fallback**:
   - Disable dynamic shadows: `g_shadowTexture.setDynamicEnabled(false)`
   - Should revert to circular blob

3. **Performance**:
   - Monitor frame rate in Shadow Man area (many objects)
   - Should have negligible impact (<1% frame time)

### Debug Logging

Add logging to verify capture:
```cpp
void ShadowTexture::_renderSilhouette(...)
{
    printf("SHADOW: Captured player geometry: %d tris\n", _numElements / 3);
    // ... rest of function
}
```

## Conclusion

The dynamic shadow system transforms the static circular blob into a living, animated shadow that accurately reflects the player's pose and movement. It leverages the existing GLideN64 rendering pipeline, RDRAM memory access, and modern FBO techniques to deliver a cinematic shadow effect with minimal performance overhead.

**Result**: Shadows now look like the actual player character, not just a circle!
