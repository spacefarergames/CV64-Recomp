/**
 * @file cv64_controller.cpp
 * @brief Castlevania 64 PC Recomp - Enhanced Controller Implementation
 * 
 * Full XINPUT controller support and keyboard/mouse control for the game.
 * Provides input abstraction layer that works with mupen64plus emulator.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_controller.h"
#include "../include/cv64_camera_patch.h"
#include "../include/cv64_m64p_integration.h"
#include "../include/cv64_memory_hook.h"

#ifdef CV64_PLATFORM_WINDOWS
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#endif

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <atomic>

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static CV64_Controller s_controllers[MAXCONTROLLERS];
static CV64_ControllerConfig s_config;
static bool s_initialized = false;
static bool s_mouse_captured = false;
static POINT s_last_mouse_pos;
static POINT s_center_pos;
static HWND s_capture_window = NULL;
static s32 s_mouse_delta_x = 0;
static s32 s_mouse_delta_y = 0;
static std::atomic<s32> s_mouse_wheel_accum{0};
static bool s_xinput_connected[MAXCONTROLLERS] = { false, false, false, false };
static DWORD s_xinput_packet_num[MAXCONTROLLERS] = { 0, 0, 0, 0 };

/* Vibration state for XInput controllers */
static bool s_vibration_enabled = true;
static XINPUT_VIBRATION s_vibration_state[MAXCONTROLLERS] = { 0 };


/*===========================================================================
 * Default Configuration
 *===========================================================================*/

static void reset_config_defaults(void) {
    memset(&s_config, 0, sizeof(s_config));
    
    /* Default button bindings */
    int idx = 0;
    
    /* A button - Z key and XInput A */
    s_config.bindings[idx].n64_button = CONT_A;
    s_config.bindings[idx].keyboard_key = 'Z';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_A;
    idx++;
    
    /* B button - X key and XInput B */
    s_config.bindings[idx].n64_button = CONT_B;
    s_config.bindings[idx].keyboard_key = 'X';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_B;
    idx++;
    
    /* Z trigger - C key and Left Shoulder */
    s_config.bindings[idx].n64_button = CONT_G;
    s_config.bindings[idx].keyboard_key = 'C';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_LEFT_SHOULDER;
    idx++;
    
    /* Start - Enter and XInput Start */
    s_config.bindings[idx].n64_button = CONT_START;
    s_config.bindings[idx].keyboard_key = VK_RETURN;
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_START;
    idx++;
    
    /* L button - Q and Left Trigger (handled separately) */
    s_config.bindings[idx].n64_button = CONT_L;
    s_config.bindings[idx].keyboard_key = 'Q';
    s_config.bindings[idx].gamepad_button = 0;  /* Trigger handled separately */
    idx++;
    
    /* R button - E and Right Trigger */
    s_config.bindings[idx].n64_button = CONT_R;
    s_config.bindings[idx].keyboard_key = 'E';
    s_config.bindings[idx].gamepad_button = 0;  /* Trigger handled separately */
    idx++;
    
    /* C buttons */
    s_config.bindings[idx].n64_button = CONT_E;  /* C-Up */
    s_config.bindings[idx].keyboard_key = 'I';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_Y;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_D;  /* C-Down */
    s_config.bindings[idx].keyboard_key = 'K';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_X;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_C;  /* C-Left */
    s_config.bindings[idx].keyboard_key = 'J';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_LEFT_THUMB;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_F;  /* C-Right */
    s_config.bindings[idx].keyboard_key = 'L';
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_RIGHT_THUMB;
    idx++;
    
    /* D-Pad */
    s_config.bindings[idx].n64_button = CONT_UP;
    s_config.bindings[idx].keyboard_key = VK_UP;
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_DPAD_UP;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_DOWN;
    s_config.bindings[idx].keyboard_key = VK_DOWN;
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_DPAD_DOWN;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_LEFT;
    s_config.bindings[idx].keyboard_key = VK_LEFT;
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_DPAD_LEFT;
    idx++;
    
    s_config.bindings[idx].n64_button = CONT_RIGHT;
    s_config.bindings[idx].keyboard_key = VK_RIGHT;
    s_config.bindings[idx].gamepad_button = CV64_XINPUT_DPAD_RIGHT;
    idx++;
    
    s_config.binding_count = idx;
    
    /* Analog settings */
    s_config.left_stick_deadzone = 0.15f;
    s_config.right_stick_deadzone = 0.15f;
    s_config.left_stick_sensitivity = 1.0f;
    s_config.right_stick_sensitivity = 1.0f;
    s_config.trigger_threshold = 0.5f;
    
    /* Mouse settings */
    s_config.mouse_sensitivity_x = 0.15f;
    s_config.mouse_sensitivity_y = 0.15f;
    s_config.invert_mouse_y = false;
    
    /* Camera patch settings */
    s_config.enable_dpad_camera = true;
    s_config.enable_mouse_camera = true;
    s_config.enable_right_stick_camera = true;
    s_config.camera_rotation_speed = 2.0f;
    s_config.camera_pitch_limit_up = 80.0f;
    s_config.camera_pitch_limit_down = -60.0f;
}

/*===========================================================================
 * XInput Helper Functions
 *===========================================================================*/

/**
 * @brief Apply circular deadzone to analog stick
 */
static void ApplyCircularDeadzone(f32* x, f32* y, f32 deadzone, f32 sensitivity) {
    f32 magnitude = sqrtf((*x) * (*x) + (*y) * (*y));
    if (magnitude > deadzone) {
        f32 scale = ((magnitude - deadzone) / (1.0f - deadzone)) * sensitivity;
        if (magnitude > 0.0f) {
            *x = (*x / magnitude) * scale;
            *y = (*y / magnitude) * scale;
        }
        /* Clamp to -1 to 1 */
        if (*x > 1.0f) *x = 1.0f;
        if (*x < -1.0f) *x = -1.0f;
        if (*y > 1.0f) *y = 1.0f;
        if (*y < -1.0f) *y = -1.0f;
    } else {
        *x = 0.0f;
        *y = 0.0f;
    }
}

/**
 * @brief Check XInput controller connection status
 */
static void CheckXInputConnections(void) {
    for (DWORD i = 0; i < MAXCONTROLLERS; i++) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        
        DWORD result = XInputGetState(i, &state);
        bool wasConnected = s_xinput_connected[i];
        s_xinput_connected[i] = (result == ERROR_SUCCESS);
        
        if (!wasConnected && s_xinput_connected[i]) {
            OutputDebugStringA("[CV64] XInput controller connected\n");
        } else if (wasConnected && !s_xinput_connected[i]) {
            OutputDebugStringA("[CV64] XInput controller disconnected\n");
        }
    }
}

/*===========================================================================
 * API Implementation
 *===========================================================================*/

bool CV64_Controller_Init(void) {
    if (s_initialized) {
        return true;
    }
    
    memset(s_controllers, 0, sizeof(s_controllers));
    memset(s_xinput_connected, 0, sizeof(s_xinput_connected));
    memset(s_xinput_packet_num, 0, sizeof(s_xinput_packet_num));
    memset(s_vibration_state, 0, sizeof(s_vibration_state));
    
    reset_config_defaults();
    
    /* Check which XInput controllers are connected */
    CheckXInputConnections();
    
    /* Mark first controller as connected by default (keyboard/mouse always available) */
    s_controllers[0].is_connected = true;
    s_controllers[0].right_joy_deadzone = s_config.right_stick_deadzone;
    s_controllers[0].mouse_sensitivity = s_config.mouse_sensitivity_x;
    
    /* Try to load saved configuration */
    CV64_Controller_LoadConfig(NULL);
    
    s_initialized = true;
    OutputDebugStringA("[CV64] Controller system initialized\n");
    return true;
}

void CV64_Controller_Shutdown(void) {
    if (!s_initialized) {
        return;
    }
    
    /* Stop any vibration */
    for (DWORD i = 0; i < MAXCONTROLLERS; i++) {
        CV64_Controller_SetVibration(i, 0, 0);
    }
    
    /* Release mouse capture */
    if (s_mouse_captured) {
        CV64_Controller_SetMouseCapture(false);
    }
    
    CV64_Controller_SaveConfig(NULL);
    s_initialized = false;
    OutputDebugStringA("[CV64] Controller system shutdown\n");
}

/**
 * @brief Process keyboard input for a controller
 */
static void ProcessKeyboardInput(CV64_Controller* ctrl) {
    /* Analog stick from WASD */
    s16 kb_x = 0, kb_y = 0;
    if (GetAsyncKeyState(CV64_KEY_STICK_UP) & 0x8000) kb_y += 80;
    if (GetAsyncKeyState(CV64_KEY_STICK_DOWN) & 0x8000) kb_y -= 80;
    if (GetAsyncKeyState(CV64_KEY_STICK_LEFT) & 0x8000) kb_x -= 80;
    if (GetAsyncKeyState(CV64_KEY_STICK_RIGHT) & 0x8000) kb_x += 80;
    
    /* Diagonal normalization to prevent faster diagonal movement */
    if (kb_x != 0 && kb_y != 0) {
        f32 diag = 80.0f * 0.7071f; /* 1/sqrt(2) */
        kb_x = (s16)(kb_x > 0 ? diag : -diag);
        kb_y = (s16)(kb_y > 0 ? diag : -diag);
    }
    
    ctrl->joy_x = kb_x;
    ctrl->joy_y = kb_y;
    
    /* Process button bindings */
    for (u32 i = 0; i < s_config.binding_count; i++) {
        if (GetAsyncKeyState(s_config.bindings[i].keyboard_key) & 0x8000) {
            ctrl->btns_held |= s_config.bindings[i].n64_button;
        }
    }
}

/**
 * @brief Process mouse input for camera control
 */
static void ProcessMouseInput(CV64_Controller* ctrl) {
    if (!s_mouse_captured || !s_capture_window) {
        ctrl->mouse_delta_x = 0;
        ctrl->mouse_delta_y = 0;
        return;
    }
    
    POINT currentPos;
    GetCursorPos(&currentPos);
    
    /* Calculate delta from center of window */
    s_mouse_delta_x = currentPos.x - s_center_pos.x;
    s_mouse_delta_y = currentPos.y - s_center_pos.y;
    
    /* Apply sensitivity */
    ctrl->mouse_delta_x = (s32)(s_mouse_delta_x * s_config.mouse_sensitivity_x);
    ctrl->mouse_delta_y = (s32)(s_mouse_delta_y * s_config.mouse_sensitivity_y);
    
    /* Invert Y if configured */
    if (s_config.invert_mouse_y) {
        ctrl->mouse_delta_y = -ctrl->mouse_delta_y;
    }
    
    /* Reset cursor to center of window to allow continuous movement */
    if (s_mouse_delta_x != 0 || s_mouse_delta_y != 0) {
        SetCursorPos(s_center_pos.x, s_center_pos.y);
    }
}

/**
 * @brief Process XInput gamepad input
 */
static void ProcessXInputGamepad(CV64_Controller* ctrl, DWORD controller_index) {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    
    DWORD result = XInputGetState(controller_index, &state);
    
    if (result == ERROR_SUCCESS) {
        ctrl->is_connected = true;
        s_xinput_connected[controller_index] = true;
        
        /* Check if state changed */
        if (state.dwPacketNumber != s_xinput_packet_num[controller_index]) {
            s_xinput_packet_num[controller_index] = state.dwPacketNumber;
        }
        
        /* Left stick - movement control */
        f32 lx = (f32)state.Gamepad.sThumbLX / 32768.0f;
        f32 ly = (f32)state.Gamepad.sThumbLY / 32768.0f;
        
        ApplyCircularDeadzone(&lx, &ly, s_config.left_stick_deadzone, s_config.left_stick_sensitivity);
        
        /* Convert to N64 analog range (-80 to 80) */
        s16 stick_x = (s16)(lx * 80.0f);
        s16 stick_y = (s16)(ly * 80.0f);
        
        /* Merge with keyboard input (take the larger magnitude) */
        if (abs(stick_x) > abs(ctrl->joy_x)) ctrl->joy_x = stick_x;
        if (abs(stick_y) > abs(ctrl->joy_y)) ctrl->joy_y = stick_y;
        
        /* Right stick - camera control */
        f32 rx = (f32)state.Gamepad.sThumbRX / 32768.0f;
        f32 ry = (f32)state.Gamepad.sThumbRY / 32768.0f;
        
        ApplyCircularDeadzone(&rx, &ry, s_config.right_stick_deadzone, s_config.right_stick_sensitivity);
        
        ctrl->right_joy_x = (s16)(rx * 128.0f);
        ctrl->right_joy_y = (s16)(ry * 128.0f);
        
        /* Triggers - L and R buttons */
        f32 leftTrigger = (f32)state.Gamepad.bLeftTrigger / 255.0f;
        f32 rightTrigger = (f32)state.Gamepad.bRightTrigger / 255.0f;
        
        if (leftTrigger > s_config.trigger_threshold) {
            ctrl->btns_held |= CONT_L;
        }
        if (rightTrigger > s_config.trigger_threshold) {
            ctrl->btns_held |= CONT_R;
        }
        
        /* Digital buttons */
        for (u32 i = 0; i < s_config.binding_count; i++) {
            if (s_config.bindings[i].gamepad_button != 0 &&
                (state.Gamepad.wButtons & s_config.bindings[i].gamepad_button)) {
                ctrl->btns_held |= s_config.bindings[i].n64_button;
            }
        }
    } else if (s_xinput_connected[controller_index]) {
        s_xinput_connected[controller_index] = false;
        OutputDebugStringA("[CV64] XInput controller disconnected\n");
    }
}

void CV64_Controller_Update(u32 controller_index) {
    if (!s_initialized || controller_index >= MAXCONTROLLERS) {
        return;
    }
    
    CV64_Controller* ctrl = &s_controllers[controller_index];
    u16 prev_buttons = ctrl->btns_held;
    ctrl->btns_held = 0;
    ctrl->btns_pressed = 0;
    
    /* Reset analog values */
    ctrl->joy_x = 0;
    ctrl->joy_y = 0;
    ctrl->right_joy_x = 0;
    ctrl->right_joy_y = 0;
    
#ifdef CV64_PLATFORM_WINDOWS
    /* Process keyboard input for controller 0 */
    if (controller_index == 0) {
        ProcessKeyboardInput(ctrl);
        ProcessMouseInput(ctrl);
    }
    
    /* Process XInput gamepad */
    ProcessXInputGamepad(ctrl, controller_index);
#endif
    
    /* Calculate pressed buttons (newly pressed this frame) */
    ctrl->btns_pressed = ctrl->btns_held & ~prev_buttons;
    
    /* Store raw D-PAD state for camera patch */
    ctrl->dpad_raw = ctrl->btns_held & (CONT_UP | CONT_DOWN | CONT_LEFT | CONT_RIGHT);
    
    /* If D-PAD camera mode is enabled, process it */
    if (ctrl->dpad_camera_mode && s_config.enable_dpad_camera) {
        /* Pass D-PAD to camera patch instead of game */
        CV64_CameraPatch_ProcessDPad(ctrl->dpad_raw, 1.0f / 60.0f);
        /* Clear D-PAD from button state so game doesn't see it */
        ctrl->btns_held &= ~(CONT_UP | CONT_DOWN | CONT_LEFT | CONT_RIGHT);
        ctrl->btns_pressed &= ~(CONT_UP | CONT_DOWN | CONT_LEFT | CONT_RIGHT);
    }
    /* Camera mode 0: allow free left/right rotation via D-PAD
     * Accumulates a yaw offset that FrameUpdate applies every frame
     * to the game's player_angle_yaw in cameraMgr_data. */
    else if (CV64_Memory_IsInitialized()) {
        u32 camMode = CV64_Memory_GetCameraMode();
        if (camMode == 0) {
            u16 lr = ctrl->dpad_raw & (CONT_LEFT | CONT_RIGHT);
            if (lr) {
                /* N64 binary angle: 0x10000 = 360 degrees.
                 * Rotate ~2.8 degrees per frame at 60fps. */
                const s32 YAW_STEP = 0x200;
                if (lr & CONT_LEFT)  CV64_Memory_AddCameraYawOffset(YAW_STEP);
                if (lr & CONT_RIGHT) CV64_Memory_AddCameraYawOffset(-YAW_STEP);
                /* Strip left/right from game input; up/down still pass through */
                ctrl->btns_held &= ~(CONT_LEFT | CONT_RIGHT);
                ctrl->btns_pressed &= ~(CONT_LEFT | CONT_RIGHT);
            }
        }
    }
    
    /* Process right stick camera control */
    if (s_config.enable_right_stick_camera) {
        CV64_CameraPatch_ProcessRightStick(ctrl->right_joy_x, ctrl->right_joy_y, 1.0f / 60.0f);
    }
    
    /* Process mouse camera control */
    if (s_config.enable_mouse_camera && ctrl->mouse_captured) {
        if (CV64_Memory_IsInitialized() && CV64_Memory_GetCameraMode() == 0 && ctrl->mouse_delta_x != 0) {
            /* Camera mode 0: mouse X accumulates yaw offset */
            CV64_Memory_AddCameraYawOffset(-ctrl->mouse_delta_x * 0x10);
        } else {
            CV64_CameraPatch_ProcessMouse(ctrl->mouse_delta_x, ctrl->mouse_delta_y, 1.0f / 60.0f);
        }
    }

    /* Mouse wheel zoom — consume accumulated wheel delta */
    {
        s32 wheel = s_mouse_wheel_accum.exchange(0, std::memory_order_relaxed);
        ctrl->mouse_wheel_delta = wheel;
        if (wheel != 0 && CV64_Memory_IsInitialized()) {
            /* Positive wheel = scroll up = zoom in (closer), negative = zoom out */
            f32 zoomStep = (wheel > 0) ? -0.05f : 0.05f;
            CV64_Memory_AddCameraZoomOffset(zoomStep);
        }
    }
    
    /* Sync controller state to emulator if running */
    CV64_M64P_ControllerState emuState;
    emuState.x_axis = (s8)(ctrl->joy_x);
    emuState.y_axis = (s8)(ctrl->joy_y);
    emuState.buttons = ctrl->btns_held;
    CV64_M64P_SetControllerOverride(controller_index, &emuState);
}

/**
 * @brief Update only camera-related input (D-PAD, right stick, mouse)
 * This does NOT affect N64 controller state or sync with emulator.
 * Use when mupen64plus handles actual game input via input-sdl,
 * but we still need to process camera control input.
 */
void CV64_Controller_UpdateCameraInput(void) {
    if (!s_initialized) {
        return;
    }
    
    const f32 deltaTime = 1.0f / 60.0f;
    u16 dpadState = 0;
    s16 rightJoyX = 0;
    s16 rightJoyY = 0;
    s32 mouseDeltaX = 0;
    s32 mouseDeltaY = 0;
    
#ifdef CV64_PLATFORM_WINDOWS
    /* Read keyboard D-PAD input */
    if (GetAsyncKeyState(VK_UP) & 0x8000) dpadState |= CONT_UP;
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) dpadState |= CONT_DOWN;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) dpadState |= CONT_LEFT;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) dpadState |= CONT_RIGHT;
    
    /* Read XInput controller state for camera input only */
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            /* D-PAD from gamepad */
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) dpadState |= CONT_UP;
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) dpadState |= CONT_DOWN;
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) dpadState |= CONT_LEFT;
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) dpadState |= CONT_RIGHT;
            
            /* Right stick */
            f32 rx = (f32)state.Gamepad.sThumbRX / 32768.0f;
            f32 ry = (f32)state.Gamepad.sThumbRY / 32768.0f;
            
            /* Apply deadzone */
            f32 deadzone = s_config.right_stick_deadzone;
            f32 magnitude = sqrtf(rx * rx + ry * ry);
            if (magnitude > deadzone) {
                f32 normalized = (magnitude - deadzone) / (1.0f - deadzone);
                rx = rx / magnitude * normalized;
                ry = ry / magnitude * normalized;
                rightJoyX = (s16)(rx * 128.0f);
                rightJoyY = (s16)(ry * 128.0f);
            }
            
            break; /* Use first connected controller */
        }
    }
    
    /* Read mouse input for camera */
    if (s_mouse_captured && s_capture_window) {
        POINT currentPos;
        GetCursorPos(&currentPos);
        
        mouseDeltaX = (s32)((currentPos.x - s_center_pos.x) * s_config.mouse_sensitivity_x);
        mouseDeltaY = (s32)((currentPos.y - s_center_pos.y) * s_config.mouse_sensitivity_y);
        
        if (s_config.invert_mouse_y) {
            mouseDeltaY = -mouseDeltaY;
        }
        
        /* Reset cursor to center */
        if (mouseDeltaX != 0 || mouseDeltaY != 0) {
            SetCursorPos(s_center_pos.x, s_center_pos.y);
        }
    }
#endif
    
    /* Process camera input */
    if (s_config.enable_dpad_camera && dpadState != 0) {
        CV64_CameraPatch_ProcessDPad(dpadState, deltaTime);
    }
    
    if (s_config.enable_right_stick_camera && (rightJoyX != 0 || rightJoyY != 0)) {
        CV64_CameraPatch_ProcessRightStick(rightJoyX, rightJoyY, deltaTime);
    }
    
    if (s_config.enable_mouse_camera && s_mouse_captured && (mouseDeltaX != 0 || mouseDeltaY != 0)) {
        CV64_CameraPatch_ProcessMouse(mouseDeltaX, -mouseDeltaY, deltaTime);
    }
}

CV64_Controller* CV64_Controller_GetState(u32 controller_index) {
    if (controller_index >= MAXCONTROLLERS) {
        return NULL;
    }
    return &s_controllers[controller_index];
}

bool CV64_Controller_IsButtonHeld(u32 controller_index, u16 button) {
    if (controller_index >= MAXCONTROLLERS) {
        return false;
    }
    return (s_controllers[controller_index].btns_held & button) != 0;
}

bool CV64_Controller_IsButtonPressed(u32 controller_index, u16 button) {
    if (controller_index >= MAXCONTROLLERS) {
        return false;
    }
    return (s_controllers[controller_index].btns_pressed & button) != 0;
}

void CV64_Controller_GetAnalogStick(u32 controller_index, s16* out_x, s16* out_y) {
    if (controller_index >= MAXCONTROLLERS) {
        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        return;
    }
    if (out_x) *out_x = s_controllers[controller_index].joy_x;
    if (out_y) *out_y = s_controllers[controller_index].joy_y;
}

void CV64_Controller_GetRightStick(u32 controller_index, s16* out_x, s16* out_y) {
    if (controller_index >= MAXCONTROLLERS) {
        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        return;
    }
    if (out_x) *out_x = s_controllers[controller_index].right_joy_x;
    if (out_y) *out_y = s_controllers[controller_index].right_joy_y;
}

void CV64_Controller_GetMouseDelta(s32* out_dx, s32* out_dy) {
    if (out_dx) *out_dx = s_mouse_delta_x;
    if (out_dy) *out_dy = s_mouse_delta_y;
}

void CV64_Controller_NotifyMouseWheel(s32 delta) {
    s_mouse_wheel_accum.fetch_add(delta, std::memory_order_relaxed);
}

void CV64_Controller_SetMouseCapture(bool captured) {
    s_mouse_captured = captured;
    s_controllers[0].mouse_captured = captured;
    
#ifdef CV64_PLATFORM_WINDOWS
    if (captured) {
        /* Get the active window for capture */
        s_capture_window = GetForegroundWindow();
        if (s_capture_window) {
            /* Calculate center of window */
            RECT windowRect;
            GetWindowRect(s_capture_window, &windowRect);
            s_center_pos.x = (windowRect.left + windowRect.right) / 2;
            s_center_pos.y = (windowRect.top + windowRect.bottom) / 2;
            
            /* Hide cursor and center it */
            ShowCursor(FALSE);
            SetCursorPos(s_center_pos.x, s_center_pos.y);
            
            /* Clip cursor to window */
            ClipCursor(&windowRect);
            
            OutputDebugStringA("[CV64] Mouse captured for camera control\n");
        }
    } else {
        /* Release cursor clip and show cursor */
        ClipCursor(NULL);
        ShowCursor(TRUE);
        s_capture_window = NULL;
        s_mouse_delta_x = 0;
        s_mouse_delta_y = 0;
        OutputDebugStringA("[CV64] Mouse released\n");
    }
#endif
}

void CV64_Controller_UpdateMouseCapture(HWND hwnd) {
    if (s_mouse_captured && hwnd) {
        s_capture_window = hwnd;
        RECT windowRect;
        GetWindowRect(hwnd, &windowRect);
        s_center_pos.x = (windowRect.left + windowRect.right) / 2;
        s_center_pos.y = (windowRect.top + windowRect.bottom) / 2;
        ClipCursor(&windowRect);
    }
}

bool CV64_Controller_LoadConfig(const char* filepath) {
    /* TODO: Implement config loading */
    return true;
}

bool CV64_Controller_SaveConfig(const char* filepath) {
    /* TODO: Implement config saving */
    return true;
}

CV64_ControllerConfig* CV64_Controller_GetConfig(void) {
    return &s_config;
}

void CV64_Controller_ApplyConfig(void) {
    /* Clamp values to valid ranges */
    if (s_config.left_stick_deadzone < 0.0f) s_config.left_stick_deadzone = 0.0f;
    if (s_config.left_stick_deadzone > 0.9f) s_config.left_stick_deadzone = 0.9f;
    if (s_config.right_stick_deadzone < 0.0f) s_config.right_stick_deadzone = 0.0f;
    if (s_config.right_stick_deadzone > 0.9f) s_config.right_stick_deadzone = 0.9f;
}

/*===========================================================================
 * D-PAD Camera Functions
 *===========================================================================*/

void CV64_Controller_GetDPadCameraInput(u32 controller_index, f32* out_yaw, f32* out_pitch) {
    if (controller_index >= MAXCONTROLLERS) {
        if (out_yaw) *out_yaw = 0.0f;
        if (out_pitch) *out_pitch = 0.0f;
        return;
    }
    
    u16 dpad = s_controllers[controller_index].dpad_raw;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    f32 speed = s_config.camera_rotation_speed;
    
    if (dpad & CONT_LEFT) yaw -= speed;
    if (dpad & CONT_RIGHT) yaw += speed;
    if (dpad & CONT_UP) pitch += speed;
    if (dpad & CONT_DOWN) pitch -= speed;
    
    if (out_yaw) *out_yaw = yaw;
    if (out_pitch) *out_pitch = pitch;
}

void CV64_Controller_SetDPadCameraMode(bool enabled) {
    s_controllers[0].dpad_camera_mode = enabled;
}

bool CV64_Controller_IsDPadCameraModeEnabled(void) {
    return s_controllers[0].dpad_camera_mode;
}

/*===========================================================================
 * Vibration/Rumble Functions
 *===========================================================================*/

void CV64_Controller_SetVibration(u32 controller_index, u16 left_motor, u16 right_motor) {
    if (controller_index >= MAXCONTROLLERS || !s_vibration_enabled) {
        return;
    }
    
#ifdef CV64_PLATFORM_WINDOWS
    s_vibration_state[controller_index].wLeftMotorSpeed = left_motor;
    s_vibration_state[controller_index].wRightMotorSpeed = right_motor;
    XInputSetState(controller_index, &s_vibration_state[controller_index]);
#endif
}

void CV64_Controller_SetVibrationEnabled(bool enabled) {
    s_vibration_enabled = enabled;
    
    /* Stop all vibration if disabling */
    if (!enabled) {
        for (DWORD i = 0; i < MAXCONTROLLERS; i++) {
            CV64_Controller_SetVibration(i, 0, 0);
        }
    }
}

bool CV64_Controller_IsVibrationEnabled(void) {
    return s_vibration_enabled;
}

/*===========================================================================
 * XInput Status Functions
 *===========================================================================*/

bool CV64_Controller_IsXInputConnected(u32 controller_index) {
    if (controller_index >= MAXCONTROLLERS) {
        return false;
    }
    return s_xinput_connected[controller_index];
}

u32 CV64_Controller_GetConnectedControllerCount(void) {
    u32 count = 0;
    for (u32 i = 0; i < MAXCONTROLLERS; i++) {
        if (s_xinput_connected[i]) {
            count++;
        }
    }
    return count;
}

void CV64_Controller_RefreshConnections(void) {
    CheckXInputConnections();
}

/*===========================================================================
 * Button Rebinding Functions
 *===========================================================================*/

bool CV64_Controller_SetKeyBinding(u16 n64_button, u32 keyboard_key) {
    for (u32 i = 0; i < s_config.binding_count; i++) {
        if (s_config.bindings[i].n64_button == n64_button) {
            s_config.bindings[i].keyboard_key = keyboard_key;
            return true;
        }
    }
    return false;
}

bool CV64_Controller_SetGamepadBinding(u16 n64_button, u32 gamepad_button) {
    for (u32 i = 0; i < s_config.binding_count; i++) {
        if (s_config.bindings[i].n64_button == n64_button) {
            s_config.bindings[i].gamepad_button = gamepad_button;
            return true;
        }
    }
    return false;
}

u32 CV64_Controller_GetKeyBinding(u16 n64_button) {
    for (u32 i = 0; i < s_config.binding_count; i++) {
        if (s_config.bindings[i].n64_button == n64_button) {
            return s_config.bindings[i].keyboard_key;
        }
    }
    return 0;
}

u32 CV64_Controller_GetGamepadBinding(u16 n64_button) {
    for (u32 i = 0; i < s_config.binding_count; i++) {
        if (s_config.bindings[i].n64_button == n64_button) {
            return s_config.bindings[i].gamepad_button;
        }
    }
    return 0;
}

void CV64_Controller_ResetBindingsToDefault(void) {
    reset_config_defaults();
}

/*===========================================================================
 * Input Mode Functions
 *===========================================================================*/

bool CV64_Controller_IsMouseCaptured(void) {
    return s_mouse_captured;
}

void CV64_Controller_ToggleMouseCapture(void) {
    CV64_Controller_SetMouseCapture(!s_mouse_captured);
}
