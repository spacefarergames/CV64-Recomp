/**
 * @file m64pcore_gliden64_stubs.c
 * @brief Wrapper functions for GLideN64 static linking
 * 
 * When GLideN64 is statically linked, it cannot use GetProcAddress/dlsym to
 * get function pointers from the core DLL (since there is no DLL). These
 * wrapper functions provide m64pcore_-prefixed aliases that GLideN64 can
 * reference directly, which forward to the actual mupen64plus-core functions.
 */

#define M64P_CORE_PROTOTYPES 1
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_config.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_vidext.h"
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_common.h"

/* Common API wrappers */
EXPORT m64p_error CALL m64pcore_PluginGetVersion(m64p_plugin_type* PluginType, int* PluginVersion, int* APIVersion, const char** PluginNamePtr, int* Capabilities) {
    return PluginGetVersion(PluginType, PluginVersion, APIVersion, PluginNamePtr, Capabilities);
}

/* Config API wrappers - match the actual mupen64plus-core signatures */
EXPORT const char* CALL m64pcore_ConfigGetSharedDataFilepath(const char* filename) {
    return ConfigGetSharedDataFilepath(filename);
}

EXPORT const char* CALL m64pcore_ConfigGetUserConfigPath(void) {
    return ConfigGetUserConfigPath();
}

EXPORT const char* CALL m64pcore_ConfigGetUserDataPath(void) {
    return ConfigGetUserDataPath();
}

EXPORT const char* CALL m64pcore_ConfigGetUserCachePath(void) {
    return ConfigGetUserCachePath();
}

EXPORT m64p_error CALL m64pcore_ConfigOpenSection(const char* SectionName, m64p_handle* ConfigSectionHandle) {
    return ConfigOpenSection(SectionName, ConfigSectionHandle);
}

EXPORT m64p_error CALL m64pcore_ConfigDeleteSection(const char* SectionName) {
    return ConfigDeleteSection(SectionName);
}

EXPORT m64p_error CALL m64pcore_ConfigSaveSection(const char* SectionName) {
    return ConfigSaveSection(SectionName);
}

EXPORT m64p_error CALL m64pcore_ConfigSaveFile(void) {
    return ConfigSaveFile();
}

EXPORT m64p_error CALL m64pcore_ConfigSetDefaultInt(m64p_handle ConfigSectionHandle, const char* ParamName, int ParamValue, const char* ParamHelp) {
    return ConfigSetDefaultInt(ConfigSectionHandle, ParamName, ParamValue, ParamHelp);
}

EXPORT m64p_error CALL m64pcore_ConfigSetDefaultFloat(m64p_handle ConfigSectionHandle, const char* ParamName, float ParamValue, const char* ParamHelp) {
    return ConfigSetDefaultFloat(ConfigSectionHandle, ParamName, ParamValue, ParamHelp);
}

EXPORT m64p_error CALL m64pcore_ConfigSetDefaultBool(m64p_handle ConfigSectionHandle, const char* ParamName, int ParamValue, const char* ParamHelp) {
    return ConfigSetDefaultBool(ConfigSectionHandle, ParamName, ParamValue, ParamHelp);
}

EXPORT m64p_error CALL m64pcore_ConfigSetDefaultString(m64p_handle ConfigSectionHandle, const char* ParamName, const char* ParamValue, const char* ParamHelp) {
    return ConfigSetDefaultString(ConfigSectionHandle, ParamName, ParamValue, ParamHelp);
}

EXPORT int CALL m64pcore_ConfigGetParamInt(m64p_handle ConfigSectionHandle, const char* ParamName) {
    return ConfigGetParamInt(ConfigSectionHandle, ParamName);
}

EXPORT float CALL m64pcore_ConfigGetParamFloat(m64p_handle ConfigSectionHandle, const char* ParamName) {
    return ConfigGetParamFloat(ConfigSectionHandle, ParamName);
}

EXPORT int CALL m64pcore_ConfigGetParamBool(m64p_handle ConfigSectionHandle, const char* ParamName) {
    return ConfigGetParamBool(ConfigSectionHandle, ParamName);
}

EXPORT const char* CALL m64pcore_ConfigGetParamString(m64p_handle ConfigSectionHandle, const char* ParamName) {
    return ConfigGetParamString(ConfigSectionHandle, ParamName);
}

EXPORT m64p_error CALL m64pcore_ConfigExternalGetParameter(m64p_handle Handle, const char* SectionName, const char* ParamName, char* ParamPtr, int ParamMaxLength) {
    return ConfigExternalGetParameter(Handle, SectionName, ParamName, ParamPtr, ParamMaxLength);
}

EXPORT m64p_error CALL m64pcore_ConfigExternalOpen(const char* FileName, m64p_handle* Handle) {
    return ConfigExternalOpen(FileName, Handle);
}

EXPORT m64p_error CALL m64pcore_ConfigExternalClose(m64p_handle Handle) {
    return ConfigExternalClose(Handle);
}

/* Video Extension API wrappers */
EXPORT m64p_error CALL m64pcore_VidExt_Init(void) {
    return VidExt_Init();
}

EXPORT m64p_error CALL m64pcore_VidExt_Quit(void) {
    return VidExt_Quit();
}

EXPORT m64p_error CALL m64pcore_VidExt_ListFullscreenModes(m64p_2d_size* SizeArray, int* NumSizes) {
    return VidExt_ListFullscreenModes(SizeArray, NumSizes);
}

EXPORT m64p_error CALL m64pcore_VidExt_ListFullscreenRates(m64p_2d_size Size, int* NumRates, int* Rates) {
    return VidExt_ListFullscreenRates(Size, NumRates, Rates);
}

EXPORT m64p_error CALL m64pcore_VidExt_SetVideoMode(int Width, int Height, int BitsPerPixel, m64p_video_mode ScreenMode, m64p_video_flags Flags) {
    return VidExt_SetVideoMode(Width, Height, BitsPerPixel, ScreenMode, Flags);
}

EXPORT m64p_error CALL m64pcore_VidExt_SetVideoModeWithRate(int Width, int Height, int RefreshRate, int BitsPerPixel, m64p_video_mode ScreenMode, m64p_video_flags Flags) {
    return VidExt_SetVideoModeWithRate(Width, Height, RefreshRate, BitsPerPixel, ScreenMode, Flags);
}

EXPORT m64p_error CALL m64pcore_VidExt_SetCaption(const char* Title) {
    return VidExt_SetCaption(Title);
}

EXPORT m64p_error CALL m64pcore_VidExt_ToggleFullScreen(void) {
    return VidExt_ToggleFullScreen();
}

EXPORT m64p_error CALL m64pcore_VidExt_ResizeWindow(int Width, int Height) {
    return VidExt_ResizeWindow(Width, Height);
}

EXPORT m64p_function CALL m64pcore_VidExt_GL_GetProcAddress(const char* Proc) {
    return VidExt_GL_GetProcAddress(Proc);
}

EXPORT m64p_error CALL m64pcore_VidExt_GL_SetAttribute(m64p_GLattr Attr, int Value) {
    return VidExt_GL_SetAttribute(Attr, Value);
}

EXPORT m64p_error CALL m64pcore_VidExt_GL_GetAttribute(m64p_GLattr Attr, int* pValue) {
    return VidExt_GL_GetAttribute(Attr, pValue);
}

EXPORT m64p_error CALL m64pcore_VidExt_GL_SwapBuffers(void) {
    return VidExt_GL_SwapBuffers();
}

EXPORT unsigned int CALL m64pcore_VidExt_GL_GetDefaultFramebuffer(void) {
    return VidExt_GL_GetDefaultFramebuffer();
}
