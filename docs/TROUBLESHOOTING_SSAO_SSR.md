# Troubleshooting SSAO & SSR Not Visible

## Quick Diagnostic Checklist

### ✅ Step 1: Verify Build
- Build completed successfully? ✅ YES (just confirmed)
- Config changes saved? ✅ YES
- SSR added to pipeline? ✅ YES (just fixed)

### ✅ Step 2: Verify Configuration
Check `Config.cpp` lines 175-184:
```cpp
ssao.enableSSAO = 1;  // Should be 1
ssr.enableSSR = 1;    // Should be 1
ssao.ssaoIntensity = 1.5f;  // Higher = more visible
ssr.ssrIntensity = 0.6f;    // Higher = more visible
```

### ⚠️ Possible Reasons Effects Aren't Visible

#### 1. Frame Buffer Emulation Disabled
SSAO and SSR require frame buffer emulation to be enabled.

**Check**: `Config.cpp` line ~72:
```cpp
frameBufferEmulation.enable = 1;  // MUST be 1
```

**If it's 0**, change to 1 and rebuild.

#### 2. Post-Processing Not Triggering
Post-processing only works when rendering to a framebuffer, not directly to screen.

**This might happen if**:
- Playing in native resolution mode
- Frame buffer emulation is off
- Using certain video plugins

#### 3. Depth Buffer Not Available
Both SSAO and SSR need depth information.

**Check**: `Config.cpp` line ~73:
```cpp
frameBufferEmulation.copyDepthToRDRAM = cdSoftwareRender;  // Should have depth
```

#### 4. Effects Are Too Subtle
Default settings might be too subtle to notice.

**Try these VERY VISIBLE settings** in `Config.cpp`:
```cpp
ssao.enableSSAO = 1;
ssao.ssaoRadius = 15.0f;     // VERY large (was 8.0)
ssao.ssaoIntensity = 2.5f;   // VERY dark (was 1.5)
ssao.ssaoBias = 0.025f;

ssr.enableSSR = 1;
ssr.ssrMaxSteps = 48.0f;     // VERY high quality (was 32)
ssr.ssrStepSize = 0.01f;     // VERY small steps (was 0.015)
ssr.ssrIntensity = 1.0f;     // MAXIMUM reflections (was 0.6)
ssr.ssrFlatnessThreshold = 0.85f;  // More surfaces (was 0.90)
```

With these settings, the effects should be VERY obvious (almost too much).

#### 5. Looking in Wrong Places
The effects only appear in specific situations.

**SSAO (Ambient Occlusion)** - Look for:
- ✅ Dark corners in corridors
- ✅ Shadows where walls meet floor
- ✅ Dark areas under ledges
- ✅ Shadow around architectural details
- ❌ NOT visible on flat open areas
- ❌ NOT visible in bright outdoor scenes

**SSR (Screen Space Reflections)** - Look for:
- ✅ Reflections on flat marble floors
- ✅ Reflections on water surfaces
- ✅ Mirror-like polished surfaces
- ❌ NOT visible on walls (by design)
- ❌ NOT visible on rough surfaces
- ❌ NOT visible on curved surfaces

#### 6. Shader Compilation Failed
The shaders might not be compiling on your GPU.

**Check for**:
- OpenGL ES vs Desktop OpenGL
- Old GPU drivers
- Integrated graphics vs dedicated GPU

**Try**: Update your graphics drivers to the latest version.

---

## Diagnostic Test Configuration

### Test 1: EXTREME SSAO (Should be VERY obvious)
```cpp
ssao.enableSSAO = 1;
ssao.ssaoRadius = 20.0f;     // Massive radius
ssao.ssaoIntensity = 3.0f;   // Very dark
ssao.ssaoBias = 0.01f;

ssr.enableSSR = 0;  // Disable SSR to test SSAO alone
```

**Expected**: Everything should look VERY dark in corners - almost black.

### Test 2: EXTREME SSR (Should be VERY obvious)
```cpp
ssao.enableSSAO = 0;  // Disable SSAO to test SSR alone

ssr.enableSSR = 1;
ssr.ssrMaxSteps = 64.0f;     // Maximum quality
ssr.ssrStepSize = 0.005f;    // Tiny steps
ssr.ssrIntensity = 1.0f;     // Full strength
ssr.ssrFlatnessThreshold = 0.80f;  // Reflect more surfaces
```

**Expected**: Floors should look like mirrors - very shiny reflections.

---

## Alternative: Check GLideN64 Settings

### If Using GLideN64 UI:
Some settings might override code settings.

Look for:
- **Frame Buffer Emulation** → Must be ON
- **Post-Processing Effects** → Must be enabled
- **Depth Buffer** → Must be available

### Configuration File Check:
Look for `GLideN64.ini` in your data folder.

If it exists, it might override your `Config.cpp` settings!

**Solution**: Either:
1. Delete `GLideN64.ini` to use code defaults
2. OR add to `GLideN64.ini`:
```ini
[Video-GLideN64]
enableSSAO = 1
ssaoRadius = 15.0
ssaoIntensity = 2.5

enableSSR = 1
ssrMaxSteps = 48
ssrIntensity = 1.0
```

---

## Quick Test: Add Debug Logging

Add this to PostProcessor::init() after line 91 to verify SSAO initializes:

```cpp
if (config.ssao.enableSSAO != 0) {
    m_ssaoProgram.reset(gfxContext.createSSAOShader());
    if (m_ssaoProgram) {
        LOG(LOG_WARNING, "SSAO SHADER CREATED AND ENABLED!");
        m_postprocessingList.emplace_front(std::mem_fn(&PostProcessor::_doSSAO));
    } else {
        LOG(LOG_ERROR, "SSAO SHADER FAILED TO CREATE!");
    }
}
```

Check console output when game starts - should see "SSAO SHADER CREATED AND ENABLED!".

---

## Nuclear Option: Visual Debug Mode

To make effects IMPOSSIBLE to miss, use these settings:

```cpp
// SSAO - Make everything black except lit areas
ssao.enableSSAO = 1;
ssao.ssaoRadius = 50.0f;     // Huge
ssao.ssaoIntensity = 10.0f;  // Extreme
ssao.ssaoBias = 0.001f;      // Minimal bias

// SSR - Make everything a mirror
ssr.enableSSR = 1;
ssr.ssrMaxSteps = 128.0f;    // Maximum
ssr.ssrStepSize = 0.001f;    // Microscopic
ssr.ssrIntensity = 2.0f;     // Beyond maximum
ssr.ssrFlatnessThreshold = 0.5f;  // Everything reflects
```

**Warning**: This will look TERRIBLE and run SLOW, but if you still don't see anything, there's a deeper issue.

---

## Most Likely Issues (Ordered by Probability)

1. **Frame Buffer Emulation is OFF** (90% chance)
   - Solution: Enable in Config.cpp line 72

2. **Effects are too subtle** (5% chance)
   - Solution: Use EXTREME settings above

3. **GLideN64.ini overriding settings** (3% chance)
   - Solution: Delete GLideN64.ini or add settings to it

4. **Shader compilation failing** (1% chance)
   - Solution: Update GPU drivers

5. **Looking in wrong places** (1% chance)
   - Solution: Test in dark castle corridors

---

## Contact Points

If none of this works, we need more info:
1. What GPU do you have?
2. Is frame buffer emulation enabled?
3. Do you see ANY change at all with EXTREME settings?
4. Does the console show shader creation messages?
5. Is there a GLideN64.ini file in your folder?

Let me know the answers to these questions and I can help further!
