/**
 * @file cv64_embedded_rom.cpp
 * @brief Embedded ROM loader implementation
 * 
 * Loads the ROM from Windows PE resource section.
 */

#include "../include/cv64_embedded_rom.h"
#include <Windows.h>
#include <cstdio>

/* mupen64plus API */
extern "C" {
#include "../RMG/Source/3rdParty/mupen64plus-core/src/api/m64p_types.h"

/* Core ROM functions - declared in m64p_frontend.h but we call directly */
typedef m64p_error (*ptr_CoreDoCommand)(m64p_command, int, void*);
extern m64p_error CoreDoCommand(m64p_command Command, int ParamInt, void* ParamPtr);
}

/* Resource ID - must match cv64_embedded_rom.rc */
#define IDR_BASEROM 101

static const uint8_t* s_embeddedRomData = nullptr;
static size_t s_embeddedRomSize = 0;
static bool s_romLoaded = false;

/**
 * Initialize embedded ROM access by finding the resource
 */
static bool InitEmbeddedRom()
{
    if (s_romLoaded) {
        return s_embeddedRomData != nullptr;
    }
    
    s_romLoaded = true;
    
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) {
        OutputDebugStringA("[CV64_ROM] Failed to get module handle\n");
        return false;
    }
    
    /* Find the ROM resource */
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_BASEROM), RT_RCDATA);
    if (!hResource) {
        OutputDebugStringA("[CV64_ROM] ROM resource not found\n");
        return false;
    }
    
    /* Get resource size */
    s_embeddedRomSize = SizeofResource(hModule, hResource);
    if (s_embeddedRomSize == 0) {
        OutputDebugStringA("[CV64_ROM] ROM resource has zero size\n");
        return false;
    }
    
    /* Load the resource */
    HGLOBAL hGlobal = LoadResource(hModule, hResource);
    if (!hGlobal) {
        OutputDebugStringA("[CV64_ROM] Failed to load ROM resource\n");
        return false;
    }
    
    /* Lock and get pointer to data */
    s_embeddedRomData = static_cast<const uint8_t*>(LockResource(hGlobal));
    if (!s_embeddedRomData) {
        OutputDebugStringA("[CV64_ROM] Failed to lock ROM resource\n");
        return false;
    }
    
    char msg[128];
    sprintf_s(msg, "[CV64_ROM] Embedded ROM loaded: %zu bytes (%.2f MB)\n", 
              s_embeddedRomSize, s_embeddedRomSize / (1024.0 * 1024.0));
    OutputDebugStringA(msg);
    
    return true;
}

extern "C" {

const uint8_t* CV64_GetEmbeddedRom(size_t* size)
{
    if (!InitEmbeddedRom()) {
        if (size) *size = 0;
        return nullptr;
    }
    
    if (size) *size = s_embeddedRomSize;
    return s_embeddedRomData;
}

int CV64_HasEmbeddedRom(void)
{
    return InitEmbeddedRom() ? 1 : 0;
}

int CV64_LoadEmbeddedRom(void)
{
    if (!InitEmbeddedRom()) {
        OutputDebugStringA("[CV64_ROM] No embedded ROM available\n");
        return (int)M64ERR_FILES;
    }
    
    OutputDebugStringA("[CV64_ROM] Loading embedded ROM into core...\n");
    
    /* Use CoreDoCommand with M64CMD_ROM_OPEN to load ROM from memory */
    /* ParamInt = ROM size, ParamPtr = ROM data */
    m64p_error result = CoreDoCommand(M64CMD_ROM_OPEN, (int)s_embeddedRomSize, (void*)s_embeddedRomData);
    
    if (result != M64ERR_SUCCESS) {
        char msg[128];
        sprintf_s(msg, "[CV64_ROM] CoreDoCommand(ROM_OPEN) failed: %d\n", result);
        OutputDebugStringA(msg);
        return result;
    }
    
    OutputDebugStringA("[CV64_ROM] Embedded ROM loaded successfully!\n");
    return M64ERR_SUCCESS;
}

} /* extern "C" */
