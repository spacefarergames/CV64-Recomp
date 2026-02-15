/**
 * @file cv64_camera_patch.h
 * @brief Castlevania 64 PC Recomp - D-PAD Camera Control Patch
 * 
 * This is the MOST IMPORTANT PATCH for the recomp port!
 * 
 * Implements free camera control using:
 * - D-PAD (original controller)
 * - Right analog stick (modern controllers)
 * - Mouse (PC)
 * 
 * The original Castlevania 64 had very limited camera control.
 * This patch adds full 360-degree camera rotation similar to
 * modern 3D games.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_CAMERA_PATCH_H
#define CV64_CAMERA_PATCH_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Camera Patch Configuration
 *===========================================================================*/

typedef struct CV64_CameraPatchConfig {
    /* Enable flags */
    bool enabled;                   /**< Master enable for camera patch */
    bool enable_dpad;               /**< Enable D-PAD camera control */
    bool enable_right_stick;        /**< Enable right analog stick */
    bool enable_mouse;              /**< Enable mouse camera control */
    
    /* Sensitivity settings */
    f32 dpad_rotation_speed;        /**< D-PAD rotation speed (degrees/frame) */
    f32 stick_sensitivity_x;        /**< Right stick X sensitivity */
    f32 stick_sensitivity_y;        /**< Right stick Y sensitivity */
    f32 mouse_sensitivity_x;        /**< Mouse X sensitivity */
    f32 mouse_sensitivity_y;        /**< Mouse Y sensitivity */
    
    /* Inversion settings */
    bool invert_x;                  /**< Invert horizontal rotation */
    bool invert_y;                  /**< Invert vertical rotation */
    
    /* Pitch limits (to prevent over-rotation) */
    f32 pitch_min;                  /**< Minimum pitch angle (looking down) */
    f32 pitch_max;                  /**< Maximum pitch angle (looking up) */
    
    /* Smoothing */
    bool enable_smoothing;          /**< Enable camera smoothing */
    f32 smoothing_factor;           /**< Smoothing interpolation factor */
    
    /* Auto-center */
    bool enable_auto_center;        /**< Auto-center camera behind player */
    f32 auto_center_delay;          /**< Delay before auto-centering (seconds) */
    f32 auto_center_speed;          /**< Auto-center rotation speed */
    
    /* Mode-specific settings */
    bool allow_in_first_person;     /**< Allow patch in first-person mode */
    bool allow_in_cutscenes;        /**< Allow patch during cutscenes */
    bool allow_in_menus;            /**< Allow patch in pause menu */
    
} CV64_CameraPatchConfig;

/*===========================================================================
 * Camera State Structure
 *===========================================================================*/

typedef struct CV64_CameraState {
    /* Current camera angles (in degrees) */
    f32 yaw;                        /**< Horizontal rotation */
    f32 pitch;                      /**< Vertical rotation */
    f32 roll;                       /**< Roll (usually 0) */
    
    /* Target angles (for smoothing) */
    f32 target_yaw;
    f32 target_pitch;
    
    /* Velocity (for momentum) */
    f32 yaw_velocity;
    f32 pitch_velocity;
    
    /* Distance from player */
    f32 distance;
    f32 target_distance;
    f32 min_distance;
    f32 max_distance;
    
    /* Camera position */
    Vec3f position;
    Vec3f target_position;
    Vec3f look_at;
    
    /* Collision state */
    bool is_colliding;
    f32 collision_distance;
    
    /* Auto-center timer */
    f32 idle_time;
    bool is_auto_centering;
    
    /* Mode flags */
    bool is_first_person;
    bool is_in_cutscene;
    bool is_locked;
    
} CV64_CameraState;

/*===========================================================================
 * Original Camera Mode Constants (from cv64 decomp)
 *===========================================================================*/

#define CV64_CAMERA_MODE_NORMAL         0
#define CV64_CAMERA_MODE_BATTLE         1
#define CV64_CAMERA_MODE_ACTION         2
#define CV64_CAMERA_MODE_BOSS           3
#define CV64_CAMERA_MODE_READING        4
#define CV64_CAMERA_MODE_FIRST_PERSON   5
#define CV64_CAMERA_MODE_CUTSCENE       6
#define CV64_CAMERA_MODE_LOCKED         7

/*===========================================================================
 * Camera Patch API Functions
 *===========================================================================*/

/**
 * @brief Initialize the camera patch system
 * @return TRUE on success
 */
CV64_API bool CV64_CameraPatch_Init(void);

/**
 * @brief Shutdown the camera patch system
 */
CV64_API void CV64_CameraPatch_Shutdown(void);

/**
 * @brief Update camera patch (call each frame)
 * @param delta_time Frame delta time in seconds
 */
CV64_API void CV64_CameraPatch_Update(f32 delta_time);

/**
 * @brief Enable or disable the camera patch
 * @param enabled TRUE to enable
 */
CV64_API void CV64_CameraPatch_SetEnabled(bool enabled);

/**
 * @brief Check if camera patch is enabled
 * @return TRUE if enabled
 */
CV64_API bool CV64_CameraPatch_IsEnabled(void);

/*===========================================================================
 * Camera Control Functions
 *===========================================================================*/

/**
 * @brief Add rotation to camera
 * @param yaw_delta Horizontal rotation delta (degrees)
 * @param pitch_delta Vertical rotation delta (degrees)
 */
CV64_API void CV64_CameraPatch_Rotate(f32 yaw_delta, f32 pitch_delta);

/**
 * @brief Set camera rotation directly
 * @param yaw Horizontal angle (degrees)
 * @param pitch Vertical angle (degrees)
 */
CV64_API void CV64_CameraPatch_SetRotation(f32 yaw, f32 pitch);

/**
 * @brief Get current camera rotation
 * @param out_yaw Output horizontal angle
 * @param out_pitch Output vertical angle
 */
CV64_API void CV64_CameraPatch_GetRotation(f32* out_yaw, f32* out_pitch);

/**
 * @brief Set camera distance from player
 * @param distance Distance in game units
 */
CV64_API void CV64_CameraPatch_SetDistance(f32 distance);

/**
 * @brief Zoom camera in/out
 * @param delta Zoom delta (positive = closer)
 */
CV64_API void CV64_CameraPatch_Zoom(f32 delta);

/**
 * @brief Reset camera to default position behind player
 */
CV64_API void CV64_CameraPatch_Reset(void);

/**
 * @brief Force camera to center behind player
 */
CV64_API void CV64_CameraPatch_CenterBehindPlayer(void);

/*===========================================================================
 * Input Processing Functions
 *===========================================================================*/

/**
 * @brief Process D-PAD input for camera control
 * @param dpad_state D-PAD button state
 * @param delta_time Frame delta time
 */
CV64_API void CV64_CameraPatch_ProcessDPad(u16 dpad_state, f32 delta_time);

/**
 * @brief Process right analog stick for camera control
 * @param stick_x Right stick X (-128 to 127)
 * @param stick_y Right stick Y (-128 to 127)
 * @param delta_time Frame delta time
 */
CV64_API void CV64_CameraPatch_ProcessRightStick(s16 stick_x, s16 stick_y, f32 delta_time);

/**
 * @brief Process mouse movement for camera control
 * @param delta_x Mouse X delta
 * @param delta_y Mouse Y delta
 * @param delta_time Frame delta time
 */
CV64_API void CV64_CameraPatch_ProcessMouse(s32 delta_x, s32 delta_y, f32 delta_time);

/*===========================================================================
 * State Access Functions
 *===========================================================================*/

/**
 * @brief Get current camera state
 * @return Pointer to camera state
 */
CV64_API CV64_CameraState* CV64_CameraPatch_GetState(void);

/**
 * @brief Set camera mode (affects patch behavior)
 * @param mode Camera mode constant
 */
CV64_API void CV64_CameraPatch_SetMode(u32 mode);

/**
 * @brief Get current camera mode
 * @return Current mode constant
 */
CV64_API u32 CV64_CameraPatch_GetMode(void);

/**
 * @brief Lock camera (prevent user control)
 * @param locked TRUE to lock
 */
CV64_API void CV64_CameraPatch_SetLocked(bool locked);

/**
 * @brief Check if camera is locked
 * @return true if camera is locked (user control disabled)
 */
CV64_API bool CV64_CameraPatch_IsLocked(void);

/**
 * @brief Set camera look-at target
 * @param look_at Target position
 */
CV64_API void CV64_CameraPatch_SetLookAt(const Vec3f* look_at);

/**
 * @brief Get camera look-at target
 * @param out_look_at Output target position
 */
CV64_API void CV64_CameraPatch_GetLookAt(Vec3f* out_look_at);

/**
 * @brief Get current camera position
 * @param out_position Output position
 */
CV64_API void CV64_CameraPatch_GetPosition(Vec3f* out_position);

/**
 * @brief Set camera position directly
 * @param position New position
 */
CV64_API void CV64_CameraPatch_SetPosition(const Vec3f* position);

/*===========================================================================
 * Configuration Functions
 *===========================================================================*/

/**
 * @brief Get camera patch configuration
 * @return Pointer to configuration
 */
CV64_API CV64_CameraPatchConfig* CV64_CameraPatch_GetConfig(void);

/**
 * @brief Apply configuration changes
 */
CV64_API void CV64_CameraPatch_ApplyConfig(void);

/**
 * @brief Load camera patch configuration from file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_CameraPatch_LoadConfig(const char* filepath);

/**
 * @brief Save camera patch configuration to file
 * @param filepath Path to config file (NULL for default)
 * @return TRUE on success
 */
CV64_API bool CV64_CameraPatch_SaveConfig(const char* filepath);

/**
 * @brief Reset configuration to defaults
 */
CV64_API void CV64_CameraPatch_ResetConfigToDefaults(void);

/*===========================================================================
 * Collision Detection Functions
 *===========================================================================*/

/**
 * @brief Check camera collision with level geometry
 * @param camera_pos Desired camera position
 * @param look_at Camera target position
 * @param out_adjusted_pos Output adjusted position (avoiding collision)
 * @return TRUE if collision detected
 */
CV64_API bool CV64_CameraPatch_CheckCollision(Vec3f* camera_pos, 
                                               Vec3f* look_at,
                                               Vec3f* out_adjusted_pos);

/*===========================================================================
 * Hook Functions (for integration with original code)
 *===========================================================================*/

/**
 * @brief Hook called when original camera manager updates
 * @param camera_mgr Pointer to original cameraMgr
 * @note This intercepts the original camera update to apply our patch
 */
CV64_API void CV64_CameraPatch_OnCameraMgrUpdate(void* camera_mgr);

/**
 * @brief Hook called when player moves
 * @param player_pos Player position
 * @param player_angle Player facing angle
 */
CV64_API void CV64_CameraPatch_OnPlayerMove(Vec3f* player_pos, s16 player_angle);

/**
 * @brief Hook called on camera mode change
 * @param new_mode New camera mode
 * @param old_mode Previous camera mode
 */
CV64_API void CV64_CameraPatch_OnModeChange(u32 new_mode, u32 old_mode);

#ifdef __cplusplus
}
#endif

#endif /* CV64_CAMERA_PATCH_H */
