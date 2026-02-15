/**
 * @file cv64_rsp_hle_static.h
 * @brief RSP-HLE Static Plugin Interface
 * 
 * Declares the RSP-HLE plugin functions for static linking.
 * The RSP-HLE plugin is compiled as a static library and these
 * functions are linked directly into the executable.
 */

#ifndef CV64_RSP_HLE_STATIC_H
#define CV64_RSP_HLE_STATIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_plugin.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"

/*===========================================================================
 * RSP-HLE Plugin Exports (from plugin.c)
 * 
 * These are the actual RSP-HLE functions we link against.
 * When building RSP-HLE as a static library with RSPHLE_STATIC_EXPORTS,
 * the functions are renamed via preprocessor to avoid symbol conflicts.
 *===========================================================================*/

/* Plugin lifecycle */
m64p_error rsphle_PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                 void (*DebugCallback)(void *, int, const char *));
m64p_error rsphle_PluginShutdown(void);
m64p_error rsphle_PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                    int *APIVersion, const char **PluginNamePtr, int *Capabilities);

/* RSP operations */
unsigned int rsphle_DoRspCycles(unsigned int Cycles);
void rsphle_InitiateRSP(RSP_INFO Rsp_Info, unsigned int *CycleCount);
void rsphle_RomClosed(void);

#ifdef __cplusplus
}
#endif

#endif /* CV64_RSP_HLE_STATIC_H */
