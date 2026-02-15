/**
 * @file cv64_input_remapping.h
 * @brief Castlevania 64 PC - Input Remapping Interface
 * 
 * Provides a user-friendly interface for remapping all controls:
 * - N64 button mappings to keyboard/gamepad
 * - Camera controls configuration
 * - Analog stick settings
 * - Mouse sensitivity and inversion
 * - Multiple control profiles
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_INPUT_REMAPPING_H
#define CV64_INPUT_REMAPPING_H

#include "cv64_types.h"
#include "cv64_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Input Mapping Structures
//=============================================================================

/**
 * @brief Button mapping entry
 */
typedef struct CV64_ButtonMapping {
    const char* displayName;        // "A Button", "B Button", etc.
    u16 n64Button;                  // CONT_A, CONT_B, etc.
    u32 keyboardKey;                // VK_* code
    u32 gamepadButton;              // XInput button mask
    bool isAxis;                    // True for triggers/analog
    int axisIndex;                  // For analog mappings
} CV64_ButtonMapping;

/**
 * @brief Analog stick configuration
 */
typedef struct CV64_AnalogConfig {
    float deadzone;                 // 0.0 - 0.5
    float sensitivity;              // 0.1 - 5.0
    bool invertX;
    bool invertY;
} CV64_AnalogConfig;

/**
 * @brief Complete input profile
 */
typedef struct CV64_InputProfile {
    char name[64];                          // Profile name
    CV64_ButtonMapping buttons[16];         // Button mappings
    int buttonCount;
    
    CV64_AnalogConfig leftStick;            // Left analog configuration
    CV64_AnalogConfig rightStick;           // Right analog configuration
    
    float mouseSensitivityX;                // 0.1 - 10.0
    float mouseSensitivityY;                // 0.1 - 10.0
    bool mouseInvertY;
    
    float triggerThreshold;                 // 0.0 - 1.0
    
    // Camera-specific settings
    bool enableDPadCamera;
    bool enableRightStickCamera;
    bool enableMouseCamera;
    float cameraRotationSpeed;              // 0.5 - 5.0
} CV64_InputProfile;

/**
 * @brief Input profile manager
 */
typedef struct CV64_InputProfileManager {
    CV64_InputProfile profiles[8];          // Up to 8 profiles
    int profileCount;
    int activeProfileIndex;
} CV64_InputProfileManager;

//=============================================================================
// Input Remapping Functions
//=============================================================================

/**
 * @brief Initialize input remapping system
 * @return TRUE on success
 */
bool CV64_InputRemapping_Init(void);

/**
 * @brief Shutdown input remapping system
 */
void CV64_InputRemapping_Shutdown(void);

/**
 * @brief Show the input remapping dialog
 * @param hParent Parent window handle
 * @return TRUE if user saved changes, FALSE if cancelled
 */
bool CV64_InputRemapping_ShowDialog(HWND hParent);

/**
 * @brief Load input profiles from configuration file
 * @param filepath Path to input config file
 * @return Number of profiles loaded
 */
int CV64_InputRemapping_LoadProfiles(const char* filepath);

/**
 * @brief Save input profiles to configuration file
 * @param filepath Path to input config file
 * @return TRUE on success
 */
bool CV64_InputRemapping_SaveProfiles(const char* filepath);

/**
 * @brief Get the active input profile
 * @return Pointer to active profile, or NULL if none
 */
CV64_InputProfile* CV64_InputRemapping_GetActiveProfile(void);

/**
 * @brief Set the active input profile by index
 * @param index Profile index (0-7)
 * @return TRUE on success
 */
bool CV64_InputRemapping_SetActiveProfile(int index);

/**
 * @brief Create a new input profile with default mappings
 * @param name Profile name
 * @return Profile index, or -1 on error
 */
int CV64_InputRemapping_CreateProfile(const char* name);

/**
 * @brief Delete an input profile
 * @param index Profile index to delete
 * @return TRUE on success
 */
bool CV64_InputRemapping_DeleteProfile(int index);

/**
 * @brief Duplicate an existing profile
 * @param sourceIndex Source profile index
 * @param newName Name for the new profile
 * @return New profile index, or -1 on error
 */
int CV64_InputRemapping_DuplicateProfile(int sourceIndex, const char* newName);

/**
 * @brief Reset a profile to default mappings
 * @param index Profile index to reset
 * @return TRUE on success
 */
bool CV64_InputRemapping_ResetProfile(int index);

/**
 * @brief Apply an input profile to the controller system
 * @param profile Profile to apply
 * @return TRUE on success
 */
bool CV64_InputRemapping_ApplyProfile(const CV64_InputProfile* profile);

/**
 * @brief Wait for user input and capture it
 * @param captureKeyboard Capture keyboard input
 * @param captureMouse Capture mouse button input
 * @param captureGamepad Capture gamepad input
 * @param timeoutMs Timeout in milliseconds
 * @param outKey Captured keyboard/mouse key (if any)
 * @param outButton Captured gamepad button (if any)
 * @return TRUE if input was captured, FALSE on timeout
 */
bool CV64_InputRemapping_CaptureInput(bool captureKeyboard, bool captureMouse, bool captureGamepad,
                                       u32 timeoutMs, u32* outKey, u32* outButton);

/**
 * @brief Get a human-readable name for a keyboard key
 * @param vkCode Virtual key code
 * @return Key name string (do not free)
 */
const char* CV64_InputRemapping_GetKeyName(u32 vkCode);

/**
 * @brief Get a human-readable name for a gamepad button
 * @param button XInput button mask
 * @return Button name string (do not free)
 */
const char* CV64_InputRemapping_GetButtonName(u32 button);

/**
 * @brief Test if an input profile is valid
 * @param profile Profile to validate
 * @return TRUE if valid
 */
bool CV64_InputRemapping_ValidateProfile(const CV64_InputProfile* profile);

/**
 * @brief Get default input profile (keyboard + Xbox controller)
 * @param outProfile Output profile structure
 */
void CV64_InputRemapping_GetDefaultProfile(CV64_InputProfile* outProfile);

/**
 * @brief Export profile to JSON format
 * @param profile Profile to export
 * @param outBuffer Output buffer
 * @param bufferSize Buffer size
 * @return TRUE on success
 */
bool CV64_InputRemapping_ExportProfileJSON(const CV64_InputProfile* profile,
                                           char* outBuffer, size_t bufferSize);

/**
 * @brief Import profile from JSON format
 * @param jsonData JSON string
 * @param outProfile Output profile structure
 * @return TRUE on success
 */
bool CV64_InputRemapping_ImportProfileJSON(const char* jsonData,
                                           CV64_InputProfile* outProfile);

//=============================================================================
// Profile Presets
//=============================================================================

/**
 * @brief Get a preset profile by type
 * @param preset Preset type
 * @param outProfile Output profile structure
 */
typedef enum {
    CV64_PRESET_DEFAULT,        // Default keyboard + Xbox controller
    CV64_PRESET_KEYBOARD_ONLY,  // Keyboard only (WASD movement)
    CV64_PRESET_CONTROLLER_ONLY,// Controller only
    CV64_PRESET_MODERN_CONSOLE, // Modern console layout (Souls-like)
    CV64_PRESET_CLASSIC_N64,    // Original N64 layout
    CV64_PRESET_SPEEDRUN,       // Optimized for speedrunning
} CV64_InputPreset;

void CV64_InputRemapping_GetPreset(CV64_InputPreset preset, CV64_InputProfile* outProfile);

#ifdef __cplusplus
}
#endif

#endif // CV64_INPUT_REMAPPING_H
