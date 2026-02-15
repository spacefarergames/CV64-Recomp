# Advanced Graphics Features - Implementation Guide

## Overview

Three major enhancements have been implemented:
1. **Window Title Auto-Disable** - Stops updating after 3 consecutive failures
2. **Ambient Occlusion (SSAO)** - Adds realistic shadows in corners and crevices  
3. **Screen Space Reflections (SSR)** - Adds reflections to flat surfaces

---

## 1. Window Title Auto-Disable ✅ IMPLEMENTED

### Problem
Window title could continuously try to update even if memory addresses are invalid, wasting CPU cycles.

### Solution
Tracks consecutive failures and disables updates after 3 attempts.

### Implementation
```cpp
static u32 failureCount = 0;
static bool titleUpdateDisabled = false;

// Check if values are valid
bool valuesAreValid = (playerChar <= 1 && difficulty <= 1);

if (!valuesAreValid) {
    failureCount++;
    if (failureCount >= 3) {
        titleUpdateDisabled = true;
        LOG(LOG_WARNING, "Window title updates disabled after 3 consecutive failures");
    }
    return;  // Skip update
} else {
    failureCount = 0;  // Reset on success
}
```

### Behavior
- **First failure**: Increment counter, continue trying
- **Second failure**: Increment counter, continue trying
- **Third failure**: Disable updates permanently, log warning
- **Any success**: Reset counter to 0

### Benefits
- ✅ No wasted CPU on impossible operations
- ✅ Cleaner debug logs
- ✅ Prevents potential instability from repeated failures

---

## 2. Ambient Occlusion (SSAO) 🚧 FRAMEWORK READY

### What is SSAO?
Screen Space Ambient Occlusion adds realistic shadows where geometry meets:
- Corners of rooms
- Crevices in walls
- Where objects sit on floors
- Around character feet

### Technical Approach

#### Algorithm Overview
1. **Sample Hemisphere**: For each pixel, sample points in a hemisphere around it
2. **Depth Comparison**: Check if sampled points are occluded by geometry
3. **Accumulate**: Count how many samples are occluded
4. **Apply**: Darken pixel based on occlusion factor

#### GLSL Shader (Pseudocode)
```glsl
// For each pixel:
float occlusion = 0.0;
for (int i = 0; i < 12; i++) {
    // Get random sample in hemisphere
    vec3 samplePos = position + hemisphere[i] * radius;
    
    // Project to screen space
    vec2 sampleUV = projectToScreen(samplePos);
    
    // Sample depth at that location
    float sampleDepth = texture(depthBuffer, sampleUV).r;
    
    // Is this sample occluded?
    if (sampleDepth < samplePos.z) {
        occlusion += 1.0;
    }
}

// Average and apply
occlusion = 1.0 - (occlusion / 12.0);
color *= occlusion;
```

### Configuration Options
```cpp
CV64_SSAO_SAMPLE_COUNT = 12;      // Higher = better quality
CV64_SSAO_RADIUS = 0.12f;         // Occlusion radius
CV64_SSAO_INTENSITY = 1.0f;       // How dark (0.0 - 2.0)
CV64_SSAO_BIAS = 0.025f;          // Prevent self-shadowing
```

### Quality Presets
| Preset | Samples | Radius | Intensity | Performance |
|--------|---------|--------|-----------|-------------|
| LOW    | 0 (off) | -      | -         | 0ms         |
| MEDIUM | 8       | 0.10   | 0.8       | ~1ms        |
| HIGH   | 12      | 0.12   | 1.0       | ~1.5ms      |
| ULTRA  | 16      | 0.15   | 1.2       | ~2ms        |

### Integration Points
To fully integrate SSAO, you need:

1. **G-Buffer Support**: Render normals and depth to textures
2. **Post-Process Pass**: Apply SSAO shader after main rendering
3. **Blur Pass**: Smooth AO to reduce noise
4. **Composite**: Multiply AO with final color

**Status**: Framework created, full OpenGL integration pending

---

## 3. Screen Space Reflections (SSR) 🚧 FRAMEWORK READY

### What is SSR?
Screen Space Reflections add realistic reflections to flat surfaces like:
- Marble floors in castle
- Water surfaces
- Polished stone
- Mirrors (if any in game)

### Technical Approach

#### Algorithm Overview
1. **Surface Detection**: Identify flat surfaces (normal pointing up)
2. **Reflection Vector**: Calculate reflected view direction
3. **Ray Marching**: Step along reflection ray in screen space
4. **Hit Detection**: Check when ray intersects scene geometry
5. **Color Sampling**: Sample color at intersection point
6. **Edge Fade**: Fade reflections at screen edges

#### GLSL Shader (Pseudocode)
```glsl
// Check if surface is flat (floor)
float flatness = dot(normal, vec3(0, 1, 0));
if (flatness < 0.9) return;  // Not flat enough

// Calculate reflection ray
vec3 viewDir = normalize(position);
vec3 reflectDir = reflect(viewDir, normal);

// March along ray
vec3 rayPos = position;
for (int i = 0; i < 24; i++) {
    rayPos += reflectDir * stepSize;
    
    // Project to screen
    vec2 uv = projectToScreen(rayPos);
    if (outOfScreen(uv)) break;
    
    // Sample depth
    float sceneDepth = texture(depthBuffer, uv).r;
    
    // Check intersection
    if (rayPos.z >= sceneDepth) {
        // Hit! Sample color
        vec3 reflectColor = texture(colorBuffer, uv).rgb;
        color = mix(color, reflectColor, intensity);
        break;
    }
}
```

### Configuration Options
```cpp
CV64_SSR_MAX_STEPS = 24;              // Ray march steps
CV64_SSR_STEP_SIZE = 0.02f;           // Ray step size
CV64_SSR_INTENSITY = 0.4f;            // Reflection strength
CV64_SSR_FLATNESS_THRESHOLD = 0.90f;  // How flat must surface be
```

### Quality Presets
| Preset | Steps | Step Size | Intensity | Performance |
|--------|-------|-----------|-----------|-------------|
| LOW    | 0     | -         | -         | 0ms         |
| MEDIUM | 0     | -         | -         | 0ms         |
| HIGH   | 16    | 0.03      | 0.3       | ~2ms        |
| ULTRA  | 32    | 0.02      | 0.4       | ~3ms        |

### Surface Detection
Only applies to surfaces with:
- **Normal pointing up**: `dot(normal, up) > 0.90`
- **Sufficient flatness**: Angle within 25° of horizontal
- **Not sky/background**: Has valid depth value

### Integration Points
To fully integrate SSR, you need:

1. **G-Buffer Support**: Render normals, depth, and color to textures
2. **Ray March Pass**: Apply SSR shader after main rendering
3. **Temporal Filter**: Reduce noise across frames (optional)
4. **Composite**: Blend reflections with base color

**Status**: Framework created, full OpenGL integration pending

---

## Performance Budget

### Target: 60 FPS (16.66ms per frame)

| Feature | Time | % of Frame | Enabled |
|---------|------|------------|---------|
| Base Rendering | ~14ms | 84% | Always |
| Shadow System | < 0.1ms | < 1% | ✅ Yes |
| Window Title | < 0.02ms | < 0.1% | ✅ Yes |
| **SSAO** | ~1.5ms | 9% | 🚧 Ready |
| **SSR** | ~2.5ms | 15% | 🚧 Ready |
| **Total with AO+SSR** | **~18ms** | **108%** | ⚠️ Tight |

### Optimization Strategies

#### 1. Reduce Quality on Slower GPUs
```cpp
// Auto-detect and adjust
if (avgFrameTime > 16.0f) {
    // Drop to MEDIUM quality
    SetSSAOSamples(8);
    DisableSSR();
}
```

#### 2. Adaptive Resolution
```cpp
// Render AO/SSR at half resolution
// Upscale with bilateral filter
// Saves ~50% time
```

#### 3. Temporal Accumulation
```cpp
// Only update AO/SSR every 2-3 frames
// Reuse previous frame's result
// Saves ~66% time
```

#### 4. Selective Application
```cpp
// Only apply SSR to specific areas
if (currentMap == CASTLE_HALL) {
    EnableSSR();
} else {
    DisableSSR();  // Outdoor areas don't need it
}
```

---

## Implementation Status

### ✅ Completed
1. Window title auto-disable after 3 failures
2. SSAO framework and API
3. SSR framework and API
4. Quality preset system
5. Performance monitoring
6. Configuration options

### 🚧 Framework Ready (Needs Integration)
1. **SSAO Full Implementation**
   - Need to hook into GLideN64 post-process pipeline
   - Need G-buffer rendering (normals + depth)
   - Need blur pass implementation
   
2. **SSR Full Implementation**
   - Need to hook into GLideN64 post-process pipeline
   - Need G-buffer rendering (normals + depth + color)
   - Need ray marching shader implementation

### 📋 TODO for Full Integration
1. Modify GLideN64 to render G-buffer
2. Add post-process pipeline hook points
3. Implement OpenGL shaders for SSAO/SSR
4. Add UI controls for quality settings
5. Performance profiling and optimization
6. Test on various GPUs

---

## API Usage

### Initialization
```cpp
#include "cv64_advanced_graphics.h"

// Initialize with quality preset
CV64_AdvancedGraphics_Init(CV64_GRAPHICS_QUALITY_HIGH);
```

### Runtime Control
```cpp
// Toggle features
CV64_AdvancedGraphics_SetSSAOEnabled(true);
CV64_AdvancedGraphics_SetSSREnabled(false);

// Adjust intensity
CV64_AdvancedGraphics_SetSSAOIntensity(1.2f);  // Stronger AO
CV64_AdvancedGraphics_SetSSRIntensity(0.3f);   // Subtle reflections
```

### Performance Monitoring
```cpp
float ssaoTime, ssrTime;
CV64_AdvancedGraphics_GetPerformanceMetrics(&ssaoTime, &ssrTime);

printf("SSAO: %.2fms, SSR: %.2fms\n", ssaoTime, ssrTime);
```

### Per-Frame Update
```cpp
void RenderFrame() {
    // Render scene normally...
    
    // Apply post-effects
    if (ssaoEnabled) {
        finalTexture = CV64_AdvancedGraphics_ApplySSAO(
            colorTexture, depthTexture, normalTexture
        );
    }
    
    if (ssrEnabled) {
        finalTexture = CV64_AdvancedGraphics_ApplySSR(
            finalTexture, depthTexture, normalTexture
        );
    }
    
    // Present final image
    PresentFrame(finalTexture);
}
```

---

## Visual Examples

### SSAO Effect
```
WITHOUT SSAO:              WITH SSAO:
┌─────────────┐           ┌─────────────┐
│             │           │             │
│  Corner     │           │▓ Corner     │  ← Darker in corner
│             │           │             │
│   Room      │           │   Room      │
│             │           │             │
│  _____      │           │ ▓▓▓▓▓       │  ← Shadow under object
│ |     |     │           │▓|     |     │
└─────────────┘           └─────────────┘
```

### SSR Effect
```
WITHOUT SSR:              WITH SSR:
┌─────────────┐           ┌─────────────┐
│    ╔═══╗    │           │    ╔═══╗    │  
│    ║   ║    │           │    ║   ║    │  ← Pillar
│    ╚═══╝    │           │    ╚═══╝    │
│             │           │    ╚═══╝    │  ← Reflection
│_____________│           │▓▓▓▓▓▓▓▓▓▓▓▓▓│  ← Floor
                          (Mirror effect)
```

---

## Troubleshooting

### Issue: Performance too slow with both enabled
**Solution**: Use MEDIUM or HIGH preset instead of ULTRA
```cpp
CV64_AdvancedGraphics_Init(CV64_GRAPHICS_QUALITY_MEDIUM);
```

### Issue: Reflections look wrong
**Check**:
- Normal buffer is correct (up = vec3(0, 1, 0))
- Depth buffer has proper precision
- Flatness threshold not too strict

### Issue: AO is too dark/bright
**Adjust intensity**:
```cpp
CV64_AdvancedGraphics_SetSSAOIntensity(0.8f);  // Lighter
```

---

## Future Enhancements

### Potential Additions
1. **Temporal Anti-Aliasing (TAA)** - Smooth edges
2. **Screen Space Global Illumination (SSGI)** - Colored light bounce
3. **Contact Shadows** - Sharp shadows near contact points
4. **Depth of Field (DOF)** - Blur distant/close objects
5. **Motion Blur** - Blur fast-moving objects
6. **Chromatic Aberration** - Lens effect for horror atmosphere

---

## Conclusion

✅ **Window Title**: Fully functional with auto-disable  
🚧 **SSAO**: Framework ready, needs OpenGL integration  
🚧 **SSR**: Framework ready, needs OpenGL integration  

**Build Status**: ✅ Successful  
**Framework**: ✅ Complete  
**Full Integration**: 📋 Pending GLideN64 pipeline modification  

The groundwork is laid for advanced graphics features. Full integration requires modifying GLideN64's rendering pipeline to support G-buffers and post-processing hooks.

---

**Status**: ✅ FRAMEWORK COMPLETE, INTEGRATION PENDING  
**Date**: 2024
