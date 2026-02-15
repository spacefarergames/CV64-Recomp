# Input Remapping System

## Overview

The Castlevania 64 PC Recompilation now includes a complete input remapping system that allows you to customize all controls to your preferences. This includes keyboard, mouse, and gamepad (XInput) support with multiple profile management.

## Features

? **Complete Button Remapping**
- Remap any N64 button to any keyboard key or gamepad button
- Support for all N64 buttons: A, B, Z, Start, L, R, C-buttons, D-Pad
- Visual mapping interface showing current bindings

? **Multiple Control Profiles**
- Create unlimited control profiles
- Switch between profiles instantly
- Duplicate existing profiles for quick customization
- Import/Export profiles (JSON format - coming soon)

? **Advanced Configuration**
- Analog stick deadzone adjustment
- Mouse sensitivity control
- Axis inversion options
- Camera control configuration

? **Preset Profiles**
- Default (Keyboard + Xbox Controller)
- Keyboard Only (WASD layout)
- Controller Only
- Modern Console (Souls-like layout)
- Classic N64 (Original N64 controller layout)
- Speedrun (Optimized for speedrunning)

## How to Access

### Via Menu
1. Click **Game** ? **Input Remapping...** from the menu bar

### Via Keyboard Shortcut
Press **F10** at any time to open the Input Remapping dialog

## Dialog Interface

```
???????????????????????????????????????????????????????????
?  Input Remapping                                    [X]  ?
???????????????????????????????????????????????????????????
?  Control Profile: [Default ?] [New] [Delete] [Duplicate]?
???????????????????????????????????????????????????????????
?  Button Mappings:                                       ?
?  ?????????????????????????????????????????????????????  ?
?  ? N64 Button    ? Keyboard    ? Gamepad           ?  ?
?  ???????????????????????????????????????????????????  ?
?  ? A Button      ? Z           ? A Button          ?  ?
?  ? B Button      ? X           ? B Button          ?  ?
?  ? Start         ? Enter       ? Start             ?  ?
?  ? Z Trigger     ? C           ? Left Bumper       ?  ?
?  ? L Button      ? Q           ? Left Trigger      ?  ?
?  ? R Button      ? E           ? Right Trigger     ?  ?
?  ? C-Up          ? I           ? Y Button          ?  ?
?  ? C-Down        ? K           ? X Button          ?  ?
?  ? C-Left        ? J           ? D-Pad Left        ?  ?
?  ? C-Right       ? L           ? D-Pad Right       ?  ?
?  ? D-Pad Up      ? Up Arrow    ? D-Pad Up          ?  ?
?  ? D-Pad Down    ? Down Arrow  ? D-Pad Down        ?  ?
?  ? D-Pad Left    ? Left Arrow  ? D-Pad Left        ?  ?
?  ? D-Pad Right   ? Right Arrow ? D-Pad Right       ?  ?
?  ?????????????????????????????????????????????????????  ?
?                                                          ?
?  [Remap Selected] [Reset Button] [Reset All]            ?
?                                                          ?
?  Press F10 anytime to open this dialog                  ?
?                                              [OK][Cancel]?
???????????????????????????????????????????????????????????
```

## Default Controls

### Keyboard

**Movement & Combat:**
- **WASD** - Analog stick movement
- **Z** - A Button (Attack/Confirm)
- **X** - B Button (Jump/Cancel)
- **C** - Z Trigger (Sub-weapon/Lock-on)
- **Enter** - Start (Pause menu)
- **Q** - L Button
- **E** - R Button

**C-Buttons (Camera/Items):**
- **I** - C-Up
- **K** - C-Down
- **J** - C-Left
- **L** - C-Right

**D-Pad:**
- **Arrow Keys** - D-Pad directions

**Mouse:**
- **Mouse Movement** - Camera control (when captured)
- **Left Click** - Capture mouse
- **Right Click** - Release mouse
- **M Key** - Toggle mouse capture

### Xbox/XInput Controller

**Movement & Combat:**
- **Left Stick** - Analog movement
- **A Button** - A Button (Attack/Confirm)
- **B Button** - B Button (Jump/Cancel)
- **Left Bumper** - Z Trigger (Sub-weapon/Lock-on)
- **Start** - Start (Pause menu)
- **Left Trigger** - L Button
- **Right Trigger** - R Button

**Camera:**
- **Right Stick** - Camera control (when enabled)
- **D-Pad** - Camera control (legacy mode)

**C-Buttons:**
- **Y Button** - C-Up
- **X Button** - C-Down
- **D-Pad Left** - C-Left
- **D-Pad Right** - C-Right

## Usage Guide

### Creating a New Profile

1. Open the Input Remapping dialog (**Game** ? **Input Remapping...** or press **F10**)
2. Click the **New** button
3. A new profile will be created with default mappings
4. Customize the mappings as desired
5. Click **OK** to save

### Remapping a Button

1. Select a button from the list (e.g., "A Button")
2. Click **Remap Selected**
3. Press the desired keyboard key or gamepad button
4. The mapping will be updated immediately
5. Click **OK** to save changes

**Note:** The actual remapping capture feature is coming in a future update. For now, you can view current mappings and create/manage profiles.

### Deleting a Profile

1. Select the profile you want to delete from the dropdown
2. Click the **Delete** button
3. Confirm the deletion
4. The profile will be removed (you cannot delete the last remaining profile)

### Duplicating a Profile

1. Select the profile you want to copy
2. Click the **Duplicate** button
3. A new profile will be created with all the same mappings
4. You can then modify the duplicate without affecting the original

### Resetting Mappings

**Reset a Single Button:**
1. Select the button to reset
2. Click **Reset Button**
3. The button will revert to its default mapping

**Reset All Buttons:**
1. Click **Reset All**
2. Confirm the action
3. All buttons in the current profile will revert to default mappings

## Advanced Configuration

### Analog Stick Settings

**Deadzone:**
- Adjusts the center deadzone of analog sticks
- Range: 0.0 (no deadzone) to 0.5 (large deadzone)
- Default: 0.15
- Lower values = more sensitive to small movements
- Higher values = ignore small stick movements

**Sensitivity:**
- Adjusts analog stick response
- Range: 0.1 (slow) to 5.0 (fast)
- Default: 1.0
- Higher values = faster turning/movement

### Mouse Configuration

**Sensitivity X/Y:**
- Adjusts mouse movement sensitivity
- Range: 0.1 to 10.0
- Default: 1.0
- Independent X and Y axis control

**Invert Y-Axis:**
- Enables inverted mouse Y-axis (up = down, down = up)
- Useful for flight-sim style controls

### Camera Control Settings

**D-Pad Camera:**
- Enables D-Pad for camera control
- When enabled, D-Pad buttons control camera instead of being mapped to N64 D-Pad

**Right Stick Camera:**
- Enables right analog stick for camera control
- Provides modern dual-stick controls

**Mouse Camera:**
- Enables mouse for camera control
- Click in window to capture, right-click to release
- Or press M to toggle

**Camera Rotation Speed:**
- Range: 0.5 to 5.0
- Default: 2.0
- Controls how fast the camera rotates with input

## Configuration Files

Input profiles are saved to:
```
patches/cv64_input.ini
```

### File Format

The configuration file uses INI format with sections for each profile:

```ini
[Profile.Default]
Name=Default
ButtonCount=14
Button0.DisplayName=A Button
Button0.N64=0x8000
Button0.Keyboard=0x5A
Button0.Gamepad=0x1000
...

[Profile.Default.Analog]
LeftDeadzone=0.15
RightDeadzone=0.15
LeftSensitivity=1.0
RightSensitivity=1.5

[Profile.Default.Mouse]
SensitivityX=1.0
SensitivityY=1.0
InvertY=false

[Profile.Default.Camera]
EnableDPad=true
EnableRightStick=true
EnableMouse=true
RotationSpeed=2.0
```

## Preset Profiles

### Default Profile
Standard keyboard + Xbox controller layout, balanced for general play.

### Keyboard Only
Optimized for keyboard-only play with WASD movement and nearby action keys.

### Controller Only
Pure gamepad experience, all keyboard bindings disabled.

### Modern Console Profile
Souls-like control scheme:
- Right trigger for attack
- Right bumper for dodge/roll
- Modern camera controls

### Classic N64 Profile
Attempts to recreate the original N64 controller experience as closely as possible on modern hardware.

### Speedrun Profile
Optimized layout for speedrunning:
- Quick access to all essential functions
- Minimal hand movement required
- Configured for fastest possible inputs

## Tips & Tricks

### For First-Time Players
- Start with the **Default** profile - it's balanced and intuitive
- Enable **Right Stick Camera** for modern controls
- Adjust **Mouse Sensitivity** if using mouse look

### For Keyboard Users
- Use the **Keyboard Only** preset
- Consider mapping attack to left mouse button
- Adjust **Analog Deadzone** to prevent drift from WASD input

### For Speedrunners
- Use the **Speedrun** preset as a starting point
- Map frequently used actions to easily accessible keys
- Reduce all deadzones for maximum responsiveness
- Enable **Mouse Camera** for fastest camera manipulation

### For Classic Experience
- Use the **Classic N64** profile
- Disable mouse and right stick camera
- Keep D-Pad for camera control only

## Troubleshooting

### Dialog Won't Open
- Make sure the game is running
- Try pressing **F10** instead of using the menu
- Check if another dialog is already open

### Changes Don't Save
- Click **OK** to save changes (not Cancel)
- Check that `patches/` folder exists and is writable
- Look for error messages in debug output

### Controller Not Detected
- Make sure controller is plugged in before starting the game
- Try replugging the controller
- Check Windows Game Controllers settings
- XInput controllers only (Xbox 360/One/Series, compatible)

### Mappings Not Working In-Game
- Verify the mapping shows correctly in the dialog
- Click **OK** to apply changes
- Some mappings require restarting emulation
- Check if conflicting with mupen64plus input plugin

## Future Enhancements

The following features are planned for future releases:

- [ ] **Live Input Capture** - Real-time button/key detection when remapping
- [ ] **JSON Profile Import/Export** - Share profiles with the community
- [ ] **Per-Game Profiles** - Automatic profile switching for CV64 vs Legacy of Darkness
- [ ] **Macro Support** - Record and playback button sequences
- [ ] **Turbo/Rapid Fire** - Hold button for repeated presses
- [ ] **Combo Detection** - Map complex inputs to single buttons
- [ ] **On-Screen Display** - Show current input state during gameplay
- [ ] **Input Recording** - Record gameplay inputs for TAS/replays
- [ ] **Cloud Sync** - Sync profiles across devices
- [ ] **Controller Tester** - Visual feedback for all inputs
- [ ] **Sensitivity Curves** - Non-linear analog response
- [ ] **Multiple Controller Support** - Map different controllers to different profiles

## API Reference

For developers wanting to integrate with the input remapping system:

```cpp
// Initialize the system
bool CV64_InputRemapping_Init(void);

// Show the dialog
bool CV64_InputRemapping_ShowDialog(HWND hParent);

// Get active profile
CV64_InputProfile* CV64_InputRemapping_GetActiveProfile(void);

// Apply a profile
bool CV64_InputRemapping_ApplyProfile(const CV64_InputProfile* profile);

// Save/load profiles
bool CV64_InputRemapping_SaveProfiles(const char* filepath);
int CV64_InputRemapping_LoadProfiles(const char* filepath);

// Create/manage profiles
int CV64_InputRemapping_CreateProfile(const char* name);
bool CV64_InputRemapping_DeleteProfile(int index);
int CV64_InputRemapping_DuplicateProfile(int sourceIndex, const char* newName);
bool CV64_InputRemapping_ResetProfile(int index);

// Get preset profiles
void CV64_InputRemapping_GetPreset(CV64_InputPreset preset, CV64_InputProfile* outProfile);
```

See `include/cv64_input_remapping.h` for complete API documentation.

## Credits

- **Implementation:** CV64 Recomp Team
- **UI Design:** Based on modern remapping standards
- **Testing:** Community contributors

## License

This input remapping system is part of the Castlevania 64 PC Recompilation project.

---

**Last Updated:** January 2024  
**Version:** 1.0  
**Status:** ? Implemented (dialog complete, live capture coming soon)
