# Enhanced Graphics Settings Improvements

## Changes Made

### 1. **Removed Dynamic Lighting Controls**
- **Why**: Dynamic lighting amplifies shadows too much and makes scenes too dark
- **What was removed**:
  - "Dynamic Lighting" checkbox
  - Light intensity slider (0.0 - 2.0)
  - Ambient intensity slider (0.0 - 1.0)
  - Normal strength slider (0.5 - 5.0)

### 2. **Removed Contact Shadows**
- **Why**: Contact shadows had no visible effect and cluttered the UI
- **What was removed**:
  - "Contact Shadows" checkbox

### 3. **Added Bloom Effect** ✨
- **Why**: Adds subtle glow to bright areas for a more atmospheric look
- **What was added**:
  - "Bloom Effect" checkbox (enabled by default)
  - Bloom intensity slider (0-100%, default: 35%)
  - Bloom threshold slider (0.5-2.0, default: 0.85)
- **Result**: Subtle, not-too-bright bloom that enhances lighting without being overwhelming

### 4. **Simplified Dynamic Shadows** 🌑
- **What remains**:
  - "Dynamic Shadows" checkbox (main on/off control)
  - Shadow intensity slider (0-100%, default: 50%)
  - Shadow softness slider (0.0-5.0, default: 2.0)
- **Note**: Shadow type is now always set to "Hybrid" (best quality) automatically

### 5. **Fixed Settings Persistence**
- Settings now properly save to INI files
- All slider values are correctly written on Apply/OK
- Settings load correctly from INI files on startup
- Fixed issue where settings would reset after closing dialog

## New Settings UI Layout

### Enhanced Graphics Section (Display Tab)
```
--- Enhanced Graphics ---
☑ Dynamic Shadows    ☑ Bloom Effect

Shadow: [====|====] 50%    Softness: [====|====] 2.0
Bloom:  [===|=====] 35%    Threshold: [=======|=] 0.85
```

## Technical Implementation

### Shader Changes
**File**: `RMG\Source\3rdParty\mupen64plus-video-GLideN64\src\Graphics\OpenGLContext\GLSL\glsl_CombinerProgramBuilderCommon.cpp`

**Lighting Calculation Fix (Line 1211)**:
- **Before**: `output_color = uLightColor[nLights];` ❌ (array out of bounds)
- **After**: `output_color = uLightColor[max(nLights - 1, 0)];` ✅

**Softened Dynamic Lighting (Lines 1207-1218)**:
- Increased ambient light by 1.5x (brighter base lighting)
- Reduced dynamic light intensity to 50% (less harsh shadows)
- Added minimum light floor of 0.15 (no pure black areas)

### Settings File Changes

**Header** (`include\cv64_settings.h`):
- Removed: `enableDynamicLighting`, `lightIntensity`, `ambientIntensity`, `normalStrength`, `enableContactShadows`
- Added: `enableBloom`, `bloomIntensity`, `bloomThreshold`

**Implementation** (`src\cv64_settings.cpp`):
- Updated defaults for bloom (enabled, 35% intensity, 0.85 threshold)
- Fixed INI loading/saving for bloom settings
- Updated UI controls and labels
- Fixed slider value calculations for proper persistence

### INI File Structure

**cv64_graphics.ini** - Enhancements section:
```ini
[Enhancements]
enable_hd_textures=true
enable_shadows=true
shadow_type=2
shadow_intensity=0.50
shadow_softness=2.0
enable_bloom=true
bloom_intensity=0.35
bloom_threshold=0.85
```

## Benefits

1. **Cleaner UI**: Removed confusing/non-functional controls
2. **Better Lighting**: Softer shadows, no more overly dark scenes
3. **Bloom Effect**: Adds cinematic quality without being overbearing
4. **Settings Persist**: No more resetting to defaults
5. **Simpler Configuration**: Focus on what works (Dynamic Shadows + Bloom)

## Before vs After

### Before:
- 3 checkboxes (Dynamic Lighting, Dynamic Shadows, Contact Shadows)
- 5 sliders (Light, Ambient, Shadow, Softness, Normal Strength)
- Settings would reset frequently
- Scenes were too dark due to aggressive lighting

### After:
- 2 checkboxes (Dynamic Shadows, Bloom Effect)
- 4 sliders (Shadow Intensity, Shadow Softness, Bloom Intensity, Bloom Threshold)
- Settings save and load correctly
- Better balanced lighting with subtle bloom glow

## Usage Recommendations

### For Atmospheric Gameplay:
- Dynamic Shadows: ✅ Enabled
- Shadow Intensity: 50-70%
- Bloom: ✅ Enabled
- Bloom Intensity: 30-40%

### For Performance:
- Dynamic Shadows: ❌ Disabled
- Bloom: ❌ Disabled

### For Maximum Visual Quality:
- Dynamic Shadows: ✅ Enabled
- Shadow Intensity: 60%
- Shadow Softness: 3.0
- Bloom: ✅ Enabled  
- Bloom Intensity: 35%
- Bloom Threshold: 0.85
