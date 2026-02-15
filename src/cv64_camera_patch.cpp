/**
 * @file cv64_camera_patch.cpp
 * @brief Castlevania 64 PC Recomp - D-PAD Camera Control Implementation
 * 
 * THE MOST IMPORTANT PATCH - Full camera control implementation!
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_camera_patch.h"
#include "../include/cv64_controller.h"
#include "../include/cv64_ini_parser.h"
#include <math.h>
#include <string.h>
#include <Windows.h>
#include <filesystem>

/*===========================================================================
 * Constants
 *===========================================================================*/

#define PI 3.14159265358979323846f
#define DEG_TO_RAD(x) ((x) * PI / 180.0f)
#define RAD_TO_DEG(x) ((x) * 180.0f / PI)

/* Default configuration values */
#define DEFAULT_DPAD_SPEED          2.0f
#define DEFAULT_STICK_SENSITIVITY   1.5f
#define DEFAULT_MOUSE_SENSITIVITY   0.15f
#define DEFAULT_PITCH_MIN          -60.0f
#define DEFAULT_PITCH_MAX           80.0f
#define DEFAULT_SMOOTHING_FACTOR    0.15f
#define DEFAULT_AUTO_CENTER_DELAY   3.0f
#define DEFAULT_AUTO_CENTER_SPEED   2.0f
#define DEFAULT_CAMERA_DISTANCE     300.0f
#define DEFAULT_MIN_DISTANCE        100.0f
#define DEFAULT_MAX_DISTANCE        600.0f

/*===========================================================================
 * Static Variables
 *===========================================================================*/

static CV64_CameraPatchConfig s_config;
static CV64_CameraState s_state;
static bool s_initialized = false;

/*===========================================================================
 * Helper Functions
 *===========================================================================*/

/**
 * @brief Clamp a value between min and max
 */
static inline f32 clamp_f32(f32 value, f32 min, f32 max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief Linear interpolation
 */
static inline f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

/**
 * @brief Normalize angle to -180 to 180 range
 */
static f32 normalize_angle(f32 angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

/**
 * @brief Calculate 3D vector length
 */
static f32 vec3f_length(Vec3f* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

/**
 * @brief Normalize 3D vector
 */
static void vec3f_normalize(Vec3f* v) {
    f32 len = vec3f_length(v);
    if (len > 0.0001f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}
static bool s_has_look_at = false;   /* becomes true once player position is known */
static bool s_is_loading = false;   /* true during map transitions */
static f32  s_loading_timer = 0.0f;  /* internal cooldown after loading */
/*===========================================================================
 * Configuration Functions
 *===========================================================================*/

static void reset_config_defaults(void) {
    memset(&s_config, 0, sizeof(s_config));
    
    /* Enable flags */
    s_config.enabled = true;
    s_config.enable_dpad = true;
    s_config.enable_right_stick = true;
    s_config.enable_mouse = true;
    
    /* Sensitivity */
    s_config.dpad_rotation_speed = DEFAULT_DPAD_SPEED;
    s_config.stick_sensitivity_x = DEFAULT_STICK_SENSITIVITY;
    s_config.stick_sensitivity_y = DEFAULT_STICK_SENSITIVITY;
    s_config.mouse_sensitivity_x = DEFAULT_MOUSE_SENSITIVITY;
    s_config.mouse_sensitivity_y = DEFAULT_MOUSE_SENSITIVITY;
    
    /* Inversion */
    s_config.invert_x = false;
    s_config.invert_y = false;
    
    /* Pitch limits */
    s_config.pitch_min = DEFAULT_PITCH_MIN;
    s_config.pitch_max = DEFAULT_PITCH_MAX;
    
    /* Smoothing */
    s_config.enable_smoothing = true;
    s_config.smoothing_factor = DEFAULT_SMOOTHING_FACTOR;
    
    /* Auto-center */
    s_config.enable_auto_center = false;
    s_config.auto_center_delay = DEFAULT_AUTO_CENTER_DELAY;
    s_config.auto_center_speed = DEFAULT_AUTO_CENTER_SPEED;
    
    /* Mode settings */
    s_config.allow_in_first_person = false;
    s_config.allow_in_cutscenes = false;
    s_config.allow_in_menus = false;
}

static void reset_state(void) {
    memset(&s_state, 0, sizeof(s_state));

    s_state.yaw = 0.0f;
    s_state.pitch = 15.0f;
    s_state.roll = 0.0f;

    s_state.target_yaw = s_state.yaw;
    s_state.target_pitch = s_state.pitch;

    s_state.distance = DEFAULT_CAMERA_DISTANCE;
    s_state.target_distance = DEFAULT_CAMERA_DISTANCE;
    s_state.min_distance = DEFAULT_MIN_DISTANCE;
    s_state.max_distance = DEFAULT_MAX_DISTANCE;

    s_state.idle_time = 0.0f;
    s_state.is_auto_centering = false;
    s_state.is_locked = false;

    /* NEW: world readiness */
    s_has_look_at = false;
    s_is_loading = true;       /* treat startup like a loading transition */
    s_loading_timer = 0.0f;
}
/* Forward declaration for internal loading handler */
static void CV64_CameraPatch_InternalLoadingTick(f32 dt);

/*===========================================================================
 * API Implementation
 *===========================================================================*/

bool CV64_CameraPatch_Init(void) {
    if (s_initialized) {
        return true;
    }
    
    reset_config_defaults();
    reset_state();
    
    /* Try to load saved configuration */
    CV64_CameraPatch_LoadConfig(NULL);
    
    s_initialized = true;
    return true;
}

void CV64_CameraPatch_Shutdown(void) {
    if (!s_initialized) {
        return;
    }
    
    /* Save configuration on shutdown */
    CV64_CameraPatch_SaveConfig(NULL);
    
    s_initialized = false;
}

void CV64_CameraPatch_Update(f32 delta_time) {
    if (!s_initialized || !s_config.enabled) {
        return;
    }

    /* NEW: handle internal loading cooldown */
    CV64_CameraPatch_InternalLoadingTick(delta_time);

    /* NEW: do not run camera logic until world is stable */
    if (s_is_loading || !s_has_look_at) {
        return;
    }

    if (s_state.is_locked) {
        return;
    }

    if (s_state.is_in_cutscene && !s_config.allow_in_cutscenes) {
        return;
    }

    if (s_state.is_first_person && !s_config.allow_in_first_person) {
        return;
    }

    /* Apply smoothing to camera angles */
    if (s_config.enable_smoothing) {
        f32 smooth = s_config.smoothing_factor;
        s_state.yaw = lerp(s_state.yaw, s_state.target_yaw, smooth);
        s_state.pitch = lerp(s_state.pitch, s_state.target_pitch, smooth);
        s_state.distance = lerp(s_state.distance, s_state.target_distance, smooth);
    } else {
        s_state.yaw = s_state.target_yaw;
        s_state.pitch = s_state.target_pitch;
        s_state.distance = s_state.target_distance;
    }
    
    /* Normalize yaw angle */
    s_state.yaw = normalize_angle(s_state.yaw);
    s_state.target_yaw = normalize_angle(s_state.target_yaw);
    
    /* Update idle timer for auto-center */
    if (s_config.enable_auto_center) {
        s_state.idle_time += delta_time;
        
        if (s_state.idle_time > s_config.auto_center_delay && !s_state.is_auto_centering) {
            s_state.is_auto_centering = true;
        }
        
        if (s_state.is_auto_centering) {
            /* Gradually rotate camera behind player */
            /* Target yaw would be calculated from player facing direction */
            f32 center_speed = s_config.auto_center_speed * delta_time;
            /* Implementation would interface with game's player angle */
        }
    }
    
    /* Calculate camera position from angles and distance */
    f32 yaw_rad = DEG_TO_RAD(s_state.yaw);
    f32 pitch_rad = DEG_TO_RAD(s_state.pitch);
    
    /* Spherical to Cartesian conversion */
    f32 horizontal_dist = s_state.distance * cosf(pitch_rad);
    s_state.position.x = s_state.look_at.x - horizontal_dist * sinf(yaw_rad);
    s_state.position.y = s_state.look_at.y + s_state.distance * sinf(pitch_rad);
    s_state.position.z = s_state.look_at.z - horizontal_dist * cosf(yaw_rad);
    
    /* Apply velocity damping */
    s_state.yaw_velocity *= 0.9f;
    s_state.pitch_velocity *= 0.9f;
}

void CV64_CameraPatch_SetEnabled(bool enabled) {
    s_config.enabled = enabled;
}

bool CV64_CameraPatch_IsEnabled(void) {
    return s_config.enabled;
}

void CV64_CameraPatch_Rotate(f32 yaw_delta, f32 pitch_delta) {
    if (!s_config.enabled) {
        return;
    }
    
    /* Apply inversion */
    if (s_config.invert_x) yaw_delta = -yaw_delta;
    if (s_config.invert_y) pitch_delta = -pitch_delta;
    
    /* Update target angles */
    s_state.target_yaw += yaw_delta;
    s_state.target_pitch += pitch_delta;
    
    /* Clamp pitch to limits */
    s_state.target_pitch = clamp_f32(s_state.target_pitch, 
                                      s_config.pitch_min, 
                                      s_config.pitch_max);
    
    /* Reset idle timer (user is controlling camera) */
    s_state.idle_time = 0.0f;
    s_state.is_auto_centering = false;
    
    /* Update velocity for momentum */
    s_state.yaw_velocity = yaw_delta;
    s_state.pitch_velocity = pitch_delta;
}

void CV64_CameraPatch_SetRotation(f32 yaw, f32 pitch) {
    s_state.target_yaw = yaw;
    s_state.target_pitch = clamp_f32(pitch, s_config.pitch_min, s_config.pitch_max);
    
    if (!s_config.enable_smoothing) {
        s_state.yaw = s_state.target_yaw;
        s_state.pitch = s_state.target_pitch;
    }
}

void CV64_CameraPatch_GetRotation(f32* out_yaw, f32* out_pitch) {
    if (out_yaw) *out_yaw = s_state.yaw;
    if (out_pitch) *out_pitch = s_state.pitch;
}

void CV64_CameraPatch_SetDistance(f32 distance) {
    s_state.target_distance = clamp_f32(distance, s_state.min_distance, s_state.max_distance);
}

void CV64_CameraPatch_Zoom(f32 delta) {
    s_state.target_distance -= delta;
    s_state.target_distance = clamp_f32(s_state.target_distance, 
                                         s_state.min_distance, 
                                         s_state.max_distance);
}

void CV64_CameraPatch_Reset(void) {
    reset_state();
}

void CV64_CameraPatch_CenterBehindPlayer(void) {
    /* This would interface with the game's player angle */
    /* For now, just reset to default */
    s_state.target_yaw = 0.0f;
    s_state.target_pitch = 15.0f;
    s_state.idle_time = 0.0f;
    s_state.is_auto_centering = false;
}

/*===========================================================================
 * Input Processing
 *===========================================================================*/

void CV64_CameraPatch_ProcessDPad(u16 dpad_state, f32 delta_time) {
    if (!s_config.enabled || !s_config.enable_dpad) {
        return;
    }
    
    f32 speed = s_config.dpad_rotation_speed * delta_time * 60.0f; /* 60 FPS baseline */
    f32 yaw_delta = 0.0f;
    f32 pitch_delta = 0.0f;
    
    /* Process D-PAD buttons */
    if (dpad_state & CONT_DPAD_LEFT) {
        yaw_delta -= speed;
    }
    if (dpad_state & CONT_DPAD_RIGHT) {
        yaw_delta += speed;
    }
    if (dpad_state & CONT_DPAD_UP) {
        pitch_delta += speed;
    }
    if (dpad_state & CONT_DPAD_DOWN) {
        pitch_delta -= speed;
    }
    
    if (yaw_delta != 0.0f || pitch_delta != 0.0f) {
        CV64_CameraPatch_Rotate(yaw_delta, pitch_delta);
    }
}

void CV64_CameraPatch_ProcessRightStick(s16 stick_x, s16 stick_y, f32 delta_time) {
    if (!s_config.enabled || !s_config.enable_right_stick) {
        return;
    }
    
    /* Apply deadzone */
    const s16 deadzone = 15;
    if (abs(stick_x) < deadzone) stick_x = 0;
    if (abs(stick_y) < deadzone) stick_y = 0;
    
    if (stick_x == 0 && stick_y == 0) {
        return;
    }
    
    /* Normalize stick input (-1.0 to 1.0) */
    f32 norm_x = (f32)stick_x / 128.0f;
    f32 norm_y = (f32)stick_y / 128.0f;
    
    /* Apply sensitivity */
    f32 yaw_delta = norm_x * s_config.stick_sensitivity_x * delta_time * 100.0f;
    f32 pitch_delta = norm_y * s_config.stick_sensitivity_y * delta_time * 100.0f;
    
    CV64_CameraPatch_Rotate(yaw_delta, pitch_delta);
}

void CV64_CameraPatch_ProcessMouse(s32 delta_x, s32 delta_y, f32 delta_time) {
    if (!s_config.enabled || !s_config.enable_mouse) {
        return;
    }
    
    if (delta_x == 0 && delta_y == 0) {
        return;
    }
    
    /* Apply sensitivity */
    f32 yaw_delta = (f32)delta_x * s_config.mouse_sensitivity_x;
    f32 pitch_delta = (f32)delta_y * s_config.mouse_sensitivity_y;
    
    CV64_CameraPatch_Rotate(yaw_delta, -pitch_delta);  /* Invert Y for natural feel */
}

/*===========================================================================
 * State Access
 *===========================================================================*/

CV64_CameraState* CV64_CameraPatch_GetState(void) {
    return &s_state;
}

void CV64_CameraPatch_SetMode(u32 mode) {
    switch (mode) {
        case CV64_CAMERA_MODE_FIRST_PERSON:
            s_state.is_first_person = true;
            s_state.is_in_cutscene = false;
            break;
        case CV64_CAMERA_MODE_CUTSCENE:
            s_state.is_first_person = false;
            s_state.is_in_cutscene = true;
            break;
        default:
            s_state.is_first_person = false;
            s_state.is_in_cutscene = false;
            break;
    }
}

u32 CV64_CameraPatch_GetMode(void) {
    if (s_state.is_first_person) return CV64_CAMERA_MODE_FIRST_PERSON;
    if (s_state.is_in_cutscene) return CV64_CAMERA_MODE_CUTSCENE;
    return CV64_CAMERA_MODE_NORMAL;
}

void CV64_CameraPatch_SetLocked(bool locked) {
    s_state.is_locked = locked;
}

bool CV64_CameraPatch_IsLocked(void) {
    return s_state.is_locked;
}

void CV64_CameraPatch_SetLookAt(const Vec3f* look_at) {
    if (look_at) {
        s_state.look_at = *look_at;
    }
}

void CV64_CameraPatch_GetLookAt(Vec3f* out_look_at) {
    if (out_look_at) {
        *out_look_at = s_state.look_at;
    }
}

void CV64_CameraPatch_GetPosition(Vec3f* out_position) {
    if (out_position) {
        *out_position = s_state.position;
    }
}

void CV64_CameraPatch_SetPosition(const Vec3f* position) {
    if (position) {
        s_state.position = *position;
        s_state.target_position = *position;
    }
}

/*===========================================================================
 * Configuration Access
 *===========================================================================*/

CV64_CameraPatchConfig* CV64_CameraPatch_GetConfig(void) {
    return &s_config;
}

void CV64_CameraPatch_ApplyConfig(void) {
    /* Clamp values to valid ranges */
    s_config.dpad_rotation_speed = clamp_f32(s_config.dpad_rotation_speed, 0.1f, 10.0f);
    s_config.stick_sensitivity_x = clamp_f32(s_config.stick_sensitivity_x, 0.1f, 5.0f);
    s_config.stick_sensitivity_y = clamp_f32(s_config.stick_sensitivity_y, 0.1f, 5.0f);
    s_config.mouse_sensitivity_x = clamp_f32(s_config.mouse_sensitivity_x, 0.01f, 1.0f);
    s_config.mouse_sensitivity_y = clamp_f32(s_config.mouse_sensitivity_y, 0.01f, 1.0f);
    s_config.pitch_min = clamp_f32(s_config.pitch_min, -89.0f, 0.0f);
    s_config.pitch_max = clamp_f32(s_config.pitch_max, 0.0f, 89.0f);
    s_config.smoothing_factor = clamp_f32(s_config.smoothing_factor, 0.01f, 1.0f);
}

bool CV64_CameraPatch_LoadConfig(const char* filepath) {
    CV64_IniParser ini;
    
    /* Determine config path */
    std::string configPath;
    if (filepath) {
        configPath = filepath;
    } else {
        /* Default: patches/cv64_camera.ini relative to EXE */
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path basePath = std::filesystem::path(exePath).parent_path();
        
        /* Try different locations */
        std::vector<std::filesystem::path> searchPaths = {
            basePath / "patches" / "cv64_camera.ini",
            basePath / "cv64_camera.ini",
            basePath.parent_path().parent_path() / "patches" / "cv64_camera.ini"
        };
        
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                configPath = path.string();
                break;
            }
        }
    }
    
    if (configPath.empty() || !ini.Load(configPath.c_str())) {
        OutputDebugStringA("[CV64_Camera] Failed to load camera config, using defaults\n");
        return false;
    }
    
    OutputDebugStringA("[CV64_Camera] Loading camera settings from: ");
    OutputDebugStringA(configPath.c_str());
    OutputDebugStringA("\n");
    
    /* [Camera] section */
    s_config.enabled = ini.GetBool("Camera", "enabled", true);
    s_config.enable_dpad = ini.GetBool("Camera", "enable_dpad", true);
    s_config.enable_right_stick = ini.GetBool("Camera", "enable_right_stick", true);
    s_config.enable_mouse = ini.GetBool("Camera", "enable_mouse", true);
    
    /* [Sensitivity] section */
    s_config.dpad_rotation_speed = ini.GetFloat("Sensitivity", "dpad_rotation_speed", DEFAULT_DPAD_SPEED);
    s_config.stick_sensitivity_x = ini.GetFloat("Sensitivity", "stick_sensitivity_x", DEFAULT_STICK_SENSITIVITY);
    s_config.stick_sensitivity_y = ini.GetFloat("Sensitivity", "stick_sensitivity_y", DEFAULT_STICK_SENSITIVITY);
    s_config.mouse_sensitivity_x = ini.GetFloat("Sensitivity", "mouse_sensitivity_x", DEFAULT_MOUSE_SENSITIVITY);
    s_config.mouse_sensitivity_y = ini.GetFloat("Sensitivity", "mouse_sensitivity_y", DEFAULT_MOUSE_SENSITIVITY);
    
    /* [Limits] section */
    s_config.pitch_min = ini.GetFloat("Limits", "pitch_min", DEFAULT_PITCH_MIN);
    s_config.pitch_max = ini.GetFloat("Limits", "pitch_max", DEFAULT_PITCH_MAX);
    
    /* [Behavior] section */
    s_config.enable_smoothing = ini.GetBool("Behavior", "enable_smoothing", true);
    s_config.smoothing_factor = ini.GetFloat("Behavior", "smoothing_factor", DEFAULT_SMOOTHING_FACTOR);
    s_config.enable_auto_center = ini.GetBool("Behavior", "enable_auto_center", false);
    s_config.auto_center_delay = ini.GetFloat("Behavior", "auto_center_delay", DEFAULT_AUTO_CENTER_DELAY);
    s_config.auto_center_speed = ini.GetFloat("Behavior", "auto_center_speed", DEFAULT_AUTO_CENTER_SPEED);
    
    /* [Inversion] section */
    s_config.invert_x = ini.GetBool("Inversion", "invert_x", false);
    s_config.invert_y = ini.GetBool("Inversion", "invert_y", false);
    
    /* [Distance] section */
    s_state.distance = ini.GetFloat("Distance", "default_distance", DEFAULT_CAMERA_DISTANCE);
    s_state.target_distance = s_state.distance;
    s_state.min_distance = ini.GetFloat("Distance", "min_distance", DEFAULT_MIN_DISTANCE);
    s_state.max_distance = ini.GetFloat("Distance", "max_distance", DEFAULT_MAX_DISTANCE);
    
    /* [Modes] section */
    s_config.allow_in_first_person = ini.GetBool("Modes", "allow_in_first_person", false);
    s_config.allow_in_cutscenes = ini.GetBool("Modes", "allow_in_cutscenes", false);
    s_config.allow_in_menus = ini.GetBool("Modes", "allow_in_menus", false);
    
    /* Apply and validate the config */
    CV64_CameraPatch_ApplyConfig();
    
    OutputDebugStringA("[CV64_Camera] Camera config loaded successfully!\n");
    return true;
}

bool CV64_CameraPatch_SaveConfig(const char* filepath) {
    CV64_IniParser ini;
    
    /* Determine config path */
    std::string configPath;
    if (filepath) {
        configPath = filepath;
    } else {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path basePath = std::filesystem::path(exePath).parent_path();
        configPath = (basePath / "patches" / "cv64_camera.ini").string();
    }
    
    /* [Camera] section */
    ini.SetBool("Camera", "enabled", s_config.enabled);
    ini.SetBool("Camera", "enable_dpad", s_config.enable_dpad);
    ini.SetBool("Camera", "enable_right_stick", s_config.enable_right_stick);
    ini.SetBool("Camera", "enable_mouse", s_config.enable_mouse);
    
    /* [Sensitivity] section */
    ini.SetFloat("Sensitivity", "dpad_rotation_speed", s_config.dpad_rotation_speed);
    ini.SetFloat("Sensitivity", "stick_sensitivity_x", s_config.stick_sensitivity_x);
    ini.SetFloat("Sensitivity", "stick_sensitivity_y", s_config.stick_sensitivity_y);
    ini.SetFloat("Sensitivity", "mouse_sensitivity_x", s_config.mouse_sensitivity_x);
    ini.SetFloat("Sensitivity", "mouse_sensitivity_y", s_config.mouse_sensitivity_y);
    
    /* [Limits] section */
    ini.SetFloat("Limits", "pitch_min", s_config.pitch_min);
    ini.SetFloat("Limits", "pitch_max", s_config.pitch_max);
    
    /* [Behavior] section */
    ini.SetBool("Behavior", "enable_smoothing", s_config.enable_smoothing);
    ini.SetFloat("Behavior", "smoothing_factor", s_config.smoothing_factor);
    ini.SetBool("Behavior", "enable_auto_center", s_config.enable_auto_center);
    ini.SetFloat("Behavior", "auto_center_delay", s_config.auto_center_delay);
    ini.SetFloat("Behavior", "auto_center_speed", s_config.auto_center_speed);
    
    /* [Inversion] section */
    ini.SetBool("Inversion", "invert_x", s_config.invert_x);
    ini.SetBool("Inversion", "invert_y", s_config.invert_y);
    
    return ini.Save(configPath.c_str());
}

void CV64_CameraPatch_ResetConfigToDefaults(void) {
    reset_config_defaults();
}

/*===========================================================================
 * Collision Detection
 *===========================================================================*/

bool CV64_CameraPatch_CheckCollision(Vec3f* camera_pos, Vec3f* look_at, Vec3f* out_adjusted_pos) {
    /* TODO: Implement ray casting against level geometry */
    /* For now, just copy position without adjustment */
    if (out_adjusted_pos) {
        *out_adjusted_pos = *camera_pos;
    }
    return false;
}

/*===========================================================================
 * Hook Functions
 *===========================================================================*/

void CV64_CameraPatch_OnCameraMgrUpdate(void* camera_mgr) {
    if (!s_config.enabled) {
        return;
    }
    
    /* This function intercepts the original camera manager update
     * and applies our custom camera angles/position */
    
    /* The implementation would:
     * 1. Read the player position from camera_mgr
     * 2. Apply our calculated camera position and angles
     * 3. Update the camera manager's internal state */
}

void CV64_CameraPatch_OnPlayerMove(Vec3f* player_pos, s16 player_angle) {
    s_state.look_at = *player_pos;
    s_state.look_at.y += 50.0f;

    /* NEW: world is now valid */
    s_has_look_at = true;

    /* If we were loading, this allows the cooldown to finish */
}


void CV64_CameraPatch_OnModeChange(u32 new_mode, u32 old_mode) {
    CV64_CameraPatch_SetMode(new_mode);
    
    /* Reset camera when entering certain modes */
    if (new_mode == CV64_CAMERA_MODE_CUTSCENE || 
        new_mode == CV64_CAMERA_MODE_FIRST_PERSON) {
        /* Allow original camera behavior */
        s_state.is_locked = true;
    } else {
        s_state.is_locked = false;
    }
    /* NEW: treat mode changes as potential transitions */
    s_is_loading = true;
    s_loading_timer = 0.0f;

}
/*===========================================================================
 * Internal Loading Handler (Self-Contained)
 *===========================================================================*/

static void CV64_CameraPatch_InternalLoadingTick(f32 dt)
{
    if (!s_is_loading)
        return;

    /* Wait ~0.5 seconds before allowing camera logic */
    s_loading_timer += dt;

    if (s_loading_timer >= 0.5f && s_has_look_at) {
        s_is_loading = false;
        s_state.is_locked = false;
    }
}