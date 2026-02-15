# ?? CV64 Threading & Performance - Quick Reference

## Hotkeys

| Key | Action |
|-----|--------|
| **F3** | Toggle Performance Overlay (OFF ? MINIMAL ? STANDARD ? DETAILED ? GRAPH ? OFF) |
| F1 | Toggle D-PAD Camera Mode |
| F2 | Toggle Camera Patch |
| F5 | Quick Save State |
| F9 | Quick Load State |
| ESC | Pause/Resume |
| SPACE | Start Emulation |

## Performance Overlay Modes

| Mode | Display |
|------|---------|
| **OFF** | No overlay (default) |
| **MINIMAL** | FPS only |
| **STANDARD** | FPS + Frame Time (avg/min/max) |
| **DETAILED** | All stats including threading |
| **GRAPH** | Frame time visualization |

## Config Files

All settings in: `patches/` folder

### cv64_threading.ini
```ini
[Threading]
enable_async_graphics=true     # ON by default - recommended
enable_async_audio=true         # ON by default - recommended  
enable_worker_threads=true      # ON by default - recommended
worker_thread_count=0           # 0 = auto-detect
graphics_queue_depth=2          # 1-3 (buffering depth)
enable_parallel_rsp=false       # EXPERIMENTAL - keep OFF

[Performance]
enable_overlay=false            # OFF by default
overlay_mode=0                  # 0=OFF, 1-4=various modes
```

## Threading System

### What's Threaded:
? **Graphics Thread** - Async frame presentation  
? **Audio Thread** - SDL callback (ring buffer)  
? **Worker Pool** - Background tasks (4 threads default)  
? **RSP Thread** - EXPERIMENTAL (disabled by default)

### Thread Count:
- **Auto mode (0):** CPU cores - 2 workers
- **Manual:** Set `worker_thread_count=4` (or any number 1-16)

## Performance Stats (DETAILED mode)

```
=== CV64 Performance ===
FPS: 59.8 (16.7 ms avg)           ? Target: 60 FPS, 16.7ms
Frame Time Range: 15.2 - 18.4 ms  ? Lower variance = smoother
Total Frames: 3582                ? Total rendered

=== Threading ===
Present Latency: 1.2 ms           ? GPU present time
Audio Underruns: 0                ? Audio buffer ran empty
GPU Sync Waits: 5                 ? CPU waited for GPU
```

### What's Good:
- **FPS:** 58-60 = excellent, 55-58 = good, <55 = needs tuning
- **Frame Time:** Lower variance = smoother gameplay
- **Audio Underruns:** 0 = perfect, <5/min = good, >10/min = bad
- **GPU Sync Waits:** <10 = good, 10-50 = okay, >50 = bottleneck

## Troubleshooting

### Issue: Low FPS
**Try:**
1. Disable threading: `enable_async_graphics=false`
2. Reduce worker threads: `worker_thread_count=1`
3. Lower graphics settings in GLideN64

### Issue: Audio Stuttering
**Try:**
1. Keep async audio ON: `enable_async_audio=true`
2. Increase SDL audio buffer size
3. Close background applications

### Issue: Crashes
**Try:**
1. Disable parallel RSP: `enable_parallel_rsp=false` (should already be false)
2. Disable all threading temporarily
3. Check for memory leaks

### Issue: Input Lag
**Try:**
1. Reduce buffering: `graphics_queue_depth=1`
2. Disable VSync temporarily
3. Check for high GPU sync waits

## Debug Output

When running, check Visual Studio Output window for:

```
[CV64_THREADING] === CV64 Threading System Initialization ===
[CV64_THREADING] Configuration:
[CV64_THREADING]   Async Graphics: YES
[CV64_THREADING]   Worker Threads: 4
[CV64_THREADING] Graphics thread created
[CV64_THREADING] Threading system initialized successfully

[CV64_PERF] Performance overlay initialized (press F3 to toggle)
```

## API Quick Reference

### For Developers:

```cpp
// Initialize
CV64_ThreadConfig config = { /* ... */ };
CV64_Threading_Init(&config);

// Queue frame
CV64_Graphics_QueueFrame(frameData, width, height);

// Queue audio
CV64_Audio_QueueSamples(samples, count, frequency);

// Queue worker task
CV64_Worker_QueueTask(taskFunc, param, callback, userdata);

// Get stats
CV64_ThreadStats stats;
CV64_Threading_GetStats(&stats);

// Shutdown
CV64_Threading_Shutdown();
```

## Settings Integration

```cpp
// Load settings
CV64_Settings_LoadAll();
const CV64_AllSettings& settings = CV64_Settings_Get();

// Apply threading config
if (settings.threading.enableAsyncGraphics) {
    // Use async graphics
}

// Save settings
CV64_Settings_SaveAll();
```

## Performance Tips

### For Best Performance:
? Enable async graphics (default ON)  
? Enable async audio (default ON)  
? Enable worker threads (default ON)  
? Use double buffering (queue_depth=2)  
? Enable VSync for smooth pacing  
? Keep parallel RSP disabled (experimental)

### For Low-End Systems:
- Disable async graphics
- Reduce worker threads to 1
- Use single buffering (queue_depth=1)
- Lower GLideN64 internal resolution

### For High-End Systems:
- Enable triple buffering (queue_depth=3)
- Increase worker threads (6-8)
- Max out GLideN64 settings
- Enable performance overlay to monitor

## System Requirements

### Minimum (Threading OFF):
- CPU: Dual-core 2.0 GHz
- RAM: 2 GB
- GPU: OpenGL 3.3

### Recommended (Threading ON):
- CPU: Quad-core 3.0 GHz
- RAM: 4 GB
- GPU: OpenGL 4.5

### Optimal:
- CPU: 6+ cores, 3.5+ GHz
- RAM: 8+ GB
- GPU: Modern GPU with OpenGL 4.6

---

**?? That's it! Press F3 to see your performance stats in action!**
