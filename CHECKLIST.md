# Static Plugin Initialization Checklist

## Critical Fix Applied: RSP Plugin PluginStartup Call

### ? Root Cause Identified
- [x] RSP plugin's `ConfigGetParamString` was NULL
- [x] Caused by missing `PluginStartup` call
- [x] Similar to how GLideN64 was already properly initialized

### ? Core Infrastructure Added

#### frontend.c
- [x] `CoreGetStaticHandle()` - Returns module handle for plugin function resolution
- [x] `GetDebugCallback()` - Returns debug callback function pointer
- [x] `GetDebugCallContext()` - Returns debug callback context
- [x] `l_DebugCallbackStored` - Stores callback in CoreStartup
- [x] `l_DebugCallContextStored` - Stores context in CoreStartup
- [x] Windows.h included for GetModuleHandleExA

### ? RSP Plugin Fixed

#### cv64_static_plugins.cpp - CV64_StaticRSP_Init()
- [x] External declarations for rsphle_PluginStartup/Shutdown
- [x] External declarations for CoreGetStaticHandle
- [x] External declarations for GetDebugCallback/Context
- [x] Call CoreGetStaticHandle() with NULL check
- [x] Call rsphle_PluginStartup(handle, context, callback)
- [x] Error handling with detailed logging
- [x] Register RSP plugin functions after successful startup

#### cv64_static_plugins.cpp - CV64_StaticRSP_Shutdown()
- [x] Call rsphle_PluginShutdown() before unregistering
- [x] Unregister plugin with CoreRegisterRspPlugin(NULL)

### ? GLideN64 Enhanced

#### cv64_static_plugins.cpp - CV64_StaticGFX_Init()
- [x] Changed from passing NULL to using CoreGetStaticHandle()
- [x] Added NULL check for handle
- [x] Kept GlideN64DebugCallback for specific filtering
- [x] Added GetDebugCallContext() for context

### ? Input Plugin Enhanced

#### cv64_static_plugins.cpp - CV64_StaticInput_Init()
- [x] Added inputPluginStartup call
- [x] Proper core handle and callback setup
- [x] Error handling
- [x] Only active with CV64_USE_CUSTOM_INPUT

#### cv64_static_plugins.cpp - CV64_StaticInput_Shutdown()
- [x] Added inputPluginShutdown call
- [x] Proper cleanup before unregistering

### ? Plugin Registration Order

#### cv64_static_plugins.cpp - CV64_RegisterStaticPlugins()
- [x] GFX first (display/RDP list processing)
- [x] Audio second (audio list processing)
- [x] Input third (controller input)
- [x] RSP last (may depend on GFX/Audio callbacks)
- [x] Documented why order matters

### ? Error Handling & Robustness

- [x] NULL checks for CoreGetStaticHandle() in all plugins
- [x] Error codes checked for all PluginStartup calls
- [x] Detailed error logging with error codes
- [x] Graceful failure with informative messages
- [x] Warning vs. Error distinction (Audio/Input can fail, GFX/RSP cannot)

## What This Fixes

### Primary Issue
- **NULL Pointer Crash**: rsphle_InitiateRSP calling NULL ConfigGetParamString
  - **Location**: 0x0000000000000000
  - **Cause**: RSP plugin's PluginStartup never called
  - **Fix**: Call rsphle_PluginStartup before using plugin

### Secondary Improvements
- **GLideN64 Robustness**: Now uses proper core handle instead of NULL
- **Input Plugin Support**: Added PluginStartup/Shutdown pattern
- **Debug Support**: All plugins get proper debug callbacks
- **Error Visibility**: Better logging for initialization failures

## Expected Behavior After Fix

### During Initialization
```
[CV64_STATIC_PLUGIN] === Registering all static plugins ===
[CV64_STATIC_PLUGIN] Registering static GFX plugin (GLideN64 (Static))...
[CV64_STATIC_PLUGIN] Calling gliden64_PluginStartup...
[CV64_STATIC_PLUGIN] gliden64_PluginStartup succeeded
[CV64_STATIC_PLUGIN] GFX plugin registered
[CV64_STATIC_PLUGIN] Registering static Audio plugin (CV64 SDL Audio)...
[CV64_STATIC_PLUGIN] Audio plugin registered
[CV64_STATIC_PLUGIN] Registering static Input plugin (CV64 Input (Static))...
[CV64_STATIC_PLUGIN] Calling inputPluginStartup...
[CV64_STATIC_PLUGIN] inputPluginStartup succeeded
[CV64_STATIC_PLUGIN] Input plugin registered
[CV64_STATIC_PLUGIN] Registering static RSP plugin (RSP-HLE (Static))...
[CV64_STATIC_PLUGIN] Calling rsphle_PluginStartup...
[CV64_STATIC_PLUGIN] rsphle_PluginStartup succeeded
[CV64_STATIC_PLUGIN] RSP plugin registered
[CV64_STATIC_PLUGIN] === All static plugins registered ===
```

### During ROM Load
```
[CV64_STATIC] Calling plugin_start for all plugins...
[GLideN64] _initiateGFX: RDRAM=..., DMEM=..., IMEM=..., HEADER=...
[CV64_AUDIO_SDL] InitiateAudio called
[CV64_AUDIO_SDL] SDL audio initialized successfully
[CV64-Input] === InitiateControllers called ===
[CV64-Input] === InitiateControllers COMPLETE ===
[M64P-RSP-HLE] RSP plugin initialized successfully  <- NO CRASH
[CV64_STATIC] All plugins started successfully
```

### During Emulation
- No crashes from NULL function pointers
- RSP can access Core Config API (ConfigGetParamString, etc.)
- RSP can set up fallback plugin if configured
- All plugins receive debug messages
- Proper error reporting if issues occur

## Testing Steps

1. **Build & Run**
   - Compile with all fixes applied
   - Run the executable

2. **Check Debug Output**
   - Verify "rsphle_PluginStartup succeeded" appears
   - Verify no "ERROR: CoreGetStaticHandle returned NULL"
   - Verify no crash at 0x0000000000000000

3. **Load ROM**
   - Press SPACE to load embedded ROM
   - Watch for "All plugins started successfully"
   - Verify no access violation

4. **Start Emulation**
   - Verify game window appears
   - Verify graphics render correctly
   - Verify audio plays
   - Verify input responds

5. **Test Shutdown**
   - Close emulation
   - Verify clean exit
   - No crashes during cleanup

## Success Criteria

- ? No crash at address 0x0000000000000000
- ? RSP plugin initializes successfully  
- ? ROM loads without errors
- ? Emulation starts and runs
- ? All plugins functional (GFX, Audio, Input, RSP)
- ? Clean shutdown without crashes

## Rollback Plan (if needed)

If issues arise, the changes are isolated to:
1. `RMG/Source/3rdParty/mupen64plus-core/src/api/frontend.c`
   - Revert CoreGetStaticHandle and accessor functions
2. `src/cv64_static_plugins.cpp`
   - Revert plugin initialization changes
   - Remove PluginStartup calls

Original backup behavior:
- GLideN64 already worked (it had PluginStartup)
- Others would need their PluginStartup calls removed
