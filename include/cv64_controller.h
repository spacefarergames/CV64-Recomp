/**
 * @file cv64_controller.h
 * @brief Castlevania 64 PC Recomp - Enhanced Controller System
 * 
 * Provides modern input handling with support for:
 * - Keyboard/Mouse input
 * - XInput/DirectInput controllers
 * - Configurable button remapping
 * - D-PAD camera control patch integration
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_CONTROLLER_H
#define CV64_CONTROLLER_H

#include "cv64_types.h"

/* Windows.h is now included by cv64_types.h */

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Controller State Structure
 *===========================================================================*/

/**
 * @brief Controller state matching original N64 structure
 * @note Extended with additional fields for PC enhancements
 */
typedef struct CV64_Controller {
    /* Original N64 controller fields */
    u16 is_connected;       /**< Controller connection status */
    u16 btns_held;          /**< Buttons currently held down */
    u16 btns_pressed;       /**< Buttons pressed this frame (edge triggered) */
    s16 joy_x;              /**< Analog stick X axis (-80 to 80) */
    s16 joy_y;              /**< Analog stick Y axis (-80 to 80) */
    s16 joy_ang;            /**< Analog stick angle (calculated) */
    s16 joy_held;           /**< Analog stick magnitude */
    
    /* PC Enhanced fields */
    s16 right_joy_x;        /**< Right analog stick X (for camera control) */
    s16 right_joy_y;        /**< Right analog stick Y (for camera control) */
    f32 right_joy_deadzone; /**< Right stick deadzone threshold */
    
    /* D-PAD state for camera patch */
    u16 dpad_raw;           /**< Raw D-PAD state before translation */
    bool dpad_camera_mode;  /**< TRUE = D-PAD controls camera */
    
    /* Mouse input (for PC) */
    s32 mouse_delta_x;      /**< Mouse X movement this frame */
    s32 mouse_delta_y;      /**< Mouse Y movement this frame */
    s32 mouse_wheel_delta;  /**< Mouse wheel scroll delta this frame */
    bool mouse_captured;    /**< Mouse capture state */
    f32 mouse_sensitivity;  /**< Mouse sensitivity multiplier */

} CV64_Controller;

/*===========================================================================
 * Controller Configuration
 *===========================================================================*/

/**
 * @brief Keyboard binding for N64 button
 */
typedef struct CV64_KeyBinding {
    u16 n64_button;         /**< N64 button mask */
    u32 keyboard_key;       /**< Windows virtual key code */
    u32 gamepad_button;     /**< XInput button mask */
} CV64_KeyBinding;

/**
 * @brief Full controller configuration
 */
typedef struct CV64_ControllerConfig {
    /* Button mappings */
    CV64_KeyBinding bindings[16];
    u32 binding_count;
    
    /* Analog settings */
    f32 left_stick_deadzone;
    f32 right_stick_deadzone;
    f32 left_stick_sensitivity;
    f32 right_stick_sensitivity;
    
    /* Trigger settings */
    f32 trigger_threshold;
    
    /* Mouse settings */
    f32 mouse_sensitivity_x;
    f32 mouse_sensitivity_y;
    bool invert_mouse_y;
    
    /* Camera patch settings */
    bool enable_dpad_camera;
    bool enable_mouse_camera;
    bool enable_right_stick_camera;
    f32 camera_rotation_speed;
    f32 camera_pitch_limit_up;
    f32 camera_pitch_limit_down;
    
} CV64_ControllerConfig;

/*===========================================================================
 * Default Key Bindings
 *===========================================================================*/

/* Default keyboard mappings for N64 controller */
#define CV64_KEY_A          'Z'
#define CV64_KEY_B          'X'
#define CV64_KEY_Z          'C'
#define CV64_KEY_START      VK_RETURN
#define CV64_KEY_L          'Q'
#define CV64_KEY_R          'E'
#define CV64_KEY_CUP        'I'
#define CV64_KEY_CDOWN      'K'
#define CV64_KEY_CLEFT      'J'
#define CV64_KEY_CRIGHT     'L'
#define CV64_KEY_DUP        VK_UP
#define CV64_KEY_DDOWN      VK_DOWN
#define CV64_KEY_DLEFT      VK_LEFT
#define CV64_KEY_DRIGHT     VK_RIGHT

/* Analog stick keyboard mapping */
#define CV64_KEY_STICK_UP   'W'
#define CV64_KEY_STICK_DOWN 'S'
#define CV64_KEY_STICK_LEFT 'A'
#define CV64_KEY_STICK_RIGHT 'D'

/*===========================================================================
 * XInput Button Masks
 *===========================================================================*/

#define CV64_XINPUT_DPAD_UP      0x0001
#define CV64_XINPUT_DPAD_DOWN    0x0002
#define CV64_XINPUT_DPAD_LEFT    0x0004
#define CV64_XINPUT_DPAD_RIGHT   0x0008
#define CV64_XINPUT_START        0x0010
#define CV64_XINPUT_BACK         0x0020
#define CV64_XINPUT_LEFT_THUMB   0x0040
#define CV64_XINPUT_RIGHT_THUMB  0x0080
#define CV64_XINPUT_LEFT_SHOULDER  0x0100
#define CV64_XINPUT_RIGHT_SHOULDER 0x0200
#define CV64_XINPUT_A            0x1000
#define CV64_XINPUT_B            0x2000
#define CV64_XINPUT_X            0x4000
#define CV64_XINPUT_Y            0x8000

/*===========================================================================
 * Controller API Functions
 *===========================================================================*/

/**
 * @brief Initialize the controller system
 * @return TRUE on success, FALSE on failure
 */
CV64_API bool CV64_Controller_Init(void);

/**
 * @brief Shutdown the controller system
 */
CV64_API void CV64_Controller_Shutdown(void);

/**
 * @brief Update controller state for current frame
 * @param controller_index Controller index (0-3)
 */
CV64_API void CV64_Controller_Update(u32 controller_index);

/**
 * @brief Get controller state
 * @param controller_index Controller index (0-3)
 * @return Pointer to controller state, or NULL if invalid index
 */
CV64_API CV64_Controller* CV64_Controller_GetState(u32 controller_index);

/**
 * @brief Check if a button is currently held
 * @param controller_index Controller index
 * @param button N64 button mask
 * @return TRUE if held
 */
CV64_API bool CV64_Controller_IsButtonHeld(u32 controller_index, u16 button);

/**
 * @brief Check if a button was just pressed this frame
 * @param controller_index Controller index
 * @param button N64 button mask
 * @return TRUE if pressed this frame
 */
CV64_API bool CV64_Controller_IsButtonPressed(u32 controller_index, u16 button);

/**
 * @brief Get analog stick position
 * @param controller_index Controller index
 * @param out_x Pointer to store X value
 * @param out_y Pointer to store Y value
 */
CV64_API void CV64_Controller_GetAnalogStick(u32 controller_index, s16* out_x, s16* out_y);

/**
 * @brief Get right analog stick position (for camera)
 * @param controller_index Controller index
 * @param out_x Pointer to store X value
 * @param out_y Pointer to store Y value
 */
CV64_API void CV64_Controller_GetRightStick(u32 controller_index, s16* out_x, s16* out_y);

/**
 * @brief Get mouse delta for camera control
 * @param out_dx Pointer to store X delta
 * @param out_dy Pointer to store Y delta
 */
CV64_API void CV64_Controller_GetMouseDelta(s32* out_dx, s32* out_dy);

/**
 * @brief Set mouse capture state
 * @param captured TRUE to capture mouse
 */
CV64_API void CV64_Controller_SetMouseCapture(bool captured);

/**
 * @brief Notify the controller system of a mouse wheel event
 * @param delta Wheel delta from WM_MOUSEWHEEL (positive = scroll up)
 */
CV64_API void CV64_Controller_NotifyMouseWheel(s32 delta);

/**
 * @brief Load controller configuration from file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Controller_LoadConfig(const char* filepath);

/**
 * @brief Save controller configuration to file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_Controller_SaveConfig(const char* filepath);

/**
 * @brief Get current controller configuration
 * @return Pointer to configuration
 */
CV64_API CV64_ControllerConfig* CV64_Controller_GetConfig(void);

/**
 * @brief Apply configuration changes
 */
CV64_API void CV64_Controller_ApplyConfig(void);

/*===========================================================================
 * D-PAD Camera Patch Functions
 *===========================================================================*/

/**
 * @brief Update only camera-related input (D-PAD, right stick, mouse)
 * This does NOT affect N64 controller state - use when mupen64plus
 * handles actual game input but we still need camera control.
 */
CV64_API void CV64_Controller_UpdateCameraInput(void);

/**
 * @brief Get D-PAD state translated for camera control
 * @param controller_index Controller index
 * @param out_yaw Output yaw rotation delta
 * @param out_pitch Output pitch rotation delta
 */
CV64_API void CV64_Controller_GetDPadCameraInput(u32 controller_index, 
                                                   f32* out_yaw, 
                                                   f32* out_pitch);

/**
 * @brief Enable/disable D-PAD camera control mode
 * @param enabled TRUE to enable
 */
CV64_API void CV64_Controller_SetDPadCameraMode(bool enabled);

/**
 * @brief Check if D-PAD camera mode is enabled
 * @return TRUE if enabled
 */
CV64_API bool CV64_Controller_IsDPadCameraModeEnabled(void);

/*===========================================================================
 * Vibration/Rumble Functions
 *===========================================================================*/

/**
 * @brief Set controller vibration motors
 * @param controller_index Controller index (0-3)
 * @param left_motor Left motor speed (0-65535)
 * @param right_motor Right motor speed (0-65535)
 */
CV64_API void CV64_Controller_SetVibration(u32 controller_index, u16 left_motor, u16 right_motor);

/**
 * @brief Enable or disable vibration globally
 * @param enabled TRUE to enable
 */
CV64_API void CV64_Controller_SetVibrationEnabled(bool enabled);

/**
 * @brief Check if vibration is enabled
 * @return TRUE if enabled
 */
CV64_API bool CV64_Controller_IsVibrationEnabled(void);

/*===========================================================================
 * XInput Status Functions
 *===========================================================================*/

/**
 * @brief Check if an XInput controller is connected
 * @param controller_index Controller index (0-3)
 * @return TRUE if connected
 */
CV64_API bool CV64_Controller_IsXInputConnected(u32 controller_index);

/**
 * @brief Get number of connected controllers
 * @return Number of connected controllers
 */
CV64_API u32 CV64_Controller_GetConnectedControllerCount(void);

/**
 * @brief Refresh controller connection status
 */
CV64_API void CV64_Controller_RefreshConnections(void);

/*===========================================================================
 * Button Rebinding Functions
 *===========================================================================*/

/**
 * @brief Set keyboard binding for an N64 button
 * @param n64_button N64 button mask
 * @param keyboard_key Windows virtual key code
 * @return TRUE on success
 */
CV64_API bool CV64_Controller_SetKeyBinding(u16 n64_button, u32 keyboard_key);

/**
 * @brief Set gamepad binding for an N64 button
 * @param n64_button N64 button mask
 * @param gamepad_button XInput button mask
 * @return TRUE on success
 */
CV64_API bool CV64_Controller_SetGamepadBinding(u16 n64_button, u32 gamepad_button);

/**
 * @brief Get keyboard binding for an N64 button
 * @param n64_button N64 button mask
 * @return Windows virtual key code
 */
CV64_API u32 CV64_Controller_GetKeyBinding(u16 n64_button);

/**
 * @brief Get gamepad binding for an N64 button
 * @param n64_button N64 button mask
 * @return XInput button mask
 */
CV64_API u32 CV64_Controller_GetGamepadBinding(u16 n64_button);

/**
 * @brief Reset all bindings to default
 */
CV64_API void CV64_Controller_ResetBindingsToDefault(void);

/*===========================================================================
 * Input Mode Functions
 *===========================================================================*/

/**
 * @brief Check if mouse is currently captured
 * @return TRUE if captured
 */
CV64_API bool CV64_Controller_IsMouseCaptured(void);

/**
 * @brief Toggle mouse capture state
 */
CV64_API void CV64_Controller_ToggleMouseCapture(void);

/**
 * @brief Update mouse capture position (call on window resize/move)
 * @param hwnd Window handle
 */
CV64_API void CV64_Controller_UpdateMouseCapture(HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif /* CV64_CONTROLLER_H */
