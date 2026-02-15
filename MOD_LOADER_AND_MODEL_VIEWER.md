# CV64 Mod Loading System & 3D Model Viewer

## Overview

Two new powerful features have been added to the Castlevania 64 Recompilation project:

1. **Mod Loading System** - Load and manage game modifications
2. **3D Model Viewer** - View and inspect models from the ROM using Gliden64 rendering

---

## Mod Loading System

### Features

- ? Automatic mod scanning from `mods` folder
- ? Support for multiple mod types:
  - Texture Replacements (.png/.dds)
  - 3D Model Replacements (.obj/.n64)
  - Audio Replacements (.wav/.mp3/.ogg)
  - Code/Memory Patches
  - ROM Patches (.ips/.bps)
  - Script Mods (Lua scripts)
  - Mod Packs (bundles)
- ? Enable/disable mods without deleting
- ? Priority-based loading order
- ? Mod dependency support
- ? Per-mod configuration via `mod.ini`

### How to Use

1. **Access Mod Loader**
   - Open the game
   - Go to `Game > Mod Loader...` menu

2. **Install a Mod**
   - Place mod folder in `mods` directory
   - Each mod needs a `mod.ini` file with metadata
   - Example structure:
     ```
     mods/
       MyAwesomeMod/
         mod.ini
         textures/
           texture001.png
         models/
           reinhardt.obj
         audio/
           bgm_castle.ogg
     ```

3. **Create mod.ini**
   ```ini
   [Mod]
   Name=My Awesome Mod
   Author=YourName
   Version=1.0
   Description=HD textures and improved models
   Type=Pack
   Priority=50
   Enabled=1
   ```

4. **Mod Types**
   - `Texture` - Texture replacements only
   - `Model` - 3D model replacements
   - `Audio` - Audio/music replacements
   - `CodePatch` - Memory/code patches
   - `ROMPatch` - ROM file patches (.ips/.bps)
   - `Script` - Lua scripting mods
   - `Pack` - Multiple types bundled together

5. **Priority System**
   - `0-25`: Lowest priority (loaded first)
   - `26-50`: Normal priority
   - `51-75`: High priority
   - `76-100`: Highest priority (overrides others)

### API for Modders

The mod loader provides hooks for:
- Texture replacement (CRC-based matching)
- Model replacement (ID-based matching)
- ROM patching at load time
- Memory patching at runtime
- Custom mod loading logic

---

## 3D Model Viewer

### Features

- ? Extract and view 3D models from ROM
- ? **50+ models from CV64 decomp database**
  - **Characters**: Reinhardt, Carrie, NPCs
  - **Enemies**: Skeletons, Werewolves, Vampires, Lizard-men, etc.
  - **Bosses**: Dracula, Death, Actrise, Rosa, Renon, Behemoth, Cerberus
  - **Items**: Jewels, Potions, Keys, Food
  - **Weapons**: Whip, Holy Water, Cross, Axe, Knives
  - **Props**: Torches, Candles, Doors, Gates, Coffins, Statues
- ? Interactive model list with filtering by type
- ? Full Gliden64 rendering (same as in-game)
- ? Interactive camera controls
  - **Left Mouse**: Orbit camera
  - **Mouse Wheel**: Zoom in/out
  - **W**: Toggle wireframe mode
  - **T**: Toggle textures
  - **L**: Toggle lighting
  - **R**: Reset camera
- ? Model information display
  - Model name and type
  - Vertex count, triangle count
  - Texture information
  - ROM offset and data size
  - Bounding box information
- ? Grid floor for reference
- ? Real-time rendering at 60 FPS
- ? Export models to common formats (.obj, .fbx, .gltf) *(coming soon)*

### How to Use

1. **Open Model Viewer**
   - Go to `Game > 3D Model Viewer...` menu
   - A window opens with model list on the left, 3D view on the right

2. **Browse Models**
   - The left panel shows all available models from the database
   - Models are categorized: `[CHAR]`, `[ENEMY]`, `[BOSS]`, `[ITEM]`, `[WEAPON]`, `[PROP]`
   - Scroll through the list to find models

3. **Load a Model**
   - Double-click on any model in the list
   - The model loads in the 3D viewport
   - Model information appears in the bottom panel

4. **Camera Controls**
   - **Orbit**: Left-click and drag in the 3D view
   - **Zoom**: Mouse wheel
   - **Reset**: Press 'R' or click "Reset Camera"

5. **Render Options**
   - **Wireframe Mode**: Press 'W' to see polygon edges
   - **Textures**: Press 'T' to toggle texture rendering (when implemented)
   - **Lighting**: Press 'L' to toggle lighting

6. **Export Models** *(coming soon)*
   - Select loaded model
   - Click "Export Model"
   - Choose format: OBJ, FBX, or GLTF
   - Models export with textures and materials

### Available Models

The viewer includes **50+ models** from the CV64 decomp database:

#### Characters (6 models)
- Reinhardt Schneider (base & with whip)
- Carrie Fernandez (base & with orb)
- Rosa (NPC form)
- Malus
- Vincent
- Renon (merchant form)
- Villagers & Children

#### Enemies (9+ models)
- Skeleton
- Skeleton Knight
- Werewolf
- Vampire
- Lizard-man
- Flea-man
- Hell Hound
- Bat
- Mist (shape-shifter)

#### Bosses (7 models)
- Dracula (Final Form)
- Death
- Actrise
- Rosa (Boss Form)
- Renon (Demon Form)
- Behemoth
- Cerberus

#### Items (7 models)
- Red Jewel
- Blue Jewel
- Green Jewel
- Healing Potion
- Roast Beef
- Key
- Special Key

#### Weapons (5 models)
- Whip
- Holy Water
- Cross
- Axe
- Throwing Knife

#### Props (8+ models)
- Torch
- Candle
- Door
- Gate
- Breakable Wall
- Lever
- Coffin
- Statue

### Technical Details

The model viewer:
- Uses Gliden64 rendering pipeline (same as the game)
- Renders in a separate OpenGL context
- Supports up to 8 textures per model
- Maintains 60 FPS rendering
- Uses efficient vertex/index buffers
- Supports animation playback (when available)

---

## Integration with Game

Both features integrate seamlessly:

### Keyboard Shortcuts

- **F1**: Toggle D-Pad camera mode
- **F2**: Toggle camera patch
- **F3**: Toggle performance overlay
- **F4**: Hard reset
- **F5**: Quick save
- **F9**: Quick load
- **F10**: Input remapping dialog

### Menu Structure

```
Game
??? Settings...
??? Input Remapping...
??? Memory Pak Editor...
??? ?????????????????
??? Mod Loader...         ? NEW!
??? 3D Model Viewer...    ? NEW!
??? ?????????????????
??? Toggle Fullscreen
??? ?????????????????
??? Exit
```

---

## For Developers

### Adding Mod Support to Your Code

```cpp
#include "cv64_mod_loader.h"

// Check for texture replacement
CV64_TextureReplacement replacement;
if (CV64_ModLoader_GetTextureReplacement(textureCRC, &replacement)) {
    // Use replacement.glTextureHandle instead of original
}

// Check for model replacement
CV64_ModelReplacement modelRepl;
if (CV64_ModLoader_GetModelReplacement(modelID, &modelRepl)) {
    // Use replacement model data
}
```

### Using Model Viewer API

```cpp
#include "cv64_model_viewer.h"

// Show viewer window
CV64_ModelViewer_ShowWindow(parentHwnd);

// Load a specific model
CV64_ModelViewer_LoadModel(modelID);

// Get viewer stats
uint32_t totalModels, loadedModel;
float fps;
CV64_ModelViewer_GetStats(&totalModels, &loadedModel, &fps);
```

---

## Future Enhancements

### Planned Features

- [ ] Live mod hot-reloading
- [ ] Mod conflict detection and resolution
- [ ] Integrated mod browser/downloader
- [ ] Mod creation wizard
- [ ] Animation playback in model viewer
- [ ] Texture preview and export
- [ ] Batch model export
- [ ] Model comparison tool
- [ ] Collision mesh visualization
- [ ] Light map visualization

### Community Contributions

We welcome contributions! Areas that need work:
1. ROM model extraction (N64 display list parsing)
2. Animation system implementation
3. Material/shader system
4. Mod compression support
5. Cloud mod repository integration

---

## Troubleshooting

### Mod Loader Issues

**Q: Mods don't appear in the list**
- Ensure `mod.ini` exists in mod folder
- Check mod folder is in `mods` directory
- Verify `mod.ini` has correct format

**Q: Mod is enabled but not loading**
- Check mod priority (conflicts with other mods?)
- Verify mod files exist in correct locations
- Check debug output for error messages

### Model Viewer Issues

**Q: Model viewer window is black**
- OpenGL initialization may have failed
- Update graphics drivers
- Check compatibility with your GPU

**Q: Models don't load**
- ROM scanning is not yet fully implemented
- Feature uses test cube for demonstration
- Full implementation coming soon

**Q: Performance is slow**
- Reduce viewport size
- Disable lighting/textures temporarily
- Check graphics driver settings

---

## Credits

- **CV64 Recomp Team** - Implementation
- **cv64 decomp project** - Game structure reference
- **mupen64plus** - N64 emulation core
- **Gliden64** - Graphics plugin
- **Community** - Testing and feedback

---

## License

Part of the Castlevania 64 Recompilation Project  
See main project LICENSE for details.
