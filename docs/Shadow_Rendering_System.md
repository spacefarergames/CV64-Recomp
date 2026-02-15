# GLideN64 Shadow Rendering System

## Overview

This document describes the shadow rendering system that has been integrated into GLideN64's rendering pipeline. The system provides procedurally generated circular shadow textures with smooth gradients, suitable for character shadows and other soft shadow effects.

## Architecture

### Components

1. **ShadowTexture.h** - Header file defining the `ShadowTexture` class interface
2. **ShadowTexture.cpp** - Implementation of shadow texture generation and management
3. **GraphicsDrawer Integration** - Hooks into GLideN64's rendering pipeline

### Key Features

- **Procedural Generation**: Shadows are generated at runtime using a radial gradient algorithm
- **Configurable Parameters**: Shadow size, intensity, and enable/disable state can be controlled
- **Smooth Gradients**: Uses smoothstep interpolation for soft, natural-looking shadow edges
- **OpenGL Integration**: Properly integrated with GLideN64's texture cache and rendering pipeline

## Class Interface

### ShadowTexture Class

```cpp
class ShadowTexture
{
public:
    void init();                          // Initialize the shadow system
    void destroy();                       // Cleanup resources
    void update();                        // Update shadow texture if needed
    
    CachedTexture* getTexture() const;    // Get the shadow texture
    
    void setEnabled(bool enabled);        // Enable/disable shadow rendering
    bool isEnabled() const;
    
    void setIntensity(float intensity);   // Set shadow opacity (0.0 - 1.0)
    float getIntensity() const;
    
    void setShadowSize(u32 size);         // Set texture dimensions
    u32 getShadowSize() const;
    
    void preRender();                     // Setup rendering state
    void postRender();                    // Restore rendering state
};
```

### Global Instance

```cpp
extern ShadowTexture g_shadowTexture;
```

## Initialization and Cleanup

The shadow texture system is automatically initialized and destroyed as part of the GraphicsDrawer lifecycle:

### Initialization (in GraphicsDrawer::_initData())

```cpp
void GraphicsDrawer::_initData()
{
    // ... other initialization ...
    g_shadowTexture.init();
    // ...
}
```

### Cleanup (in GraphicsDrawer::_destroyData())

```cpp
void GraphicsDrawer::_destroyData()
{
    // ...
    g_shadowTexture.destroy();
    // ... other cleanup ...
}
```

## Shadow Texture Generation

### Algorithm

The shadow blob is generated using a radial gradient:

1. **Distance Calculation**: For each pixel, calculate distance from center
2. **Normalization**: Normalize distance to range [0, 1]
3. **Gradient Application**: Apply smoothstep function for soft falloff
4. **Intensity Scaling**: Multiply by intensity parameter
5. **Alpha Generation**: Convert to 8-bit alpha value

### Smoothstep Formula

```cpp
alpha = 1.0f - normalizedDist;
alpha = alpha * alpha * (3.0f - 2.0f * alpha);  // Smoothstep
alpha *= intensity;
```

### Texture Format

- **Format**: RGBA8
- **Layout**: Black (RGB=0,0,0) with varying alpha channel
- **Size**: Configurable, default 64x64 pixels
- **Filtering**: Linear filtering for smooth edges

## Usage Examples

### Basic Usage

```cpp
// The texture is automatically initialized when DisplayWindow starts
// Access it through the global instance
CachedTexture* shadowTex = g_shadowTexture.getTexture();

// Check if enabled
if (g_shadowTexture.isEnabled()) {
    // Use shadow texture in rendering
}
```

### Configuring Shadow Properties

```cpp
// Set shadow intensity (0.0 = transparent, 1.0 = fully opaque)
g_shadowTexture.setIntensity(0.7f);

// Change shadow texture size (power of 2 recommended)
g_shadowTexture.setShadowSize(128);

// Disable shadows temporarily
g_shadowTexture.setEnabled(false);
```

### Rendering with Shadows

```cpp
// Before rendering shadow geometry
g_shadowTexture.preRender();  // Sets up alpha blending

// Render shadow quads/geometry here
// Bind g_shadowTexture.getTexture() to your desired texture unit

// After rendering shadows
g_shadowTexture.postRender();  // Restores previous state
```

## Integration Points

### Where to Render Shadows

The shadow system is designed to integrate at various points in the rendering pipeline:

1. **After Opaque Geometry**: Render shadows before transparent objects
2. **Custom Combiner**: Use with custom combiners that support alpha blending
3. **Post-Processing**: Can be incorporated into post-processing effects

### Recommended Rendering Flow

```cpp
void renderScene() {
    // 1. Render opaque geometry
    drawOpaqueObjects();
    
    // 2. Render shadows
    if (g_shadowTexture.isEnabled()) {
        g_shadowTexture.preRender();
        drawShadowGeometry();
        g_shadowTexture.postRender();
    }
    
    // 3. Render transparent geometry
    drawTransparentObjects();
}
```

## Performance Considerations

### Memory Usage

- Default 64x64 RGBA texture: **16 KB**
- 128x128 RGBA texture: **64 KB**
- 256x256 RGBA texture: **256 KB**

### Optimization Tips

1. **Size Selection**: Use smallest size that provides acceptable quality
   - 64x64: Suitable for most character shadows
   - 128x128: Higher quality, minimal performance impact
   - 256x256: Only for close-up or high-detail shadows

2. **Update Frequency**: Shadow texture is generated once at initialization
   - Only regenerates when `setShadowSize()` or `setIntensity()` is called
   - Call `update()` only when parameters change

3. **Texture Reuse**: Single shadow texture can be used for multiple objects
   - Saves memory compared to per-object textures
   - Scale shadow quads in world space instead of generating multiple textures

## Advanced Usage

### Custom Shadow Shapes

To implement custom shadow shapes, modify `_generateShadowBlob()`:

```cpp
void ShadowTexture::_generateShadowBlob(u32* data, u32 size)
{
    // Custom implementation here
    // Example: Elliptical shadow, directional shadow, etc.
}
```

### Dynamic Shadows

For dynamic shadow intensity/size:

```cpp
// Called per frame or when game state changes
if (playerMoving) {
    g_shadowTexture.setIntensity(0.5f);  // Lighter shadow when moving
} else {
    g_shadowTexture.setIntensity(0.8f);  // Darker shadow when stationary
}
g_shadowTexture.update();  // Apply changes
```

### Multiple Shadow Types

While the system provides one global shadow texture, you can create additional instances:

```cpp
ShadowTexture softShadow;   // Large, soft shadow
ShadowTexture hardShadow;   // Small, sharp shadow

softShadow.init();
softShadow.setShadowSize(128);
softShadow.setIntensity(0.5f);

hardShadow.init();
hardShadow.setShadowSize(64);
hardShadow.setIntensity(0.9f);
```

## Troubleshooting

### Shadows Not Appearing

1. Check if shadow system is enabled:
   ```cpp
   assert(g_shadowTexture.isEnabled());
   ```

2. Verify texture was initialized:
   ```cpp
   assert(g_shadowTexture.getTexture() != nullptr);
   ```

3. Ensure alpha blending is enabled in your rendering code

### Shadow Quality Issues

1. **Blocky Shadows**: Increase shadow texture size
2. **Too Transparent**: Increase intensity value
3. **Too Dark**: Decrease intensity value
4. **Harsh Edges**: Verify linear filtering is enabled (done automatically)

### Performance Issues

1. Reduce shadow texture size
2. Limit number of shadow-casting objects
3. Consider LOD system for distant shadows

## API Reference

### Functions

#### `void init()`
Initializes the shadow texture system. Must be called before using any other functions.
- Creates texture on GPU
- Allocates CPU-side buffer
- Generates initial shadow blob
- **Called automatically by GraphicsDrawer**

#### `void destroy()`
Cleans up all resources associated with the shadow texture.
- Removes texture from cache
- Frees CPU buffer
- **Called automatically by GraphicsDrawer**

#### `void update()`
Regenerates and uploads shadow texture if parameters have changed.
- Only updates if `m_needsUpdate` flag is set
- Call after changing intensity or size

#### `void setEnabled(bool enabled)`
Enables or disables shadow rendering.
- **Parameters**: `enabled` - true to enable, false to disable
- Default: enabled (true)

#### `void setIntensity(float intensity)`
Sets the shadow opacity/darkness.
- **Parameters**: `intensity` - value between 0.0 (transparent) and 1.0 (opaque)
- Default: 0.7
- Triggers regeneration on next `update()` call

#### `void setShadowSize(u32 size)`
Changes the shadow texture dimensions.
- **Parameters**: `size` - texture width and height in pixels
- Must be called before `init()` or will require reinitialization
- Triggers regeneration on next `update()` call

#### `void preRender()`
Sets up rendering state for shadow drawing.
- Enables alpha blending
- Sets blend mode to (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)

#### `void postRender()`
Placeholder for restoring rendering state after shadow drawing.
- Currently empty, can be extended for state restoration

## Implementation Details

### Texture Cache Integration

The shadow texture is registered with GLideN64's texture cache using:
```cpp
m_pTexture = textureCache().addFrameBufferTexture(textureTarget::TEXTURE_2D);
```

This ensures proper lifecycle management and compatibility with the rest of the system.

### Pixel Format

Shadows use RGBA8 format with:
- **R, G, B**: Always 0 (black)
- **A**: Radial gradient from center (255) to edge (0)

This format is compatible with standard alpha blending and requires 4 bytes per pixel.

### Thread Safety

The current implementation is **not thread-safe**. All shadow texture operations must be called from the rendering thread.

## Future Enhancements

Possible improvements to the shadow system:

1. **Multiple Gradient Types**: Linear, quadratic, custom falloff curves
2. **Shadow Shapes**: Elliptical, rectangular, custom shapes
3. **Animation Support**: Animated shadows for dynamic effects
4. **HDR Support**: High dynamic range shadow intensities
5. **Texture Compression**: Reduce memory footprint
6. **Soft Shadow Variants**: Different blur amounts
7. **Colored Shadows**: Tinted shadows for special effects

## Conclusion

The shadow rendering system provides a simple, efficient way to add soft circular shadows to your N64 game renderings. It integrates seamlessly with GLideN64's existing pipeline and can be extended for more advanced shadow effects.

For questions or issues, refer to the source code in `ShadowTexture.h` and `ShadowTexture.cpp`.
