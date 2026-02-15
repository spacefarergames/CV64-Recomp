/**
 * @file cv64_model_viewer.h
 * @brief CV64 3D Model Viewer - View and inspect models from ROM using Gliden64
 * 
 * This module provides a 3D model viewer that extracts models from the
 * Castlevania 64 ROM and renders them using Gliden64 in a separate window.
 * Supports rotation, zoom, texture viewing, and animation playback.
 * 
 * Features:
 * - Extract 3D models from ROM
 * - Render using Gliden64 (same as in-game)
 * - Interactive camera controls (orbit, pan, zoom)
 * - Texture preview and replacement
 * - Animation playback (if available)
 * - Export to common 3D formats (.obj, .fbx)
 * - Material and lighting preview
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_MODEL_VIEWER_H
#define CV64_MODEL_VIEWER_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CV64_MODEL_MAX_NAME 128
#define CV64_MODEL_MAX_PATH 512

// Model types
typedef enum {
    CV64_MODEL_TYPE_CHARACTER = 0,    // Player characters (Reinhardt, Carrie)
    CV64_MODEL_TYPE_ENEMY,            // Enemy models
    CV64_MODEL_TYPE_BOSS,             // Boss models
    CV64_MODEL_TYPE_ITEM,             // Item/pickup models
    CV64_MODEL_TYPE_PROP,             // Environmental props
    CV64_MODEL_TYPE_ARCHITECTURE,     // Buildings, walls, etc.
    CV64_MODEL_TYPE_WEAPON,           // Weapon models
    CV64_MODEL_TYPE_EFFECT,           // Effect models (particles, etc.)
    CV64_MODEL_TYPE_UNKNOWN
} CV64_ModelType;

// Model information
typedef struct {
    uint32_t modelID;
    char name[CV64_MODEL_MAX_NAME];
    CV64_ModelType type;
    
    // Geometry data
    uint32_t vertexCount;
    uint32_t triangleCount;
    uint32_t textureCount;
    
    // Bounding box
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    
    // Animation data
    bool hasAnimation;
    uint32_t frameCount;
    uint32_t boneCount;
    
    // ROM location
    uint32_t romOffset;
    uint32_t dataSize;
    
    // Runtime data
    bool loaded;
    void* glVertexBuffer;
    void* glIndexBuffer;
    void* textures[8];  // Up to 8 textures per model
} CV64_ModelInfo;

// Camera control state
typedef struct {
    float orbitAngleX;      // Horizontal rotation (degrees)
    float orbitAngleY;      // Vertical rotation (degrees)
    float distance;         // Distance from model
    float panX, panY;       // Pan offset
    float fov;              // Field of view
    
    bool isDragging;
    int lastMouseX, lastMouseY;
} CV64_ViewerCamera;

// Viewer state
typedef enum {
    CV64_VIEWER_STATE_IDLE = 0,
    CV64_VIEWER_STATE_LOADING,
    CV64_VIEWER_STATE_RENDERING,
    CV64_VIEWER_STATE_EXPORTING,
    CV64_VIEWER_STATE_ERROR
} CV64_ViewerState;

/**
 * @brief Initialize the 3D model viewer system
 * @return true if initialization succeeded
 */
bool CV64_ModelViewer_Init(void);

/**
 * @brief Shutdown the 3D model viewer system
 */
void CV64_ModelViewer_Shutdown(void);

/**
 * @brief Show the model viewer window
 * @param hWndParent Parent window handle
 */
void CV64_ModelViewer_ShowWindow(HWND hWndParent);

/**
 * @brief Hide the model viewer window
 */
void CV64_ModelViewer_HideWindow(void);

/**
 * @brief Check if model viewer window is visible
 * @return true if window is visible
 */
bool CV64_ModelViewer_IsVisible(void);

/**
 * @brief Scan ROM and extract all available models
 * @param romPath Path to the ROM file
 * @return Number of models found
 */
uint32_t CV64_ModelViewer_ScanROM(const char* romPath);

/**
 * @brief Get list of available models
 * @param outModels Output array to receive model info
 * @param maxModels Maximum number of models to return
 * @return Number of models returned
 */
uint32_t CV64_ModelViewer_GetModels(CV64_ModelInfo* outModels, uint32_t maxModels);

/**
 * @brief Load and display a specific model
 * @param modelID Model ID to load
 * @return true if model was loaded successfully
 */
bool CV64_ModelViewer_LoadModel(uint32_t modelID);

/**
 * @brief Unload the current model
 */
void CV64_ModelViewer_UnloadModel(void);

/**
 * @brief Get currently loaded model info
 * @param outInfo Output model info
 * @return true if a model is currently loaded
 */
bool CV64_ModelViewer_GetCurrentModel(CV64_ModelInfo* outInfo);

/**
 * @brief Update the viewer camera
 * @param deltaTime Time since last update (seconds)
 */
void CV64_ModelViewer_UpdateCamera(float deltaTime);

/**
 * @brief Render the current model
 */
void CV64_ModelViewer_Render(void);

/**
 * @brief Reset camera to default position
 */
void CV64_ModelViewer_ResetCamera(void);

/**
 * @brief Set camera orbit angles
 * @param angleX Horizontal angle (degrees)
 * @param angleY Vertical angle (degrees)
 */
void CV64_ModelViewer_SetCameraAngles(float angleX, float angleY);

/**
 * @brief Set camera distance from model
 * @param distance Distance value
 */
void CV64_ModelViewer_SetCameraDistance(float distance);

/**
 * @brief Export current model to file
 * @param outputPath Output file path
 * @param format Format string ("obj", "fbx", "gltf")
 * @return true if export succeeded
 */
bool CV64_ModelViewer_ExportModel(const char* outputPath, const char* format);

/**
 * @brief Toggle wireframe rendering
 * @param enabled true to enable wireframe mode
 */
void CV64_ModelViewer_SetWireframe(bool enabled);

/**
 * @brief Toggle texture rendering
 * @param enabled true to enable textures
 */
void CV64_ModelViewer_SetTexturesEnabled(bool enabled);

/**
 * @brief Toggle lighting
 * @param enabled true to enable lighting
 */
void CV64_ModelViewer_SetLightingEnabled(bool enabled);

/**
 * @brief Set animation playback state
 * @param playing true to play animation
 */
void CV64_ModelViewer_SetAnimationPlaying(bool playing);

/**
 * @brief Set animation frame
 * @param frame Frame number
 */
void CV64_ModelViewer_SetAnimationFrame(uint32_t frame);

/**
 * @brief Get current viewer state
 * @return Current state
 */
CV64_ViewerState CV64_ModelViewer_GetState(void);

/**
 * @brief Get viewer statistics
 * @param outTotalModels Total models available
 * @param outLoadedModel Currently loaded model ID (or 0 if none)
 * @param outFPS Current rendering FPS
 */
void CV64_ModelViewer_GetStats(uint32_t* outTotalModels, uint32_t* outLoadedModel, float* outFPS);

/**
 * @brief Handle mouse input for camera control
 * @param button Mouse button (0=left, 1=right, 2=middle)
 * @param pressed true if button pressed, false if released
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 */
void CV64_ModelViewer_HandleMouseButton(int button, bool pressed, int x, int y);

/**
 * @brief Handle mouse movement for camera control
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 */
void CV64_ModelViewer_HandleMouseMove(int x, int y);

/**
 * @brief Handle mouse wheel for zoom
 * @param delta Wheel delta
 */
void CV64_ModelViewer_HandleMouseWheel(int delta);

#ifdef __cplusplus
}
#endif

#endif // CV64_MODEL_VIEWER_H
