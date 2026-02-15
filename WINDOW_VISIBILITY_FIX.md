# Window Visibility Fix - GLideN64 Display Issue

## Problem
After fixing the plugin initialization crash, the game would start and **audio could be heard**, but **no graphics were displayed**. The GL window was never shown.

## Root Cause
The code was hiding the main window with `ShowWindow(hWnd, SW_HIDE)` when emulation started (line 610). However, GLideN64 renders **directly into the main window** via the custom video extension (`cv64_vidext.cpp`). When the window was hidden, GLideN64's OpenGL rendering was invisible.

### The Video Extension Architecture
```
Main Window (g_hWnd)
    ?
CV64_VidExt (video extension)
    ?
GLideN64 renders via OpenGL to main window's HDC
```

The video extension (`cv64_vidext.cpp`) was specifically designed to embed mupen64plus rendering into the main CV64 window, eliminating the need for a separate plugin window. Hiding that window defeated the entire purpose!

## The Fix

### 1. Keep Window Visible During Emulation (CV64_RMG.cpp)

#### Before (BROKEN)
```cpp
if (LoadDefaultROM()) {
    if (CV64_M64P_Start()) {
        g_emulationStarted = true;
        // Hide the main window while emulation runs
        ShowWindow(hWnd, SW_HIDE);  // ? WRONG!
    }
}
```

#### After (FIXED)
```cpp
if (LoadDefaultROM()) {
    if (CV64_M64P_Start()) {
        g_emulationStarted = true;
        // Keep window visible - GLideN64 renders into it via video extension
        SetWindowTextA(g_hWnd, "CASTLEVANIA");
        InvalidateRect(g_hWnd, NULL, TRUE);
    }
}
```

### 2. Skip GDI Rendering During Emulation (CV64_RMG.cpp)

The WM_PAINT handler was painting the splash screen bitmap even during emulation, which could interfere with GLideN64's OpenGL rendering.

#### Before
```cpp
case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RenderFrame(hdc);  // Always renders splash screen
        EndPaint(hWnd, &ps);
    }
    break;
```

#### After (FIXED)
```cpp
case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        // Only render splash screen when emulation is not running
        // GLideN64 handles rendering via OpenGL when emulation is active
        if (!g_emulationStarted || !CV64_M64P_IsRunning()) {
            RenderFrame(hdc);
        }
        
        EndPaint(hWnd, &ps);
    }
    break;
```

### 3. Simplified Emulation Stop Handler (CV64_RMG.cpp)

Since the window is no longer hidden, we don't need the complex show/z-order logic when emulation stops.

#### Before
```cpp
if (!stillRunning) {
    g_emulationStarted = false;
    // Show the main window again, set to back z-order initially
    ShowWindow(g_hWnd, SW_SHOW);
    // Set z-order to bottom first, then bring to front (prevents flicker)
    SetWindowPos(g_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetForegroundWindow(g_hWnd);
    SetWindowTextW(g_hWnd, L"CASTLEVANIA 64 RECOMP - Press SPACE to start");
    InvalidateRect(g_hWnd, NULL, TRUE);
}
```

#### After (FIXED)
```cpp
if (!stillRunning) {
    g_emulationStarted = false;
    // Restore splash screen title and force repaint
    SetWindowTextW(g_hWnd, L"CASTLEVANIA 64 RECOMP - Press SPACE to start");
    InvalidateRect(g_hWnd, NULL, TRUE);
}
```

## How It Works Now

### Startup Sequence
1. **Window Creation**: Main window is created and shown
2. **Splash Screen**: WM_PAINT renders the Castlevania splash bitmap
3. **User Presses SPACE**: ROM loads, emulation starts
4. **Window Stays Visible**: Title changes to "CASTLEVANIA"
5. **GLideN64 Takes Over**: Renders game via OpenGL to the main window
6. **No More GDI**: WM_PAINT skips rendering during emulation

### Rendering Modes

#### Before Emulation Starts
```
Main Window
    ?
WM_PAINT (GDI)
    ?
Draws splash bitmap with BitBlt
```

#### During Emulation
```
Main Window
    ?
Video Extension
    ?
GLideN64 OpenGL Context
    ?
Game renders in real-time
```

#### After Emulation Stops
```
Main Window
    ?
WM_PAINT (GDI)
    ?
Draws splash bitmap again
```

## Why This Design?

The custom video extension (`cv64_vidext.cpp`) was specifically created to:
- **Eliminate separate plugin windows** (no more bordered mupen64plus window)
- **Embed rendering directly** into the main application window
- **Provide seamless experience** - one window, no window management issues
- **Allow custom UI overlays** (future: HUD, menus, etc.)

Hiding the main window completely defeated this design!

## Expected Behavior After Fix

1. **Splash Screen Visible**: Shows Castlevania artwork with "Press SPACE to start"
2. **Press SPACE**: ROM loads, window title changes to "CASTLEVANIA"
3. **Graphics Appear**: N64 game graphics render in the window via GLideN64
4. **Audio Plays**: Game sounds play through SDL audio
5. **Input Works**: Controller/keyboard input responds
6. **Single Window**: No additional windows, no window juggling

## Files Modified

**CV64_RMG.cpp**:
- Removed `ShowWindow(hWnd, SW_HIDE)` from emulation start
- Added check in WM_PAINT to skip GDI rendering during emulation
- Simplified emulation stop handler (no window show/hide logic)
- Changed window title to "CASTLEVANIA" when emulation starts

## Testing Checklist

- [x] Window stays visible when emulation starts
- [x] Graphics render correctly (GLideN64 output visible)
- [x] Audio plays (was already working)
- [x] Input responds (controller/keyboard)
- [x] Splash screen shows when not emulating
- [x] No flickering or rendering conflicts
- [x] Window title updates correctly
- [x] Emulation stop restores splash screen

## Result

**The game now displays graphics properly!** The window remains visible throughout, GLideN64 renders the game, and the user sees exactly what they expect: Castlevania 64 running in a single, clean window.
