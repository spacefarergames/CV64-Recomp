# Quick Fix: SSAO & SSR Not Showing

## Most Likely Issue: Game is Still Running

**Error**: `cannot open file 'CV64_Recomp.exe'`

**Solution**: 
1. Close the game completely
2. Rebuild (Ctrl+Shift+B or F5)
3. Run again

---

## The Settings Are Now EXTREME

With the latest changes, the effects should be IMPOSSIBLE to miss:

### SSAO (Ambient Occlusion):
- **Radius**: 20.0 (HUGE - was 5.0)
- **Intensity**: 3.0 (VERY DARK - was 1.0)
- Corners should look almost BLACK

### SSR (Screen Space Reflections):
- **Steps**: 64 (MAXIMUM QUALITY - was 24)
- **Intensity**: 1.0 (FULL MIRROR - was 0.4)
- Floors should look like MIRRORS

---

## Testing Instructions

### Close Game → Rebuild → Test:

1. **Close CV64_Recomp.exe** (important!)
2. **Build** the project
3. **Run** the game
4. **Go to castle entrance hall**

### What You Should See:

**SSAO Test - Look at Corners**:
- Corners should be VERY dark (almost black)
- Walls meeting floor should have strong shadows
- Any crevice should be dark
- **If you see this**: SSAO is working!
- **If corners look normal**: SSAO isn't working

**SSR Test - Look at Floor**:
- Marble floors should be mirror-like
- You should see reflections of walls/ceiling
- Water should reflect environment strongly
- **If you see reflections**: SSR is working!
- **If floor looks normal**: SSR isn't working

---

## If Still Not Working After Rebuild:

### Check #1: Is Frame Buffer Emulation On?
The game needs to use framebuffers for post-processing to work.

**Verify**: When you start the game, does it look different than before? Any performance change?

### Check #2: Are You Using the Right Build?
Make sure you're running the newly built exe, not an old one.

**Verify**: Check the exe timestamp - should be from today.

### Check #3: OpenGL ES vs Desktop OpenGL
If you're on integrated graphics or mobile GPU, the shaders might not compile.

**Check**: What GPU do you have?
- Intel integrated: Might not work
- NVIDIA/AMD dedicated: Should work

---

## Diagnostic: Add Console Output

If you want to verify the shaders are loading, add this to `PostProcessor.cpp` line ~91:

```cpp
if (config.ssao.enableSSAO != 0) {
    m_ssaoProgram.reset(gfxContext.createSSAOShader());
    if (m_ssaoProgram) {
        printf("✅ SSAO SHADER LOADED SUCCESSFULLY\n");
        m_postprocessingList.emplace_front(std::mem_fn(&PostProcessor::_doSSAO));
    } else {
        printf("❌ SSAO SHADER FAILED TO LOAD\n");
    }
}
```

And after line ~98 for SSR:

```cpp
if (config.ssr.enableSSR != 0) {
    m_ssrProgram.reset(gfxContext.createSSRShader());
    if (m_ssrProgram) {
        printf("✅ SSR SHADER LOADED SUCCESSFULLY\n");
        m_postprocessingList.emplace_front(std::mem_fn(&PostProcessor::_doSSR));
    } else {
        printf("❌ SSR SHADER FAILED TO LOAD\n");
    }
}
```

Check the console window when you start the game - you should see the success messages.

---

## Next Steps:

1. **Close the game** if it's running
2. **Rebuild** the project
3. **Run** and test in castle
4. **Report back**: Do you see VERY dark corners and mirror-like floors?

If yes → Effects are working, we can tune them back to normal
If no → We need to debug further (GPU info, console output, etc.)

The settings are now SO extreme that if you don't see anything, there's a deeper issue with shader compilation or pipeline activation.
