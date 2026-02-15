# Shadow Rendering Integration Example

## Quick Start Guide

This guide shows how to use the GLideN64 shadow rendering system in your code.

## Basic Setup (Already Done)

The shadow system is automatically initialized when GLideN64 starts:

```cpp
// In GraphicsDrawer::_initData()
g_shadowTexture.init();

// In GraphicsDrawer::_destroyData()
g_shadowTexture.destroy();
```

## Example 1: Simple Shadow Rendering

```cpp
// In your rendering code
void renderCharacterWithShadow() {
    // 1. Render the character geometry
    drawCharacterModel();
    
    // 2. Setup shadow rendering
    g_shadowTexture.preRender();  // Enables alpha blending
    
    // 3. Get shadow texture
    CachedTexture* shadowTex = g_shadowTexture.getTexture();
    
    // 4. Bind shadow texture to your desired texture unit
    Context::BindTextureParameters bindParams;
    bindParams.texture = shadowTex->name;
    bindParams.textureUnitIndex = textureIndices::Tex[0];
    bindParams.target = textureTarget::TEXTURE_2D;
    gfxContext.bindTexture(bindParams);
    
    // 5. Draw shadow quad at character's feet
    // (position it on the ground below the character)
    drawShadowQuad(characterX, groundY, characterZ);
    
    // 6. Restore rendering state
    g_shadowTexture.postRender();
}
```

## Example 2: Drawing a Shadow Quad

```cpp
void drawShadowQuad(float x, float y, float z) {
    // Define shadow quad vertices (flat on ground)
    float shadowSize = 2.0f;  // World units
    
    RectVertex shadowVerts[4] = {
        // Position (x, y, z, w)              // Tex coords (s, t)
        {x - shadowSize, y, z - shadowSize, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f},
        {x + shadowSize, y, z - shadowSize, 1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        {x + shadowSize, y, z + shadowSize, 1.0f,  1.0f, 1.0f, 0.0f, 0.0f},
        {x - shadowSize, y, z + shadowSize, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f}
    };
    
    // Draw the quad (this is pseudo-code, adapt to your pipeline)
    drawTexturedQuad(shadowVerts, 4);
}
```

## Example 3: Dynamic Shadow Intensity

```cpp
// Adjust shadow based on time of day or lighting
void updateShadowIntensity(float timeOfDay) {
    // Noon: Strong shadows (0.8)
    // Dawn/Dusk: Weak shadows (0.3)
    float intensity = 0.3f + 0.5f * abs(cos(timeOfDay * M_PI));
    
    g_shadowTexture.setIntensity(intensity);
    g_shadowTexture.update();
}

// Adjust shadow based on distance from light source
void updateShadowByDistance(float distanceToLight) {
    float intensity = 1.0f - (distanceToLight / maxLightDistance);
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    
    g_shadowTexture.setIntensity(intensity);
    g_shadowTexture.update();
}
```

## Example 4: Multiple Objects with Same Shadow

```cpp
void renderMultipleCharactersWithShadows() {
    // Setup once
    g_shadowTexture.preRender();
    CachedTexture* shadowTex = g_shadowTexture.getTexture();
    
    // Bind shadow texture once
    Context::BindTextureParameters bindParams;
    bindParams.texture = shadowTex->name;
    bindParams.textureUnitIndex = textureIndices::Tex[0];
    bindParams.target = textureTarget::TEXTURE_2D;
    gfxContext.bindTexture(bindParams);
    
    // Draw multiple shadows
    for (auto& character : characters) {
        drawShadowQuad(
            character.position.x,
            character.groundY,
            character.position.z
        );
    }
    
    // Cleanup once
    g_shadowTexture.postRender();
}
```

## Example 5: Configurable Shadow System

```cpp
struct ShadowConfig {
    bool enabled = true;
    float intensity = 0.7f;
    u32 textureSize = 64;
};

void initializeShadowSystem(const ShadowConfig& config) {
    g_shadowTexture.setEnabled(config.enabled);
    g_shadowTexture.setIntensity(config.intensity);
    g_shadowTexture.setShadowSize(config.textureSize);
    
    // Shadow system is already initialized by GraphicsDrawer
    // Just update parameters
    g_shadowTexture.update();
}

// Usage
void setupGraphics() {
    ShadowConfig config;
    config.enabled = true;
    config.intensity = 0.8f;
    config.textureSize = 128;  // Higher quality
    
    initializeShadowSystem(config);
}
```

## Example 6: Integrating with Existing Rendering Pipeline

```cpp
// Hook into drawTriangles or similar method
void GraphicsDrawer::drawSceneWithShadows() {
    // Draw regular geometry
    drawTriangles();
    
    // Optional: Draw shadows for specific objects
    if (shouldDrawShadows()) {
        renderObjectShadows();
    }
}

bool shouldDrawShadows() {
    // Check game state, settings, performance, etc.
    return g_shadowTexture.isEnabled() &&
           config.graphicsEnhancements.shadows &&
           frameRate > 30.0f;  // Only if performance is good
}

void renderObjectShadows() {
    // This would be called after main geometry is drawn
    // but before UI/HUD elements
    
    g_shadowTexture.preRender();
    
    // Draw shadows for all shadow-casting objects
    // Implementation depends on your game's object system
    
    g_shadowTexture.postRender();
}
```

## Example 7: Shadow LOD System

```cpp
// Use different shadow sizes based on distance from camera
void updateShadowLOD(float cameraDistance) {
    u32 shadowSize;
    
    if (cameraDistance < 10.0f) {
        shadowSize = 128;  // High detail, close up
    } else if (cameraDistance < 50.0f) {
        shadowSize = 64;   // Medium detail
    } else {
        shadowSize = 32;   // Low detail, far away
    }
    
    if (g_shadowTexture.getShadowSize() != shadowSize) {
        g_shadowTexture.setShadowSize(shadowSize);
        g_shadowTexture.update();
    }
}
```

## Example 8: Conditional Shadow Rendering

```cpp
void renderCharacter(const Character& character) {
    // Always render the character
    drawCharacterModel(character);
    
    // Only render shadow if:
    // 1. Shadow system is enabled
    // 2. Character is on ground (not jumping)
    // 3. Character is visible to camera
    if (g_shadowTexture.isEnabled() &&
        character.isOnGround &&
        character.isVisible) {
        
        g_shadowTexture.preRender();
        drawShadowQuad(
            character.position.x,
            character.groundY,
            character.position.z
        );
        g_shadowTexture.postRender();
    }
}
```

## Example 9: Shadow Color Tinting (Advanced)

While the default shadow is black, you can create tinted shadows:

```cpp
void drawTintedShadow(float x, float y, float z, float r, float g, float b, float a) {
    g_shadowTexture.preRender();
    
    // Set a color that will modulate with the shadow texture
    // This requires custom combiner setup
    gDP.primColor.r = r;
    gDP.primColor.g = g;
    gDP.primColor.b = b;
    gDP.primColor.a = a;
    
    // Bind shadow texture
    CachedTexture* shadowTex = g_shadowTexture.getTexture();
    // ... bind texture ...
    
    drawShadowQuad(x, y, z);
    
    g_shadowTexture.postRender();
}

// Usage: Blue-tinted shadow
drawTintedShadow(x, y, z, 0.2f, 0.2f, 0.8f, 0.7f);
```

## Example 10: Performance Monitoring

```cpp
struct ShadowStats {
    u32 shadowsDrawn = 0;
    float lastUpdateTime = 0.0f;
};

ShadowStats shadowStats;

void renderFrameWithShadows() {
    shadowStats.shadowsDrawn = 0;
    float frameStart = getCurrentTime();
    
    // Render scene with shadows
    if (g_shadowTexture.isEnabled()) {
        g_shadowTexture.preRender();
        
        for (auto& obj : visibleObjects) {
            if (obj.castsShadow) {
                drawShadowQuad(obj.position);
                shadowStats.shadowsDrawn++;
            }
        }
        
        g_shadowTexture.postRender();
    }
    
    shadowStats.lastUpdateTime = getCurrentTime() - frameStart;
    
    // Log if performance is poor
    if (shadowStats.lastUpdateTime > 1.0f / 60.0f) {
        LOG_WARNING("Shadow rendering took %f ms", 
                    shadowStats.lastUpdateTime * 1000.0f);
    }
}
```

## Best Practices

1. **Initialize Once**: Let GraphicsDrawer handle init/destroy
2. **Batch Shadow Rendering**: Draw all shadows in one pass
3. **Reuse Texture**: One shadow texture can serve multiple objects
4. **Check Enabled State**: Always check `isEnabled()` before rendering
5. **Update Sparingly**: Only call `update()` when parameters change
6. **Use Appropriate Size**: Start with 64x64, increase only if needed
7. **Consider LOD**: Use smaller textures for distant shadows
8. **Profile Performance**: Monitor frame time impact of shadow rendering

## Common Issues and Solutions

### Issue: Shadows not visible
**Solution**: Check alpha blending is enabled and shadow intensity is > 0

### Issue: Shadows too sharp
**Solution**: Increase shadow texture size or adjust gradient falloff

### Issue: Performance drops
**Solution**: Reduce shadow count, texture size, or disable for distant objects

### Issue: Shadows wrong color
**Solution**: Verify texture uses correct RGBA format with black RGB values

## Next Steps

- Review `docs/Shadow_Rendering_System.md` for detailed documentation
- Examine `ShadowTexture.cpp` for implementation details
- Integrate shadow rendering into your specific use case
- Experiment with intensity and size values for best visual results

