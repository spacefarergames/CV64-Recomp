# Mempak Screenshot Feature

## Overview
Automatic screenshot capture when mempak (Controller Pak) saves are written, with display in the Memory Pak Editor.

## How It Works

### 1. Save Detection
- **File**: `RMG/Source/3rdParty/mupen64plus-core/src/backends/file_storage.c`
- When `file_storage_save()` detects a mempak file save (filename contains "mempak")
- Calls `CV64_MempakEditor_OnMempakSave()` to trigger screenshot capture

### 2. Screenshot Scheduling
- **File**: `src/cv64_mempak_editor.cpp`
- Schedules a screenshot to be taken 3 seconds after the mempak save
- Screenshot path: `save/<mempak_filename>.png`
  - Example: `save/mempak0.png` for `save/mempak0.mpk`
- Uses a pending screenshot system to avoid threading issues with OpenGL

### 3. Screenshot Capture
- **Files**: 
  - `RMG/Source/3rdParty/mupen64plus-core/src/main/screenshot.c`
  - `RMG/Source/3rdParty/mupen64plus-core/src/main/screenshot.h`
- New function: `TakeScreenshotToPath(const char* filepath)`
  - Reads the current framebuffer from the graphics plugin
  - Saves to a specific path (instead of auto-incrementing names)
  - Uses PNG format via libpng

### 4. Main Loop Integration
- **File**: `RMG/Source/3rdParty/mupen64plus-video-GLideN64/src/VI.cpp`
- `CV64_MempakEditor_ProcessPendingScreenshot()` called every frame in the VI update
- After 3 seconds have elapsed, captures the screenshot from the render thread
- Ensures OpenGL context is available for `gfx.readScreen()`

### 5. Display in Editor
- **File**: `src/cv64_mempak_editor.cpp`
- Memory Pak Editor dialog loads the associated `.png` screenshot
- Uses GDI+ to load PNG files into HBITMAP format
- Custom draw handler (`WM_DRAWITEM`) displays scaled screenshot
- Falls back to "No Screenshot" text if image doesn't exist

## File Locations

### Screenshots Saved To:
```
save/mempak0.png   (for save/mempak0.mpk)
save/mempak1.png   (for save/mempak1.mpk)
save/mempak2.png   (for save/mempak2.mpk)
save/mempak3.png   (for save/mempak3.mpk)
```

### Regular Screenshots (F12 key):
```
screenshot/<romname>-###.png
```
Where `###` is an auto-incrementing number (000-999)

## Key Features

1. **Automatic Trigger**: No user action needed - screenshots are captured automatically
2. **3-Second Delay**: Gives time for the save confirmation message to clear
3. **Thread-Safe**: Uses pending screenshot system to capture from main render thread
4. **Save Folder**: Keeps screenshots next to the save files for easy organization
5. **GDI+ Integration**: Properly loads and displays PNG images in the editor
6. **Scaled Display**: Screenshots are scaled to fit the preview window while maintaining aspect ratio

## API Functions

### Public API (cv64_mempak_editor.h)
```c
// Set/get screenshot path for current mempak
void CV64_MempakEditor_SetScreenshot(const char* screenshotPath);
const char* CV64_MempakEditor_GetScreenshot(void);

// Called when mempak is saved (internal use)
void CV64_MempakEditor_OnMempakSave(void);

// Process pending screenshots (called from VI loop)
void CV64_MempakEditor_ProcessPendingScreenshot(void);
```

### Screenshot API (screenshot.h)
```c
// Take screenshot to auto-generated path
void TakeScreenshot(int iFrameNumber);

// Take screenshot to specific path
void TakeScreenshotToPath(const char* filepath);
```

## Technical Details

### Threading Model
- **Problem**: OpenGL functions must be called from the render thread
- **Solution**: Use a pending screenshot system with timestamp
  - Schedule screenshot with target time (current time + 3 seconds)
  - Check each frame if it's time to capture
  - Execute capture on render thread via VI update loop

### Image Loading
- Uses GDI+ for PNG support in Windows
- `Bitmap::GetHBITMAP()` converts to Windows bitmap format
- Initialized with `GdiplusStartup()` and cleaned up with `GdiplusShutdown()`

### Memory Management
- Screenshot bitmap stored in `g_mempak.screenshotBitmap`
- Automatically cleaned up when:
  - New screenshot is loaded
  - Different mempak file is opened
  - Editor is shut down

## Usage

### For Users
1. Play the game normally
2. When the game saves to mempak, a screenshot will automatically be captured after 3 seconds
3. Open the Memory Pak Editor (Tools menu)
4. The screenshot will be displayed in the preview area

### For Developers
To manually trigger a screenshot to a specific path:
```c
#include "main/screenshot.h"

// Capture to specific location
TakeScreenshotToPath("save/my_custom_screenshot.png");
```

## Debugging

Enable debug output to see screenshot activity:
```
[CV64] Mempak save detected: save/mempak0.mpk
[CV64] Screenshot scheduled to: save/mempak0.png (will capture in 3 seconds)
[CV64] Taking screenshot to: save/mempak0.png
[CV64] Screenshot saved to: save/mempak0.png
[CV64] Screenshot loaded successfully: save/mempak0.png (640x480)
```

## Future Enhancements

Potential improvements:
- Configurable delay time
- Thumbnail generation for multiple save slots
- Screenshot carousel/gallery view
- Automatic cleanup of old screenshots
- Video capture support
