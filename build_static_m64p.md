# Building mupen64plus as Static Libraries for CV64_RMG

## Overview
Instead of loading mupen64plus as DLLs at runtime, we'll build the core and plugins as static libraries (.lib) and link them directly into CV64_RMG.exe. This creates a single, self-contained executable with no external dependencies.

## Step 1: Add Projects to Solution

We need to add these projects from RMG to the CV64_RMG solution:

### Core (Required):
- `RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj`

### Plugins (Required):
- `RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj` (RSP)
- `RMG\Source\3rdParty\mupen64plus-video-GLideN64\projects\msvc\GLideN64.vcxproj` (Video)

### Plugins (Optional but Recommended):
- `RMG\Source\RMG-Audio` (Audio wrapper)
- `RMG\Source\RMG-Input` (Input wrapper)

## Step 2: Convert to Static Libraries

Each project needs to be configured to build as a static library instead of a DLL:

### Project Properties Changes:
```
Configuration: All Configurations
Platform: x64

General:
  Configuration Type: Static Library (.lib)
  
C/C++:
  Code Generation:
    Runtime Library: Multi-threaded (/MT) for Release
    Runtime Library: Multi-threaded Debug (/MTd) for Debug
  
  Preprocessor Definitions:
    Remove: _USRDLL
    Add: M64P_CORE_STATIC (for static linking)
```

## Step 3: Link Libraries to CV64_RMG

Add to CV64_RMG project properties:

### Linker > Input > Additional Dependencies:
```
mupen64plus-core.lib
mupen64plus-rsp-hle.lib
GLideN64.lib
opengl32.lib
SDL2.lib
zlib.lib
libpng.lib
```

### Linker > General > Additional Library Directories:
```
$(SolutionDir)RMG\Source\3rdParty\mupen64plus-core\projects\msvc\$(Platform)\$(Configuration)\
$(SolutionDir)RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\$(Platform)\$(Configuration)\
$(SolutionDir)RMG\Source\3rdParty\mupen64plus-video-GLideN64\projects\msvc\$(Platform)\$(Configuration)\
```

## Step 4: Add Project References

In CV64_RMG project:
1. Right-click ? Add ? Reference
2. Check all the mupen64plus projects
3. This ensures they build before CV64_RMG

## Step 5: Update Integration Code

Modify `src/cv64_m64p_integration.cpp` to directly call functions instead of LoadLibrary:

### Before (Dynamic Loading):
```cpp
static HMODULE s_coreLib = NULL;
static ptr_CoreStartup s_coreStartup = NULL;

// Load library
s_coreLib = LoadLibraryW(corePath.c_str());
s_coreStartup = (ptr_CoreStartup)GetProcAddress(s_coreLib, "CoreStartup");
```

### After (Static Linking):
```cpp
// Declare extern functions from static library
extern "C" {
    m64p_error CoreStartup(int, const char*, const char*, void*, void*, void*, void*);
    m64p_error CoreShutdown(void);
    m64p_error CoreDoCommand(m64p_command, int, void*);
    // ... etc
}

// Use directly
m64p_error ret = CoreStartup(...);
```

## Step 6: Handle Plugin Registration

Since plugins are now statically linked, we need to register them differently:

### Before (Dynamic DLL Loading):
```cpp
LoadLibrary("mupen64plus-video-GLideN64.dll");
PluginStartup(...);
CoreAttachPlugin(M64PLUGIN_GFX, pluginHandle);
```

### After (Static Registration):
```cpp
extern "C" {
    // Video plugin exports
    m64p_error PluginStartup_Video(m64p_dynlib_handle, void*, void(*)(void*, int, const char*));
    m64p_error PluginGetVersion_Video(m64p_plugin_type*, int*, int*, const char**, int*);
    
    // RSP plugin exports
    m64p_error PluginStartup_RSP(m64p_dynlib_handle, void*, void(*)(void*, int, const char*));
    // ... etc
}

// Register plugins
PluginStartup_Video(NULL, "Video", DebugCallback);
CoreAttachPlugin(M64PLUGIN_GFX, STATIC_PLUGIN_HANDLE);
```

## Step 7: Dependencies

You'll still need SDL2 and zlib, but these can also be built as static libraries:

### SDL2 Static:
- Get SDL2 source
- Build as static library with `/MT`
- Link against SDL2main.lib and SDL2-static.lib

### Zlib Static:
- Use zlib included in RMG/Source/3rdParty
- Build as static library

## Benefits

### ? Advantages:
1. **Single Executable** - No external DLLs needed
2. **No LoadLibrary** - Direct function calls (faster)
3. **Better Optimization** - Compiler can inline across library boundaries
4. **Easier Distribution** - Just one .exe file
5. **No DLL Hell** - No version conflicts
6. **Better Debugging** - Step through core code
7. **Link-Time Optimization** - LTCG across entire project

### ?? Considerations:
1. **Larger Executable** - ~5-10 MB instead of 200 KB
2. **Longer Build Time** - Must rebuild core when it changes
3. **Memory Layout** - All code in same address space
4. **Plugin Updates** - Requires recompilation

## Implementation Plan

### Phase 1: Core Only (Simplest)
1. Add mupen64plus-core as static library
2. Update integration code to use direct calls
3. Keep plugins as DLLs initially
4. Test that core works

### Phase 2: Add RSP Plugin
1. Add mupen64plus-rsp-hle as static library
2. Register it statically
3. Test emulation works

### Phase 3: Add Video Plugin
1. Add GLideN64 as static library (or alternative)
2. This is the most complex (has many dependencies)
3. May need to build libGLideNHQ static too

### Phase 4: Optional Plugins
1. Add audio/input as static libraries
2. Or use your custom implementations

## Alternative: Hybrid Approach

Keep core static, allow plugins to be either static or dynamic:

```cpp
#ifdef M64P_VIDEO_STATIC
    extern "C" m64p_error PluginStartup_Video(...);
    PluginStartup_Video(...);
#else
    LoadLibrary("mupen64plus-video-GLideN64.dll");
    // ... dynamic loading
#endif
```

This gives flexibility:
- Ship with static core + RSP
- Allow users to swap video plugins
- Custom audio/input always static

## Next Steps

I'll help you with whichever approach you prefer:

1. **Full Static** - Everything in one EXE
2. **Core Static** - Core+RSP static, plugins dynamic
3. **Hybrid** - Compile-time choice per plugin

Which would you like me to implement?
