/**
 * @file cv64_config_bridge.h
 * @brief CV64 INI to mupen64plus/GLideN64 Configuration Bridge Header
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_CONFIG_BRIDGE_H
#define CV64_CONFIG_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Apply all CV64 INI configurations to mupen64plus and GLideN64
 * 
 * This function loads cv64_graphics.ini, cv64_controls.ini, cv64_audio.ini,
 * and cv64_patches.ini from the patches/ directory and applies their settings
 * to the mupen64plus core and GLideN64 plugin configuration systems.
 * 
 * Should be called after CoreStartup() but before loading plugins.
 * 
 * @return true if configuration was applied successfully
 */
bool CV64_Config_ApplyAll(void);

/**
 * @brief Get the absolute path to HD texture packs directory
 * @return Absolute path string (valid until next call)
 */
const char* CV64_Config_GetHDTexturePath(void);

/**
 * @brief Get the absolute path to texture cache directory
 * @return Absolute path string (valid until next call)
 */
const char* CV64_Config_GetCachePath(void);

/**
 * @brief Check if HD textures are enabled in configuration
 * @return true if HD textures should be loaded
 */
bool CV64_Config_IsHDTexturesEnabled(void);

/**
 * @brief Check if a specific patch is enabled
 * @param section INI section name (e.g., "CameraPatches", "ControlPatches")
 * @param key INI key name (e.g., "DPAD_CAMERA", "MODERN_CONTROLS")
 * @return true if the patch is enabled
 */
bool CV64_Config_IsPatchEnabled(const char* section, const char* key);

#ifdef __cplusplus
}
#endif

#endif /* CV64_CONFIG_BRIDGE_H */
