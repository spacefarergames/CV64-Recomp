# Example Mod Structure

This directory contains an example mod structure for the CV64 Mod Loading System.

## Directory Structure

```
ExampleMod/
??? mod.ini                 # Mod metadata (REQUIRED)
??? README.md              # Mod documentation
??? textures/              # Texture replacements
?   ??? texture_001.png
?   ??? texture_map.txt    # Texture ID mapping
??? models/                # 3D model replacements
?   ??? reinhardt.obj
?   ??? reinhardt.mtl
?   ??? model_map.txt      # Model ID mapping
??? audio/                 # Audio replacements
?   ??? bgm_castle.ogg
?   ??? audio_map.txt      # Audio ID mapping
??? scripts/               # Lua scripts
?   ??? gameplay.lua
??? patches/               # ROM/memory patches
    ??? gameplay.ips
    ??? patches.txt        # Patch descriptions
```

## mod.ini Template

```ini
[Mod]
Name=Example HD Texture Pack
Author=YourName
Version=1.0.0
Description=High definition texture replacements for Castlevania 64
Type=Texture
Priority=50
Enabled=1

[Dependencies]
; Comma-separated list of required mods
Required=

[Compatibility]
; Game version compatibility
GameVersion=1.2
MinLoaderVersion=1.0

[Features]
; What this mod changes
TextureCount=150
ModelCount=0
AudioCount=0
PatchCount=0
```

## Texture Mapping (texture_map.txt)

```
# Format: TextureCRC32 = filename
# CRC32 is calculated from original N64 texture data

0x12345678 = textures/texture_001.png
0x9ABCDEF0 = textures/texture_002.png
0xFEDCBA98 = textures/character_skin.png
```

## Model Mapping (model_map.txt)

```
# Format: ModelID = filename
# ModelID is the in-game model identifier

0x0001 = models/reinhardt.obj
0x0002 = models/carrie.obj
0x0010 = models/skeleton.obj
```

## Audio Mapping (audio_map.txt)

```
# Format: AudioID = filename
# AudioID is the in-game audio identifier

0x0001 = audio/bgm_castle.ogg
0x0010 = audio/sfx_whip.wav
0x0020 = audio/voice_reinhardt.mp3
```

## Lua Script Example (scripts/gameplay.lua)

```lua
-- Example gameplay modification script
-- Called by the mod loader at specific hook points

function OnModLoad()
    print("Example mod loaded!")
end

function OnPlayerSpawn(player)
    -- Modify player stats
    player.health = player.health * 1.5
    player.maxHealth = player.maxHealth * 1.5
end

function OnEnemySpawn(enemy)
    -- Make enemies more challenging
    enemy.health = enemy.health * 1.2
    enemy.damage = enemy.damage * 1.1
end

function OnFrameUpdate()
    -- Called every frame
end
```

## ROM Patch Format

IPS and BPS patch files are supported. Place them in the `patches/` folder.

### patches.txt Example

```
# Format: filename | description
gameplay.ips | Fixes softlock in Castle Center
difficulty.bps | Increases enemy aggression
animation.ips | Improves character animations
```

## Best Practices

### Textures
- Use PNG format for quality and transparency
- Maintain power-of-2 dimensions (256x256, 512x512, etc.)
- Keep original aspect ratios
- Include mipmaps for better performance
- Maximum recommended size: 2048x2048

### Models
- Export as OBJ with MTL material files
- Keep polygon count reasonable (original N64 limits)
- Use proper vertex normals
- Include UV coordinates for textures
- Maximum recommended: 5000 triangles per model

### Audio
- Use OGG Vorbis for music (smaller file size)
- Use WAV for sound effects (low latency)
- Match original sample rates when possible
- Normalize audio levels

### Scripts
- Keep scripts lightweight
- Avoid expensive operations in per-frame hooks
- Test thoroughly for crashes
- Comment your code
- Handle errors gracefully

## Testing Your Mod

1. Place mod folder in `mods/` directory
2. Launch CV64 Recomp
3. Go to `Game > Mod Loader...`
4. Enable your mod
5. Click "Reload Mods"
6. Test in-game

## Debugging

Enable debug output in mod.ini:

```ini
[Debug]
EnableLogging=1
LogLevel=Verbose
DumpLoadedAssets=1
```

Check the debug console for:
- Loading status
- File not found errors
- Format errors
- Compatibility warnings

## Publishing Your Mod

When ready to share:

1. Include comprehensive README.md
2. Add screenshots/preview images
3. List all changes clearly
4. Specify compatibility requirements
5. Provide installation instructions
6. Include license information
7. Credit original assets if modified

## Support

For help creating mods:
- Check the CV64 Recomp documentation
- Join the community Discord
- Post in the modding forum
- Report bugs on GitHub

Happy modding!
