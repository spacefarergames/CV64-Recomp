# Shadow Texture Visual Reference

## Generated Shadow Texture Appearance

The shadow texture is a circular blob with a radial gradient. Here's a visual representation:

```
64x64 Texture (Default)
Intensity: 0.7

   ████████████████████████████████
  ████████████████████████████████████
 ██████▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██████
████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓████
██▓▓▒▒▒▒░░░░░░░░░░░░░░░░░░░░▒▒▒▒▓▓██
██▓▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓▓██
██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░░░  ░░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░      ░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░          ░░░░░░░░░▒▓██
██▓▒░░░░░░              ░░░░░░░▒▓██
██▓▒░░░░░                ░░░░░░▒▓██
██▓▒░░░░                  ░░░░░▒▓██
██▓▒░░░░                  ░░░░░▒▓██
██▓▒░░░░░                ░░░░░░▒▓██
██▓▒░░░░░░              ░░░░░░░▒▓██
██▓▒░░░░░░░░          ░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░      ░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░░░  ░░░░░░░░░░░░░▒▓██
██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
██▓▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓▓██
██▓▓▒▒▒▒░░░░░░░░░░░░░░░░░░░░▒▒▒▒▓▓██
████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓████
 ██████▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██████
  ████████████████████████████████████
   ████████████████████████████████

Legend:
█ = Solid (Alpha ≈ 255, Intensity × 1.0)
▓ = Dark  (Alpha ≈ 192, Intensity × 0.75)
▒ = Med   (Alpha ≈ 128, Intensity × 0.5)
░ = Light (Alpha ≈ 64,  Intensity × 0.25)
  = Clear (Alpha = 0)
```

## Alpha Channel Gradient

The texture's alpha channel varies from center to edge:

```
Center (distance = 0.0):
    Alpha = 255 × intensity
    Example (intensity=0.7): α = 178

25% Radius (distance = 0.25):
    Alpha ≈ 200 × intensity
    Example (intensity=0.7): α = 140

50% Radius (distance = 0.5):
    Alpha ≈ 128 × intensity
    Example (intensity=0.7): α = 90

75% Radius (distance = 0.75):
    Alpha ≈ 36 × intensity
    Example (intensity=0.7): α = 25

Edge (distance = 1.0):
    Alpha = 0 (fully transparent)
```

## Gradient Curve (Smoothstep)

```
1.0 |█
    |████
0.9 |    ███
    |       ██
0.8 |         ██
    |           ██
0.7 |             ██
    |               █
0.6 |                ██
    |                  ██
0.5 |                    ██
    |                      ██
0.4 |                        ██
    |                          ██
0.3 |                            ██
    |                              ██
0.2 |                                ███
    |                                   ███
0.1 |                                      ████
    |_________________________________________█████
0.0  0.0  0.1  0.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9  1.0
        Distance from Center (normalized)

Formula: alpha = (1-d)² × (3 - 2(1-d))
```

## RGB Channels

All RGB channels are set to 0 (black):
```
R = 0
G = 0
B = 0
A = gradient (described above)
```

## Texture Data Layout

For a 4×4 example texture (scaled down for illustration):

```
Pixel Format: RGBA8 (32-bit per pixel)

    X:  0        1        2        3
Y:0 [00000000][0000003F][0000003F][00000000]
Y:1 [0000003F][000000B2][000000B2][0000003F]
Y:2 [0000003F][000000B2][000000B2][0000003F]
Y:3 [00000000][0000003F][0000003F][00000000]

Decoded (RGBA):
Y:0 [(0,0,0,0)]  [(0,0,0,63)]  [(0,0,0,63)]  [(0,0,0,0)]
Y:1 [(0,0,0,63)] [(0,0,0,178)] [(0,0,0,178)] [(0,0,0,63)]
Y:2 [(0,0,0,63)] [(0,0,0,178)] [(0,0,0,178)] [(0,0,0,63)]
Y:3 [(0,0,0,0)]  [(0,0,0,63)]  [(0,0,0,63)]  [(0,0,0,0)]
```

## How It Renders

When rendered with alpha blending:

```
Ground Color: FFFFFF (white)
Shadow at various alpha values:

α=255 (1.0):   000000 ██ (fully black)
α=192 (0.75):  404040 ▓▓ (dark gray)
α=128 (0.5):   808080 ▒▒ (medium gray)
α=64  (0.25):  BFBFBF ░░ (light gray)
α=0   (0.0):   FFFFFF    (fully white, no shadow)

Blending Formula:
    finalColor = shadowColor × α + groundColor × (1-α)
    finalColor = (0,0,0) × α + (1,1,1) × (1-α)
    finalColor = (1-α, 1-α, 1-α)
```

## Size Comparison

### 32×32 (Low Detail)
```
    ████████
  ██▓▓▓▓▓▓▓▓██
 ██▓▒▒░░░░▒▒▓██
██▓▒░░    ░░▒▓██
██▓▒░      ░▒▓██
██▓▒░░    ░░▒▓██
 ██▓▒▒░░░░▒▒▓██
  ██▓▓▓▓▓▓▓▓██
    ████████

Memory: 4 KB
Quality: Basic, suitable for distant shadows
```

### 64×64 (Default)
```
       ████████████
    ████▓▓▓▓▓▓▓▓████
   ██▓▓▓▒▒▒▒▒▒▒▒▓▓▓██
  ██▓▒▒▒░░░░░░░░▒▒▒▓██
 ██▓▒▒░░░░░░░░░░░░▒▒▓██
 ██▓▒░░░░░░░░░░░░░░▒▓██
██▓▒░░░░░░  ░░░░░░░░▒▓██
██▓▒░░░░      ░░░░░░▒▓██
██▓▒░░░░      ░░░░░░▒▓██
██▓▒░░░░░░  ░░░░░░░░▒▓██
 ██▓▒░░░░░░░░░░░░░░▒▓██
 ██▓▒▒░░░░░░░░░░░░▒▒▓██
  ██▓▒▒▒░░░░░░░░▒▒▒▓██
   ██▓▓▓▒▒▒▒▒▒▒▒▓▓▓██
    ████▓▓▓▓▓▓▓▓████
       ████████████

Memory: 16 KB
Quality: Good, recommended default
```

### 128×128 (High Detail)
```
               ████████████████████
         ████████▓▓▓▓▓▓▓▓▓▓▓▓████████
       ████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓████
      ██▓▓▓▒▒▒▒▒▒░░░░░░░░░░░░▒▒▒▒▒▒▓▓▓██
    ██▓▓▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▓▓██
   ██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓██
  ██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓██
  ██▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
 ██▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
 ██▓▒░░░░░░░░░░░░░░      ░░░░░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░░░            ░░░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░                ░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░                    ░░░░░░░░░░▒▓██
██▓▒░░░░░░                        ░░░░░░░░▒▓██
██▓▒░░░░                            ░░░░░░▒▓██
██▓▒░░░░                            ░░░░░░▒▓██
██▓▒░░░░░░                        ░░░░░░░░▒▓██
██▓▒░░░░░░░░                    ░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░                ░░░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░░░            ░░░░░░░░░░░░░░▒▓██
 ██▓▒░░░░░░░░░░░░░░      ░░░░░░░░░░░░░░░░▒▓██
 ██▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
  ██▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▓██
  ██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓██
   ██▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▓██
    ██▓▓▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▓▓██
      ██▓▓▓▒▒▒▒▒▒░░░░░░░░░░░░▒▒▒▒▒▒▓▓▓██
       ████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓████
         ████████▓▓▓▓▓▓▓▓▓▓▓▓████████
               ████████████████████

Memory: 64 KB
Quality: Excellent, for close-up shadows
```

## Intensity Comparison (64×64)

### Intensity = 0.3 (Light Shadow)
```
   ████████████
 ██░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
██░░░░░░░░░░░░░░██
 ██░░░░░░░░░░░░██
   ████████████

Almost transparent, subtle shadow
```

### Intensity = 0.7 (Default)
```
   ████████████
 ██▓▓▒▒░░░░▒▒▓▓██
██▓▒░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░▒▓██
██▓▒░░░░░░░░░░▒▓██
 ██▓▓▒▒░░░░▒▒▓▓██
   ████████████

Good balance, recommended
```

### Intensity = 1.0 (Dark Shadow)
```
   ████████████
 ████▓▓▓▒▒▒▒▓▓▓███
███▓▓▒▒░░░░░░▒▒▓███
███▓▒░░░░░░░░░░▒▓███
███▓▒░░░░░░░░░░▒▓███
███▓▒░░░░░░░░░░▒▓███
███▓▒░░░░░░░░░░▒▓███
███▓▓▒▒░░░░░░▒▒▓▓███
 ████▓▓▓▒▒▒▒▓▓▓████
   ████████████

Very dark, strong shadow
```

## In-Game Appearance

When rendered as a ground decal under a character:

```
Side View:
                Character
                   |█|
                   |█|
                   |█|
            ┌──────┴─┴──────┐
            │                │
         ██████████████████████
       ████▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓████
      ████▓▓▒▒▒▒░░░░░░▒▒▒▒▓▓████
     ████▓▒░░░░░░░░░░░░░░░░▒▓████
─────┴─────────────────────────────┴─── Ground

Top View:
           Character
              ██
              ██
              
     ░░░░▒▒▓▓██▓▓▒▒░░░░
   ░░░▒▒▓▓████████▓▓▒▒░░░
  ░░▒▒▓▓██        ██▓▓▒▒░░
 ░░▒▓▓██            ██▓▓▒░░
 ░▒▓▓██              ██▓▓▒░
 ░▒▓██                ██▓▒░
 ░▒▓██      Shadow     ██▓▒░
 ░▒▓▓██              ██▓▓▒░
 ░░▒▓▓██            ██▓▓▒░░
  ░░▒▒▓▓██        ██▓▓▒▒░░
   ░░░▒▒▓▓████████▓▓▒▒░░░
     ░░░░▒▒▓▓██▓▓▒▒░░░░
```

## Filtering Comparison

### Without Linear Filtering (Point Sampling)
```
   ████████
  ██▓▓▓▓▓▓██
 ██▓▒▒▒▒▒▒▓██
██▓▒░░░░░░▒▓██
██▓▒░░░░░░▒▓██
 ██▓▒▒▒▒▒▒▓██
  ██▓▓▓▓▓▓██
   ████████

Blocky, harsh edges
Aliasing artifacts
```

### With Linear Filtering (Default)
```
     ▓▓▓▓
   ▓▓▒▒▒▒▓▓
  ▓▒▒░░░░▒▒▓
 ▓▒░░░░░░░░▒▓
 ▓▒░░░░░░░░▒▓
  ▓▒▒░░░░▒▒▓
   ▓▓▒▒▒▒▓▓
     ▓▓▓▓

Smooth, soft edges
Anti-aliased appearance
```

## Summary

The shadow texture is:
- ✅ **Circular** - Symmetrical radial gradient
- ✅ **Smooth** - Smoothstep interpolation, no hard edges
- ✅ **Black** - RGB=(0,0,0), only alpha varies
- ✅ **Soft** - Linear filtering creates anti-aliased edges
- ✅ **Configurable** - Size and intensity adjustable
- ✅ **Efficient** - Single texture, reusable for many objects

**Default Settings** (recommended):
- Size: 64×64 pixels (16 KB)
- Intensity: 0.7 (70% opacity at center)
- Format: RGBA8
- Filtering: Linear
- Wrapping: Clamp to edge
