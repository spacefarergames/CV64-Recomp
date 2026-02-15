/**
 * @file cv64_input_remapping.cpp
 * @brief Castlevania 64 PC - Input Remapping Interface Implementation
 * 
 * Provides a complete input remapping system with:
 * - Graphical dialog for remapping controls
 * - Multiple profile support
 * - Preset configurations
 * - Save/load functionality
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_input_remapping.h"
#include "../include/cv64_controller.h"
#include "../CV64_RMG.h"
#include "../Resource.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <string.h>
#include <stdio.h>
#include <xinput.h>
#include <string>

#pragma comment(lib, "Comctl32.lib")

//=============================================================================
// Constants
//=============================================================================

#define MAX_PROFILES 8
#define CONFIG_FILENAME "patches\\cv64_input.ini"
#define CAPTURE_TIMEOUT_MS 5000
#define WM_UPDATE_BUTTON (WM_USER + 100)

//=============================================================================
// Static Variables
//=============================================================================

static CV64_InputProfileManager s_profileManager;
static bool s_initialized = false;
static HWND s_dialogHwnd = NULL;
static int s_editingButtonIndex = -1;
static bool s_capturingInput = false;

// External reference to application instance
extern HINSTANCE g_hInst;

//=============================================================================
// Virtual Key Name Lookup Table
//=============================================================================

typedef struct {
    u32 vkCode;
    const char* name;
} KeyNameEntry;

static const KeyNameEntry s_keyNames[] = {
{ VK_LBUTTON, "Mouse Left" },
{ VK_RBUTTON, "Mouse Right" },
{ VK_MBUTTON, "Mouse Middle" },
{ VK_XBUTTON1, "Mouse X1 (Back)" },
{ VK_XBUTTON2, "Mouse X2 (Forward)" },
{ VK_BACK, "Backspace" },
    { VK_TAB, "Tab" },
    { VK_RETURN, "Enter" },
    { VK_SHIFT, "Shift" },
    { VK_CONTROL, "Ctrl" },
    { VK_MENU, "Alt" },
    { VK_PAUSE, "Pause" },
    { VK_CAPITAL, "Caps Lock" },
    { VK_ESCAPE, "Escape" },
    { VK_SPACE, "Space" },
    { VK_PRIOR, "Page Up" },
    { VK_NEXT, "Page Down" },
    { VK_END, "End" },
    { VK_HOME, "Home" },
    { VK_LEFT, "Left Arrow" },
    { VK_UP, "Up Arrow" },
    { VK_RIGHT, "Right Arrow" },
    { VK_DOWN, "Down Arrow" },
    { VK_INSERT, "Insert" },
    { VK_DELETE, "Delete" },
    { '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
    { '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
    { 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" },
    { 'F', "F" }, { 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" },
    { 'K', "K" }, { 'L', "L" }, { 'M', "M" }, { 'N', "N" }, { 'O', "O" },
    { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" }, { 'S', "S" }, { 'T', "T" },
    { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" }, { 'Y', "Y" },
    { 'Z', "Z" },
    { VK_NUMPAD0, "Numpad 0" }, { VK_NUMPAD1, "Numpad 1" },
    { VK_NUMPAD2, "Numpad 2" }, { VK_NUMPAD3, "Numpad 3" },
    { VK_NUMPAD4, "Numpad 4" }, { VK_NUMPAD5, "Numpad 5" },
    { VK_NUMPAD6, "Numpad 6" }, { VK_NUMPAD7, "Numpad 7" },
    { VK_NUMPAD8, "Numpad 8" }, { VK_NUMPAD9, "Numpad 9" },
    { VK_MULTIPLY, "Numpad *" }, { VK_ADD, "Numpad +" },
    { VK_SUBTRACT, "Numpad -" }, { VK_DECIMAL, "Numpad ." },
    { VK_DIVIDE, "Numpad /" },
    { VK_F1, "F1" }, { VK_F2, "F2" }, { VK_F3, "F3" }, { VK_F4, "F4" },
    { VK_F5, "F5" }, { VK_F6, "F6" }, { VK_F7, "F7" }, { VK_F8, "F8" },
    { VK_F9, "F9" }, { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
    { VK_LSHIFT, "Left Shift" }, { VK_RSHIFT, "Right Shift" },
    { VK_LCONTROL, "Left Ctrl" }, { VK_RCONTROL, "Right Ctrl" },
    { VK_LMENU, "Left Alt" }, { VK_RMENU, "Right Alt" },
    { VK_OEM_1, ";" }, { VK_OEM_PLUS, "=" }, { VK_OEM_COMMA, "," },
    { VK_OEM_MINUS, "-" }, { VK_OEM_PERIOD, "." }, { VK_OEM_2, "/" },
    { VK_OEM_3, "`" }, { VK_OEM_4, "[" }, { VK_OEM_5, "\\" },
    { VK_OEM_6, "]" }, { VK_OEM_7, "'" },
    { 0, NULL }
};

//=============================================================================
// XInput Button Name Lookup
//=============================================================================

typedef struct {
    u32 button;
    const char* name;
} ButtonNameEntry;

static const ButtonNameEntry s_buttonNames[] = {
    { XINPUT_GAMEPAD_A, "A Button" },
    { XINPUT_GAMEPAD_B, "B Button" },
    { XINPUT_GAMEPAD_X, "X Button" },
    { XINPUT_GAMEPAD_Y, "Y Button" },
    { XINPUT_GAMEPAD_DPAD_UP, "D-Pad Up" },
    { XINPUT_GAMEPAD_DPAD_DOWN, "D-Pad Down" },
    { XINPUT_GAMEPAD_DPAD_LEFT, "D-Pad Left" },
    { XINPUT_GAMEPAD_DPAD_RIGHT, "D-Pad Right" },
    { XINPUT_GAMEPAD_LEFT_SHOULDER, "Left Bumper" },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, "Right Bumper" },
    { XINPUT_GAMEPAD_LEFT_THUMB, "Left Stick Click" },
    { XINPUT_GAMEPAD_RIGHT_THUMB, "Right Stick Click" },
    { XINPUT_GAMEPAD_START, "Start" },
    { XINPUT_GAMEPAD_BACK, "Back/Select" },
    { 0x1000, "Left Trigger" },  // Custom codes for triggers
    { 0x2000, "Right Trigger" },
    { 0, NULL }
};

//=============================================================================
// Helper Functions
//=============================================================================

const char* CV64_InputRemapping_GetKeyName(u32 vkCode) {
    for (int i = 0; s_keyNames[i].name != NULL; i++) {
        if (s_keyNames[i].vkCode == vkCode) {
            return s_keyNames[i].name;
        }
    }
    
    static char buffer[32];
    sprintf_s(buffer, sizeof(buffer), "Key 0x%02X", vkCode);
    return buffer;
}

const char* CV64_InputRemapping_GetButtonName(u32 button) {
    for (int i = 0; s_buttonNames[i].name != NULL; i++) {
        if (s_buttonNames[i].button == button) {
            return s_buttonNames[i].name;
        }
    }
    
    static char buffer[32];
    sprintf_s(buffer, sizeof(buffer), "Button 0x%04X", button);
    return buffer;
}

static void InitializeDefaultProfile(CV64_InputProfile* profile, const char* name) {
    memset(profile, 0, sizeof(CV64_InputProfile));
    strncpy_s(profile->name, sizeof(profile->name), name, _TRUNCATE);
    
    // Default button mappings
    int idx = 0;
    
    // A Button
    profile->buttons[idx].displayName = "A Button";
    profile->buttons[idx].n64Button = CONT_A;
    profile->buttons[idx].keyboardKey = 'Z';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_A;
    idx++;
    
    // B Button
    profile->buttons[idx].displayName = "B Button";
    profile->buttons[idx].n64Button = CONT_B;
    profile->buttons[idx].keyboardKey = 'X';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_B;
    idx++;
    
    // Start
    profile->buttons[idx].displayName = "Start";
    profile->buttons[idx].n64Button = CONT_START;
    profile->buttons[idx].keyboardKey = VK_RETURN;
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_START;
    idx++;
    
    // Z Trigger
    profile->buttons[idx].displayName = "Z Trigger";
    profile->buttons[idx].n64Button = CONT_G;
    profile->buttons[idx].keyboardKey = 'C';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_LEFT_SHOULDER;
    idx++;
    
    // L Button
    profile->buttons[idx].displayName = "L Button";
    profile->buttons[idx].n64Button = CONT_L;
    profile->buttons[idx].keyboardKey = 'Q';
    profile->buttons[idx].gamepadButton = 0x1000; // Left Trigger
    profile->buttons[idx].isAxis = true;
    idx++;
    
    // R Button
    profile->buttons[idx].displayName = "R Button";
    profile->buttons[idx].n64Button = CONT_R;
    profile->buttons[idx].keyboardKey = 'E';
    profile->buttons[idx].gamepadButton = 0x2000; // Right Trigger
    profile->buttons[idx].isAxis = true;
    idx++;
    
    // C-Up
    profile->buttons[idx].displayName = "C-Up";
    profile->buttons[idx].n64Button = CONT_E;
    profile->buttons[idx].keyboardKey = 'I';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_Y;
    idx++;
    
    // C-Down
    profile->buttons[idx].displayName = "C-Down";
    profile->buttons[idx].n64Button = CONT_D;
    profile->buttons[idx].keyboardKey = 'K';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_X;
    idx++;
    
    // C-Left
    profile->buttons[idx].displayName = "C-Left";
    profile->buttons[idx].n64Button = CONT_C;
    profile->buttons[idx].keyboardKey = 'J';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_LEFT;
    idx++;
    
    // C-Right
    profile->buttons[idx].displayName = "C-Right";
    profile->buttons[idx].n64Button = CONT_F;
    profile->buttons[idx].keyboardKey = 'L';
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_RIGHT;
    idx++;
    
    // D-Pad Up
    profile->buttons[idx].displayName = "D-Pad Up";
    profile->buttons[idx].n64Button = CONT_UP;
    profile->buttons[idx].keyboardKey = VK_UP;
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_UP;
    idx++;
    
    // D-Pad Down
    profile->buttons[idx].displayName = "D-Pad Down";
    profile->buttons[idx].n64Button = CONT_DOWN;
    profile->buttons[idx].keyboardKey = VK_DOWN;
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_DOWN;
    idx++;
    
    // D-Pad Left
    profile->buttons[idx].displayName = "D-Pad Left";
    profile->buttons[idx].n64Button = CONT_LEFT;
    profile->buttons[idx].keyboardKey = VK_LEFT;
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_LEFT;
    idx++;
    
    // D-Pad Right
    profile->buttons[idx].displayName = "D-Pad Right";
    profile->buttons[idx].n64Button = CONT_RIGHT;
    profile->buttons[idx].keyboardKey = VK_RIGHT;
    profile->buttons[idx].gamepadButton = XINPUT_GAMEPAD_DPAD_RIGHT;
    idx++;
    
    profile->buttonCount = idx;
    
    // Analog settings
    profile->leftStick.deadzone = 0.15f;
    profile->leftStick.sensitivity = 1.0f;
    profile->leftStick.invertX = false;
    profile->leftStick.invertY = false;
    
    profile->rightStick.deadzone = 0.15f;
    profile->rightStick.sensitivity = 1.5f;
    profile->rightStick.invertX = false;
    profile->rightStick.invertY = false;
    
    // Mouse settings
    profile->mouseSensitivityX = 1.0f;
    profile->mouseSensitivityY = 1.0f;
    profile->mouseInvertY = false;
    
    // Trigger threshold
    profile->triggerThreshold = 0.5f;
    
    // Camera settings
    profile->enableDPadCamera = true;
    profile->enableRightStickCamera = true;
    profile->enableMouseCamera = true;
    profile->cameraRotationSpeed = 2.0f;
}

//=============================================================================
// Public API Implementation
//=============================================================================

bool CV64_InputRemapping_Init(void) {
    if (s_initialized) {
        return true;
    }
    
    memset(&s_profileManager, 0, sizeof(s_profileManager));
    
    // Create default profile
    InitializeDefaultProfile(&s_profileManager.profiles[0], "Default");
    s_profileManager.profileCount = 1;
    s_profileManager.activeProfileIndex = 0;
    
    // Try to load profiles from config file
    CV64_InputRemapping_LoadProfiles(CONFIG_FILENAME);
    
    s_initialized = true;
    
    // Apply the active profile settings immediately so they take effect on game load
    // (instead of requiring the user to open/close the input remapping dialog)
    CV64_InputProfile* activeProfile = CV64_InputRemapping_GetActiveProfile();
    if (activeProfile) {
        CV64_InputRemapping_ApplyProfile(activeProfile);
        OutputDebugStringA("[CV64] Input remapping profile applied on initialization\n");
    }
    
    return true;
}

void CV64_InputRemapping_Shutdown(void) {
    if (!s_initialized) {
        return;
    }
    
    // Save profiles before shutdown
    CV64_InputRemapping_SaveProfiles(CONFIG_FILENAME);
    
    s_initialized = false;
}

CV64_InputProfile* CV64_InputRemapping_GetActiveProfile(void) {
    if (!s_initialized || s_profileManager.profileCount == 0) {
        return NULL;
    }
    
    return &s_profileManager.profiles[s_profileManager.activeProfileIndex];
}

bool CV64_InputRemapping_SetActiveProfile(int index) {
    if (!s_initialized || index < 0 || index >= s_profileManager.profileCount) {
        return false;
    }
    
    s_profileManager.activeProfileIndex = index;
    
    // Apply the profile
    CV64_InputRemapping_ApplyProfile(&s_profileManager.profiles[index]);
    
    return true;
}

int CV64_InputRemapping_CreateProfile(const char* name) {
    if (!s_initialized || s_profileManager.profileCount >= MAX_PROFILES) {
        return -1;
    }
    
    int index = s_profileManager.profileCount;
    InitializeDefaultProfile(&s_profileManager.profiles[index], name);
    s_profileManager.profileCount++;
    
    return index;
}

bool CV64_InputRemapping_DeleteProfile(int index) {
    if (!s_initialized || index < 0 || index >= s_profileManager.profileCount) {
        return false;
    }
    
    // Don't allow deleting the last profile
    if (s_profileManager.profileCount == 1) {
        return false;
    }
    
    // Shift profiles down
    for (int i = index; i < s_profileManager.profileCount - 1; i++) {
        memcpy(&s_profileManager.profiles[i], 
               &s_profileManager.profiles[i + 1],
               sizeof(CV64_InputProfile));
    }
    
    s_profileManager.profileCount--;
    
    // Adjust active profile index if needed
    if (s_profileManager.activeProfileIndex >= s_profileManager.profileCount) {
        s_profileManager.activeProfileIndex = s_profileManager.profileCount - 1;
    }
    
    return true;
}

int CV64_InputRemapping_DuplicateProfile(int sourceIndex, const char* newName) {
    if (!s_initialized || sourceIndex < 0 || sourceIndex >= s_profileManager.profileCount) {
        return -1;
    }
    
    if (s_profileManager.profileCount >= MAX_PROFILES) {
        return -1;
    }
    
    int newIndex = s_profileManager.profileCount;
    memcpy(&s_profileManager.profiles[newIndex],
           &s_profileManager.profiles[sourceIndex],
           sizeof(CV64_InputProfile));
    
    strncpy_s(s_profileManager.profiles[newIndex].name,
              sizeof(s_profileManager.profiles[newIndex].name),
              newName, _TRUNCATE);
    
    s_profileManager.profileCount++;
    
    return newIndex;
}

bool CV64_InputRemapping_ResetProfile(int index) {
    if (!s_initialized || index < 0 || index >= s_profileManager.profileCount) {
        return false;
    }
    
    char name[64];
    strncpy_s(name, sizeof(name), s_profileManager.profiles[index].name, _TRUNCATE);
    InitializeDefaultProfile(&s_profileManager.profiles[index], name);
    
    return true;
}

bool CV64_InputRemapping_ApplyProfile(const CV64_InputProfile* profile) {
    if (!profile) {
        return false;
    }
    
    // Apply to controller system
    CV64_ControllerConfig* config = CV64_Controller_GetConfig();
    if (!config) {
        return false;
    }
    
    // Copy button mappings
    config->binding_count = profile->buttonCount;
    for (int i = 0; i < profile->buttonCount; i++) {
        config->bindings[i].n64_button = profile->buttons[i].n64Button;
        config->bindings[i].keyboard_key = profile->buttons[i].keyboardKey;
        config->bindings[i].gamepad_button = profile->buttons[i].gamepadButton;
    }
    
    // Copy analog settings
    config->left_stick_deadzone = profile->leftStick.deadzone;
    config->right_stick_deadzone = profile->rightStick.deadzone;
    config->left_stick_sensitivity = profile->leftStick.sensitivity;
    config->right_stick_sensitivity = profile->rightStick.sensitivity;
    
    // Copy mouse settings
    config->mouse_sensitivity_x = profile->mouseSensitivityX;
    config->mouse_sensitivity_y = profile->mouseSensitivityY;
    config->invert_mouse_y = profile->mouseInvertY;
    
    // Copy trigger settings
    config->trigger_threshold = profile->triggerThreshold;
    
    // Copy camera settings
    config->enable_dpad_camera = profile->enableDPadCamera;
    config->enable_mouse_camera = profile->enableMouseCamera;
    config->enable_right_stick_camera = profile->enableRightStickCamera;
    config->camera_rotation_speed = profile->cameraRotationSpeed;
    
    return true;
}

void CV64_InputRemapping_GetDefaultProfile(CV64_InputProfile* outProfile) {
    if (!outProfile) {
        return;
    }
    
    InitializeDefaultProfile(outProfile, "Default");
}

bool CV64_InputRemapping_ValidateProfile(const CV64_InputProfile* profile) {
    if (!profile) {
        return false;
    }
    
    // Check button count
    if (profile->buttonCount < 0 || profile->buttonCount > 16) {
        return false;
    }
    
    // Check analog settings are in valid ranges
    if (profile->leftStick.deadzone < 0.0f || profile->leftStick.deadzone > 0.5f) {
        return false;
    }
    
    if (profile->leftStick.sensitivity < 0.1f || profile->leftStick.sensitivity > 5.0f) {
        return false;
    }
    
    return true;
}

//=============================================================================
// File I/O
//=============================================================================

// Helper: Get exe directory for config path
static std::string GetExeDirectory() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

// Helper: Trim whitespace from string
static std::string TrimString(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

int CV64_InputRemapping_LoadProfiles(const char* filepath) {
    std::string fullPath = GetExeDirectory() + filepath;
    
    FILE* file = nullptr;
    if (fopen_s(&file, fullPath.c_str(), "r") != 0 || !file) {
        OutputDebugStringA("[CV64] Input config file not found, using defaults\n");
        return s_profileManager.profileCount;
    }
    
    char line[512];
    std::string currentSection;
    int currentProfileIndex = -1;
    int currentButtonIndex = 0;
    
    while (fgets(line, sizeof(line), file)) {
        std::string lineStr = TrimString(line);
        if (lineStr.empty() || lineStr[0] == ';' || lineStr[0] == '#') continue;
        
        // Check for section header
        if (lineStr[0] == '[' && lineStr.back() == ']') {
            currentSection = lineStr.substr(1, lineStr.size() - 2);
            
            // Check if this is a profile section
            if (currentSection.find("Profile") == 0 || currentSection == "Default") {
                // Find or create profile
                bool found = false;
                for (int i = 0; i < s_profileManager.profileCount; i++) {
                    if (strcmp(s_profileManager.profiles[i].name, currentSection.c_str()) == 0) {
                        currentProfileIndex = i;
                        currentButtonIndex = 0;
                        found = true;
                        break;
                    }
                }
                
                if (!found && s_profileManager.profileCount < MAX_PROFILES) {
                    currentProfileIndex = s_profileManager.profileCount;
                    InitializeDefaultProfile(&s_profileManager.profiles[currentProfileIndex], currentSection.c_str());
                    s_profileManager.profileCount++;
                    currentButtonIndex = 0;
                }
            }
            continue;
        }
        
        // Parse key=value pairs
        size_t eqPos = lineStr.find('=');
        if (eqPos == std::string::npos) continue;
        
        std::string key = TrimString(lineStr.substr(0, eqPos));
        std::string value = TrimString(lineStr.substr(eqPos + 1));
        
        // Global settings section
        if (currentSection == "General") {
            if (key == "ActiveProfile") {
                int idx = atoi(value.c_str());
                if (idx >= 0 && idx < s_profileManager.profileCount) {
                    s_profileManager.activeProfileIndex = idx;
                }
            }
        }
        // Profile-specific settings
        else if (currentProfileIndex >= 0 && currentProfileIndex < s_profileManager.profileCount) {
            CV64_InputProfile* profile = &s_profileManager.profiles[currentProfileIndex];
            
            // Button mappings (format: Button_0_Keyboard=90)
            if (key.find("Button_") == 0 && key.find("_Keyboard") != std::string::npos) {
                int btnIdx = atoi(key.c_str() + 7); // Skip "Button_"
                if (btnIdx >= 0 && btnIdx < profile->buttonCount) {
                    profile->buttons[btnIdx].keyboardKey = (u32)atoi(value.c_str());
                }
            }
            else if (key.find("Button_") == 0 && key.find("_Gamepad") != std::string::npos) {
                int btnIdx = atoi(key.c_str() + 7);
                if (btnIdx >= 0 && btnIdx < profile->buttonCount) {
                    profile->buttons[btnIdx].gamepadButton = (u32)strtoul(value.c_str(), nullptr, 0);
                }
            }
            // Analog settings
            else if (key == "LeftStickDeadzone") {
                profile->leftStick.deadzone = (float)atof(value.c_str());
            }
            else if (key == "RightStickDeadzone") {
                profile->rightStick.deadzone = (float)atof(value.c_str());
            }
            else if (key == "LeftStickSensitivity") {
                profile->leftStick.sensitivity = (float)atof(value.c_str());
            }
            else if (key == "RightStickSensitivity") {
                profile->rightStick.sensitivity = (float)atof(value.c_str());
            }
            else if (key == "LeftStickInvertX") {
                profile->leftStick.invertX = (atoi(value.c_str()) != 0);
            }
            else if (key == "LeftStickInvertY") {
                profile->leftStick.invertY = (atoi(value.c_str()) != 0);
            }
            else if (key == "RightStickInvertX") {
                profile->rightStick.invertX = (atoi(value.c_str()) != 0);
            }
            else if (key == "RightStickInvertY") {
                profile->rightStick.invertY = (atoi(value.c_str()) != 0);
            }
            // Mouse settings
            else if (key == "MouseSensitivityX") {
                profile->mouseSensitivityX = (float)atof(value.c_str());
            }
            else if (key == "MouseSensitivityY") {
                profile->mouseSensitivityY = (float)atof(value.c_str());
            }
            else if (key == "MouseInvertY") {
                profile->mouseInvertY = (atoi(value.c_str()) != 0);
            }
            // Trigger settings
            else if (key == "TriggerThreshold") {
                profile->triggerThreshold = (float)atof(value.c_str());
            }
            // Camera settings
            else if (key == "EnableDPadCamera") {
                profile->enableDPadCamera = (atoi(value.c_str()) != 0);
            }
            else if (key == "EnableRightStickCamera") {
                profile->enableRightStickCamera = (atoi(value.c_str()) != 0);
            }
            else if (key == "EnableMouseCamera") {
                profile->enableMouseCamera = (atoi(value.c_str()) != 0);
            }
            else if (key == "CameraRotationSpeed") {
                profile->cameraRotationSpeed = (float)atof(value.c_str());
            }
        }
    }
    
    fclose(file);
    
    char msg[256];
    sprintf_s(msg, sizeof(msg), "[CV64] Loaded %d input profile(s) from %s\n", 
              s_profileManager.profileCount, fullPath.c_str());
    OutputDebugStringA(msg);
    
    return s_profileManager.profileCount;
}

bool CV64_InputRemapping_SaveProfiles(const char* filepath) {
    std::string fullPath = GetExeDirectory() + filepath;
    
    // Ensure directory exists
    std::string dirPath = fullPath.substr(0, fullPath.find_last_of("\\/"));
    CreateDirectoryA(dirPath.c_str(), NULL);
    
    FILE* file = nullptr;
    if (fopen_s(&file, fullPath.c_str(), "w") != 0 || !file) {
        char msg[256];
        sprintf_s(msg, sizeof(msg), "[CV64] Failed to save input config to %s\n", fullPath.c_str());
        OutputDebugStringA(msg);
        return false;
    }
    
    // Write header
    fprintf(file, "; Castlevania 64 PC Recomp - Input Configuration\n");
    fprintf(file, "; Auto-generated file - edit with caution\n\n");
    
    // Write general settings
    fprintf(file, "[General]\n");
    fprintf(file, "ActiveProfile=%d\n", s_profileManager.activeProfileIndex);
    fprintf(file, "ProfileCount=%d\n\n", s_profileManager.profileCount);
    
    // Write each profile
    for (int p = 0; p < s_profileManager.profileCount; p++) {
        CV64_InputProfile* profile = &s_profileManager.profiles[p];
        
        fprintf(file, "[%s]\n", profile->name);
        
        // Write button mappings
        for (int i = 0; i < profile->buttonCount; i++) {
            fprintf(file, "Button_%d_Keyboard=%u\n", i, profile->buttons[i].keyboardKey);
            fprintf(file, "Button_%d_Gamepad=0x%X\n", i, profile->buttons[i].gamepadButton);
        }
        
        // Write analog settings
        fprintf(file, "LeftStickDeadzone=%.3f\n", profile->leftStick.deadzone);
        fprintf(file, "RightStickDeadzone=%.3f\n", profile->rightStick.deadzone);
        fprintf(file, "LeftStickSensitivity=%.3f\n", profile->leftStick.sensitivity);
        fprintf(file, "RightStickSensitivity=%.3f\n", profile->rightStick.sensitivity);
        fprintf(file, "LeftStickInvertX=%d\n", profile->leftStick.invertX ? 1 : 0);
        fprintf(file, "LeftStickInvertY=%d\n", profile->leftStick.invertY ? 1 : 0);
        fprintf(file, "RightStickInvertX=%d\n", profile->rightStick.invertX ? 1 : 0);
        fprintf(file, "RightStickInvertY=%d\n", profile->rightStick.invertY ? 1 : 0);
        
        // Write mouse settings
        fprintf(file, "MouseSensitivityX=%.3f\n", profile->mouseSensitivityX);
        fprintf(file, "MouseSensitivityY=%.3f\n", profile->mouseSensitivityY);
        fprintf(file, "MouseInvertY=%d\n", profile->mouseInvertY ? 1 : 0);
        
        // Write trigger settings
        fprintf(file, "TriggerThreshold=%.3f\n", profile->triggerThreshold);
        
        // Write camera settings
        fprintf(file, "EnableDPadCamera=%d\n", profile->enableDPadCamera ? 1 : 0);
        fprintf(file, "EnableRightStickCamera=%d\n", profile->enableRightStickCamera ? 1 : 0);
        fprintf(file, "EnableMouseCamera=%d\n", profile->enableMouseCamera ? 1 : 0);
        fprintf(file, "CameraRotationSpeed=%.3f\n", profile->cameraRotationSpeed);
        
        fprintf(file, "\n");
    }
    
    fclose(file);
    
    char msg[256];
    sprintf_s(msg, sizeof(msg), "[CV64] Saved %d input profile(s) to %s\n", 
              s_profileManager.profileCount, fullPath.c_str());
    OutputDebugStringA(msg);
    
    return true;
}

//=============================================================================
// Input Capture
//=============================================================================

bool CV64_InputRemapping_CaptureInput(bool captureKeyboard, bool captureMouse, bool captureGamepad,
                                       u32 timeoutMs, u32* outKey, u32* outButton) {
    if (outKey) *outKey = 0;
    if (outButton) *outButton = 0;
    
    DWORD startTime = GetTickCount();
    
    // Clear any currently held keys/buttons
    Sleep(100);
    
    while ((GetTickCount() - startTime) < timeoutMs) {
        // Check mouse input first (if enabled)
        if (captureMouse) {
            // Check mouse buttons
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8001) {
                while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                    Sleep(10);
                }
                if (outKey) *outKey = VK_LBUTTON;
                return true;
            }
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8001) {
                while (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                    Sleep(10);
                }
                if (outKey) *outKey = VK_RBUTTON;
                return true;
            }
            if (GetAsyncKeyState(VK_MBUTTON) & 0x8001) {
                while (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                    Sleep(10);
                }
                if (outKey) *outKey = VK_MBUTTON;
                return true;
            }
            // Mouse side buttons (X1 and X2)
            if (GetAsyncKeyState(VK_XBUTTON1) & 0x8001) {
                while (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                    Sleep(10);
                }
                if (outKey) *outKey = VK_XBUTTON1;
                return true;
            }
            if (GetAsyncKeyState(VK_XBUTTON2) & 0x8001) {
                while (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                    Sleep(10);
                }
                if (outKey) *outKey = VK_XBUTTON2;
                return true;
            }
        }
        
        // Check keyboard input
        if (captureKeyboard) {
            for (int vk = 1; vk < 256; vk++) {
                // Skip modifier keys and mouse buttons when checking for new press
                if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
                    vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
                    vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
                    continue;
                }
                
                if (GetAsyncKeyState(vk) & 0x8001) {
                    // Key is pressed - wait for release to confirm
                    while (GetAsyncKeyState(vk) & 0x8000) {
                        Sleep(10);
                    }
                    if (outKey) *outKey = (u32)vk;
                    return true;
                }
            }
        }
        
        // Check gamepad input
        if (captureGamepad) {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));
                
                if (XInputGetState(i, &state) == ERROR_SUCCESS) {
                    // Check digital buttons
                    if (state.Gamepad.wButtons != 0) {
                        u32 btn = state.Gamepad.wButtons;
                        // Wait for release
                        while (true) {
                            ZeroMemory(&state, sizeof(XINPUT_STATE));
                            if (XInputGetState(i, &state) != ERROR_SUCCESS || 
                                state.Gamepad.wButtons == 0) {
                                break;
                            }
                            Sleep(10);
                        }
                        if (outButton) *outButton = btn;
                        return true;
                    }
                    
                    // Check triggers
                    if (state.Gamepad.bLeftTrigger > 200) {
                        while (true) {
                            ZeroMemory(&state, sizeof(XINPUT_STATE));
                            if (XInputGetState(i, &state) != ERROR_SUCCESS || 
                                state.Gamepad.bLeftTrigger < 50) {
                                break;
                            }
                            Sleep(10);
                        }
                        if (outButton) *outButton = 0x1000; // Left Trigger
                        return true;
                    }
                    
                    if (state.Gamepad.bRightTrigger > 200) {
                        while (true) {
                            ZeroMemory(&state, sizeof(XINPUT_STATE));
                            if (XInputGetState(i, &state) != ERROR_SUCCESS || 
                                state.Gamepad.bRightTrigger < 50) {
                                break;
                            }
                            Sleep(10);
                        }
                        if (outButton) *outButton = 0x2000; // Right Trigger
                        return true;
                    }
                }
            }
        }
        
        // Allow cancellation with Escape
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8001) {
            return false;
        }
        
        Sleep(10);
    }
    
    return false; // Timeout
}

//=============================================================================
// Dialog Implementation
//=============================================================================

// Dialog control IDs
#define IDC_PROFILE_COMBO       1001
#define IDC_BUTTON_LIST         1002
#define IDC_BTN_REMAP           1003
#define IDC_BTN_RESET_BUTTON    1004
#define IDC_BTN_RESET_ALL       1005
#define IDC_BTN_NEW_PROFILE     1006
#define IDC_BTN_DELETE_PROFILE  1007
#define IDC_BTN_DUPLICATE       1008
#define IDC_TAB_CONTROL         1009
#define IDC_SLIDER_LEFT_DEADZONE    1010
#define IDC_SLIDER_RIGHT_DEADZONE   1011
#define IDC_SLIDER_MOUSE_SENS_X     1012
#define IDC_SLIDER_MOUSE_SENS_Y     1013
#define IDC_CHECK_INVERT_Y          1014
#define IDC_CHECK_DPAD_CAMERA       1015
#define IDC_CHECK_RSTICK_CAMERA     1016
#define IDC_CHECK_MOUSE_CAMERA      1017

static INT_PTR CALLBACK InputRemappingDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static void PopulateProfileCombo(HWND hDlg) {
    HWND hCombo = GetDlgItem(hDlg, IDC_PROFILE_COMBO);
    SendMessageA(hCombo, CB_RESETCONTENT, 0, 0);
    
    for (int i = 0; i < s_profileManager.profileCount; i++) {
        SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)s_profileManager.profiles[i].name);
    }
    
    SendMessageA(hCombo, CB_SETCURSEL, s_profileManager.activeProfileIndex, 0);
}

// Forward declarations
static void PopulateButtonList(HWND hDlg);

// Define button IDs that don't conflict with standard Windows IDs
#define IDC_REMAP_KEYBOARD  101
#define IDC_REMAP_MOUSE     102
#define IDC_REMAP_GAMEPAD   103
#define IDC_REMAP_CANCEL    104

// Input type selection dialog procedure
static INT_PTR CALLBACK InputTypeSelectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_REMAP_KEYBOARD:
            EndDialog(hDlg, 1);
            return TRUE;
        case IDC_REMAP_MOUSE:
            EndDialog(hDlg, 2);
            return TRUE;
        case IDC_REMAP_GAMEPAD:
            EndDialog(hDlg, 3);
            return TRUE;
        case IDC_REMAP_CANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return FALSE;
}

// Helper function to perform button remapping
static void PerformButtonRemap(HWND hDlg, int buttonIndex) {
    CV64_InputProfile* profile = CV64_InputRemapping_GetActiveProfile();
    if (!profile || buttonIndex < 0 || buttonIndex >= profile->buttonCount) {
        return;
    }
    
    // Create a simple selection dialog using task dialog or message box with custom buttons
    // Since TaskDialog might not be available, we'll create a simple popup window
    
    // Create selection window
    const int dlgWidth = 280;
    const int dlgHeight = 160;
    const int btnWidth = 80;
    const int btnHeight = 30;
    const int btnSpacing = 10;
    
    HWND hSelectWnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC", "",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - dlgWidth) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - dlgHeight) / 2,
        dlgWidth, dlgHeight,
        hDlg, NULL, g_hInst, NULL);
    
    if (!hSelectWnd) {
        return;
    }
    
    char title[128];
    sprintf_s(title, sizeof(title), "Remap: %s", profile->buttons[buttonIndex].displayName);
    SetWindowTextA(hSelectWnd, title);
    
    // Create label
    CreateWindowExA(0, "STATIC", "Select input type to bind:",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 15, dlgWidth - 20, 20,
        hSelectWnd, NULL, g_hInst, NULL);
    
    // Create buttons
    int startX = (dlgWidth - (btnWidth * 3 + btnSpacing * 2)) / 2;
    int btnY = 50;
    
    HWND hBtnKeyboard = CreateWindowExA(0, "BUTTON", "Keyboard",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        startX, btnY, btnWidth, btnHeight,
        hSelectWnd, (HMENU)IDC_REMAP_KEYBOARD, g_hInst, NULL);
    
    HWND hBtnMouse = CreateWindowExA(0, "BUTTON", "Mouse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        startX + btnWidth + btnSpacing, btnY, btnWidth, btnHeight,
        hSelectWnd, (HMENU)IDC_REMAP_MOUSE, g_hInst, NULL);
    
    HWND hBtnGamepad = CreateWindowExA(0, "BUTTON", "Gamepad",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        startX + (btnWidth + btnSpacing) * 2, btnY, btnWidth, btnHeight,
        hSelectWnd, (HMENU)IDC_REMAP_GAMEPAD, g_hInst, NULL);
    
    // Create cancel button
    CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (dlgWidth - btnWidth) / 2, btnY + btnHeight + 20, btnWidth, btnHeight,
        hSelectWnd, (HMENU)IDC_REMAP_CANCEL, g_hInst, NULL);
    
    ShowWindow(hSelectWnd, SW_SHOW);
    UpdateWindow(hSelectWnd);
    
    // Run a simple message loop for the selection window
    int choice = 0;
    MSG msg;
    bool running = true;
    
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.hwnd == hSelectWnd || IsChild(hSelectWnd, msg.hwnd)) {
            if (msg.message == WM_LBUTTONUP || msg.message == WM_KEYUP) {
                // Check which button was clicked
                HWND hFocus = GetFocus();
                if (hFocus == hBtnKeyboard) {
                    choice = 1;
                    running = false;
                } else if (hFocus == hBtnMouse) {
                    choice = 2;
                    running = false;
                } else if (hFocus == hBtnGamepad) {
                    choice = 3;
                    running = false;
                }
            }
            
            if (msg.message == WM_COMMAND) {
                int cmd = LOWORD(msg.wParam);
                if (cmd == IDC_REMAP_KEYBOARD) { choice = 1; running = false; }
                else if (cmd == IDC_REMAP_MOUSE) { choice = 2; running = false; }
                else if (cmd == IDC_REMAP_GAMEPAD) { choice = 3; running = false; }
                else if (cmd == IDC_REMAP_CANCEL) { running = false; }
            }
        }
        
        // Check for button clicks via BN_CLICKED
        if (msg.message == WM_COMMAND && HIWORD(msg.wParam) == BN_CLICKED) {
            int cmd = LOWORD(msg.wParam);
            if (cmd == IDC_REMAP_KEYBOARD) { choice = 1; running = false; }
            else if (cmd == IDC_REMAP_MOUSE) { choice = 2; running = false; }
            else if (cmd == IDC_REMAP_GAMEPAD) { choice = 3; running = false; }
            else if (cmd == IDC_REMAP_CANCEL) { running = false; }
        }
        
        // Handle close button
        if (msg.message == WM_CLOSE || msg.message == WM_QUIT) {
            running = false;
        }
        
        // Check for escape key
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            running = false;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    DestroyWindow(hSelectWnd);
    
    if (choice == 0) {
        return; // Cancelled
    }
    
    // Determine what type of input to capture
    const char* inputType = (choice == 1) ? "key" : ((choice == 2) ? "mouse button" : "gamepad button");
    
    // Show waiting message
    char msg2[128];
    sprintf_s(msg2, sizeof(msg2), 
        "Press a %s to bind to '%s'\n\n(Press ESC to cancel)",
        inputType, profile->buttons[buttonIndex].displayName);
    
    
    HWND hPrompt = CreateWindowExA(
        WS_EX_TOPMOST,
        "STATIC", msg2,
        WS_POPUP | WS_BORDER | SS_CENTER,
        (GetSystemMetrics(SM_CXSCREEN) - 320) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 100) / 2,
        320, 100,
        hDlg, NULL, g_hInst, NULL);
    
    if (hPrompt) {
        ShowWindow(hPrompt, SW_SHOW);
        UpdateWindow(hPrompt);
    }
    
    // Capture input based on selection
    u32 capturedKey = 0;
    u32 capturedButton = 0;
    bool captured = CV64_InputRemapping_CaptureInput(
        (choice == 1),  // keyboard
        (choice == 2),  // mouse
        (choice == 3),  // gamepad
        CAPTURE_TIMEOUT_MS, &capturedKey, &capturedButton);
    
    if (hPrompt) {
        DestroyWindow(hPrompt);
    }
    
    if (captured) {
        if ((choice == 1 || choice == 2) && capturedKey != 0) {
            profile->buttons[buttonIndex].keyboardKey = capturedKey;
        } else if (choice == 3 && capturedButton != 0) {
            profile->buttons[buttonIndex].gamepadButton = capturedButton;
        }
        // Refresh the list
        PopulateButtonList(hDlg);
    }
}

static void PopulateButtonList(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_BUTTON_LIST);
    ListView_DeleteAllItems(hList);
    
    CV64_InputProfile* profile = CV64_InputRemapping_GetActiveProfile();
    if (!profile) {
        return;
    }
    
    LVITEMA lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    
    for (int i = 0; i < profile->buttonCount; i++) {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = (LPSTR)profile->buttons[i].displayName;
        SendMessageA(hList, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
        
        // Set keyboard column
        LVITEMA lviSub = { 0 };
        lviSub.iSubItem = 1;
        lviSub.pszText = (LPSTR)CV64_InputRemapping_GetKeyName(profile->buttons[i].keyboardKey);
        SendMessageA(hList, LVM_SETITEMTEXTA, (WPARAM)i, (LPARAM)&lviSub);
        
        // Set gamepad column
        lviSub.iSubItem = 2;
        lviSub.pszText = (LPSTR)CV64_InputRemapping_GetButtonName(profile->buttons[i].gamepadButton);
        SendMessageA(hList, LVM_SETITEMTEXTA, (WPARAM)i, (LPARAM)&lviSub);
    }
}

static void InitializeListView(HWND hList) {
    // Set extended styles
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    
    // Add columns
    LVCOLUMNA lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    
    lvc.pszText = (LPSTR)"N64 Button";
    lvc.cx = 120;
    SendMessageA(hList, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvc);
    
    lvc.pszText = (LPSTR)"Keyboard";
    lvc.cx = 100;
    SendMessageA(hList, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvc);
    
    lvc.pszText = (LPSTR)"Gamepad";
    lvc.cx = 120;
    SendMessageA(hList, LVM_INSERTCOLUMNA, 2, (LPARAM)&lvc);
}

static INT_PTR CALLBACK InputRemappingDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
switch (message) {
case WM_INITDIALOG:
    s_dialogHwnd = hDlg;
        
    // Enable dark mode for title bar only (full dark mode looks bad with list views)
    EnableDarkModeTitleBarOnly(hDlg);
        
    // Initialize list view
    InitializeListView(GetDlgItem(hDlg, IDC_BUTTON_LIST));
        
    // Populate controls
    PopulateProfileCombo(hDlg);
    PopulateButtonList(hDlg);
        
    // Center dialog
    RECT rc;
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);
        
    return TRUE;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PROFILE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = (int)SendDlgItemMessage(hDlg, IDC_PROFILE_COMBO, CB_GETCURSEL, 0, 0);
                CV64_InputRemapping_SetActiveProfile(sel);
                PopulateButtonList(hDlg);
            }
            break;
            
        case IDC_BTN_NEW_PROFILE:
            {
                char name[64];
                sprintf_s(name, sizeof(name), "Profile %d", s_profileManager.profileCount + 1);
                int index = CV64_InputRemapping_CreateProfile(name);
                if (index >= 0) {
                    PopulateProfileCombo(hDlg);
                    SendDlgItemMessage(hDlg, IDC_PROFILE_COMBO, CB_SETCURSEL, index, 0);
                    CV64_InputRemapping_SetActiveProfile(index);
                    PopulateButtonList(hDlg);
                }
            }
            break;
            
        case IDC_BTN_DELETE_PROFILE:
            if (s_profileManager.profileCount > 1) {
                if (MessageBoxA(hDlg, "Delete this profile?", "Confirm", MB_YESNO) == IDYES) {
                    CV64_InputRemapping_DeleteProfile(s_profileManager.activeProfileIndex);
                    PopulateProfileCombo(hDlg);
                    PopulateButtonList(hDlg);
                }
            }
            break;
            
        case IDC_BTN_RESET_ALL:
            if (MessageBoxA(hDlg, "Reset all button mappings to default?", "Confirm", MB_YESNO) == IDYES) {
                CV64_InputRemapping_ResetProfile(s_profileManager.activeProfileIndex);
                PopulateButtonList(hDlg);
            }
            break;
            
        case IDC_BTN_REMAP:
            {
                // Get selected item from list
                int sel = ListView_GetNextItem(GetDlgItem(hDlg, IDC_BUTTON_LIST), -1, LVNI_SELECTED);
                if (sel < 0) {
                    MessageBoxA(hDlg, "Please select a button to remap first.", "Remap", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                
                PerformButtonRemap(hDlg, sel);
            }
            break;
            
        case IDOK:
            // Save current profile
            CV64_InputRemapping_ApplyProfile(CV64_InputRemapping_GetActiveProfile());
            CV64_InputRemapping_SaveProfiles(CONFIG_FILENAME);
            EndDialog(hDlg, IDOK);
            return TRUE;
            
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
        
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_BUTTON_LIST) {
            if (((LPNMHDR)lParam)->code == NM_DBLCLK) {
                // Double-click to remap
                int sel = ListView_GetNextItem(GetDlgItem(hDlg, IDC_BUTTON_LIST), -1, LVNI_SELECTED);
                if (sel >= 0) {
                    PerformButtonRemap(hDlg, sel);
                }
            }
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    
    return FALSE;
}

bool CV64_InputRemapping_ShowDialog(HWND hParent) {
if (!s_initialized) {
    CV64_InputRemapping_Init();
}
    
// Check if resource exists
HRSRC hRes = FindResourceA(g_hInst, MAKEINTRESOURCEA(IDD_INPUT_REMAPPING), (LPCSTR)RT_DIALOG);
if (!hRes) {
    MessageBoxA(hParent, "Input remapping dialog resource not found.", 
                "Error", MB_OK | MB_ICONERROR);
    return false;
}
    
// Show the input remapping dialog
INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INPUT_REMAPPING), 
                            hParent, InputRemappingDialogProc);
    
if (result == -1) {
        char msg[256];
        sprintf_s(msg, sizeof(msg), "Failed to create dialog. Error: %d", GetLastError());
        MessageBoxA(hParent, msg, "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return (result == IDOK);
}

//=============================================================================
// Preset Profiles
//=============================================================================

void CV64_InputRemapping_GetPreset(CV64_InputPreset preset, CV64_InputProfile* outProfile) {
    if (!outProfile) {
        return;
    }
    
    switch (preset) {
    case CV64_PRESET_DEFAULT:
        InitializeDefaultProfile(outProfile, "Default");
        break;
        
    case CV64_PRESET_KEYBOARD_ONLY:
        InitializeDefaultProfile(outProfile, "Keyboard Only");
        // Modify to use WASD for movement, etc.
        break;
        
    case CV64_PRESET_CONTROLLER_ONLY:
        InitializeDefaultProfile(outProfile, "Controller Only");
        break;
        
    case CV64_PRESET_MODERN_CONSOLE:
        InitializeDefaultProfile(outProfile, "Modern Console");
        // Souls-like layout
        break;
        
    case CV64_PRESET_CLASSIC_N64:
        InitializeDefaultProfile(outProfile, "Classic N64");
        break;
        
    case CV64_PRESET_SPEEDRUN:
        InitializeDefaultProfile(outProfile, "Speedrun");
        // Optimized for speedrunning
        break;
    }
}

//=============================================================================
// JSON Import/Export (Stub)
//=============================================================================

bool CV64_InputRemapping_ExportProfileJSON(const CV64_InputProfile* profile,
                                           char* outBuffer, size_t bufferSize) {
    // TODO: Implement JSON export
    return false;
}

bool CV64_InputRemapping_ImportProfileJSON(const char* jsonData,
                                           CV64_InputProfile* outProfile) {
    // TODO: Implement JSON import
    return false;
}
