# Castlevania 64 - Player Character Shadow Implementation
## Memory Address Research

### System Work Structure
**Base Address:** `0x80363AB8`

This is the main game state structure containing pointers to all major game objects.

### Player Object Pointer
**Address:** `0x80363AB8 + 0x26228 = 0x80389CE0`
- **Type:** Pointer to Player object
- **Size:** 4 bytes (32-bit pointer)
- **Usage:** Read this pointer to get the current player object address

### Player Object Structure (Estimated Offsets)
Based on typical N64 game structures and CV64 decomp patterns:

```c
// Player / Figure structure (common N64 entity pattern)
struct Player {
    // Header (0x00 - 0x20)
    u32 object_type;              // 0x00
    u32 object_flags;             // 0x04
    void* prev;                   // 0x08 - linked list
    void* next;                   // 0x0C - linked list
    
    // Transform (0x10 - 0x30)
    Vec3f position;               // 0x10 - {x, y, z} - CRITICAL FOR SHADOW
    Vec3f rotation;               // 0x1C - {rx, ry, rz}
    Vec3f scale;                  // 0x28 - {sx, sy, sz}
    
    // Physics (0x34 - 0x50)
    Vec3f velocity;               // 0x34
    Vec3f acceleration;           // 0x40
    f32 ground_y;                 // 0x4C - Y position of ground below player
    
    // State (0x50+)
    u16 state_flags;              // 0x50
    u16 animation_id;             // 0x52
    s16 health;                   // 0x54
    s16 max_health;               // 0x56
    // ... more fields
};
```

### Key Addresses for Shadow Implementation

#### Player Position (XYZ)
- **X Position:** `PlayerPtr + 0x10` (float, 4 bytes)
- **Y Position:** `PlayerPtr + 0x14` (float, 4 bytes)  
- **Z Position:** `PlayerPtr + 0x18` (float, 4 bytes)

#### Ground Y Position (for shadow placement)
- **Ground Y:** `PlayerPtr + 0x4C` (float, 4 bytes)
- This tells us where the ground is beneath the player
- Shadow should be rendered at this Y position

### Reading Player Position - Code Example

```cpp
// Step 1: Read player pointer from system_work
u32 playerPtr = CV64_ReadU32(rdram, 0x80389CE0);

if (playerPtr != 0 && playerPtr >= 0x80000000) {
    // Step 2: Read position from player object
    f32 playerX = CV64_ReadF32(rdram, playerPtr + 0x10);
    f32 playerY = CV64_ReadF32(rdram, playerPtr + 0x14);
    f32 playerZ = CV64_ReadF32(rdram, playerPtr + 0x18);
    
    // Step 3: Read ground Y for shadow placement
    f32 groundY = CV64_ReadF32(rdram, playerPtr + 0x4C);
    
    // Step 4: Render shadow at (playerX, groundY, playerZ)
    RenderCharacterShadow(playerX, groundY, playerZ);
}
```

### Shadow Rendering Strategy

1. **Shadow Type:** Circular blob (like Mario 64)
2. **Shadow Texture:** Pre-rendered circular gradient (black center, transparent edges)
3. **Shadow Size:** Scale based on height above ground (higher = larger/fainter)
4. **Shadow Opacity:** Fade based on distance from ground

### Shadow Calculation

```cpp
// Calculate shadow properties
float heightAboveGround = playerY - groundY;
float shadowScale = 1.0f + (heightAboveGround * 0.01f); // Grows with height
float shadowOpacity = 1.0f / (1.0f + heightAboveGround * 0.05f); // Fades with height

// Clamp values
shadowScale = min(shadowScale, 3.0f);  // Max 3x size
shadowOpacity = max(shadowOpacity, 0.1f);  // Min 10% opacity
```

### Implementation Notes

1. **Verify Offsets:** These offsets are estimated based on common N64 patterns
   - Test with actual game to confirm
   - Player structure may vary between Reinhardt/Carrie

2. **Ground Detection:**
   - The `ground_y` field (0x4C) is likely updated by the game's collision system
   - If not available, may need to raycast or use player Y position directly

3. **Performance:**
   - Read player position once per frame
   - Cache the shadow quad to avoid recreation
   - Only update when player moves significantly

4. **Integration Point:**
   - Hook into GLideN64's rendering pipeline AFTER 3D geometry
   - Render shadow as a textured quad in world space
   - Use alpha blending for smooth edges

### Testing Addresses

To verify these addresses are correct:

```cpp
// Print player position each frame
u32 playerPtr = CV64_ReadU32(rdram, 0x80389CE0);
if (playerPtr) {
    f32 x = CV64_ReadF32(rdram, playerPtr + 0x10);
    f32 y = CV64_ReadF32(rdram, playerPtr + 0x14);
    f32 z = CV64_ReadF32(rdram, playerPtr + 0x18);
    printf("Player: X=%.2f Y=%.2f Z=%.2f\n", x, y, z);
}
```

If positions update realistically as the player moves, offsets are correct!

### Next Steps

1. **Verify player pointer address** (0x80389CE0)
2. **Test position offsets** (0x10, 0x14, 0x18)
3. **Confirm ground_y offset** (0x4C) or implement raycast
4. **Create shadow texture** (circular blob, 64x64 or 128x128)
5. **Implement rendering hook** in GLideN64
6. **Add shadow scaling/fading logic**

### References
- CV64 Decomp: `C:\Users\patte\cv64\`
- Symbol Addresses: `C:\Users\patte\cv64\linker\symbol_addrs.txt`
- System Work: `src/game/system_work.h`
- Player/Figure: `src/game/figure/figure.h`
