# Fullscreen Mode & Windows Dark Mode Support

## Overview
The CV64 Recomp application now supports modern fullscreen functionality with auto-hiding menu bars and Windows Dark Mode theming for a polished, professional look.

## Features

### 1. Fullscreen Mode

#### Borderless Fullscreen
- Uses borderless windowed mode for seamless Alt+Tab experience
- Automatically fills the entire monitor containing the window
- Preserves window position and size when exiting fullscreen

#### Multiple Ways to Toggle Fullscreen
1. **Menu Option**: Game ? Toggle Fullscreen
2. **Keyboard Shortcut**: ALT+ENTER (works at any time)
3. **Programmatic**: Via `CV64_VidExt_ToggleFullscreen()` API

#### Auto-Hiding Menu Bar
- Menu bar automatically hides when entering fullscreen mode
- Menu bar automatically shows when returning to windowed mode
- Provides an immersive, distraction-free gaming experience
- Menu can still be accessed via ALT key in fullscreen

### 2. Windows Dark Mode Support

#### Modern Theming
- Automatically applies Windows 10/11 Dark Mode to window title bar
- Uses native Windows APIs for proper dark mode rendering
- Window borders and title bar respect system dark mode settings
- Compatible with Windows 10 version 1809 and later
- **All dialogs support dark mode** - Settings, About, and Input Remapping dialogs

#### Implementation Details
- Uses `DwmSetWindowAttribute` with `DWMWA_USE_IMMERSIVE_DARK_MODE`
- Backwards compatible with older Windows 10 versions (tries attribute 19 if 20 fails)
- Applies modern Windows 11 backdrop effects when available
- Centralized `EnableDarkModeForDialog()` function for all dialogs

## Technical Implementation

### Architecture

```
???????????????????????
?   CV64_RMG.cpp      ?
?  (Main Window)      ?
???????????????????????
? • Dark Mode Init    ?
? • Menu Management   ?
? • Fullscreen Toggle ?
???????????????????????
           ?
           ? Callback
           ?
???????????????????????
? cv64_vidext.cpp     ?
? (Video Extension)   ?
???????????????????????
? • Window Resizing   ?
? • GL Viewport       ?
? • GLideN64 Notify   ?
? • FS State Tracking ?
???????????????????????
```

### Key Functions

#### Main Window (CV64_RMG.cpp)
- `EnableDarkModeForWindow(HWND hwnd)`
  - Enables Windows Dark Mode for the application window
  - Uses DWM (Desktop Window Manager) APIs
  - Backwards compatible with Windows 10 1809+

- `EnableDarkModeForDialog(HWND hDlg)` [Public API]
  - Enables dark mode for any dialog window
  - Called automatically in all dialog procedures
  - Exported via CV64_RMG.h for use by any module

- `UpdateMenuBarVisibility(bool fullscreen)`
  - Shows/hides menu bar based on fullscreen state
  - Called automatically via fullscreen callback
  - Uses `SetMenu()` and `DrawMenuBar()` APIs

#### Video Extension (cv64_vidext.cpp)
- `CV64_VidExt_ToggleFullscreen(void)`
  - Switches between windowed and borderless fullscreen
  - Updates OpenGL viewport and notifies GLideN64
  - Triggers registered callbacks

- `CV64_VidExt_SetFullscreenCallback(callback)`
  - Registers a callback for fullscreen state changes
  - Allows main window to respond to fullscreen toggles
  - Used for menu bar auto-hide feature

### Window Style Changes

#### Windowed Mode
```cpp
Style: WS_OVERLAPPEDWINDOW
       (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | 
        WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
ExStyle: Default
Menu: Visible
```

#### Fullscreen Mode
```cpp
Style: WS_POPUP | WS_VISIBLE
ExStyle: WS_EX_APPWINDOW
Menu: Hidden
Position: Full monitor dimensions
```

## Usage

### For Users

**Toggle Fullscreen:**
1. Press ALT+ENTER at any time
2. OR use Game menu ? Toggle Fullscreen
3. Works before starting emulation and during gameplay

**Dark Mode:**
- Automatically applied on startup
- Respects Windows system theme settings
- No configuration needed
- **Works on all windows and dialogs** (main window, settings, about, input remapping)

### For Developers

**Using Dark Mode in Custom Dialogs:**
```cpp
// Include the main header
#include "../CV64_RMG.h"

// In your dialog procedure
INT_PTR CALLBACK MyDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        // Enable dark mode
        EnableDarkModeForDialog(hDlg);
        
        // ... rest of initialization
        return TRUE;
    }
    return FALSE;
}
```

**Registering Fullscreen Callbacks:**
```cpp
// Register a callback to respond to fullscreen state changes
void OnFullscreenChanged(bool isFullscreen) {
    if (isFullscreen) {
        // Hide UI elements
        HideMenuBar();
    } else {
        // Show UI elements
        ShowMenuBar();
    }
}

CV64_VidExt_SetFullscreenCallback(OnFullscreenChanged);
```

**Checking Fullscreen State:**
```cpp
if (CV64_VidExt_IsFullscreen()) {
    // We're in fullscreen mode
}
```

**Manual Toggle:**
```cpp
CV64_VidExt_ToggleFullscreen();
```

## Benefits

### User Experience
- **Immersive Gaming**: No distractions in fullscreen mode
- **Modern Look**: Dark mode matches Windows 11 aesthetics
- **Seamless Transitions**: Smooth switching between modes
- **Multi-Monitor Support**: Works correctly on any monitor

### Technical Benefits
- **No Screen Mode Changes**: Borderless windowed prevents black screen flashes
- **Fast Alt+Tab**: Window style allows instant task switching
- **Proper Rendering**: OpenGL viewport updates correctly
- **State Preservation**: Window position/size saved and restored

## Compatibility

### Windows Versions
- **Windows 11**: Full dark mode and theming support
- **Windows 10 (1809+)**: Dark mode title bar support
- **Windows 10 (earlier)**: Graceful fallback, no dark mode
- **Windows 7/8**: Standard theming (dark mode not supported)

### Display Configurations
- Single monitor setups
- Multi-monitor setups
- Mixed DPI configurations
- Different refresh rates

## Known Limitations

1. **Menu Access in Fullscreen**
   - Menu bar is hidden in fullscreen
   - Press ALT to temporarily show menu
   - Or use keyboard shortcuts (F3, F5, F9, F10, etc.)

2. **Dark Mode**
   - Requires Windows 10 1809 or later for title bar
   - Menu bar itself doesn't support dark rendering (Windows limitation)
   - Some third-party themes may override dark mode

3. **Fullscreen Behavior**
   - Uses borderless windowed, not exclusive fullscreen
   - May have slightly higher latency than exclusive mode (negligible for N64 emulation)
   - Does not change monitor refresh rate or resolution

## Future Enhancements

Potential improvements for future versions:

1. **Exclusive Fullscreen Mode**
   - Option for true exclusive fullscreen
   - Better for competitive gaming (lower latency)
   - Automatic resolution switching

2. **Custom Menu Theming**
   - Owner-drawn dark menus
   - Custom colors matching game aesthetic
   - Animated menu transitions

3. **Fullscreen HUD**
   - Show minimal UI in fullscreen
   - Performance overlay
   - Controller status indicator

4. **Per-Monitor DPI Awareness**
   - Better handling of mixed DPI setups
   - Correct scaling on each monitor
   - High-DPI icon support

## Troubleshooting

### Menu Bar Stuck Visible in Fullscreen
- Exit fullscreen and re-enter: ALT+ENTER twice
- Check if another window has focus
- Restart application if issue persists

### Dark Mode Not Working
- Verify Windows 10 version 1809 or later
- Check Windows Settings ? Personalization ? Colors ? "Choose your app mode"
- Ensure Windows updates are installed
- May not work with custom Windows themes

### Fullscreen on Wrong Monitor
- Move window to desired monitor before entering fullscreen
- Window will fill the monitor it's currently on
- Use Windows+Shift+Arrow keys to move between monitors

## References

### APIs Used
- `DwmSetWindowAttribute()` - Dark mode and window composition
- `SetWindowLongPtr()` - Window style modification
- `SetWindowPos()` - Window positioning and sizing
- `SetMenu()` - Menu bar visibility
- `MonitorFromWindow()` - Multi-monitor support

### Related Files
- `CV64_RMG.cpp` - Main window and dark mode implementation
- `CV64_RMG.h` - Public API exports (including `EnableDarkModeForDialog()`)
- `src/cv64_vidext.cpp` - Video extension and fullscreen logic
- `src/cv64_settings.cpp` - Settings dialog with dark mode
- `src/cv64_input_remapping.cpp` - Input remapping dialog with dark mode
- `include/cv64_vidext.h` - Video extension API declarations
- `Resource.h` - Menu resource definitions
- `CV64_RMG.rc` - Menu resource file

## Changelog

### Version 1.1 (Current)
- ? Added fullscreen toggle via ALT+ENTER
- ? Added fullscreen menu option
- ? Implemented auto-hiding menu bar in fullscreen
- ? Added Windows Dark Mode support
- ? Modern Windows 11 theming integration
- ? Multi-monitor fullscreen support
- ? Fullscreen state callback system
- ? **Dark mode for all dialogs** (Settings, About, Input Remapping)
- ? **Centralized dark mode API** for easy integration

### Version 1.0 (Previous)
- Basic windowed mode only
- No dark mode support
- Menu bar always visible
