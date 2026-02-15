/**
 * @file cv64_static_plugins.h
 * @brief Static plugin registration for fully embedded mupen64plus
 * 
 * This header declares functions to register statically linked plugins
 * directly into the mupen64plus core, bypassing DLL loading.
 */

#ifndef CV64_STATIC_PLUGINS_H
#define CV64_STATIC_PLUGINS_H

#include "cv64_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all statically linked plugins with the core
 * 
 * This must be called after CoreStartup() but before loading a ROM.
 * It directly sets the plugin function pointers in the core.
 * 
 * @return true on success
 */
bool CV64_RegisterStaticPlugins(void);

/**
 * @brief Initialize the static GFX plugin (GLideN64)
 * @return true on success
 */
bool CV64_StaticGFX_Init(void);

/**
 * @brief Shutdown the static GFX plugin
 */
void CV64_StaticGFX_Shutdown(void);

/**
 * @brief Initialize the static RSP plugin (RSP-HLE)
 * @return true on success
 */
bool CV64_StaticRSP_Init(void);

/**
 * @brief Shutdown the static RSP plugin
 */
void CV64_StaticRSP_Shutdown(void);

/**
 * @brief Initialize the static Audio plugin
 * @return true on success
 */
bool CV64_StaticAudio_Init(void);

/**
 * @brief Shutdown the static Audio plugin
 */
void CV64_StaticAudio_Shutdown(void);

/**
 * @brief Initialize the static Input plugin (uses our custom implementation)
 * @return true on success
 */
bool CV64_StaticInput_Init(void);

/**
 * @brief Shutdown the static Input plugin
 */
void CV64_StaticInput_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_STATIC_PLUGINS_H */
