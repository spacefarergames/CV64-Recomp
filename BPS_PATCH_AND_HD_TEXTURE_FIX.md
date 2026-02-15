# BPS Patch & HD Texture Fix Summary

## Changes Made

### 1. BPS Patch Auto-Apply System

**New Files:**
- `include/cv64_bps_patch.h` - Header for BPS patch system
- `src/cv64_bps_patch.cpp` - BPS patch implementation

**Features:**
- Automatically scans the `patches/` folder for `.bps` files on ROM load
- Validates patch CRC32 before applying
- Applies patches that match the ROM's CRC32
- Supports standard BPS format (Beat Patch System)
- Logs all patching activity to debug output

**Integration:**
- Modified `src/cv64_m64p_integration_static.cpp` to call `CV64_BPS_ApplyPatches()` 
  before loading the ROM into the emulator core
- Works for both embedded ROMs and file-loaded ROMs

**Usage:**
1. Place any `.bps` patch files in the `patches/` folder
2. The patch source CRC must match the ROM's CRC32
3. Patches are automatically applied at ROM load time
4. Check debug output for patching status

### 2. HD Texture Loading Fix

**Problem:**
The GLideN64 video plugin was using `TxFilterStub.cpp` which is a stub implementation
that does nothing - all texture filter functions returned 0/false.

**Solution:**
Replaced `TxFilterStub.cpp` with the actual GLideNHQ library files:

**Files Added to GLideN64 Build:**
- `GLideNHQ/TxFilterExport.cpp`
- `GLideNHQ/TxFilter.cpp`
- `GLideNHQ/TxCache.cpp`
- `GLideNHQ/TxHiResCache.cpp`
- `GLideNHQ/TxHiResLoader.cpp`
- `GLideNHQ/TxHiResNoCache.cpp`
- `GLideNHQ/TxImage.cpp`
- `GLideNHQ/TxQuantize.cpp`
- `GLideNHQ/TxReSample.cpp`
- `GLideNHQ/TxTexCache.cpp`
- `GLideNHQ/TxUtil.cpp`
- `GLideNHQ/TxDbg.cpp`
- `GLideNHQ/TextureFilters.cpp`
- `GLideNHQ/TextureFilters_2xsai.cpp`
- `GLideNHQ/TextureFilters_hq2x.cpp`
- `GLideNHQ/TextureFilters_hq4x.cpp`
- `GLideNHQ/TextureFilters_xbrz.cpp`

**Include Paths Added:**
- `GLideNHQ/` folder
- `GLideNHQ/inc/` folder (for PNG headers)
- `zlib-1.2.13/include/`
- `libpng-1.6.39/include/`

### 3. Project Files Updated

**CV64_RMG.vcxproj:**
- Added `src/cv64_bps_patch.cpp` to compilation
- Added `include/cv64_bps_patch.h` to headers

**mupen64plus-video-gliden64-static.vcxproj:**
- Removed `TxFilterStub.cpp`
- Added all GLideNHQ source files
- Added required include directories for zlib and libpng

## HD Texture Setup

For HD textures to work:

1. **Enable in config:** In `patches/cv64_graphics.ini`:
   ```ini
   [Enhancements]
   tx_hires_enable=true
   tx_hires_full_alpha=true
   
   [TextureFilter]
   tx_hires_alt_crc=true    ; Required for Rice format packs
   tx_strong_crc=true
   tx_hires_file_storage=true
   ```

2. **Place textures:** Put HD texture packs in:
   - `assets/hires_texture/CASTLEVANIA/` or
   - `assets/hires_texture/CASTLEVANIA_HIRESTEXTURES/`

3. **Supported formats:**
   - GLideN64 Native format (CRC-based naming)
   - Rice format (requires `tx_hires_alt_crc=true`)
   - HTX compressed packs

## BPS Patch File Format

BPS patches should:
- Have `.bps` extension
- Be placed in the `patches/` folder
- Have a source CRC that matches the ROM you're patching

The system will:
1. Scan for all `.bps` files in `patches/`
2. For each patch, check if the source CRC matches the ROM
3. Apply matching patches
4. Log success/failure to debug output

## Build Status

? Build successful - both BPS patch system and HD texture support are compiled and ready.
